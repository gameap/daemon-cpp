#!/usr/bin/env bash

apt-get -y -qq update
apt-get -y install curl software-properties-common python-software-properties

curl -fsSL http://packages.gameap.ru/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/ubuntu precise main" > /etc/apt/sources.list.d/gameap.list
add-apt-repository -y ppa:mhier/libboost-latest
add-apt-repository -y ppa:ubuntu-toolchain-r/test

apt-get -y -qq update

apt-get -y install gcc-4.8-base gcc-4.8-plugin-dev g++-4.8 gcc-4.8 libgcc-4.8-dev make cmake pkg-config

apt-get -y install \
    libbinn-dev \
    librestclientcpp-dev \
    libjsoncpp-dev \
    libssl-dev \
    libcurl4-openssl-dev

# Compile Boost
wget http://packages.gameap.ru/others/boost_1_65_1.tar.gz
tar -xvf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
./b2 install --link=static

cmake ..
make
