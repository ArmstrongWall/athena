//
// Created by wzq on 18-8-1.
//

//
// Created by wzq on 18-5-26.
//

/**
* This file is part of DSO.
*
* Copyright 2016 Technical University of Munich and Intel.
* Developed by Jakob Engel <engelj at in dot tum dot de>,
* for more information see <http://vision.in.tum.de/dso>.
* If you use this code, please cite the respective publications as
* listed on the above website.
*
* DSO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DSO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DSO. If not, see <http://www.gnu.org/licenses/>.
*/



#include <thread>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <chrono>

#include "IOWrapper/Output3DWrapper.h"
#include "IOWrapper/ImageDisplay.h"


#include <boost/thread.hpp>
#include "util/settings.h"
#include "util/globalFuncs.h"
#include "util/DatasetReader.h"
#include "util/globalCalib.h"

#include "util/NumType.h"
#include "FullSystem/FullSystem.h"
#include "OptimizationBackend/MatrixAccumulators.h"
#include "FullSystem/PixelSelector2.h"



#include "IOWrapper/Pangolin/PangolinDSOViewer.h"
#include "IOWrapper/OutputWrapper/SampleOutputWrapper.h"

#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <memory>


std::string vignette = "";
std::string gammaCalib = "";
std::string source = "";
std::string calib = "";
double rescale = 1;
bool reverse = false;
bool disableROS = false;
int start=0;
int end=100000;
bool prefetch = false;
float playbackSpeed=0;	// 0 for linearize (play as fast as possible, while sequentializing tracking & mapping). otherwise, factor on timestamps.
bool preload=false;
bool useSampleOutput=false;


int mode=0;

bool firstRosSpin=false;

using namespace dso;


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

    firstRosSpin=true;
    while(true) pause();
}

void settingsDefault(int preset)
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

    if(preset == 2 || preset == 3)
    {
        printf("FAST settings:\n"
               "- %s real-time enforcing\n"
               "- 800 active points\n"
               "- 4-6 active frames\n"
               "- 1-4 LM iteration each KF\n"
               "- 424 x 320 image resolution\n", preset==0 ? "no " : "5x");

        playbackSpeed = (preset==2 ? 0 : 5);
        preload = preset==3;
        setting_desiredImmatureDensity = 600;
        setting_desiredPointDensity = 800;
        setting_minFrames = 4;
        setting_maxFrames = 6;
        setting_maxOptIterations=4;
        setting_minOptIterations=1;

        benchmarkSetting_width =  424;
        benchmarkSetting_height = 320;

        setting_logStuff = false;
    }

    printf("==============================================\n");
}

