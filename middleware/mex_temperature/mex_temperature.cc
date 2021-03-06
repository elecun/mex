/**
 * @file    main.cc
 * @brief   MEX Application for read temperature data from serial comm.
 * @author  bh.hwang@iae.re.kr
 */

#include "mex_temperature.hpp"


int _pub_inteval_sec = 1;
vector<char> receive_buf;
bool _terminate = false;
bool _run = true;

float g_temperature_1 = 0.0;
float g_temperature_2 = 0.0;
float g_temperature_3 = 0.0;

/* publish rpm & temperature data */
void pub_thread_proc(){
    while(1){

        //continuously transferring data
        if(g_mqtt && _run){

            //2. temperature data publish
            json _temp_pubdata;
            _temp_pubdata["temperature_1"] = g_temperature_1;
            _temp_pubdata["temperature_2"] = g_temperature_2;
            _temp_pubdata["temperature_3"] = g_temperature_3;
            string str_temp_data = _temp_pubdata.dump();
            if(mosquitto_publish(g_mqtt, nullptr, MEX_TEMPERATURE_VALUE_TOPIC, str_temp_data.size(), str_temp_data.c_str(), 2, false)!=MOSQ_ERR_SUCCESS)
                spdlog::error("Data publish error for Temperature data");

            spdlog::info("Temperature 1 :{}, Temperature 2 :{}, Temperature 3 :{}", g_temperature_1, g_temperature_2, g_temperature_3);
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(_pub_inteval_sec));
        if(_terminate)
            break;
    }
}


/* post process */
static void postprocess(json& msg){

    if(msg.contains("temperature1")){
        g_temperature_1 = msg["temperature1"].get<float>();
    }

    if(msg.contains("temperature2")){
        g_temperature_2 = msg["temperature2"].get<float>();
    }

    if(msg.contains("temperature3")){
        g_temperature_3 = msg["temperature3"].get<float>();
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
    bool match_temp_topic = false;
	mosquitto_topic_matches_sub(MEX_TEMPERATURE_CONTROL_TOPIC, message->topic, &match_temp_topic);

    if(match_temp_topic){
        try{
            json ctrl_data = json::parse((char*)message->payload);
            if(ctrl_data.contains("interval")){
                _pub_inteval_sec = ctrl_data["interval"].get<int>();
                spdlog::info("Changed interval : {}", _pub_inteval_sec);
            }
            if(ctrl_data.contains("run")){
                _run = ctrl_data["run"].get<int>();
                spdlog::info("Change running status : {}(0=stop, 1=start)", _run);
            }
        }
        catch(json::parse_error& e){
                spdlog::error("Control set parse error : {}", e.what());
        }
    }
}

/* termination */
void terminate() {

    if(g_pSerial){
        g_pSerial->stop();
        delete g_pSerial;
        g_pSerial = nullptr;
    }

    _terminate = true;

    mosquitto_loop_stop(g_mqtt, true);
    mosquitto_destroy(g_mqtt);
    mosquitto_lib_cleanup();

    //g_pub_thread->join();

    spdlog::info("Successfully terminated");
    exit(EXIT_SUCCESS);
}

/* event catch */
void cleanup(int sig) {
  switch(sig){
    case SIGSEGV: { spdlog::warn("Segmentation violation"); } break;
    case SIGABRT: { /*spdlog::warn("Abnormal termination");*/ } break;
    case SIGKILL: { spdlog::warn("Process killed"); } break;
    case SIGBUS: { spdlog::warn("Bus Error"); } break;
    case SIGTERM: { spdlog::warn("Termination requested"); } break;
    case SIGINT: { spdlog::warn("interrupted"); } break;
    default:
      spdlog::info("Cleaning up the program");
  }
  ::terminate(); 
}

/* program signal settings */
void signal_set(){
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
}


/* Entry*/
int main(int argc, char* argv[])
{  
    signal_set();
    spdlog::stdout_color_st("console");

    int optc = 0;
    string _device_port = "/dev/ttyAP0"; //defualt port
    int _baudrate = 9600;   //default baudrate

    string _mqtt_broker = "0.0.0.0";

    while((optc=getopt(argc, argv, "p:b:t:i:h"))!=-1)
    {
        switch(optc){
            case 'p': { _device_port = optarg; } break; /* device port */
            case 'b': { _baudrate = atoi(optarg); } break; /* baudrate */
            case 't': { _mqtt_broker = optarg; } break; /* target ip to pub */
            case 'i': { _pub_inteval_sec = atoi(optarg); } break; /* mqtt publish interval */
            case 'h':
            default:
                cout << fmt::format("MEX Temperatures (built {}/{})", __DATE__, __TIME__) << endl;
                cout << "Usage: mex_temperature [-p port] [-b baudrate] [-t broker ip] [-i interval]" << endl;
                exit(EXIT_FAILURE);
            break;
        }
    }

    // show arguments
    spdlog::info("> set device port : {}", _device_port);
    spdlog::info("> set port baudrate : {}", _baudrate);
    spdlog::info("> set broker IP : {}", _mqtt_broker);
    spdlog::info("> set interval : {}", _pub_inteval_sec);

    try {

        mosquitto_lib_init();
        g_mqtt = mosquitto_new(MQTT_CLIENT_ID, true, 0);

        if(g_mqtt){
		    mosquitto_connect_callback_set(g_mqtt, connect_callback);
		    mosquitto_message_callback_set(g_mqtt, message_callback);
            int mqtt_rc = mosquitto_connect(g_mqtt, _mqtt_broker.c_str(), 1883, 60);
            if(!mqtt_rc){
                spdlog::info("MQTT Connected successfully");
                mosquitto_subscribe(g_mqtt, NULL, MEX_TEMPERATURE_CONTROL_TOPIC, 2);
                mosquitto_loop_start(g_mqtt);
            }
        }
        
        if(!g_pSerial){
            g_pSerial = new serialbus(_device_port.c_str(), _baudrate);
            if(g_pSerial->is_open()){
                g_pSerial->set_processor(postprocess);
                g_pSerial->add_subport(1, new temperature("temperature1", 1));
                g_pSerial->add_subport(2, new temperature("temperature2", 2));
                g_pSerial->add_subport(3, new temperature("temperature3", 3));
                g_pSerial->start();

                g_pub_thread = new boost::thread(&pub_thread_proc); //mqtt publish periodically
            }
        }

        ::pause();

    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}