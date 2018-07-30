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

//mode=1
        setting_photometricCalibration = 0;
        setting_affineOptModeA = 0; //-1: fix. >=0: optimize (with prior, if > 0).
        setting_affineOptModeB = 0; //-1: fix. >=0: optimize (with prior, if > 0).
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

    std::string mode_ = config_.localization_mode_().c_str();

    if(mode_.compare("dataset")==0) {
        LOG(INFO) << "This is DATASET mode";
        dataset_init();
        localization_thread_ = std::thread(&Localization::localization_dataset_thread_func, this);
    }
    else if(mode_.compare("live")==0) {
        LOG(INFO) << "This is LIVE mode";
        live_init();
        localization_thread_ = std::thread(&Localization::localization_live_thread_func, this);
    }

    return true;
}

bool Localization::dataset_init() {

    source_file_ = config_.source_file_().c_str();
    calib_       = config_.calib_().c_str();
    settingsDefault(0);

    reader.reset(new (std::nothrow) ImageFolderReader(source_file_+"/image_0", calib_, gammaCalib_, vignette_));
    if(!reader.get()) {
        LOG(ERROR)<<"Localization Error: reader init failed";
        return false;
    }

    reader_right.reset(new (std::nothrow) ImageFolderReader(source_file_+"/image_1", calib_, gammaCalib_, vignette_));
    if(!reader_right.get()) {
        LOG(ERROR)<<"Localization Error: reader init failed";
        return false;
    }

    reader->setGlobalCalibration();
    reader_right->setGlobalCalibration();

    if(setting_photometricCalibration > 0 && reader->getPhotometricGamma() == 0)
    {
        printf("ERROR: dont't have photometric calibation. Need to use commandline options mode=1 or mode=2 ");
        exit(1);
    }



    fullSystem.reset(new (std::nothrow) FullSystem());
    if(!fullSystem.get()) {
        LOG(ERROR)<<"Localization Error: fullSystem init failed";
        return false;
    }

    fullSystem->setGammaFunction(reader->getPhotometricGamma());
    fullSystem->linearizeOperation = (playbackSpeed==0);


    if(!disableAllDisplay) {
        viewer = new IOWrap::PangolinDSOViewer(wG[0],hG[0], false);
        fullSystem->outputWrapper.push_back(viewer);
    }
    if(useSampleOutput)
        fullSystem->outputWrapper.push_back(new IOWrap::SampleOutputWrapper());

    return true;
}

bool Localization::live_init() {
    setero_camera_.reset(new sensor::SeteroCamera());
    setero_camera_->init();
}

void Localization::parseArgument() {

}

void Localization::localization_live_thread_func() {
    while (run_) {
        setero_camera_->bino->Grab();
        setero_camera_->bino->getRectImage(left_image_, right_image_);
        cv::imshow("image_left", left_image_);
        cv::imshow("image_right", right_image_);
        cv::waitKey(2);
    }

    run_ = false;

}

