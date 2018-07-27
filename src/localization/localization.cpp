//
// Created by wzq on 18-7-7.
//

#include "localization.h"
#include "config/athena_gflags.cpp"


namespace athena {
namespace localization {

void Localization::settingsDefault(int preset)
{
    printf("\n=============== PRESET Settings: ===============\n");
    if(preset == 0 || preset == 1)
    {
        printf("DEFAULT settings:\n"
               "- %s real-time enforcing\n"
               "- 2000 active points\n"
               "- 5-7 active frames\n"
               "- 1-6 LM iteration each KF\n"
               "- original image resolution\n", preset==0 ? "no " : "1x");

        playbackSpeed = (preset==0 ? 0 : 1);
        preload = preset==1;

        setting_desiredImmatureDensity = 1500;    //original 1500. set higher
        setting_desiredPointDensity = 2000;       //original 2000
        setting_minFrames = 5;
        setting_maxFrames = 7;
        setting_maxOptIterations=6;
        setting_minOptIterations=1;

        setting_kfGlobalWeight=0.3;   // original is 1.0. 0.3 is a balance between speed and accuracy. if tracking lost, set this para higher
        setting_maxShiftWeightT= 0.04f * (640 + 128);   // original is 0.04f * (640+480); this para is depend on the crop size.
        setting_maxShiftWeightR= 0.04f * (640 + 128);    // original is 0.0f * (640+480);
        setting_maxShiftWeightRT= 0.02f * (640 + 128);  // original is 0.02f * (640+480);

        setting_logStuff = false;
    }
     printf("==============================================\n");
}


bool Localization::init(){



    if (!until::parse_config_text(FLAGS_localization_conf_file, &config_)) {
        LOG(INFO) << "There is No config text, please touch a config file or No right path for config file(modify athena_conf.pb.txt)";
        return false;
    }

    if (!config_.has_localization_mode_()) {
        LOG(ERROR)<<"Localization Error: Config file must provide the localization_mode";
        return false;
    }

    if(config_.localization_mode_().c_str() == "dataset")
    {
        dataset_init();
    }





    return true;
}

bool Localization::dataset_init() {

}

bool Localization::live_init() {
    setero_camera_.reset(new sensor::SeteroCamera());
    setero_camera_->init();
    localization_thread_ = std::thread(&Localization::localization_thread_func, this);
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