void parseArgument(char* arg)
{
    int option;
    float foption;
    char buf[1000];


    if(1==sscanf(arg,"sampleoutput=%d",&option))
    {
        if(option==1)
        {
            useSampleOutput = true;
            printf("USING SAMPLE OUTPUT WRAPPER!\n");
        }
        return;
    }

    if(1==sscanf(arg,"quiet=%d",&option))
    {
        if(option==1)
        {
            setting_debugout_runquiet = true;
            printf("QUIET MODE, I'll shut up!\n");
        }
        return;
    }

    if(1==sscanf(arg,"preset=%d",&option))
    {
        settingsDefault(option);
        return;
    }


    if(1==sscanf(arg,"rec=%d",&option))
    {
        if(option==0)
        {
            disableReconfigure = true;
            printf("DISABLE RECONFIGURE!\n");
        }
        return;
    }


    if(1==sscanf(arg,"noros=%d",&option))
    {
        if(option==1)
        {
            disableROS = true;
            disableReconfigure = true;
            printf("DISABLE ROS (AND RECONFIGURE)!\n");
        }
        return;
    }

    if(1==sscanf(arg,"nolog=%d",&option))
    {
        if(option==1)
        {
            setting_logStuff = false;
            printf("DISABLE LOGGING!\n");
        }
        return;
    }
    if(1==sscanf(arg,"reverse=%d",&option))
    {
        if(option==1)
        {
            reverse = true;
            printf("REVERSE!\n");
        }
        return;
    }
    if(1==sscanf(arg,"nogui=%d",&option))
    {
        if(option==1)
        {
            disableAllDisplay = true;
            printf("NO GUI!\n");
        }
        return;
    }
    if(1==sscanf(arg,"nomt=%d",&option))
    {
        if(option==1)
        {
            multiThreading = false;
            printf("NO MultiThreading!\n");
        }
        return;
    }
    if(1==sscanf(arg,"prefetch=%d",&option))
    {
        if(option==1)
        {
            prefetch = true;
            printf("PREFETCH!\n");
        }
        return;
    }
    if(1==sscanf(arg,"start=%d",&option))
    {
        start = option;
        printf("START AT %d!\n",start);
        return;
    }
    if(1==sscanf(arg,"end=%d",&option))
    {
        end = option;
        printf("END AT %d!\n",start);
        return;
    }

    if(1==sscanf(arg,"files=%s",buf))
    {
        source = buf;
        printf("loading data from %s!\n", source.c_str());
        return;
    }

    if(1==sscanf(arg,"calib=%s",buf))
    {
        calib = buf;
        printf("loading calibration from %s!\n", calib.c_str());
        return;
    }

    if(1==sscanf(arg,"vignette=%s",buf))
    {
        vignette = buf;
        printf("loading vignette from %s!\n", vignette.c_str());
        return;
    }

    if(1==sscanf(arg,"gamma=%s",buf))
    {
        gammaCalib = buf;
        printf("loading gammaCalib from %s!\n", gammaCalib.c_str());
        return;
    }

    if(1==sscanf(arg,"rescale=%f",&foption))
    {
        rescale = foption;
        printf("RESCALE %f!\n", rescale);
        return;
    }

    if(1==sscanf(arg,"speed=%f",&foption))
    {
        playbackSpeed = foption;
        printf("PLAYBACK SPEED %f!\n", playbackSpeed);
        return;
    }

    if(1==sscanf(arg,"save=%d",&option))
    {
        if(option==1)
        {
            debugSaveImages = true;
            if(42==system("rm -rf images_out")) printf("system call returned 42 - what are the odds?. This is only here to shut up the compiler.\n");
            if(42==system("mkdir images_out")) printf("system call returned 42 - what are the odds?. This is only here to shut up the compiler.\n");
            if(42==system("rm -rf images_out")) printf("system call returned 42 - what are the odds?. This is only here to shut up the compiler.\n");
            if(42==system("mkdir images_out")) printf("system call returned 42 - what are the odds?. This is only here to shut up the compiler.\n");
            printf("SAVE IMAGES!\n");
        }
        return;
    }

    if(1==sscanf(arg,"mode=%d",&option))
    {
        mode = option;
        if(option==0)
        {
            printf("PHOTOMETRIC MODE WITH CALIBRATION!\n");
        }
        if(option==1)
        {
            printf("PHOTOMETRIC MODE WITHOUT CALIBRATION!\n");
            setting_photometricCalibration = 0;
            setting_affineOptModeA = 0; //-1: fix. >=0: optimize (with prior, if > 0).
            setting_affineOptModeB = 0; //-1: fix. >=0: optimize (with prior, if > 0).
        }
        if(option==2)
        {
            printf("PHOTOMETRIC MODE WITH PERFECT IMAGES!\n");
            setting_photometricCalibration = 0;
            setting_affineOptModeA = -1; //-1: fix. >=0: optimize (with prior, if > 0).
            setting_affineOptModeB = -1; //-1: fix. >=0: optimize (with prior, if > 0).
            setting_minGradHistAdd=3;
        }
        return;
    }

    printf("could not parse argument \"%s\"!!!!\n", arg);
}

