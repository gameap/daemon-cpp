#!/bin/bash

apt-get -y -qq update
apt-get -y install curl gnupg2

curl -fsSL http://packages.gameap.ru/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/debian stretch main" >> /etc/apt/sources.list
echo "deb http://ftp.de.debian.org/debian stretch-backports main " >> /etc/apt/sources.list

apt-get -qq update

apt-get -y install build-essential make cmake pkg-config

apt-get -y install libbinn-dev librestclientcpp-dev libjsoncpp-dev libssl-dev libcurl4-openssl-dev
apt-get -y -t stretch-backports install libboost1.67-dev libboost-system1.67-dev libboost-filesystem1.67-dev libboost-iostreams1.67-dev libboost-thread1.67-dev

cmake ..
make
