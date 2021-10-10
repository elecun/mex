
#include "instance.hpp"

namespace mex::app {

    #define CONFIG_PATH   config["registry"]["path"]
    #define CONFIG_HOSTNAME config["registry"]["hostname"]
    #define CONFIG_TASKS    config["required"]["tasks"] //forced (remove not allowable)
    #define CONFIG_SYSTEM   config["system"]

    //app initialize
    bool initialize(const char* conf_file){

        spdlog::info("* Process ID = {}", getpid());
        spdlog::info("* System CPUs = {}", get_nprocs());
        spdlog::info("* System Clock Ticks = {}", sysconf(_SC_CLK_TCK));

        json config;
        try {
            if(!util::exist(conf_file)){
                spdlog::error("Configuration file does not exist");
                return false;
            }

            std::ifstream file(conf_file);
            file >> config;
        }
        catch(const json::exception& e){
            spdlog::error("Config file load failed : {}", e.what());
            return false;
        }
        catch(std::ifstream::failure& e){
            spdlog::error("Configuration file load error : {}", e.what());
            return false;
        }

        //set dumped system configuration into the registry
        if(config.find("system")!=config.end()){
            for(json::iterator it = CONFIG_SYSTEM.begin(); it != CONFIG_SYSTEM.end(); ++it){
                registry->insert(it.key(), std::make_any<std::string>(it.value().dump()));
            }
        }

        //insert registry
        for(json::iterator it = CONFIG_PATH.begin(); it != CONFIG_PATH.end(); ++it) {
            if(it.value().is_string()){
                registry->insert(it.key(), std::make_any<std::string>(CONFIG_PATH[it.key()].get<std::string>()));
                spdlog::info("+ Registry : {}={}",it.key(),(registry->get<std::string>(it.key())));
            }
        }

        //getting hostname (default hostname will be set by configuration file)
        char hostname[256] = {0,};
        if(!gethostname(hostname, sizeof(hostname))){
            registry->insert("HOST_NAME", std::make_any<std::string>(hostname));
            spdlog::info("+ Registry : Hostname = {}", hostname);
        }
        else {
            std::string host = CONFIG_HOSTNAME.get<std::string>();
            spdlog::warn("Cannot be recognized the hostname. Default hostname({}) will be set.", host);
        }
            
        //install default tasks
        vector<string> default_tasks = CONFIG_TASKS.get<std::vector<string>>();
        for(string& task:default_tasks){
            edge_task_manager->install(task.c_str());
        }
        spdlog::info("Totally installed : {}", edge_task_manager->size());
    
        return true;
    }

    //start edge
    void run(){
        edge_task_manager->run();
    }

    void cleanup(){
        edge_task_manager->uninstall();
    }

} //namespace edge