/**
 * @file    main.cc
 * @brief   MEX Application (temnperature, rpm, relay)
 * @author  bh.hwang@iae.re.kr
 */

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fmt/bin_to_hex.h"
#include <csignal>
#include <array>
#include <deque>
#include <sys/types.h>
#include <unistd.h>
#include "json.hpp"
#include <map>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "mosquitto/mosquitto.h"
#include "serialbus.hpp"
#include "temperature.hpp"  //for temperature data
#include "realy.hpp"    //for relay control
#include "rpm.hpp"  //for rpm data

using namespace std;
using namespace nlohmann;

#define MEX_RELAY_CONTROL_TOPIC    "jstec/mex/relay/control"
#define MEX_RELAY_VALUE_TOPIC    "jstec/mex/relay"
#define MEX_RPM_VALUE_TOPIC         "jstec/mex/rpm"
#define MEX_TEMPERATURE_VALUE_TOPIC         "jstec/mex/temperature"

//global variables
serialbus* _serialbus = nullptr;
struct mosquitto* _mqtt = nullptr;
int _pub_inteval_sec = 1;

map<string, int> _relay_command {
            {"on", 1},
            {"off", 2}
        };


/* serial com. processing function  */
deque<char> _serial_buffer;
void process(char* rbuf, int size){

    // // insert to buffer
    // std::vector<char> data(rbuf, rbuf+size);
    // for(auto& i:data){
    //     _serial_buffer.emplace_back(i);
    // }

    // //data alignment & save into packet container
    // vector<char> packet;
    // while(1){
    //     if(_serial_buffer.size()<14)
    //         return;

    //     if(_serial_buffer[0]==0x0d && _serial_buffer[1]==0x0a && _serial_buffer[12]==0x0d && _serial_buffer[13]==0x0a && _serial_buffer[8]==0x2e){
    //         std::copy(_serial_buffer.begin()+2, _serial_buffer.begin()+12, std::back_inserter(packet));
    //         spdlog::info("data : {:x}", spdlog::to_hex(packet));
    //         _serial_buffer.erase(_serial_buffer.begin(), _serial_buffer.begin()+14);
    //         break;
    //     }
    //     else{
    //         spdlog::info("re-aligning");
    //         _serial_buffer.pop_front();
    //     }
    // }

    // data publish to database
    // float value = atof(&packet[1]);
    // json _pubdata;
    // _pubdata["loadcell"]["value"] = value;
    // string strdata = _pubdata.dump();
    
    // if(_mqtt){
    //     int ret = mosquitto_publish(_mqtt, nullptr, MEX_LOADCELL_VALUE_TOPIC, strdata.size(), strdata.c_str(), 2, false);
    //     mosquitto_loop(_mqtt, 3, 1);
    //     if(ret){
    //         spdlog::error("Broker connection error");
    //     }
    // }

}

static void postprocess(json& msg){
    string strdata = msg.dump();
    spdlog::info("publishing : {}", strdata);
    // if(_mqtt){
    //     int ret = mosquitto_publish(_mqtt, nullptr, MEX_LOADCELL_VALUE_TOPIC, strdata.size(), strdata.c_str(), 2, false);
    //     mosquitto_loop(_mqtt, 3, 1);
    //     if(ret){
    //         spdlog::error("Broker connection error");
    //     }
    // }

}

void subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    spdlog::info("subscribed");
}

/* MQTT Connection callback */
void connect_callback(struct mosquitto* mosq, void *obj, int result)
{
    spdlog::info("Connected to broker = {}", result);
}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
    const int max_buffer = 2048;

    // read buffer
    char* buffer = new char[max_buffer];
    memset(buffer, 0, sizeof(char)*max_buffer);
    memcpy(buffer, message->payload, sizeof(char)*message->payloadlen);
    string topic(message->topic);
    string strmsg = buffer;
    delete []buffer;

    // control message process
    bool match = false;
	mosquitto_topic_matches_sub("jstec/mex/relay/control", message->topic, &match);
    if(match){
        try{
            json msg = json::parse(strmsg);

            int id = 0;
            if(msg.find("id")!=msg.end()){
                id = msg["id"].get<int>();
            }
            else return;

            //command
            if(msg.find("command")!=msg.end()){
                string command = msg["command"].get<string>();
                std::transform(command.begin(), command.end(), command.begin(),[](unsigned char c){ return std::tolower(c); }); //to lower case

                if(_relay_command.count(command)){
                    switch(_relay_command[command]){
                        case 1: { //on
                            spdlog::info("do command : {}", command);
                            relay* _p = dynamic_cast<relay*>(_serialbus->get_subport(id));
                            if(_p){
                                spdlog::info("casting");
                                vector<unsigned char> packet;
                                _p->set_on(packet);
                                // _serialbus->push_write(&packet[0], packet.size());
                                // spdlog::info("turn on");
                            }
                            
                        } break;
                        case 2: { //off
                            relay* _p = dynamic_cast<relay*>(_serialbus->get_subport(id));
                            vector<unsigned char> packet;
                            _p->set_off(packet);
                            _serialbus->push_write(&packet[0], packet.size());
                            spdlog::info("turn on");
                            spdlog::info("turn off");
                        } break;
                    }
                }
            }
        }
        catch(json::exception& e){
            spdlog::error("Message Error : {}", e.what());
        }

    }

    spdlog::info("on message");
	// bool match = 0;
	// printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);


}

