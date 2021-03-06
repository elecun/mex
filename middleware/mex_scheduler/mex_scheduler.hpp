
#ifndef _MEX_SCHEDULER_APPLICATION_HPP_
#define _MEX_SCHEDULER_APPLICATION_HPP_

#include <include/cxxopts.hpp>
#include <include/spdlog/spdlog.h>
#include <include/json.hpp>
#include <include/spdlog/sinks/stdout_color_sinks.h>
#include <include/spdlog/fmt/bin_to_hex.h>
#include <csignal>
#include <array>
#include <deque>
#include <map>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cctype>
#include <algorithm>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/chrono.hpp>

#include <mosquitto.h>
#include "scheduler.hpp"
#include "step_mqtt.hpp"
#include "def.hpp"


using namespace std;
using namespace nlohmann;


//global variables
scheduler* g_scheduler = nullptr;
struct mosquitto* g_mqtt = nullptr; //global mqtt instance
boost::thread* g_pub_thread = nullptr;


#endif