

#ifndef _MEX_SCHEDULER_STEP_MQTT_HPP_
#define _MEX_SCHEDULER_STEP_MQTT_HPP_


#include <include/json.hpp>
#include <mosquitto.h>
#include <string>
#include <include/spdlog/spdlog.h>
#include "def.hpp"
#include <map>

using namespace nlohmann;
using namespace std;

#define MEX_STEP_CONTROL_TOPIC  "mex/step/ctrl"
#define MEX_STEP_PROGRAM_TOPIC  "mex/step/program"
#define MEX_STEP_STATUS_TOPIC  "mex/step/status"
#define MQTT_CLIENT_ID "jstec_mex_step"

#define MEX_STEP_PLC_CONTROL_TOPIC "mex/plc/ctrl"
#define MEX_STEP_RELAY_CONTROL_TOPIC "mex/relay/ctrl"

#define MEX_LOADCELL_VALUE_TOPIC    "mex/sensor/load"


map<string, int> g_commandset = {
    {"ready", _STATE_::READY },
    {"stop", _STATE_::STOP },
    {"pause", _STATE_::PAUSE },
    {"start", _STATE_::START }
};


/* notify the stop state */
void publish_step_state(struct mosquitto* mqtt, int state){

    json status;

    switch(state){
        case _STATE_::READY: {
            status["state"] = "ready";
            spdlog::info("STATE : READY");
        } break;
        case _STATE_::STOP: {
            status["state"] = "stop";
            spdlog::info("STATE : STOP");
        } break;
        case _STATE_::PAUSE: {
            status["state"] = "pause";
            spdlog::info("STATE : PAUSE");
        } break;
        case _STATE_::START: {
            status["state"] = "start";
            spdlog::info("STATE : START");
        } break;
    }
    
    string dumped_status = status.dump();
    if(mosquitto_publish(mqtt, nullptr, MEX_STEP_STATUS_TOPIC, dumped_status.size(), dumped_status.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
        spdlog::info("stopped step program");
    }
}

#endif