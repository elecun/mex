/**
 * @file    relay.hpp
 * @brief   relay control with modbus RTU
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_AP0_RELAY_HPP_
#define _MEX_AP0_RELAY_HPP_

#include <string>
#include "spdlog/spdlog.h"
#include "subport.hpp"
#include "crc_table.hpp"
#include <queue>
#include <vector>

using namespace std;


class relay : public subport {

    public:
        relay(int code, int id):_id(id), _code(code){

        }
        virtual ~relay() { }

        virtual int read(unsigned char* buffer){

        }

        virtual bool write(unsigned char* buffer, int size){

            if(size!=8)
                return false;

            vector<unsigned char> tmp;
            for(int i=0;i<size;i++)
                tmp.emplace_back(buffer[i]);

            _write_buffer.push(tmp);
            return true;
        }


        virtual void request(boost::asio::serial_port* bus, json& response){

            //read request
            unsigned char frame[] = { (unsigned char)_code, 0x01, 0x00, (unsigned char)_id, 0x00, 0x01, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
            int write_len = bus->write_some(boost::asio::buffer(frame, 9));
            //spdlog::info("requested read realy {}", _id);

            vector<char> wpacket(frame, frame+write_len);
            //spdlog::info("write data : {:x}", spdlog::to_hex(wpacket));

            #define MAX_READ_BUFFER 1024
            unsigned char rbuffer[MAX_READ_BUFFER] = {0, };
            int read_len = bus->read_some(boost::asio::buffer(rbuffer, MAX_READ_BUFFER));
            //spdlog::info("{}bytes read", read_len);

            vector<char> rpacket(rbuffer, rbuffer+read_len);
            //spdlog::info("read data : {:x}", spdlog::to_hex(rpacket));

            //parse data
            bool value = (rbuffer[3]==0xff)?true:false;
            spdlog::info("relay {} : {}", _id, value);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(250));    //must sleep

            response["relay"]["id"] = _id;
            response["relay"]["value"] = value;
        }

        void set_on(){
            unsigned char frame[] = { 0x01, 0x05, 0x00, (char)_id, 0xff, 0x00, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
        }

        void set_off(){
            unsigned char frame[] = { 0x01, 0x05, 0x00, (char)_id, 0x00, 0x00, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
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
        int _id = 0;
        int _code = 0;
        queue<vector<unsigned char>> _write_buffer;

};

#endif