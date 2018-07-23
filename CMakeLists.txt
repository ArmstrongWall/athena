cmake_minimum_required(VERSION 3.1)
project(athena)
#set(BUILD_TYPE Release)
set(BUILD_TYPE RelWithDebInfo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# flags
add_definitions("-DENABLE_SSE")
set(CMAKE_CXX_FLAGS "${SSE_FLAGS} -O3 -fPIC -g -std=c++0x -march=native")

# required libraries
find_package(SuiteParse REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenCV QUIET)
find_package(Boost COMPONENTS system thread)

# optional libraries
find_package(LibZip REQUIRED)
find_package(Pangolin 0.2 REQUIRED)



# glog
#include("cmake/FindGlog.cmake")
#find_package(lib_glog REQUIRED)
#include_directories(SYSTEM ${GLOG_INCLUDE_DIRS})

# gflags
#include("cmake/FindGFlags.cmake")
find_package(GFlags REQUIRED)
#include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})

find_package(Protobuf REQUIRED)
set(PROTO_FILES
        src/config/config.proto
        )

# Localization Sources files
set(localization_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/src/localization/localization.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/FullSystem.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/FullSystemOptimize.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/FullSystemOptPoint.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/FullSystemDebugStuff.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/FullSystemMarginalize.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/Residuals.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/CoarseTracker.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/CoarseInitializer.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/ImmaturePoint.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/HessianBlocks.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/FullSystem/PixelSelector2.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/OptimizationBackend/EnergyFunctional.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/OptimizationBackend/AccumulatedTopHessian.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/OptimizationBackend/AccumulatedSCHessian.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/OptimizationBackend/EnergyFunctionalStructs.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/util/settings.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/util/Undistort.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/util/globalCalib.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/IOWrapper/Pangolin/KeyFrameDisplay.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/IOWrapper/Pangolin/PangolinDSOViewer.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/IOWrapper/OpenCV/ImageDisplay_OpenCV.cpp
        ${PROJECT_SOURCE_DIR}/src/localization/IOWrapper/OpenCV/ImageRW_OpenCV.cpp
        )



PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HDR ${PROTO_FILES})


include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/localization
        ${PROJECT_SOURCE_DIR}/src/sensor
        ${PROJECT_SOURCE_DIR}/thirdparty/Sophus
        ${OpenCV_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR}
        ${CSPARSE_INCLUDE_DIR}
        ${CHOLMOD_INCLUDE_DIR}
        ${GFLAGS_INCLUDE_DIRS}
)

#add_library(config ${config_SOURCE_FILES})
add_library(localization ${localization_SOURCE_FILES})

add_executable(athena src/main.cpp ${PROTO_SRC} ${PROTO_HDR} ${PROTO_FILES})


target_link_libraries(athena
        ${OpenCV_LIBS}
        localization
        ${PROJECT_SOURCE_DIR}/thirdparty/libBino_Stereo_SDK.so
        boost_system
        boost_thread
        cxsparse
        ${Pangolin_LIBRARIES}
        gflags
        ${PROTOBUF_LIBRARIES}

        )