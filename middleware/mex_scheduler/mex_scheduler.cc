/**
 * @file    main.cc
 * @brief   MEX Application for step program
 * @author  bh.hwang@iae.re.kr
 */

#include "mex_scheduler.hpp"
#include "step_mqtt.hpp"

using namespace std;


int _pub_inteval_sec = 1;
bool _terminate = false;
static int _state = _STATE_::READY;
static bool _on_paused = false;

struct _step_tag {
    long current_step = 0;
    long total_time_sec = 0;
    long time_elapsed_sec = 0;
    long total_steps = 0;
    long current_rpm = 0;
    long max_rpm = 0;
    long accdec = 0;

    double product_size = 0.0;
    double roller_size = 0.0;
    double ratio = 1.0;

    deque<json> step_container;
    json raw;
    void clear(){
        this->current_step = 0;
        this->total_time_sec = 0; //total time (user set the value in hours)
        this->time_elapsed_sec = 0;
        this->current_rpm = 0;
        this->max_rpm = 0;
        this->accdec = 0;
        this->product_size = 0.0;
        this->roller_size = 0.0;
        this->ratio = 1.0;
        raw.clear();
        step_container.clear();
    }
};

static _step_tag g_step_info;
static float g_loadcell_value = 0.0; //(added 22.06.24)

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
                g_step_info.clear();

                //motor off & cylinder stop
                if(g_mqtt){

                    json moveset = {{"command", "move_stop"}};
                    string str_moveset = moveset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_moveset.size(), str_moveset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while move stop");
                    }

                    //(added 22.06.24)
                    json cylinderset = {{"command", "cylinder_stop"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder move stop");
                    }

                    json motorset = {{"command", "motor_off"}};
                    string str_motorset = motorset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_motorset.size(), str_motorset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while motor off");
                    }
                    
                    spdlog::info("Set PLC Motor OFF : {}", str_motorset);
                    _state++;
                }
                else {
                    spdlog::error("STEP perform failed while motor turning off..");
                }

                publish_step_state(g_mqtt, _STATE_::STOP);    //notify the stop state
                _state = _STATE_::READY;
            } break;

            /* step program pause */
            case _STATE_::PAUSE: {
                _on_paused = true;

                json motorset = {{"command", "move_stop"}};
                string str_motorset = motorset.dump();
                if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_motorset.size(), str_motorset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                    spdlog::error("STEP perform error while motor move stop");
                }

                //(added 22.06.24)
                json cylinderset = {{"command", "cylinder_stop"}};
                string str_cylinderset = cylinderset.dump();
                if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                    spdlog::error("STEP perform error while cylinder move stop");
                }

                publish_step_state(g_mqtt, _STATE_::PAUSE);    //notify the stop state
                _state++;
            } break;

            case _STATE_::PAUSE+1: {
                //waiting
                spdlog::info("holding pause state.. waiting for start(=resume) command");
            } break;

            case _STATE_::START: { //start init

                if(_on_paused){
                    _state = _STATE_::START+1;
                    _on_paused = false;
                    break;
                }

                if(g_mqtt){
                    //motor on
                    json motorset = {{"command", "motor_on"}};
                    string str_motorset = motorset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_motorset.size(), str_motorset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while motor on");
                    }
                    spdlog::info("Set PLC Motor ON : {}", str_motorset);

                    //cylinder moves upward (added 22.06.24)
                    json cylinderset = {{"command", "cylinder_up"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder moves upward");
                    }
                    spdlog::info("Set Cylinder Up : {}", cylinderset);

                    _state++;
                }
                else {
                    spdlog::error("STEP perform failed while initializing..");
                    _state = _STATE_::STOP;
                }
                
            } break;


            // cylinder moves upward
            case _STATE_::START+1:{
                if(g_loadcell_value<=0.0){
                    //cylinder moves upward (added 22.06.24)
                    json cylinderset = {{"command", "cylinder_stop"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder move stop");
                    }
                    spdlog::info("Set Cylinder Stop : {}", cylinderset);

                    _state++;
                }
            } break;

            /* step program start */
            case _STATE_::START+2: { //set rpm parameters responding to acc/dec (increase mode)

                // reach the end of steps
                if(g_step_info.current_step>=(long)g_step_info.step_container.size()){
                    _state = _STATE_::STOP;
                    break;
                }
                    
                //do start with status
                if(g_mqtt && !g_step_info.step_container.empty()){
                    
                    // getting current step data
                    json cur_step = g_step_info.step_container[g_step_info.current_step]; //get current step

                    // if meet 'goto' step, read target id then moves
                    int command_id = cur_step["command"].get<int>();
                    if(command_id==3){
                        if(cur_step.contains("goto")){
                            const long goto_step = cur_step["goto"].get<long>();
                            g_step_info.current_step = goto_step-1; //0 indexing
                        }
                        else {
                            spdlog::error("Nothing to jump to step index");
                            _state = _STATE_::STOP;
                        }
                        break;
                    }

                    //const double ratio = (double)g_step_info.product_size/(double)g_step_info.roller_size;
                    // const double cur_step_accdec = ratio*(double)target_accdec; //real accdec
                    // const double cur_step_rpm = ratio*(((double)target_speed-40)*5.3)+1030); //real speed

                    spdlog::info("Current Step : {}", cur_step.dump());
                    //set RPM to PLC
                    if(command_id!=0){
                        if(cur_step.contains("accdec") && cur_step.contains("speed")){
                            const long target_accdec = cur_step["accdec"].get<long>();
                            const long target_speed = cur_step["speed"].get<long>();

                            g_step_info.current_rpm += target_accdec;
                            //rpm saturation
                            if(g_step_info.current_rpm>=target_speed)
                                g_step_info.current_rpm = target_speed;

                            spdlog::info("Set PLC Target RPM parameter : {}",g_step_info.current_rpm);
                        }
                        else {
                            _state = _STATE_::STOP;
                            spdlog::error("Could not find ACC/DEC value");
                            break;
                        }
                    }

                    //write motor rpm
                    json paramset = {{"command", "param_set"}, {"rpm", g_step_info.current_rpm},{"product_size",g_step_info.product_size}, {"roller_size", g_step_info.roller_size}, {"ratio", g_step_info.ratio}};
                    string str_param = paramset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_param.size(), str_param.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while parameter set");
                    }
                    spdlog::info("Set PLC RPM Parameter : {}", str_param);

                    //write motor control
                    json plc_controlset;
                    switch(command_id){
                        case 0: { //stop
                            plc_controlset["command"] = "move_stop";
                            //_state++;
                        } break;
                        case 1: { //cw
                            plc_controlset["command"] = "move_cw";
                        } break;
                        case 2: { //ccw
                            plc_controlset["command"] = "move_ccw";
                        } break;
                    }
                    string str_plc_controlset = plc_controlset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_plc_controlset.size(), str_plc_controlset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while PLC control set");
                    }

                    //write sload control
                    bool sload  = cur_step["sload"].get<bool>();
                    json relay_controlset;
                    if(sload)
                        relay_controlset["p1"] = 1; //on
                    else
                        relay_controlset["p1"] = 0; //off
                    string str_relay_controlset = relay_controlset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_RELAY_CONTROL_TOPIC, str_relay_controlset.size(), str_relay_controlset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while PLC control set");
                    }

                    //cylinder moves upward (added 22.06.24)
                    json cylinderset = {{"command", "cylinder_down"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder move downward");
                    }
                    spdlog::info("Set Cylinder Down : {}", cylinderset);
                    
                    // reach the target speed, then moves next step (if not move_stop)
                    if(cur_step.contains("speed") && command_id!=0){
                        const long target_speed = cur_step["speed"].get<long>();
                        if(g_step_info.current_rpm>=(long)(target_speed)){
                            _state++;
                        }
                    }
                    else if(command_id==0){ //move stop
                        spdlog::info("enter move stop...");
                        _state++;
                    }
                }
                else {
                    spdlog::error("STEP perform failed while starting..");
                    _state = _STATE_::STOP; //abnormally occurred
                }
            } break;

            case _STATE_::START+3: { //waiting until time reach, do 
                spdlog::info("start+2 step");
                static long elapsed = 0;
                elapsed++;
                
                json cur_step = g_step_info.step_container[g_step_info.current_step]; //get current step
                const long required_time_sec = cur_step["time"].get<long>();
                spdlog::info("Starting Time Elapsed : {}/{}", elapsed, required_time_sec);

                if(elapsed>=required_time_sec){
                    spdlog::info("time end");
                    _state++; //move next step
                    elapsed = 0;
                }
    
            } break;

            case _STATE_::START+4: {
                if(g_mqtt){
                    //cylinder moves upward (added 22.06.24)
                    json cylinderset = {{"command", "cylinder_stop"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder move stop");
                    }
                    spdlog::info("Set Cylinder Stop : {}", cylinderset);
                    _state++; //move next step
                }
                else {
                    spdlog::error("STEP perform failed while cylinder stop");
                    _state = _STATE_::STOP;
                }

            } break;

            case _STATE_::START+5:{ //decrease mode
                if(g_mqtt && !g_step_info.step_container.empty()){
                    json cur_step = g_step_info.step_container[g_step_info.current_step]; //get current step
                    const double ratio = (double)g_step_info.product_size/(double)g_step_info.roller_size*g_step_info.ratio;
                    int command_id = cur_step["command"].get<int>();

                    spdlog::info("Current Step : {}", cur_step.dump());
                    //set RPM to PLC
                    if(command_id!=0){
                        if(cur_step.contains("accdec")){
                            const long target_accdec = cur_step["accdec"].get<long>();
                            const double cur_step_accdec = ratio*(double)target_accdec; //real accdec
                            g_step_info.current_rpm -= cur_step_accdec;
                            //rpm saturation
                            if(g_step_info.current_rpm<=0)
                                g_step_info.current_rpm = 0;
                        }
                        else {
                            _state = _STATE_::STOP;
                            spdlog::error("Could not find ACC/DEC value");
                            break;
                        }

                        //write motor rpm
                        json paramset = {{"command", "param_set"}, {"rpm", g_step_info.current_rpm},{"product_size",g_step_info.product_size}, {"roller_size", g_step_info.roller_size}, {"ratio", g_step_info.ratio}};
                        string str_param = paramset.dump();
                        if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_param.size(), str_param.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                            spdlog::error("STEP perform error while parameter set");
                        }
                    }

                    //write motor control
                    json plc_controlset;
                    switch(command_id){
                        case 0: { //stop
                            plc_controlset["command"] = "move_stop";
                        } break;
                        case 1: { //cw
                            plc_controlset["command"] = "move_cw";
                        } break;
                        case 2: { //ccw
                            plc_controlset["command"] = "move_ccw";
                        } break;
                    }
                    string str_plc_controlset = plc_controlset.dump();

                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_plc_controlset.size(), str_plc_controlset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while PLC control set");
                    }

                    //cylinder moves upward (added 22.06.24)
                    json cylinderset = {{"command", "cylinder_up"}};
                    string str_cylinderset = cylinderset.dump();
                    if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_PLC_CONTROL_TOPIC, str_cylinderset.size(), str_cylinderset.c_str(), 2, false)!=MOSQ_ERR_SUCCESS){
                        spdlog::error("STEP perform error while cylinder moves upward");
                    }
                    spdlog::info("Set Cylinder Up : {}", cylinderset);

                    // reach the 0 speed, then moves next step
                    if(g_step_info.current_rpm<=0 || command_id==0){
                        g_step_info.current_step++;
                        _state = _STATE_::START+1;
                    }
                }
                else {
                    spdlog::error("STEP perform failed while starting(decelerating)");
                    _state = _STATE_::STOP;
                }

            } break;
        }

        //status update & publish
        if(_state>=_STATE_::START && _state<=_STATE_::START+9){
            json step_status = {
                {"step_size", g_step_info.step_container.size()},
                {"step_current", g_step_info.current_step+1},
                {"total_time", g_step_info.total_time_sec},
                {"current_elapsed", g_step_info.time_elapsed_sec},
                {"state", "start"}
            };
            string str_step_status = step_status.dump();
            if(mosquitto_publish(g_mqtt, nullptr, MEX_STEP_STATUS_TOPIC, str_step_status.size(), str_step_status.c_str(), 2, false)!=MOSQ_ERR_SUCCESS)
                spdlog::error("Step Status Update failed");

            //working time check
            if(g_step_info.time_elapsed_sec>=g_step_info.total_time_sec){
                _state = _STATE_::STOP;
            }
        }

        // increase the time elapsed
        g_step_info.time_elapsed_sec++;
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

    //1. clear step information
    g_step_info.clear();

    //2. copy raw data & calc total working time
    g_step_info.raw = data; //raw steps
    if(data.find("wtime")!=data.end()) { 
        g_step_info.total_time_sec = (long)(data["wtime"].get<double>()*60*60); 
        spdlog::info("Set STEP Program name : {}", data["name"]);
        spdlog::info("Total Time : {}sec", g_step_info.total_time_sec);
    } //total testing time

    json steps = data["steps"];
    spdlog::info("Number of Steps : {}", steps.size());

    for(auto& step: steps){ 
        g_step_info.step_container.emplace_back(step); 
    } //parse steps

    g_step_info.total_steps = (long)g_step_info.step_container.size(); //step size
    if(steps.find("limit_rpm_max")!=steps.end()){ 
        g_step_info.max_rpm = (long)(steps["limit_rpm_max"].get<long>()); 
    } //max rpm

    //general setting info.
    g_step_info.product_size = (double)(data["product_size"].get<double>());
    g_step_info.roller_size = (double)(data["roller_size"].get<double>());
    g_step_info.ratio = (double)(data["ratio"].get<double>());

}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
    //processing for publishing step data
	bool match_program_topic = false;
	mosquitto_topic_matches_sub(MEX_STEP_PROGRAM_TOPIC, message->topic, &match_program_topic);

    if(match_program_topic){
        try{
            json ctrl_data = json::parse((char*)message->payload);

            spdlog::info("mqtt : {}", ctrl_data.dump());
            
            if(ctrl_data.contains("command")){
                string cmd = ctrl_data["command"].get<string>();
                if(g_commandset.find(cmd)!=g_commandset.end()){
                    int icmd = g_commandset[cmd];
                    switch(icmd){
                        case _STATE_::READY: {
                            spdlog::info("Waiting Step program...");
                        } break; //ready(= idle)
                        case _STATE_::STOP: {
                            _state = _STATE_::STOP;
                            spdlog::info("Stop Step program...");
                        } break; //stop
                        case _STATE_::PAUSE: {
                            if(_state!=_STATE_::PAUSE){
                                spdlog::info("Pause Step program...");
                            }
                            else
                                spdlog::warn("Already opn start state...");
                        } break; //pause
                        case _STATE_::START: {
                            if(_state!=_STATE_::START){
                                spdlog::info("Start Step program...");
                                if(!_on_paused)
                                    parse_steps(ctrl_data["data"]);
                            }
                            else
                                spdlog::warn("Already on start state..");
                        } break; //start
                    }

                    _state = icmd;
                }
            }
        }
        catch(json::parse_error& e){
            spdlog::error("Control set parse error : {}", e.what());
        }
    }

    //processing for publishing loadcell data (added 22.06.24)
    bool match_loadcell = false;
    mosquitto_topic_matches_sub(MEX_LOADCELL_VALUE_TOPIC, message->topic, &match_loadcell);
    if(match_loadcell){
        try{
            json loadcell_data = json::parse((char*)message->payload);
            if(loadcell_data.contains("load")){
                g_loadcell_value = loadcell_data["load"].get<float>();
            }
            
        }
        catch(json::parse_error& e){
            spdlog::error("Loadcell value parse error : {}", e.what());
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
                mosquitto_subscribe(g_mqtt, NULL, MEX_LOADCELL_VALUE_TOPIC, 2); //(added 22.06.24)
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
