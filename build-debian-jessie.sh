#!/bin/bash

apt-get -qq update
apt-get -y install git wget

wget -qO - http://packages.gameap.ru/debian/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/debian jessie main" >> /etc/apt/sources.list

apt-get -qq update

apt-get -y install build-essential cmake

apt-get -y install libboost-dev libboost-system-dev libboost-filesystem-dev libboost-iostreams-dev libboost-thread-dev
apt-get -y install libboost-process-dev libbinn-dev
apt-get -y install libjsoncpp-dev libssl-dev libmysqlclient-dev

git clone https://github.com/GameAP/GDaemon2
cd GDaemon2

cmake .
make