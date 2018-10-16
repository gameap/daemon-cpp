#!/bin/bash

apt-get -y -qq update
apt-get -y install curl software-properties-common

curl -fsSL http://packages.gameap.ru/ubuntu/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/ubuntu trusty main" > /etc/apt/sources.list.d/gameap.list
add-apt-repository -y ppa:mhier/libboost-latest

apt-get -qq update

apt-get -y install build-essential make cmake

apt-get -y install libboost1.67-dev libbinn-dev librestclientcpp-dev libjsoncpp-dev libssl-dev

cmake .
make
