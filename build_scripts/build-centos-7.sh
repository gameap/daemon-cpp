#!/usr/bin/env bash

yum -y install centos-release-scl-rh \
    git \
    wget \
    zlib-devel \
    bzip2-devel

# GCC 9,
yum -y install devtoolset-9-gcc devtoolset-9-gcc-c++
scl enable devtoolset-9 'bash'

# CMake
yum -y install llvm-toolset-7-cmake
scl enable llvm-toolset-7 'bash'

# Compile Boost
cd $ROOTDIR
wget http://packages.gameap.ru/others/boost_1_70_0.tar.gz
tar -xvf boost_1_70_0.tar.gz
cd boost_1_70_0
./bootstrap.sh
./b2 install --link=static --with-system --with-filesystem --with-iostreams --with-thread

# Compile Binn
cd $ROOTDIR
git clone https://github.com/liteserver/binn
cd binn
make
ar rcs libbinn.a binn.o
cp libbinn.a /usr/lib/libbinn.a
cp src/binn.h /usr/include/binn.h

# OpenSSL
yum install openssl-devel

# CURL
yum install libcurl-devel

# Compile Rest Client Library
cd $ROOTDIR
git clone https://github.com/mrtazz/restclient-cpp
cd restclient-cpp
./autogen.sh
./configure
make
make install

# Compile Json Cpp
cd $ROOTDIR
git clone https://github.com/open-source-parsers/jsoncpp
cd jsoncpp
cmake .
make
make install
mkdir /usr/local/include/jsoncpp
mv /usr/local/include/json /usr/local/include/jsoncpp/json

# Compile GDAEMON
#--------------------------
cd $ROOTDIR
cmake -DBUILD_STATIC=1 -DUSE_STATIC_BOOST=1 ..
make