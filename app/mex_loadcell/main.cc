/**
 * @file    main.cc
 * @brief   MEX Application for read loadcell data from serial comm.
 * @author  bh.hwang@iae.re.kr
 */

#include "cxxopts.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <csignal>

#include <boost/asio/serial_port.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <functional>

#include <mosquitto/cpp/mosquittopp.h>

using namespace std;

// global
boost::asio::serial_port* _port = nullptr;

void terminate() {
  spdlog::info("Terminated");
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

class mqttConnector : public mosqpp::mosquittopp {
    public:
        mqttConnector():mosqpp::mosquittopp(){

        }
        virtual ~mqttConnector() = default;

        //MQTT Callback functions
        void on_connect(int rc) override;
		void on_disconnect(int rc) override;
		void on_publish(int mid) override;
		virtual void on_message(const struct mosquitto_message* message) override;
		void on_subscribe(int mid, int qos_count, const int* granted_qos) override;
		void on_unsubscribe(int mid) override;
		void on_log(int level, const char* str) override;
		void on_error() override;

    protected:
        string _manage_topic {""};
        bool _connected = false;
        string _broker_address { "127.0.0.1" };
        int _broker_port {1883};
        string _mqtt_pub_topic = {"undefined"};
        int _mqtt_pub_qos = 2;
        int _mqtt_keep_alive = {60};
        vector<string> _mqtt_sub_topics;
}


int main(int argc, char* argv[])
{
    boost::thread _asyncReadThread;
    boost::asio::io_service _io_service;
    boost::mutex _mutex;

    spdlog::stdout_color_st("console");

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

    int optc = 0;
    string _device_port = "/dev/ttyS0";
    int _baudrate = 9600;
    int _databits = 8;
    int _parity = 0;
    int _stopbits = 1;
    int _flowcontrol = 0;

    while((optc=getopt(argc, argv, "p:b:h"))!=-1)
    {
        switch(optc){
            case 'p': { /* device port */
            spdlog::info("Use device port : {}", optarg);
            _device_port = optarg;
            }
            break;

            case 'b': { /* baudrate */
            spdlog::info("Use port baudrate : {}", optarg);
            _baudrate = atoi(optarg);
            }
            break;

            case 'h': /* help */
            default:
            cout << fmt::format("MEX loadcell (built {}/{})", __DATE__, __TIME__) << endl;
            cout << "Usage: mex_loadcell [-p port] [-b baudrate]" << endl;
            exit(EXIT_FAILURE);
            break;
        }
    }

    try{
        boost::system::error_code ec;

        if(!_port){
            _port = new boost::asio::serial_port(_io_service);
            _port->open(_device_port, ec);
            if(ec){
                spdlog::error("{} port open failed : {}", _device_port, ec.message());
                ::terminate();
            }
            _port->set_option(boost::asio::serial_port_base::baud_rate((unsigned int)_baudrate));
            _port->set_option(boost::asio::serial_port_base::character_size(_databits)); //databits
            _port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one)); // stopbits 1
            _port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none)); //parity none
            _port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none)); //flowcontrol none

        }
            
        //do here
        pause(); //wait until getting SIGINT
    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}