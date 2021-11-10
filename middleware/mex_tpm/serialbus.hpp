/**
 * @file    serial.hpp
 * @brief   serial commnunocation with boost library
 * @author  Byunghun Hwang
 */

#ifndef _MEX_SERIAL_HPP_
#define _MEX_SERIAL_HPP_

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
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
#include "subport.hpp"
#include "safeq.hpp"

using namespace std;


class serialbus {
    #define MAX_READ_BUFFER 2048

    public:

        typedef void(*ptrProcess)(json&);

        serialbus(const char* dev, int baudrate):_service(), _port(_service, dev), _operation(false){

            _port.set_option(boost::asio::serial_port_base::parity());	// default none
            _port.set_option(boost::asio::serial_port_base::character_size(8));
            _port.set_option(boost::asio::serial_port_base::stop_bits());	// default one
            _port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
            _port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none)); // default none

        }
        virtual ~serialbus(){
            
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
            _port.close();
        }

        void add_subport(int idx, subport* port){
            _subport_container.insert(std::make_pair(idx, port));
        }

        subport* get_subport(const int idx){
            if(_subport_container.find(idx)==_subport_container.end()){
                spdlog::warn("Nothing to control for subport id {}", idx);
                return nullptr;
            }
                
            return _subport_container[idx];
        }

        /* push the write data */
        void push_write(unsigned char* buffer, int size){
            vector<unsigned char> data(buffer, buffer+size);
            _write_buffer.produce(std::move(data));
            //spdlog::info("write buffer size : {}", _write_buffer.size());
        }
        
        /* port open check */
        bool is_open(){
            return _port.is_open();
        }

    private:

        void thread_assign(){
            boost::function<void(void)> read_handler = [&](void) {
                while(_operation){
                    if(_port.is_open()){

                        for(auto& sub: _subport_container){
                            json response;
                            sub.second->request(&_port, response);

                            //post process
                            postprocess(response);
                        }
                    }
                    else
                        spdlog::error("port is not opened");

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
        char _rbuffer[MAX_READ_BUFFER];
        boost::asio::io_service _service;
        boost::shared_ptr<boost::asio::io_service::work> _worker;
        boost::thread_group _ts;
        boost::asio::serial_port _port;
        ptrProcess _proc;
        atomic<bool> _operation;

        map<int, subport*> _subport_container;
        safeQueue<vector<unsigned char>> _write_buffer;

};

#endif