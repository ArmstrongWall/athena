/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

//
// Created by wzq on 18-7-2.
//

#ifndef MODULES_UNREAL_CHASSIS_VEHICLE_H
#define MODULES_UNREAL_CHASSIS_VEHICLE_H

#include "modules/common/apollo_app.h"
#include "modules/common/macro.h"
#include "modules/common/monitor_log/monitor_log_buffer.h"

#include "modules/unreal/common/usim_api.h"
#include "modules/unreal/common/until.h"

#include "modules/canbus/proto/chassis.pb.h"
#include "modules/unreal/chassis/client/client.h"

#include "modules/common/adapters/adapter_manager.h"
#include "modules/unreal/common/unreal_gflags.h"

#include "modules/control/proto/control_cmd.pb.h"

namespace apollo {
namespace unreal {
namespace chassis {


template<typename MessageType,template<typename MessageType>class Client>
class Vehicle{
public:
    Vehicle() = default;
    Vehicle(const apollo::drivers::gnss::config::Config &config_,
            Client<MessageType> client) :run_(true),client_(client){
        client_.init(config_);
        publish_chassis_thread_ = std::thread(&Vehicle::publish_chassis_thread_func, this);

//        const auto &update_func = [this] { write_chassis_thread_func(); };
//        write_chassis_thread_.reset(new std::thread(update_func));

        control_command_subscriber =
                nh.subscribe("/apollo/control", 10, &Vehicle::OnControlCommand, this);
    }

    void read_chassis_data(Client<MessageType> client_){
        client_.read(message_type_);
    }
    void write_chassis_data(Client<MessageType> client_){
        client_.write();
    }

private:
    ros::Publisher _chassis_publisher;
    ros::Subscriber control_command_subscriber;

    apollo::canbus::Chassis chassis_;
    MessageType message_type_;
    Client<MessageType> client_;

    std::thread                     publish_chassis_thread_;
    std::unique_ptr<std::thread>    write_chassis_thread_;

    bool run_;

    void publish_chassis_thread_func();
    void OnControlCommand(const apollo::control::ControlCommand &control_command);
    void write_chassis_thread_func();

    ros::NodeHandle nh ;
    ros::NodeHandle nh_chassis_write_ ;

};


template<typename MessageType,template<typename MessageType>class Client>
void Vehicle<MessageType,Client>::publish_chassis_thread_func(){

    std::chrono::microseconds period(10000);

    _chassis_publisher = nh.advertise<apollo::canbus::Chassis>("/apollo/canbus/chassis", 64);

    while(run_) {
        auto start_time = std::chrono::system_clock::now();

        message_type_ = client_.read(message_type_);
        chassis_.set_driving_mode(apollo::canbus::Chassis::COMPLETE_AUTO_DRIVE);
        chassis_.set_speed_mps(message_type_.forward_speed);
        chassis_.set_steering_percentage(message_type_.steering_angle *100.0 / 180.0);
        _chassis_publisher.publish(chassis_);

        auto end_time   = std::chrono::system_clock::now();
        auto elapse_time  = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        if (period > elapse_time) {
            std::this_thread::sleep_for(period - elapse_time);
        } else {
            std::cout << "Too much time for calculation: " << elapse_time.count() << std::endl;
        }
        auto end2   = std::chrono::system_clock::now();
        std::cout<<"thread cost time: "<< std::chrono::duration_cast<std::chrono::microseconds>(end2 - start_time).count() <<std::endl;
    }
}


template<typename MessageType,template<typename MessageType>class Client>
void Vehicle<MessageType,Client>::OnControlCommand(const apollo::control::ControlCommand &control_command){

    MessageType ctrl;
    ctrl.steering_angle = control_command.steering_target()/100.0*35.0;
    ctrl.forward_speed  = control_command.throttle()/100*80.001;

    if(!client_.write(ctrl)) {
        return;
    }

}

}
}
}



#endif //XCREATOR_APOLLO_VEHICLE_H