int main( int argc, char** argv )
{
    //setlocale(LC_ALL, "");

    for(int i=1; i<argc;i++)
        parseArgument(argv[i]);

    cv::Mat cv_image_left,cv_image_right;

    std::shared_ptr<BinoCamera>  camera;
    BinoCameraParameterList paraList;


    paraList.devPath = "/dev/video1";
    paraList.intParameterPath = "thirdparty/params/intrinsics.yml";
    paraList.extParameterPath = "thirdparty/params/extrinsics.yml";

    camera.reset(new BinoCamera(paraList));

    for (int i = 0; i<25; i++) {
        camera->Grab();//get img
        camera->getOrgImage(cv_image_left, cv_image_right);//RectImage
        //cv::imshow("image_rect_left", cv_image_left);
        //cv::imshow("image_rect_right", cv_image_right);
        cv::waitKey(1);
    }


    // hook crtl+C.
    boost::thread exThread = boost::thread(exitThread);

    ImageFolderReader* reader = new ImageFolderReader(source+"/image_0", calib, gammaCalib, vignette);
    ImageFolderReader* reader_right = new ImageFolderReader(source+"/image_1", calib, gammaCalib, vignette);
    reader->setGlobalCalibration();
    reader_right->setGlobalCalibration();

    if(setting_photometricCalibration > 0 && reader->getPhotometricGamma() == 0)
    {
        printf("ERROR: dont't have photometric calibation. Need to use commandline options mode=1 or mode=2 ");
        exit(1);
    }


    // build system
    FullSystem* fullSystem = new FullSystem();
    fullSystem->setGammaFunction(reader->getPhotometricGamma());
    fullSystem->linearizeOperation = (playbackSpeed==0);


    IOWrap::PangolinDSOViewer* viewer = 0;
    if(!disableAllDisplay)
    {
        viewer = new IOWrap::PangolinDSOViewer(wG[0],hG[0], false);
        fullSystem->outputWrapper.push_back(viewer);
    }

    if(useSampleOutput)
        fullSystem->outputWrapper.push_back(new IOWrap::SampleOutputWrapper());

    // to make MacOS happy: run this in dedicated thread -- and use this one to run the GUI.
    std::thread runthread([&]() {
        // timing
//        struct timeval tv_start;
//        gettimeofday(&tv_start, NULL);
//        clock_t started = clock();
//        double sInitializerOffset=0;


        for(int ii=0; ; ii++)
        {
//			if(!fullSystem->initialized)	// if not initialized: reset start time.
//			{
//				gettimeofday(&tv_start, NULL);
//				started = clock();
//				sInitializerOffset = timesToPlayAt[ii];
//			}

            int i = ii;

            ImageAndExposure* img_left;
            ImageAndExposure* img_right;


            camera->Grab();//get img
            camera->getRectImage(cv_image_left, cv_image_right);//RectImage
            //cv::imshow("image_rect_left", left);
            //cv::imshow("image_rect_right", right);

            //cv::waitKey(1);



            if(cv_image_left.rows*cv_image_left.cols==0)
            {
                printf("cv::imread could not read image %s! this may segfault. \n", reader->files[i].c_str());
                return 0;
            }
            if(cv_image_left.type() != CV_8U)
            {
                printf("cv::imread did something strange! this may segfault. \n");
                return 0;
            }
            MinimalImageB* img = new MinimalImageB(cv_image_left.cols, cv_image_left.rows);
            memcpy(img->data, cv_image_left.data, cv_image_left.rows*cv_image_left.cols);

            img_left = reader->undistort->undistort<unsigned char>(img,1, 0, 1.0f);

            delete img;


            //cv::Mat cv_image_right = right;

            if(cv_image_right.rows*cv_image_right.cols==0)
            {
                printf("cv::imread could not read image %s! this may segfault. \n", reader_right->files[i].c_str());
                return 0;
            }
            if(cv_image_right.type() != CV_8U)
            {
                printf("cv::imread did something strange! this may segfault. \n");
                return 0;
            }
            MinimalImageB* img_r = new MinimalImageB(cv_image_right.cols, cv_image_right.rows);
            memcpy(img_r->data, cv_image_right.data, cv_image_right.rows*cv_image_right.cols);

            img_right = reader_right->undistort->undistort<unsigned char>(img_r,1, 0, 1.0f);

            delete img_r;






            // if MODE_SLAM is true, it runs slam.
            bool MODE_SLAM = true;
            // if MODE_STEREOMATCH is true, it does stereo matching and output idepth image.
            bool MODE_STEREOMATCH = false;

            if(MODE_SLAM)
            {
                fullSystem->addActiveFrame(img_left, img_right, i);
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
                    delete fullSystem;

                    for(IOWrap::Output3DWrapper* ow : wraps) ow->reset();

                    fullSystem = new FullSystem();
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
//        clock_t ended = clock();
//        struct timeval tv_end;
//        gettimeofday(&tv_end, NULL);


        fullSystem->printResult("/home/wzq/result.txt");


    });


    if(viewer != 0)
        viewer->run();

    runthread.join();

    for(IOWrap::Output3DWrapper* ow : fullSystem->outputWrapper)
    {
        ow->join();
        delete ow;
    }

    printf("DELETE FULLSYSTEM!\n");
    delete fullSystem;

    printf("DELETE READER!\n");
    delete reader;

    printf("EXIT NOW!\n");
    return 0;
}