/**
 * @file    main.cc
 * @brief   MEX Application for step program
 * @author  bh.hwang@iae.re.kr
 */

#include "mex_scheduler.hpp"
#include "step_mqtt.hpp"

using namespace std;


int _pub_inteval_sec = 1;
vector<char> receive_buf;
bool _terminate = false;
int _run = -1; //idle
string _command = "ready"; //default
static int _state = _STATE_::READY;

json g_steps;
int g_step_idx = 0;
bool g_option_repeat = false;
long g_step_total_time = 0;
long g_step_current_time_elapsed = 0;
long g_next_step_time = 0;


typedef struct _step_tag {
    long current_step = 0;
    long total_time_sec = 0;
    long time_elapsed_sec = 0;
    long total_steps = 0;
    long current_rpm = 0;
    long max_rpm = 0;
    long accdec = 0;
    deque<json> step_list;
    json raw;
    void clear(){
        this->current_step = 0;
        this->total_time_sec = 0; //total time (user set the value in hours)
        this->time_elapsed_sec = 0;
        this->current_rpm = 0;
        this->max_rpm = 0;
        this->accdec = 0;
        raw.clear();
        step_list.clear();
    }
};

static _step_tag g_step_info;

/* publish working state */
void pub_thread_proc(){
    while(1){
        switch(_state){
            case _STATE_::READY: {
                //no work, only waiting for command changing..
                publish_step_state(g_mqtt, _STATE_::READY);    //notify the stop state
            } break;

            /* step program stop */
            case _STATE_::STOP: { //stop
                g_step_total_time = 0;
                g_step_idx = 0;
                g_step_current_time_elapsed = 0;
                g_step_total_time = 0;
                g_next_step_time = 0;
                g_steps.clear();
                g_step_info.clear();

                publish_step_state(g_mqtt, _STATE_::STOP);    //notify the stop state
                _state = _STATE_::READY;
            } break;

            /* step program pause */
            case _STATE_::PAUSE: {
                publish_step_state(g_mqtt, _STATE_::PAUSE);    //notify the stop state
            }

            /* step program start */
            case _STATE_::START: { //set parameters

                //do start with status
                if(g_mqtt && !g_step_info.step_list.empty()){
                    
                    json cur_step = g_step_info.step_list[g_step_info.current_step]; //get current step

                    spdlog::info("Current Step : {}", cur_step.dump());
                    //set RPM to PLC
                    if(cur_step.contains("accdec")){
                        g_step_info.current_rpm += cur_step["accdec"].get<long>();
                        //rpm saturation
                        if(g_step_info.current_rpm>=g_step_info.max_rpm)
                            g_step_info.current_rpm = g_step_info.max_rpm;
                        spdlog::info("Set PLC RPM parameter : {}",g_step_info.current_rpm);
                    }
                    else {
                        _state = _STATE_::STOP;
                        spdlog::error("Could not find ACCDEC value");
                        break;
                    }

                    //write motor rpm by accdec
                    json param = {{"command", "param_set"}, {"rpm", g_step_info.current_rpm}};
                    string str_param = param.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_param.size(), str_param.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while parameter set");
                    }
                    _state++;
                }
                else {
                    _state = _STATE_::STOP;
                }
            } break;
            case _STATE_::START+1: {
                _state = _STATE_::STOP;
            } break;


                // if(g_mqtt && !g_steps.empty())
                // {
                //     // if(g_step_idx<g_steps["steps"].size())
                //     // {

                //     //     json step = g_steps["steps"][g_step_idx];
                //     //     int step_index = step["Step"].get<int>();
                //     //     int step_command = step["Command"].get<int>();
                //     //     int step_time = step["Time"].get<int>();
                //     //     int step_speed = step["Speed"].get<int>();
                //     //     int step_load = step["Load"].get<int>();
                //     //     int step_accdec = step["Acc/Dec"].get<int>();

                //     //     if(g_step_current_time_elapsed>=g_next_step_time){
                //     //         g_next_step_time += step_time;
                //     //         spdlog::info("nex step time : {}", g_next_step_time);

                //     //         //step control publish
                //     //         string str_step_data = step.dump();
                //     //         if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_CONTROL_TOPIC, str_step_data.size(), str_step_data.c_str(), 2, false)==MOSQ_ERR_SUCCESS)
                //     //             spdlog::info("{} : (command :{}), (time:{}), (speed:{}), (load:{}), (acc/dec:{})", step_index, step_command, step_time, step_time, step_speed, step_load, step_accdec);
                //     //     }

                //     //     //step running status publish
                //     //     json step_status;
                //     //     step_status["step_size"] = g_steps["steps"].size();
                //     //     step_status["step_current"] = step_index;
                //     //     step_status["total_time"] = g_step_total_time;
                //     //     step_status["current_elapsed"] = ++g_step_current_time_elapsed;
                //     //     step_status["run"] = _run;
                //     //     string str_step_status = step_status.dump();
                //     //     if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_STATUS_TOPIC, str_step_status.size(), str_step_status.c_str(), 2, false)==MOSQ_ERR_SUCCESS)
                //     //         spdlog::info("step status : {}", step_status.dump());


                //     //     if(g_step_current_time_elapsed>=g_next_step_time){
                //     //         g_step_idx++;
                //     //     }

                //     // }
                    
                //     // if(g_repeat && g_step_idx>=g_steps.size()){
                //     //     g_step_idx = 0;
                //     // }
                //     // if(g_step_current_time_elapsed>=g_step_total_time){
                //     //     if(g_repeat)
                //     //         g_step_idx = 0;
                //     //     else
                //     //         _run = 0;
                //     // }
                    
                // }
            //}
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(_pub_inteval_sec));
        if(_terminate)
            break;
    }
}


