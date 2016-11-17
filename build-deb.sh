#!/bin/bash

wget -qO - http://packages.gameap.ru/debian/gameap-rep.gpg.key | sudo apt-key add -
echo "deb http://packages.gameap.ru/debian jessie main" >> /etc/apt/sources.list

apt-get -qq update

apt-get -y install build-essential make cmake gcc-4.8 g++-4.8

apt-get -y install libboost-system1.55-dev libboost-filesystem1.55-dev libboost-iostreams1.55-dev libboost-thread1.55-dev
apt-get -y install libboost-process1.55-dev libbinn-dev
apt-get -y install libjsoncpp-dev libssl-dev libmcrypt-dev

cmake .
make
