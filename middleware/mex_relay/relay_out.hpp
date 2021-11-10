/**
 * @file    relay.hpp
 * @brief   relay control with modbus RTU
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_RELAY_OUT_HPP_
#define _MEX_RELAY_OUT_HPP_

#include <string>
#include <include/spdlog/spdlog.h>
#include "subport.hpp"
#include "crc_table.hpp"
#include <queue>
#include <vector>
#include <mutex>

using namespace std;


class relay_out : public subport {

    const int _max_read_buffer_ = 2048;
    typedef struct _wpacket {
        unsigned char data[8] = {0,};
        void copy(unsigned char* pdata, int size){
            memcpy(data, pdata, size);
        };
    } wpack;

    public:
        relay_out(const char* subport_name, int code, int id):subport(subport_name, id), _code(code){

        }
        virtual ~relay_out() {

        }

        virtual int read(unsigned char* buffer){

        }

        virtual bool write(unsigned char* buffer, int size){

            // if(size!=8)
            //     return false;

            // vector<unsigned char> tmp;
            // for(int i=0;i<size;i++)
            //     tmp.emplace_back(buffer[i]);

            // _write_buffer.produce(tmp);
            // return true;
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
                    unsigned short crc = _crc16(wdata.data, 6);
                    wdata.data[6] = (crc >> 8) & 0xff;
                    wdata.data[7] = crc & 0xff;
                    int write_len = bus->write_some(boost::asio::buffer(wdata.data, 9));

                    unsigned char rbuffer[_max_read_buffer_] = {0, };
                    int read_len = bus->read_some(boost::asio::buffer(rbuffer, _max_read_buffer_));
                    
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(250));    //must sleep
                }
            }
            

            //read status request
            unsigned char frame[] = { 0x01, 0x01, 0x00, (unsigned char)_id, 0x00, 0x01, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
            int write_len = bus->write_some(boost::asio::buffer(frame, 8));
            //spdlog::info("{}:requested read realy {}",_subname, _id);

            //vector<char> wpacket(frame, frame+write_len);
            //spdlog::info("{}:write data : {:x}", _subname,spdlog::to_hex(wpacket));

            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));    //waiting for receiving

            unsigned char rbuffer[_max_read_buffer_] = {0, };
            int read_len = bus->read_some(boost::asio::buffer(rbuffer, _max_read_buffer_));
            //spdlog::info("{}:{}bytes read", _subname,read_len);

            //vector<char> rpacket(rbuffer, rbuffer+read_len);
            //spdlog::info("{}:read data : {:x}", _subname, spdlog::to_hex(rpacket));

            //parse data
            bool value = (rbuffer[3]==0xff)?true:false;
            //spdlog::info("{}:relay {} : {}", _subname, _id, value);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));    //must sleep

            response[_subname] = value;
        }

        virtual void readsome(boost::asio::serial_port* bus, json& data){
            //no required
        }

        void set_on(){
            unsigned char frame[] = { 0x01, 0x05, 0x00, (unsigned char)_id, 0xff, 0x00, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;

            std::lock_guard<std::mutex> lock(_mutex);
            wpack data;
            data.copy(frame, sizeof(frame));
            _write_buffer.push(std::move(data));
        }

        void set_off(){
            unsigned char frame[] = { 0x01, 0x05, 0x00, (char)_id, 0x00, 0x00, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;

            std::lock_guard<std::mutex> lock(_mutex);
            wpack data;
            data.copy(frame, sizeof(frame));
            _write_buffer.push(std::move(data));
        }

        void get_on(){
            unsigned char frame[] = { 0x01, 0x01, 0x00, (char)_id, 0x00, 0x01, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
        }

        unsigned short _crc16(unsigned char* buffer, unsigned short buffer_len){
            uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
            uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
            unsigned int i; /* will index into CRC lookup */

            /* pass through message buffer */
            while (buffer_len--) {
                i = crc_hi ^ *buffer++; /* calculate the CRC  */
                crc_hi = crc_lo ^ table_crc_hi[i];
                crc_lo = table_crc_lo[i];
            }

            return (crc_hi << 8 | crc_lo);
        }

    private:
        int _code = 0;
        std::queue<wpack> _write_buffer;
        std::mutex _mutex;


};

#endif