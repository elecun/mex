/**
 * @file    main.cc
 * @brief   MEX Application for read loadcell data from serial comm.
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

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "mosquitto/mosquitto.h"
#include "serial.hpp"

using namespace std;
using namespace nlohmann;

#define MEX_LOADCELL_VALUE_TOPIC    "jstec/mex/loadcell"
#define MEX_LOADCELL_CONTROL_TOPIC  "jstec/mex/loadcell/control"

//global variables
serial* _pSerial = nullptr;
struct mosquitto* _mqtt = nullptr;
int _pub_inteval_sec = 1;


/* serial com. processing function  */
deque<char> _serial_buffer;
void process(char* rbuf, int size){

    // insert to buffer
    std::vector<char> data(rbuf, rbuf+size);
    for(auto& i:data){
        _serial_buffer.emplace_back(i);
    }

    //data alignment & save into packet container
    vector<char> packet;
    while(1){
        if(_serial_buffer.size()<14)
            return;

        if(_serial_buffer[0]==0x0d && _serial_buffer[1]==0x0a && _serial_buffer[12]==0x0d && _serial_buffer[13]==0x0a && _serial_buffer[8]==0x2e){
            std::copy(_serial_buffer.begin()+2, _serial_buffer.begin()+12, std::back_inserter(packet));
            spdlog::info("data : {:x}", spdlog::to_hex(packet));
            _serial_buffer.erase(_serial_buffer.begin(), _serial_buffer.begin()+14);
            break;
        }
        else{
            spdlog::info("re-aligning");
            _serial_buffer.pop_front();
        }
    }

    // data publish to database
    float value = atof(&packet[1]);
    json _pubdata;
    _pubdata["loadcell"]["value"] = value;
    string strdata = _pubdata.dump();
    if(_mqtt){
        int ret = mosquitto_publish(_mqtt, nullptr, MEX_LOADCELL_VALUE_TOPIC, strdata.size(), strdata.c_str(), 2, false);
        mosquitto_loop(_mqtt, 3, 1);
        if(ret)
            spdlog::error("Broker connection error");
        spdlog::info("published : {}", strdata);
    }

}

/* MQTT Connection callback */
void connect_callback(struct mosquitto* mosq, void *obj, int result)
{
    spdlog::info("Connected to broker = {}", result);
}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
	// bool match = 0;
	// printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	// mosquitto_topic_matches_sub("/devices/wb-adc/controls/+", message->topic, &match);
	// if (match) {
	// 	printf("got message for ADC topic\n");
	// }

}

void terminate() {

    if(_pSerial){
        _pSerial->stop();

        delete _pSerial;
        _pSerial = nullptr;
    }

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
    string _device_port = "/dev/ttyS0";
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

            case 'i' : { /* interval */
                _pub_inteval_sec = atoi(optarg);
            }
            break;
            
            case 'h':
            default:
            cout << fmt::format("MEX loadcell (built {}/{})", __DATE__, __TIME__) << endl;
            cout << "Usage: mex_loadcell [-p port] [-b baudrate] [-t broker ip] [-i interval]" << endl;
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
		    mosquitto_connect_callback_set(_mqtt, connect_callback);
		    mosquitto_message_callback_set(_mqtt, message_callback);
            _mqtt_rc = mosquitto_connect(_mqtt, _mqtt_broker.c_str(), 1883, 60);
            mosquitto_subscribe(_mqtt, NULL, MEX_LOADCELL_CONTROL_TOPIC, 2);
        }
        
        if(!_pSerial){
            _pSerial = new serial(_device_port.c_str(), _baudrate);
            _pSerial->set_processor(process);
            _pSerial->start();
            spdlog::info("start serial");
        }

        ::pause();

    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}