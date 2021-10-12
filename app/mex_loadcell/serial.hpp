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
#include "spdlog/spdlog.h"
#include <functional>
#include <atomic>

using namespace std;


class serial {
    #define MAX_READ_BUFFER 2048

    public:

        typedef void(*ptrProcess)(unsigned char*, int);

        serial(const char* dev, int baudrate):_io(), _port(_io, dev), _stop(false){

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

        void start(){ /* start read some */
            _t = boost::make_shared<boost::thread>((boost::bind(&boost::asio::io_service::run, &_io)));

            while(1){
                if(_stop) break;

                read_some();        
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }
        }

        void stop(){ /* stop read */
            _stop = true;            
            if(_port.is_open()){
                _port.close();
            }
            _io.stop();
            spdlog::info("stopping...");
        }

    private:
        void handler(const boost::system::error_code& error, size_t bytes_tranfered){
            _rbuffer[bytes_tranfered] = 0;
            if(bytes_tranfered>0){
                if(_proc){
                    unsigned char* _tmpBuffer = new unsigned char[bytes_tranfered];
                    memcpy(_tmpBuffer, _rbuffer, sizeof(unsigned char)*(bytes_tranfered));
                    _proc(_tmpBuffer, static_cast<int>(bytes_tranfered));
                    delete []_tmpBuffer;
                }
            }

            read_some();
        }

        void read_some(){
            _port.async_read_some(
                boost::asio::buffer(_rbuffer, MAX_READ_BUFFER),
                boost::bind(&serial::handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }


    private:
        unsigned char _rbuffer[MAX_READ_BUFFER];
        boost::asio::io_service _io;
        boost::asio::serial_port _port;
        ptrProcess _proc;
        boost::shared_ptr<boost::thread> _t;
        atomic<bool> _stop;

};

#endif