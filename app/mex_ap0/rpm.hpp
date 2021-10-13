/**
 * @file    rpm.hpp
 * @brief   read rpm
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_AP0_RPM_HPP_
#define _MEX_AP0_RPM_HPP_

#include <string>
#include "spdlog/spdlog.h"
#include "subport.hpp"
#include "crc_table.hpp"

using namespace std;


class rpm : public subport {

    public:
        rpm(int code):_code(code){

        }
        virtual ~rpm() { }

        virtual int read(unsigned char* buffer){

        }

        virtual bool write(unsigned char* buffer, int size){
            return false;
        }

        virtual void request(boost::asio::serial_port* bus, json& response){
            //read request
            unsigned char frame[] = { (unsigned char)_code, 0x04, 0x03, 0xe9, 0x00, 0x02, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
            int write_len = bus->write_some(boost::asio::buffer(frame, 8));
            spdlog::info("requested read realy {}", _code);

            vector<char> wpacket(frame, frame+write_len);
            spdlog::info("write data : {:x}", spdlog::to_hex(wpacket));

            #define MAX_READ_BUFFER 1024
            unsigned char rbuffer[MAX_READ_BUFFER] = {0, };
            int read_len = bus->read_some(boost::asio::buffer(rbuffer, MAX_READ_BUFFER));
            spdlog::info("{}bytes read", read_len);

            vector<char> rpacket(rbuffer, rbuffer+read_len);
            spdlog::info("read data : {:x}", spdlog::to_hex(rpacket));

            //parse data
            int value = parse_value(rbuffer, read_len);
            spdlog::info("rpm {} : {}", _code, value);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(250));    //must sleep

            response["rpm"]["id"] = _code;
            response["rpm"]["value"] = value;
            
        }

        void get_value(){
            unsigned char frame[] = { (char)_code, 0x04, 0x03, 0xe9, 0x00, 0x02, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
        }

        int parse_value(unsigned char* buffer, int size){
            unsigned char* frame = new unsigned char[size];
            memcpy(frame, buffer, sizeof(unsigned char)*size);

            //checksum validation
            unsigned short crc = _crc16(frame, 7);
            unsigned char crc1 = (crc >> 8) & 0xff;
            unsigned char crc2 = crc & 0xff;

            if(crc1!=frame[size-2] || crc2!=frame[size-1]){
                spdlog::error("CRC is invalid");
            }

            //parse data
            int rpm = 0;
            if(frame[0]==(unsigned char)_code){
                char tmp[2] = {0x00, };
                memcpy(tmp, &frame[4], 2);
                rpm = atoi(tmp);
            }

            delete []frame;
            
            return rpm;
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
};

#endif