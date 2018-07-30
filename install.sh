#!/usr/bin/env bash
#install opencv
cd ~/
rm -rf 3rdparty/
mkdir 3rdparty/
cd 3rdparty/
wget https://github.com/opencv/opencv/archive/3.2.0.tar.gz
tar -xzvf 3.2.0.tar.gz
cd 3.2.0
sudo apt-get install build-essential
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ..
make -j7
sudo make install


#install Pangolin
cd ~/3rdparty/
git clone git@github.com:stevenlovegrove/Pangolin.git
sudo apt-get install libglew-dev
sudo apt-get install libpython2.7-dev
sudo apt-get install ffmpeg libavcodec-dev libavutil-dev libavformat-dev libswscale-dev libavdevice-dev
sudo apt-get install libdc1394-22-dev libraw1394-dev
sudo apt-get install libjpeg-dev libpng12-dev libtiff5-dev libopenexr-dev
cd Pangolin
mkdir build
cd build
cmake ..
cmake --build .
sudo make install

## Install Glog Gflags
sudo apt-get install vim cmake git
sudo apt-get install libprotobuf-dev libleveldb-dev libsnappy-dev libopencv-dev libboost-all-dev libhdf5-serial-dev libgflags-dev libgoogle-glog-dev liblmdb-dev protobuf-compiler
## Install Protobuf
sudo apt-get install protobuf-compiler libprotobuf-dev libprotoc-dev
sudo apt-get install libprotobuf-c0-dev protobuf-c-compiler

sudo apt-get install libsuitesparse-dev libeigen3-dev libboost-all-dev


cd ~/git/athena
mkdir build
cd build
cmake ..
make -j7

