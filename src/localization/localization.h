//
// Created by wzq on 18-7-7.
//

#ifndef ATHENA_LOCALIZATION_H
#define ATHENA_LOCALIZATION_H

#include "../sensor/camera/setero_camera.h"

namespace athena {
namespace localization {

class Localization {
public:
    Localization():run_(true){}
    ~Localization() = default;

    bool init();
    void localization_thread_func();

public:
    std::unique_ptr<sensor::SeteroCamera> setero_camera_;
    std::string name_;
    std::thread localization_thread_;
    // thread run or not
    bool run_;

private:
    cv::Mat left,right;
};

}
}
#endif //ATHENA_LOCALIZATION_H
