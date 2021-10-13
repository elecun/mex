/**
 * @file    serialbus.hpp
 * @brief   serial(RS485) commnunocation with boost library
 * @author  Byunghun Hwang
 */

#ifndef _MEX_SERIAL_BUS_HPP_
#define _MEX_SERIAL_BUS_HPP_

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include "spdlog/spdlog.h"
#include <functional>
#include <atomic>
#include <boost/smart_ptr.hpp>
#include "subport.hpp"
#include <map>
#include "json.hpp"

using namespace std;
using namespace nlohmann;


class serialbus {
    #define MAX_READ_BUFFER 2048

    public:
        typedef void(*ptrPostProcess)(json&);

        serialbus(const char* dev, int baudrate):_service(), _port(_service, dev), _operation(false), _io_relay(_service), _io_rpm(_service), _io_temperature(_service){

            _port.set_option(boost::asio::serial_port_base::parity());	// default none
            _port.set_option(boost::asio::serial_port_base::character_size(8));
            _port.set_option(boost::asio::serial_port_base::stop_bits());	// default one
            _port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
            _port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none)); // default none

        }
        virtual ~serialbus(){
            
        }

        void set_postprocess(ptrPostProcess ptr){ 
            _post_proc = ptr;
        }

        /* write to bus */
        void write(unsigned char* data, int size){

        }

        void start(){ /* start read */
            _operation = true;
            _worker.reset(new boost::asio::io_service::work(_service));
            read_assign();
            _tg.create_thread(boost::bind(&boost::asio::io_service::run, boost::ref(_service)));
            _tg.create_thread([&](void){_service.run();});
        }

        void stop(){ /* stop read */
            _operation = false;
            _worker.reset();
            _service.stop();
            _tg.join_all();
            _service.reset();
        }
        
        /* adding sub port class instance */
        void add_subport(int idx, subport* port){
            _subport_container.insert(std::make_pair(idx, port));
        }

    private:
        void read_assign(){
            boost::function<void(void)> read_handler = [&](void) {
                while(_operation){
                    if(_port.is_open()){
                        for(auto& port: _subport_container){
                            json response;
                            port.second->request(&_port, response);
                            call_post(response);
                        }
                    }
                    else
                        spdlog::error("port is not opened");
                }
            };
            _service.post(read_handler);
        }

        void call_post(json& response){
            if(_post_proc){
                _post_proc(response);
            }
        }

        

        void handler(const boost::system::error_code& error, size_t bytes_transfered){
            _rbuffer[bytes_transfered] = 0;
            if(bytes_transfered>0){
                spdlog::info("{}bytes read", bytes_transfered);

                if(_proc){
                    char* _tmpBuffer = new char[bytes_transfered];
                    memcpy(_tmpBuffer, _rbuffer, sizeof(char)*(bytes_transfered));
                    _proc(_tmpBuffer, static_cast<int>(bytes_transfered));
                    delete []_tmpBuffer;
                }
            }
        }

    private:
        char _rbuffer[MAX_READ_BUFFER];
        boost::asio::io_service _service;
        boost::shared_ptr<boost::asio::io_service::work> _worker;
        boost::thread_group _tg;
        boost::scoped_ptr<boost::thread> _t;
        boost::asio::serial_port _port;
        ptrPostProcess _post_proc;
        atomic<bool> _operation;

        boost::asio::io_service::strand _io_relay;
        boost::asio::io_service::strand _io_temperature;
        boost::asio::io_service::strand _io_rpm;

        map<int, subport*> _subport_container; /* port container, use only for sync mode. */

};

#endif