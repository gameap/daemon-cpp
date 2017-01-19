#!/bin/bash

yum update
yum -y install git wget

yum-config-manager --add-repo http://dl.fedoraproject.org/pub/epel/7/x86_64/
wget http://dl.fedoraproject.org/pub/epel/RPM-GPG-KEY-EPEL-7
rpm --import RPM-GPG-KEY-EPEL-7
yum update

yum groupinstall -y 'Development Tools'

yum -y install cmake boost-devel
yum -y install jsoncpp-devel openssl-devel.x86_64 mariadb-devel

# Compile dependencies
git clone https://github.com/liteserver/binn
gcc -c ./binn/src/binn.c -o ./binn/src/binn.o
ar rcs ./binn/src/libbinn.a ./binn/src/binn.o

mv ./binn/src/libbinn.a /usr/lib/libbinn.a
mv ./binn/src/binn.h /usr/include/binn.h

wget http://packages.gameap.ru/others/boost-process.tar.gz
tar xvf boost-process.tar.gz -C /

wget http://packages.gameap.ru/others/libmysqlclient-dev.tar.gz
tar xvf libmysqlclient-dev.tar.gz -C /

git clone https://github.com/GameAP/GDaemon2
cd GDaemon2

cmake .
make
