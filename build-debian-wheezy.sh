#!/usr/bin/env bash

apt-get -y -qq update
apt-get -y install curl gnupg2

curl -fsSL http://packages.gameap.ru/gameap-rep.gpg.key | apt-key add -
echo "deb http://packages.gameap.ru/debian wheezy main" > /etc/apt/sources.list.d/gameap.list

apt-get -y -qq update

apt-get -y install build-essential make cmake pkg-config
apt-get -y install libbinn-dev librestclientcpp-dev libjsoncpp-dev libssl-dev libcurl4-openssl-dev

# Compile Boost
wget http://packages.gameap.ru/others/boost_1_65_1.tar.gz
tar -xvf boost_1_65_1.tar.gz
cd boost_1_65_1
./bootstrap.sh
./b2 install --link=static

cmake .
make
