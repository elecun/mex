/**
 * @file    plc.hpp
 * @brief   MEX PLC
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_PLC_HPP_
#define _MEX_PLC_HPP_

#include <string>
#include <include/spdlog/spdlog.h>
#include "subport.hpp"
#include <queue>
#include <vector>
#include <mutex>
#include <thread>

using namespace std;

#define STX 0x05
#define ETX 0x04

class plc : public subport {

    const int _max_read_buffer_ = 2048;
    typedef struct _wpacket {
        unsigned char data[128] = {0,};
        int length = 0;
        void copy(unsigned char* pdata, int size){
            memset(data, 0, sizeof(data));
            memcpy(data, pdata, size);
            length = size;
        };
    } wpack;

    public:
        plc(const char* subport_name, int id):subport(subport_name, 1){
            _cylinder_thread = new std::thread(&plc::cylinder_loop, this);
        }
        virtual ~plc() {
            spdlog::info("closing PLC");
            _cylinder_loop = false;
            _cylinder_thread->join();
        }

        virtual int read(unsigned char* buffer){

        }

        virtual bool write(unsigned char* buffer, int size){

        }


        virtual void request(boost::asio::serial_port* bus, json& response){

            //consume to write
            {
                std::lock_guard<std::mutex> lock(_mutex);
                while(1){
                    if(_write_buffer.empty())
                        break;

                    wpack wdata = _write_buffer.front();
                    _write_buffer.pop();

                    //write
                    int write_len = bus->write_some(boost::asio::buffer(wdata.data, wdata.length));
                    spdlog::info("write {}bytes", write_len);

                    // unsigned char rbuffer[_max_read_buffer_] = {0, };
                    // int read_len = bus->read_some(boost::asio::buffer(rbuffer, _max_read_buffer_));
                    
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));    //must sleep
                }
            }
        }

        virtual void readsome(boost::asio::serial_port* bus, json& data){
            //no required
        }

        void str2ascii(string letter, unsigned char* frame){
            for (int i = 0; i < letter.length(); i++)
                frame[i] = (int)letter.at(i);
        }

        void write_buffer(string strpacket){
            unsigned char* frame = new unsigned char[strpacket.length()+2];
            frame[0] = STX;
            str2ascii(strpacket, frame+1);
            frame[strpacket.length()+1] = ETX;

            //verify
            //vector<unsigned char> data(frame, frame+strpacket.length()+2);
            //spdlog::info("write data : {:x}", spdlog::to_hex(data));

            std::lock_guard<std::mutex> lock(_mutex);
            wpack data;
            data.copy(frame, strpacket.length()+2);
            _write_buffer.push(std::move(data));

            delete []frame;
        }

        /* PLC interface function */
        void motor_off(){ write_buffer("00WSS0107%MX004601"); }
        void motor_on(){ write_buffer("00WSS0107%MX004600");}
        void cylinder_up(){
            std::unique_lock<std::mutex> lock(_m);
            _cylinder_move = 1;
            lock.unlock();
        }

        void cylinder_down(){ 
            std::unique_lock<std::mutex> lock(_m);
            _cylinder_move = 2; 
            lock.unlock();
        }

        void cylinder_stop(){ 
            std::unique_lock<std::mutex> lock(_m);
            _cylinder_move = 0; 
            lock.unlock();
        }

        void move_cw(){ write_buffer("00WSS0307%MX00420107%MX00430007%MX011E00"); }
        void move_ccw(){ write_buffer("00WSS0307%MX00420007%MX00430107%MX011E00");}
        void move_stop(){ write_buffer("00WSS0307%MX00420007%MX00430007%MX011E01");}
        void param_set(){ write_buffer("00WSB07%DW350303012C003C06C5");}
        void test_start(){ write_buffer("00WSS0107%MX008001");}
        void test_pause(){ write_buffer("00WSS0207%MX011F0107%MX008000");}
        void test_stop(){ write_buffer("00WSS0207%MX011F0107%MX008000");}
        void program_connect(){ write_buffer("00RSB07%MW000001");}
        void program_verify(){ write_buffer("00WSS0107%MX000301");}

        void cylinder_loop(){
            while(_cylinder_loop){
                switch(_cylinder_move){
                    case 0: { } break;
                    case 1: { write_buffer("00WSS0207%MX00440007%MX004400"); } break; //up
                    case 2: { write_buffer("00WSS0207%MX00440007%MX004500"); } break; //down
                }
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
            }
        }

    private:
        std::queue<wpack> _write_buffer;
        std::mutex _mutex;

        bool _cylinder_loop = true;
        std::thread* _cylinder_thread = nullptr;
        int _cylinder_move = 0;

        std::mutex _m;

};

#endif