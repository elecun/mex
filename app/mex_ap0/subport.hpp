/**
 * @file    subport.hpp
 * @brief   abstraction for subport on serial bus
 * @author  Byunghun Hwang<bh.hwang@iae.re.kr>
 */

#ifndef _MEX_AP0_SUBPORT_HPP_
#define _MEX_AP0_SUBPORT_HPP_

#include <array>
#include <boost/asio/serial_port.hpp>
#include "json.hpp"

using namespace std;
using namespace nlohmann;;

class subport {
    public:
        virtual void request(boost::asio::serial_port* bus, json& response) = 0;
        virtual int read(unsigned char* buffer) = 0;
        virtual bool write(unsigned char* buffer, int size) = 0;
};

#endif

