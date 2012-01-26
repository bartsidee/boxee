#!/bin/bash
source ../common.sh
export CFLAGS=""
export CXXFLAGS="$CFLAGS"
export CPPFLAGS="$CFLAGS"
export LD=${CROSSBIN}gcc
pushd libvpx-v0.9.5
make clean
./configure --target=x86-linux-gcc --enable-shared --prefix=/opt/canmore/local
make -j6
#sudo DIST_DIR=/opt/canmore/local make install
popd
