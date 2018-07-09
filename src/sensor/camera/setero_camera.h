//
// Created by wzq on 18-7-9.
//

#ifndef ATHENA_SETERO_H
#define ATHENA_SETERO_H

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "BinoCamera.h"

namespace athena {
namespace sensor {

class SeteroCamera {
public:
    SeteroCamera() = default;
    ~SeteroCamera() = default;
    bool init(){
        paraList.devPath = "/dev/video1";
        paraList.intParameterPath = "../thirdparty/params/intrinsics.yml";
        paraList.extParameterPath = "../thirdparty/params/extrinsics.yml";
        bino.reset(new BinoCamera(paraList));
    }

    std::shared_ptr<BinoCamera> bino;
    BinoCameraParameterList paraList;
};

}
}


#endif //ATHENA_SETERO_H
