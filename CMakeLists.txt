cmake_minimum_required(VERSION 3.10)
project(Athena)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# required libraries
find_package(SuiteParse REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Boost COMPONENTS system thread)

# optional libraries
find_package(LibZip QUIET)
find_package(Pangolin 0.2 QUIET)
find_package(OpenCV QUIET)

# Add the source in project root directory
aux_source_directory(. DIRSRCS)
# Add header file include directories
include_directories(./ ./localization)
# Add block directories
add_subdirectory(src/localization)

# Sources files
set(ATHENA_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/src/localization/localization.cpp
        )



add_executable(Athena src/main.cpp ${ATHENA_SOURCE_FILES})
target_link_libraries(Athena
        boost_system
        boost_thread
        cxsparse
        ${LIBZIP_LIBRARY}
        ${Pangolin_LIBRARIES}
        ${OpenCV_LIBS}
        ${PROJECT_SOURCE_DIR}/thirdparty/libBino_Stereo_SDK.so
        )