void terminate() {

    if(_serialbus){
        _serialbus->stop();

        delete _serialbus;
        _serialbus = nullptr;
    }

    mosquitto_loop_stop(_mqtt, true);
    mosquitto_destroy(_mqtt);
    mosquitto_lib_cleanup();

    spdlog::info("Successfully terminated");
    exit(EXIT_SUCCESS);
}

void cleanup(int sig) {
  switch(sig){
    case SIGSEGV: { spdlog::warn("Segmentation violation"); } break;
    case SIGABRT: { spdlog::warn("Abnormal termination"); } break;
    case SIGKILL: { spdlog::warn("Process killed"); } break;
    case SIGBUS: { spdlog::warn("Bus Error"); } break;
    case SIGTERM: { spdlog::warn("Termination requested"); } break;
    case SIGINT: { spdlog::warn("interrupted"); } break;
    default:
      spdlog::info("Cleaning up the program");
  }
  ::terminate(); 
}



int main(int argc, char* argv[])
{  
    const int signals[] = { SIGINT, SIGTERM, SIGBUS, SIGKILL, SIGABRT, SIGSEGV };
    for(const int& s:signals)
        signal(s, cleanup);

    //signal masking
    sigset_t sigmask;
    if(!sigfillset(&sigmask)){
        for(int signal:signals)
            sigdelset(&sigmask, signal); //delete signal from mask
    }
    else {
        spdlog::error("Signal Handling Error");
        ::terminate(); //if failed, do termination
    }

    if(pthread_sigmask(SIG_SETMASK, &sigmask, nullptr)!=0){ // signal masking for this thread(main)
        spdlog::error("Signal Masking Error");
        ::terminate();
    }

    spdlog::stdout_color_st("console");

    int optc = 0;
    string _device_port = "/dev/ttyAP0";
    int _baudrate = 9600;

    string _mqtt_broker = "0.0.0.0";
    int _mqtt_rc = 0;

    while((optc=getopt(argc, argv, "p:b:t:i:h"))!=-1)
    {
        switch(optc){
            case 'p': { /* device port */
                _device_port = optarg;
            }
            break;

            case 'b': { /* baudrate */
                _baudrate = atoi(optarg);
            }
            break;

            case 't': { /* target ip to pub */
                _mqtt_broker = optarg;
            }
            break;
            
            case 'h':
            default:
            cout << fmt::format("MEX relay control (built {}/{})", __DATE__, __TIME__) << endl;
            cout << "Usage: mex_relay [-p port] [-b baudrate] [-t broker ip]" << endl;
            exit(EXIT_FAILURE);
            break;
        }
    }

    // show arguments
    spdlog::info("> set device port : {}", _device_port);
    spdlog::info("> set port baudrate : {}", _baudrate);
    spdlog::info("> set broker IP : {}", _mqtt_broker);

    try {

        mosquitto_lib_init();
        _mqtt = mosquitto_new("jstec", true, 0);

        if(_mqtt){
            spdlog::info("connecting to broker...");
		    mosquitto_connect_callback_set(_mqtt, connect_callback);
		    mosquitto_message_callback_set(_mqtt, message_callback);
            mosquitto_subscribe_callback_set(_mqtt, subscribe_callback);
            _mqtt_rc = mosquitto_connect(_mqtt, _mqtt_broker.c_str(), 1883, 60);
            spdlog::info("mqtt connection : {}", _mqtt_rc);
            mosquitto_subscribe(_mqtt, NULL, MEX_RELAY_CONTROL_TOPIC, 2);
            mosquitto_subscribe(_mqtt, NULL, MEX_TEMPERATURE_VALUE_TOPIC, 2);
            mosquitto_subscribe(_mqtt, NULL, MEX_RPM_VALUE_TOPIC, 2);
            mosquitto_loop_start(_mqtt);
        }
        
        if(!_serialbus){
            _serialbus = new serialbus(_device_port.c_str(), _baudrate);
            _serialbus->set_postprocess(postprocess);
            _serialbus->add_subport(1, new relay(1, 0));
            _serialbus->add_subport(3, new relay(1, 3));    //for zero set
            _serialbus->add_subport(2, new temperature(1));
            _serialbus->add_subport(4, new temperature(2));
            _serialbus->add_subport(6, new temperature(3));
            _serialbus->add_subport(5, new rpm(4));
            _serialbus->start();
        }

        ::pause();

    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}