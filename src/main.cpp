#include <iostream>
#include <glog/logging.h>
#include "localization/localization.h"


using namespace athena;

void my_exit_handler(int s)
{
    printf("Caught signal %d\n",s);
    exit(1);
}

void exitThread()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    while(true) pause();
}


int main(int argc, char *argv[]) {

    // hook crtl+C.
    boost::thread exThread = boost::thread(exitThread);

    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "./log";
    LOG(INFO) << "Start";



    std::unique_ptr<localization::Localization> localization;
    localization.reset(new localization::Localization());

    localization->init();

    //while (1);

    return 0;
}