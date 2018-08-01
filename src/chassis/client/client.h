//
// Created by wzq on 18-7-3.
//


#ifndef MODULES_UNREAL_CHASSIS_CLIENT_H
#define MODULES_UNREAL_CHASSIS_CLIENT_H

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include "modules/common/apollo_app.h"
#include "modules/common/macro.h"
#include "modules/common/monitor_log/monitor_log_buffer.h"

#include "modules/unreal/common/usim_api.h"
#include "modules/drivers/gnss/proto/config.pb.h"

namespace apollo {
namespace unreal {
namespace chassis {

using apollo::common::ErrorCode;
using apollo::common::Status;

template<typename MessageType>
class Client {
public:
    Client() = default;
    ~Client() = default;
    //virtual apollo::common::Status init(const apollo::drivers::gnss::config::Config &config_) = 0;
};

template<typename MessageType> class RpcClient : public Client<MessageType>{
public:
    RpcClient() = default;
    ~RpcClient() = default;
    apollo::common::Status init(const apollo::drivers::gnss::config::Config &config_);//
    MessageType& read(MessageType &frame);
    bool         write(const MessageType &frame);

private:
    std::shared_ptr<usim::RemoteClient> usim_client_;

};

template<typename MessageType>
Status RpcClient<MessageType>::init(const apollo::drivers::gnss::config::Config &config_) {
    usim_client_.reset(new (std::nothrow) usim::RemoteClient(
            config_.data().rpc().address().c_str(),
            config_.data().rpc().port(),
            config_.data().rpc().timeout_s()*1000));
    if (nullptr == usim_client_.get()) {
        std::string ERROR_MESSAGE = "Chassis RpcClient new failed!";
        ROS_ERROR_STREAM(ERROR_MESSAGE);
        return Status(ErrorCode::CONTROL_ERROR, ERROR_MESSAGE);
    }

    int32_t ret = usim_client_->ConfirmConnection();
    if (ret) {
        std::string ERROR_MESSAGE ="Chassis RpcClient Connection failed!";
        ROS_ERROR_STREAM(ERROR_MESSAGE);
        return Status(ErrorCode::CONTROL_ERROR, ERROR_MESSAGE);
    }
    //std::cout<< "Chassis RpcClient init ok"<<std::endl;

//    // waiting server side simulation in running state
//    std::string err_msg;
//    ret = usim_client_->WaitSimRunning(&err_msg);
//    if (ret != usim::kRetSuccess) {
//        std::string ERROR_MESSAGE = "Chassis RpcClient  WaitSimRunning ret "+err_msg;
//        ROS_ERROR_STREAM(ERROR_MESSAGE);
//        return Status(ErrorCode::CONTROL_ERROR, ERROR_MESSAGE);
//    }
    return Status::OK();
}

template<typename MessageType>
MessageType& RpcClient<MessageType>::read(MessageType &frame) {
    std::string err_msg;
    int32_t ret = usim_client_->GetVehicleState(&frame, &err_msg);

    if (ret != usim::kRetSuccess) {
        std::string ERROR_MESSAGE = "GetVehicleState ret failed, ret:"+err_msg;
        ROS_ERROR_STREAM(ERROR_MESSAGE);
    }
    return frame;
}

template<typename MessageType>
bool RpcClient<MessageType>::write(const MessageType &frame) {

    const usim::VehicleControls ctrl = {
            0,         // id
            0,         // time_stamp
            1,         // mode
            frame.steering_angle ,  // steering_angle, need set
            0,         // throttle
            0,         // brake
            1,         // gear
            frame.forward_speed   // expect_speed, need set, max speed is 5 m/s
    };

    std::string err_msg;
    int32_t ret = usim_client_->SetVehicleControls(ctrl, &err_msg);

    if (ret != usim::kRetSuccess) {
        std::string ERROR_MESSAGE = "SetVehicleControls failed, ret:"+err_msg;
        ROS_ERROR_STREAM(ERROR_MESSAGE);
        return false;
    }
    return true;
}


}
}
}





#endif //XCREATOR_APOLLO_CLIENT_H
