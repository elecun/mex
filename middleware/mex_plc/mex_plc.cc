/**
 * @file    main.cc
 * @brief   MEX Application for control plc
 * @author  bh.hwang@iae.re.kr
 */

#include "mex_plc.hpp"


int _pub_inteval_sec = 1;
vector<char> receive_buf;
bool _terminate = false;
bool _run = true;

// bool g_relay_sload = false;
// bool g_relay_zeroset = false;
// bool g_relay_emerency = false;

/* publish rpm & temperature data */
// void pub_thread_proc(){
//     while(1){

//         //continuously transferring data
//         if(g_mqtt && _run){

//             //2. temperature data publish
//             json _relay_pubdata;
//             _relay_pubdata["relay_sload"] = g_relay_sload;
//             _relay_pubdata["relay_zeroset"] = g_relay_zeroset;
//             _relay_pubdata["relay_emergency"] = g_relay_emerency;

//             string str_relay_data = _relay_pubdata.dump();
//             if(mosquitto_publish(g_mqtt, nullptr, MEX_RELAY_VALUE_TOPIC, str_relay_data.size(), str_relay_data.c_str(), 2, false)!=MOSQ_ERR_SUCCESS)
//                 spdlog::error("Data publish error for Relay data");

//             spdlog::info("Sload :{}, Zeroset :{}, Emergency :{}", g_relay_sload, g_relay_zeroset, g_relay_emerency);
//         }

//         boost::this_thread::sleep_for(boost::chrono::seconds(_pub_inteval_sec));
//         if(_terminate)
//             break;
//     }
// }


/* post process */
static void postprocess(json& msg){

    // if(msg.contains("relay_emergency")){
    //     g_relay_emerency = msg["relay_emergency"].get<bool>();
    // }

    // if(msg.contains("relay_sload")){
    //     g_relay_sload = msg["relay_sload"].get<bool>();
    // }

    // if(msg.contains("relay_zeroset")){
    //     g_relay_zeroset = msg["relay_zeroset"].get<bool>();
    // }
}


/* MQTT Connection callback */
void connect_callback(struct mosquitto* mosq, void *obj, int result)
{
    spdlog::info("Connected to broker = {}", result);
}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
    //processing for publishing plc data
	bool match_plc_topic = false;
	mosquitto_topic_matches_sub(MEX_PLC_CONTROL_TOPIC, message->topic, &match_plc_topic);

    if(match_plc_topic){
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
            
            map<string, int> commandset = {
                {"motor_off", 0},
                {"motor_on", 1},
                {"cylinder_up", 2},
                {"cylinder_down", 3},
                {"move_cw", 4},
                {"move_ccw", 5},
                {"move_stop", 6},
                {"param_set", 7},
                {"test_start", 8},
                {"test_pause", 9},
                {"test_stop", 10},
                {"program_connect", 11},
                {"program_verify", 12},
                {"cylinder_stop", 13},
            };

            if(ctrl_data.contains("command")){
                string _command = ctrl_data["command"].get<string>();
                if(commandset.find(_command)==commandset.end())
                    return;

                if(g_pSerial->is_open()){
                    subport* sp = g_pSerial->get_subport("plc");
                    if(sp){
                        plc* r = dynamic_cast<plc*>(sp);
                        if(r){
                            switch(commandset[_command]){
                                case 0: {  r->motor_off();  } break;
                                case 1: {  r->motor_on(); } break;
                                case 2: {  r->cylinder_up(); } break;
                                case 3: {  r->cylinder_down(); } break;
                                case 4: {  r->move_cw(); } break;
                                case 5: {  r->move_ccw(); } break;
                                case 6: {  r->move_stop(); } break;
                                case 7: {  r->param_set(); } break;
                                case 8: {  r->test_start(); } break;
                                case 9: {  r->test_pause(); } break;
                                case 10: {  r->test_stop(); } break;
                                case 11: {  r->program_connect(); } break;
                                case 12: {  r->program_verify(); } break;
                                case 13: {  r->cylinder_stop(); } break;
                                default:
                                    spdlog::warn("Unknown PLC Command : {}", _command);
                            }
                        }
                    }
                }
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
    string _device_port = "/dev/ttyAP1"; //defualt port
    int _baudrate = 9600;   //default baudrate

    string _mqtt_broker = "0.0.0.0";

    while((optc=getopt(argc, argv, "p:b:t:i:h"))!=-1)
    {
        switch(optc){
            case 'p': { _device_port = optarg; } break; /* device port */
            case 'b': { _baudrate = atoi(optarg); } break; /* baudrate */
            case 't': { _mqtt_broker = optarg; } break; /* target ip to pub */
            case 'i' : { _pub_inteval_sec = atoi(optarg); } break; /* mqtt publish interval */
            case 'h':
            default:
                cout << fmt::format("MEX Temperatures & RPM (built {}/{})", __DATE__, __TIME__) << endl;
                cout << "Usage: mex_tpm [-p port] [-b baudrate] [-t broker ip] [-i interval]" << endl;
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
                mosquitto_subscribe(g_mqtt, NULL, MEX_PLC_CONTROL_TOPIC, 2);
                mosquitto_loop_start(g_mqtt);
            }
        }
        
        if(!g_pSerial){
            g_pSerial = new serialbus(_device_port.c_str(), _baudrate);
            if(g_pSerial->is_open()){
                g_pSerial->set_processor(postprocess);
                g_pSerial->add_subport("plc", new plc("plc", 1));
                g_pSerial->start(); // start PLC
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