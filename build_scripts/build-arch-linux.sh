#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o xtrace
set -o pipefail

ROOTDIR=$(pwd)

pacman --noconfirm -S cmake boost openssl curl jsoncpp base-devel

# Compile Binn
cd "${ROOTDIR}"
git clone https://github.com/liteserver/binn
cd binn
make
ar rcs libbinn.a binn.o
cp libbinn.a /usr/lib/libbinn.a
cp src/binn.h /usr/include/binn.h

# Compile Rest Client Library
cd "${ROOTDIR}"
git clone https://github.com/mrtazz/restclient-cpp
cd restclient-cpp
../configure
make
make install