void Localization::localization_dataset_thread_func() {

    std::vector<int> idsToPlay;				// left images
    std::vector<double> timesToPlayAt;

    std::vector<int> idsToPlayRight;		// right images
    std::vector<double> timesToPlayAtRight;

    int linc = 1;

    int lstart=start;
    int lend = end;

    for(int i=lstart;i>= 0 && i< reader->getNumImages() && linc*i < linc*lend;i+=linc)
    {
        idsToPlay.push_back(i);
        if(timesToPlayAt.size() == 0)
        {
            timesToPlayAt.push_back((double)0);
        }
        else
        {
            double tsThis = reader->getTimestamp(idsToPlay[idsToPlay.size()-1]);
            double tsPrev = reader->getTimestamp(idsToPlay[idsToPlay.size()-2]);
            timesToPlayAt.push_back(timesToPlayAt.back() +  fabs(tsThis-tsPrev)/playbackSpeed);
        }
    }

    for(int i=lstart;i>= 0 && i< reader_right->getNumImages() && linc*i < linc*lend;i+=linc)
    {
        idsToPlayRight.push_back(i);
        if(timesToPlayAtRight.size() == 0)
        {
            timesToPlayAtRight.push_back((double)0);
        }
        else
        {
            double tsThis = reader_right->getTimestamp(idsToPlay[idsToPlay.size()-1]);
            double tsPrev = reader_right->getTimestamp(idsToPlay[idsToPlay.size()-2]);
            timesToPlayAtRight.push_back(timesToPlayAtRight.back() +  fabs(tsThis-tsPrev)/playbackSpeed);
        }
    }



    std::vector<ImageAndExposure*> preloadedImagesLeft;
    std::vector<ImageAndExposure*> preloadedImagesRight;
    if(preload)
    {
        printf("LOADING ALL IMAGES!\n");
        for(int ii=0;ii<(int)idsToPlay.size(); ii++)
        {
            int i = idsToPlay[ii];
            preloadedImagesLeft.push_back(reader->getImage(i));
            preloadedImagesRight.push_back(reader_right->getImage(i));
        }
    }

    // timing
    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);
    clock_t started = clock();
    double sInitializerOffset=0;


    for(int ii=0; ii<(int)idsToPlay.size(); ii++)
    {
        if(!fullSystem->initialized)	// if not initialized: reset start time.
        {
            gettimeofday(&tv_start, NULL);
            started = clock();
            sInitializerOffset = timesToPlayAt[ii];
        }

        int i = idsToPlay[ii];


        ImageAndExposure* img_left;
        ImageAndExposure* img_right;
        if(preload){
            img_left = preloadedImagesLeft[ii];
            img_right = preloadedImagesRight[ii];
        }
        else{
            img_left = reader->getImage(i);
            img_right = reader_right->getImage(i);
        }

        bool skipFrame=false;
        if(playbackSpeed!=0)
        {
            struct timeval tv_now; gettimeofday(&tv_now, NULL);
            double sSinceStart = sInitializerOffset + ((tv_now.tv_sec-tv_start.tv_sec) + (tv_now.tv_usec-tv_start.tv_usec)/(1000.0f*1000.0f));

            if(sSinceStart < timesToPlayAt[ii])
                usleep((int)((timesToPlayAt[ii]-sSinceStart)*1000*1000));
            else if(sSinceStart > timesToPlayAt[ii]+0.5+0.1*(ii%2))
            {
                printf("SKIPFRAME %d (play at %f, now it is %f)!\n", ii, timesToPlayAt[ii], sSinceStart);
                skipFrame=true;
            }
        }

        // if MODE_SLAM is true, it runs slam.
        bool MODE_SLAM = true;
        // if MODE_STEREOMATCH is true, it does stereo matching and output idepth image.
        bool MODE_STEREOMATCH = false;

        if(MODE_SLAM)
        {
            if(!skipFrame) fullSystem->addActiveFrame(img_left, img_right, i);
        }

        if(MODE_STEREOMATCH)
        {
            std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

            cv::Mat idepthMap(img_left->h, img_left->w, CV_32FC3, cv::Scalar(0,0,0));
            cv::Mat &idepth_temp = idepthMap;
            fullSystem->stereoMatch(img_left, img_right, i, idepth_temp);

            std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            double ttStereoMatch = std::chrono::duration_cast<std::chrono::duration<double>>(t1 -t0).count();
            std::cout << " casting time " << ttStereoMatch << std::endl;
        }

        delete img_left;
        delete img_right;

        // initializer fail
        if(fullSystem->initFailed || setting_fullResetRequested)
        {
            if(ii < 250 || setting_fullResetRequested)
            {
                printf("RESETTING!\n");

                std::vector<IOWrap::Output3DWrapper*> wraps = fullSystem->outputWrapper;
                fullSystem.release();

                for(IOWrap::Output3DWrapper* ow : wraps) ow->reset();

                fullSystem.reset(new (std::nothrow) FullSystem());
                fullSystem->setGammaFunction(reader->getPhotometricGamma());
                fullSystem->linearizeOperation = (playbackSpeed==0);


                fullSystem->outputWrapper = wraps;

                setting_fullResetRequested=false;
            }
        }

        if(fullSystem->isLost)
        {
            printf("LOST!!\n");
            break;
        }

    }


    fullSystem->blockUntilMappingIsFinished();
    clock_t ended = clock();
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);


    fullSystem->printResult("/home/jiatianwu/project/sdso/result.txt");


    int numFramesProcessed = abs(idsToPlay[0]-idsToPlay.back());
    double numSecondsProcessed = fabs(reader->getTimestamp(idsToPlay[0])-reader->getTimestamp(idsToPlay.back()));
    double MilliSecondsTakenSingle = 1000.0f*(ended-started)/(float)(CLOCKS_PER_SEC);
    double MilliSecondsTakenMT = sInitializerOffset + ((tv_end.tv_sec-tv_start.tv_sec)*1000.0f + (tv_end.tv_usec-tv_start.tv_usec)/1000.0f);
    printf("\n======================"
           "\n%d Frames (%.1f fps)"
           "\n%.2fms per frame (single core); "
           "\n%.2fms per frame (multi core); "
           "\n%.3fx (single core); "
           "\n%.3fx (multi core); "
           "\n======================\n\n",
           numFramesProcessed, numFramesProcessed/numSecondsProcessed,
           MilliSecondsTakenSingle/numFramesProcessed,
           MilliSecondsTakenMT / (float)numFramesProcessed,
           1000 / (MilliSecondsTakenSingle/numSecondsProcessed),
           1000 / (MilliSecondsTakenMT / numSecondsProcessed));
    //fullSystem->printFrameLifetimes();
    if(setting_logStuff)
    {
        std::ofstream tmlog;
        tmlog.open("logs/time.txt", std::ios::trunc | std::ios::out);
        tmlog << 1000.0f*(ended-started)/(float)(CLOCKS_PER_SEC*reader->getNumImages()) << " "
              << ((tv_end.tv_sec-tv_start.tv_sec)*1000.0f + (tv_end.tv_usec-tv_start.tv_usec)/1000.0f) / (float)reader->getNumImages() << "\n";
        tmlog.flush();
        tmlog.close();
    }

}

}
}