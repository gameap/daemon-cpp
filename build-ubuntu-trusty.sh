#!/bin/bash

apt-get -y -qq update
apt-get -y install curl

curl -fsSL http://packages.gameap.ru/ubuntu/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/ubuntu trusty main" > /etc/apt/sources.list.d/gameap.list

apt-get -qq update

apt-get -y install build-essential make cmake

apt-get -y install libboost-dev libboost-system-dev libboost-filesystem-dev libboost-iostreams-dev libboost-thread-dev
apt-get -y install libboost-process-dev libbinn-dev librestclientcpp-dev libjsoncpp-dev libssl-dev

cmake .
make
