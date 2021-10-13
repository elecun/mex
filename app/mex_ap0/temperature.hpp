/**
 * @file    temperature.hpp
 * @brief   read temperature
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_AP0_TEMPERATURE_HPP_
#define _MEX_AP0_TEMPERATURE_HPP_

#include <string>
#include "spdlog/spdlog.h"
#include "subport.hpp"
#include "spdlog/fmt/bin_to_hex.h"
#include "json.hpp"


using namespace std;
using namespace nlohmann;

#define STX 0x02
#define ETX 0x03
#define ACK 0x06

class temperature : public subport {

    public:
        temperature(int id):_id(id){

        }
        virtual ~temperature() { }

        virtual int read(unsigned char* buffer){

        }

        virtual bool write(unsigned char* buffer, int size){
            return false;
        }

        virtual void request(boost::asio::serial_port* bus, json& response){
            //read request
            unsigned char frame[] = { STX, 0x30, 0x30+(unsigned char)_id, 0x52, 0x58, 0x50, 0x30, ETX, 0x00};
            frame[8] = _checksum(frame, 8);
            int write_len = bus->write_some(boost::asio::buffer(frame, 9));
            spdlog::info("requested read temperature {}", _id);

            vector<char> wpacket(frame, frame+write_len);
            //spdlog::info("write data : {:x}", spdlog::to_hex(wpacket));

            #define MAX_READ_BUFFER 1024
            unsigned char rbuffer[MAX_READ_BUFFER] = {0, };
            int read_len = bus->read_some(boost::asio::buffer(rbuffer, MAX_READ_BUFFER));
            //spdlog::info("{}bytes read", read_len);

            vector<char> rpacket(rbuffer, rbuffer+read_len);
            //spdlog::info("read data : {:x}", spdlog::to_hex(rpacket));

            //parse data
            float value = parse_value(rbuffer, read_len);
            //spdlog::info("temperature {} : {}", _id, value);

            boost::this_thread::sleep_for(boost::chrono::milliseconds(250));    //must sleep

            response["temperature"]["id"] = _id;
            response["temperature"]["value"] = value;
        }

    private:

        float parse_value(unsigned char* data, int size){

            unsigned char* frame = new unsigned char[size];
            memcpy(frame, data, sizeof(unsigned char)*size);

            //checksum validation
            unsigned char chksum = _checksum(frame, size-1);
            if(chksum!=frame[size-1]){
                spdlog::error("BCC is invalid");
            }

            //parse data
            float value = 0.0;
            if(frame[0]==ACK && frame[1]==STX && frame[14]==ETX && frame[3]==(0x30+(unsigned char)_id)){
                char vt[4] = {0, };
                memcpy(vt, &frame[9], 4);
                value = atof(vt);
                if(frame[13]!=0x30){
                    value = value*pow(0.1,(frame[13]-0x30));
                }
                if(frame[8]==0x2d)
                    value *= -1;
            }
            else
                spdlog::error("failed parse {:x}, {:x}", frame[3], (0x30+(unsigned char)_id));
            delete []frame;

            return value;
        }

    private:
        unsigned char _checksum(unsigned char* buffer, int size){
            unsigned char chk = 0x00;
            for(int i=0;i<size;i++){
                chk ^= buffer[i];
            }
            return chk;
        }



    private:
        int _id = 0;
};

#endif