/* MQTT Connection callback */
void connect_callback(struct mosquitto* mosq, void *obj, int result)
{
    spdlog::info("Connected to broker = {}", result);
}

/* parse step program */
void parse_steps(json& data){
    g_step_info.clear();

    g_step_info.raw = data; //raw steps
    if(data.find("wtime")!=data.end()) { 
        g_step_info.total_time_sec = (long)(data["wtime"].get<double>()*60*60); 
        spdlog::info("Total Time : {}", g_step_info.total_time_sec);
    } //total testing time

    json steps = data["steps"];
    spdlog::info("parsed : {}", steps);
    spdlog::info("size : {}", steps.size());

    for(auto& step: steps){ 
    //     g_step_info.step_list.emplace_back(step); 
        spdlog::info("> STEP : {}", step.dump());
    } //parse steps

    g_step_info.total_steps = (long)g_step_info.step_list.size(); //step size
    if(steps.find("limit_rpm_max")!=steps.end()){ 
        g_step_info.max_rpm = (long)(steps["limit_rpm_max"].get<long>()); 
    } //max rpm

}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
    //processing for publishing step data
	bool match_step_topic = false;
	mosquitto_topic_matches_sub(MEX_STEP_PROGRAM_TOPIC, message->topic, &match_step_topic);

    if(match_step_topic){
        try{
            json ctrl_data = json::parse((char*)message->payload);

            spdlog::info("mqtt : {}", ctrl_data.dump());
            spdlog::info("step size : {}", ctrl_data["steps"].size());
            
            if(ctrl_data.contains("command")){
                string cmd = ctrl_data["command"].get<string>();
                if(g_commandset.find(cmd)!=g_commandset.end()){
                    int icmd = g_commandset[cmd];
                    switch(icmd){
                        case _STATE_::READY: {
                            spdlog::info("Waiting Step program...");
                        } break; //ready(= idle)
                        case _STATE_::STOP: {
                            g_step_info.clear();
                            spdlog::info("Stop Step program...");
                        } break; //stop
                        case _STATE_::PAUSE: {
                            spdlog::info("Pause Step program...");
                        } break; //pause
                        case _STATE_::START: {
                            spdlog::info("Start Step program...");
                            parse_steps(ctrl_data["data"]);
                        } break; //start
                    }

                    _state = icmd;
                    spdlog::info("Change running state : {}", _state);
                }
            }

            // if(ctrl_data.contains("repeat")){
            //     g_repeat = ctrl_data["repeat"].get<bool>();
            // }
        }
        catch(json::parse_error& e){
            spdlog::error("Control set parse error : {}", e.what());
        }
    }
}

/* termination */
void terminate() {

    _terminate = true;

    mosquitto_loop_stop(g_mqtt, true);
    mosquitto_destroy(g_mqtt);
    mosquitto_lib_cleanup();

    g_scheduler->stop();

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
    string _mqtt_broker = "0.0.0.0";

    while((optc=getopt(argc, argv, "p:b:t:i:h"))!=-1)
    {
        switch(optc){
            case 't': { _mqtt_broker = optarg; } break; /* target ip to pub */
            case 'i' : { _pub_inteval_sec = atoi(optarg); } break; /* mqtt publish interval */
            case 'h':
            default:
                cout << fmt::format("MEX STEP Scheduler (built {}/{})", __DATE__, __TIME__) << endl;
                cout << "Usage: mex_scheduler [-t broker ip][-i interval]" << endl;
                exit(EXIT_FAILURE);
            break;
        }
    }

    // show arguments
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
                mosquitto_subscribe(g_mqtt, NULL, MEX_STEP_PROGRAM_TOPIC, 2);
                mosquitto_loop_start(g_mqtt);
            }
        }
        
        if(!g_scheduler){
            g_scheduler = new scheduler();
            g_scheduler->start();

            g_pub_thread = new boost::thread(&pub_thread_proc); //mqtt publish periodically (status)
        }
        ::pause();

    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}