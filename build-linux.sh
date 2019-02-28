#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o xtrace
set -o pipefail

ROOTDIR=$(pwd)

# Compile GCC 7
# wget http://mirror.linux-ia64.org/gnu/gcc/releases/gcc-7.4.0/gcc-7.4.0.tar.gz
# tar -xvf gcc-7.4.0.tar.gz
# cd gcc-7.4.0
# ./contrib/download_prerequisites
# mkdir objdir && cd objdir
# $PWD/../configure --enable-multilib --disable-shared --with-newlib
# make

# Download CMake
wget https://cmake.org/files/v3.10/cmake-3.10.3-Linux-x86_64.tar.gz
tar -xvf cmake-3.10.3-Linux-x86_64.tar.gz
cp -R cmake-3.10.3-Linux-x86_64/* /

# Compile Boost
cd $ROOTDIR
wget http://packages.gameap.ru/others/boost_1_65_1.tar.gz
tar -xvf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
./b2 install --link=static

# Compile Binn
cd $ROOTDIR
git clone https://github.com/liteserver/binn
cd binn
make
ar rcs libbinn.a binn.o
cp libbinn.a /usr/lib/libbinn.a
cp src/binn.h /usr/include/binn.h

# OpenSSL
cd $ROOTDIR
wget https://github.com/openssl/openssl/archive/OpenSSL_1_1_1a.tar.gz
tar -xvf OpenSSL_1_1_1a.tar.gz
cd openssl-OpenSSL_1_1_1a
./config
make
make install
#ln -s /usr/local/lib64/libssl.so.1.1 /usr/lib64/libssl.so.1.1
#ln -s /usr/local/lib64/libcrypto.so.1.1 /usr/lib64/libcrypto.so.1.1

#ln -s /usr/local/lib64/libssl.a /usr/lib64/libssl.a
#ln -s /usr/local/lib64/libcrypto.a /usr/lib64/libcrypto.a

# Compile Curl Library
cd $ROOTDIR
wget https://curl.haxx.se/download/curl-7.63.0.tar.gz
tar -xvf curl-7.63.0.tar.gz
cd curl-7.63.0
cmake -DCMAKE_USE_OPENSSL=on .
make
make install

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
#ln -s /usr/local/lib64/libjsoncpp.a /usr/lib64/libjsoncpp.a

# Compile GDAEMON
#--------------------------
cd $ROOTDIR
git clone https://github.com/gameap/GDaemon2
cd GDaemon2
cmake -DUSE_STATIC_BOOST=1 .
make