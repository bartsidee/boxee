#!/bin/bash
source ../common.sh
pushd bzip2-1.0.6
patch -p1 < ../Makefile.patch
make -j6
#sudo make install
popd
