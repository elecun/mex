/**
 * @file    rpm.hpp
 * @brief   read rpm
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_RPM_HPP_
#define _MEX_RPM_HPP_

#include <string>
#include <include/spdlog/spdlog.h>
#include <include/json.hpp>
#include "subport.hpp"
#include "crc_table.hpp"
#include <deque>
#include <include/spdlog/fmt/bin_to_hex.h>

using namespace std;


class rpm : public subport {

    const int _max_read_buffer_ = 2048;

    public:
        rpm(const char* subport_name, int id):subport(subport_name, id){

        }
        virtual ~rpm() { }

        virtual int read(unsigned char* buffer){
            return 0;
        }

        virtual bool write(unsigned char* buffer, int size){
            return false;
        }

        virtual void request(boost::asio::serial_port* bus, json& response){
            //read request
            unsigned char frame[] = { (unsigned char)_id, 0x04, 0x03, 0xe9, 0x00, 0x02, 0x00, 0x00};
            unsigned short crc = _crc16(frame, 6);
            frame[6] = (crc >> 8) & 0xff;
            frame[7] = crc & 0xff;
            int write_len = bus->write_some(boost::asio::buffer(frame, 8));
            //spdlog::info("requested read realy {}", _id);

            //vector<char> wpacket(frame, frame+write_len);
            //spdlog::info("write data : {:x}", spdlog::to_hex(wpacket));

            boost::this_thread::sleep_for(boost::chrono::milliseconds(200));

            unsigned char rbuffer[_max_read_buffer_] = {0, };
            int read_len = bus->read_some(boost::asio::buffer(rbuffer, _max_read_buffer_));
            //spdlog::info("{}bytes read", read_len);

            //vector<char> rpacket(rbuffer, rbuffer+read_len);
            //spdlog::info("read data : {:x}", spdlog::to_hex(rpacket));

            //parse data
            int value = parse_value(rbuffer, read_len);
            //spdlog::info("rpm {} : {}", _id, value);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(500));    //must sleep

            response[_subname] = value;

        }

        virtual void readsome(boost::asio::serial_port* bus, json& data){

                const int max_len = 1024;
                unsigned char rbuffer[max_len] = {0, };
                int read_len = bus->read_some(boost::asio::buffer(rbuffer, max_len));

                //insert to queue
                for(int i=0;i<read_len;i++){
                    _qbuffer.push_back(rbuffer[i]);
                }

                union {
                    unsigned long value;
                    float f_value;
                } u;

                const int pack_size = 36;

                //process with queue
                while(1){
                    if(_qbuffer.size()>=pack_size){
                        if(_qbuffer[0]==0x02 && _qbuffer[1]==0x55 && _qbuffer[34]==0x03 && _qbuffer[35]==0xaa){
                            u.value = ((_qbuffer[5]&0xff)<<24)|((_qbuffer[4]&0xff)<<16)|((_qbuffer[3]&0xff)<<8)|(_qbuffer[2]&0xff);
                            data["Ch1"] = u.f_value;
                            
                            u.value = ((_qbuffer[9]&0xff)<<24)|((_qbuffer[8]&0xff)<<16)|((_qbuffer[7]&0xff)<<8)|(_qbuffer[6]&0xff);
                            data["Ch2"] = u.f_value;

                            u.value = ((_qbuffer[13]&0xff)<<24)|((_qbuffer[12]&0xff)<<16)|((_qbuffer[11]&0xff)<<8)|(_qbuffer[10]&0xff);
                            data["Ch3"] = u.f_value;

                            u.value = ((_qbuffer[17]&0xff)<<24)|((_qbuffer[16]&0xff)<<16)|((_qbuffer[15]&0xff)<<8)|(_qbuffer[14]&0xff);
                            data["Ch4"] = u.f_value;

                            u.value = ((_qbuffer[21]&0xff)<<24)|((_qbuffer[20]&0xff)<<16)|((_qbuffer[19]&0xff)<<8)|(_qbuffer[18]&0xff);
                            data["Ch5"] = u.f_value;

                            u.value = ((_qbuffer[25]&0xff)<<24)|((_qbuffer[24]&0xff)<<16)|((_qbuffer[23]&0xff)<<8)|(_qbuffer[22]&0xff);
                            data["Ch6"] = u.f_value;

                            u.value = ((_qbuffer[29]&0xff)<<24)|((_qbuffer[28]&0xff)<<16)|((_qbuffer[27]&0xff)<<8)|(_qbuffer[26]&0xff);
                            data["Ch7"] = u.f_value;

                            u.value = ((_qbuffer[33]&0xff)<<24)|((_qbuffer[32]&0xff)<<16)|((_qbuffer[31]&0xff)<<8)|(_qbuffer[30]&0xff);
                            data["Ch8"] = u.f_value;

                            //remove
                            for(int i=0;i<pack_size;i++)
                                _qbuffer.pop_front();
                        }
                        else {
                            _qbuffer.pop_front();
                        }
                    }
                    else
                        break;
                }


                vector<char> rpacket(rbuffer, rbuffer+read_len);

                //process received data

                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

            }

        void get_value(){
            unsigned char frame[] = { (char)_id, 0x04, 0x03, 0xe9, 0x00, 0x02, 0x00, 0x00};
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
            if(frame[0]==(unsigned char)_id){
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
            std::deque<unsigned char> _qbuffer;
};

#endif