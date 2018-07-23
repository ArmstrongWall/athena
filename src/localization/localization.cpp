//
// Created by wzq on 18-7-7.
//

#include "localization.h"
#include "config/athena_gflags.cpp"


namespace athena {
namespace localization {

bool Localization::init(){
    //setero_camera_.reset(new sensor::SeteroCamera());
    //setero_camera_->init();
    //localization_thread_ = std::thread(&Localization::localization_thread_func, this);
    std::cout << FLAGS_unreal_conf_file;
    return true;
}

void Localization::parseArgument() {

}

void Localization::localization_thread_func() {
    while (run_) {
        setero_camera_->bino->Grab();
        setero_camera_->bino->getRectImage(left_image_, right_image_);
        cv::imshow("image_left", left_image_);
        cv::imshow("image_right", right_image_);
        cv::waitKey(2);
    }

    run_ = false;

}

}
}