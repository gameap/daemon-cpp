#!/bin/bash

sudo wget -qO - http://packages.gameap.ru/debian/gameap-rep.gpg.key | sudo apt-key add -
sudo echo "deb http://packages.gameap.ru/debian jessie main" >> /etc/apt/sources.list

sudo apt-get -qq update

sudo apt-get -y install build-essential make cmake gcc-4.8 g++-4.8

sudo apt-get -y install libboost-system1.55-dev libboost-filesystem1.55-dev libboost-iostreams1.55-dev libboost-thread1.55-dev
sudo apt-get -y install libboost-process1.55-dev libbinn-dev
sudo apt-get -y install libjsoncpp-dev libssl-dev libmcrypt-dev

cmake .
make
