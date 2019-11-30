#!/usr/bin/env bash

apt-get -y -qq update
apt-get -y install curl software-properties-common

curl -fsSL http://packages.gameap.ru/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/ubuntu cosmic main" > /etc/apt/sources.list.d/gameap.list
add-apt-repository -y ppa:mhier/libboost-latest

apt-get -y -qq update

apt-get -y install build-essential make cmake pkg-config

apt-get -y install \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-thread-dev \
    libbinn-dev \
    librestclientcpp-dev \
    libjsoncpp-dev \
    libssl-dev \
    libcurl4-openssl-dev

cmake ..
make
