
#ifndef _MEX_RELAY_APPLICATION_HPP_
#define _MEX_RELAY_APPLICATION_HPP_

#include <include/cxxopts.hpp>
#include <include/spdlog/spdlog.h>
#include <include/json.hpp>
#include <include/spdlog/sinks/stdout_color_sinks.h>
#include <include/spdlog/fmt/bin_to_hex.h>
#include <csignal>
#include <array>
#include <deque>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/chrono.hpp>

#include <mosquitto.h>
#include "serialbus.hpp"
#include "relay_out.hpp"
#include "relay_in.hpp"


using namespace std;
using namespace nlohmann;

#define MEX_RELAY_VALUE_TOPIC    "mex/sensor/relay"
#define MEX_RELAY_CONTROL_TOPIC  "mex/relay/ctrl"

#define MQTT_CLIENT_ID "jstec_mex_relay"

const char _start_bytes[2] = {0x0d, 0x0a};
const char _end_bytes[2] = {0x0d, 0x0a};

//global variables
serialbus* g_pSerial = nullptr;
struct mosquitto* g_mqtt = nullptr; //global mqtt instance
boost::thread* g_pub_thread = nullptr;


#endif