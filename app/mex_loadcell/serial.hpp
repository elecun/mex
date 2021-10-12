/**
 * @file    serial.hpp
 * @brief   serial commnunocation with boost library
 * @author  Byunghun Hwang
 */

#ifndef _MEX_LOADCELL_SERIAL_HPP_
#define _MEX_LOADCELL_SERIAL_HPP_

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

using namespace std;


class serial {
    #define MAX_READ_BUFFER 2048

    public:

        typedef void(*ptrProcess)(char*, int);

        serial(const char* dev, int baudrate):_service(), _port(_service, dev), _operation(false){

            _port.set_option(boost::asio::serial_port_base::parity());	// default none
            _port.set_option(boost::asio::serial_port_base::character_size(8));
            _port.set_option(boost::asio::serial_port_base::stop_bits());	// default one
            _port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
            _port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none)); // default none

        }
        virtual ~serial(){
            
        }

        void set_processor(ptrProcess ptr){ /* processor pointer */
            _proc = ptr;
        }

        void start(){ /* start read */

            _operation = true;
            _worker.reset(new boost::asio::io_service::work(_service));
            read_assign();
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

        void read_assign(){
            boost::function<void(void)> read_handler = [&](void) {
                while(_operation){
                    if(_port.is_open()){
                        _port.async_read_some(
                            boost::asio::buffer(_rbuffer, MAX_READ_BUFFER),
                            boost::bind(&serial::handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                    }
                else
                    spdlog::error("port is not opened");
                    
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                }
            };
            _service.post(read_handler);
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
        boost::thread_group _ts;
        boost::scoped_ptr<boost::thread> _t;
        boost::asio::serial_port _port;
        ptrProcess _proc;
        atomic<bool> _operation;

};

#endif