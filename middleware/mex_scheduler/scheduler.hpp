/**
 * @file    scheduler.hpp
 * @brief   step scheduler
 * @author  Byunghun Hwang
 */

#ifndef _MEX_SCHEDULER_HPP_
#define _MEX_SCHEDULER_HPP_

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <include/spdlog/spdlog.h>
#include <functional>
#include <atomic>
#include <boost/smart_ptr.hpp>
#include <include/json.hpp>

using namespace std;
using namespace nlohmann;


class scheduler {

    public:
        typedef void(*ptrProcess)(json&);

        scheduler():_service(), _operation(false){

        }
        virtual ~scheduler(){
            
        }

        void set_processor(ptrProcess ptr){ /* processor pointer */
            _proc = ptr;
        }

        void start(){ /* start read */

            _operation = true;
            _worker.reset(new boost::asio::io_service::work(_service));
            thread_assign();
            _ts.create_thread(boost::bind(&boost::asio::io_service::run, boost::ref(_service)));
            _ts.create_thread([&](void){_service.run();});
        }

        void stop(){ /* stop read */              
            _operation = false;
            _worker.reset();
            _service.stop();
            _ts.join_all();
            _service.reset();
        }

    private:

        void thread_assign(){
            boost::function<void(void)> read_handler = [&](void) {
                while(_operation){
                    // if(_port.is_open()){
                    //     for(auto& sub: _subport_container){
                    //         json response;
                    //         sub.second->request(&_port, response);
                    //         postprocess(response);
                    //     }
                    // }
                    // else
                    //     spdlog::error("port is not opened");

                    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
                }
            };
            _service.post(read_handler);
        }

        /* post process function */
        void postprocess(json& response){
            if(_proc){
                _proc(response);
            }
        }

    private:
        boost::asio::io_service _service;
        boost::shared_ptr<boost::asio::io_service::work> _worker;
        boost::thread_group _ts;
        ptrProcess _proc;
        atomic<bool> _operation;

};

#endif