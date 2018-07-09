//
// Created by wzq on 18-7-7.
//

#include "localization.h"

namespace athena {
namespace localization {

bool Localization::init(){
    setero_camera_.reset(new sensor::SeteroCamera());
    setero_camera_->init();
    localization_thread_ = std::thread(&Localization::localization_thread_func, this);
    return true;
}

void Localization::localization_thread_func() {
    while (run_) {
        setero_camera_->bino->Grab();
        setero_camera_->bino->getRectImage(left, right);
        cv::imshow("image_left", left);
        cv::imshow("image_right", right);
        cv::waitKey(2);
    }

    run_ = false;

}

}
}