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

using namespace std;

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
        //do here
        pause(); //wait until getting SIGINT
    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}