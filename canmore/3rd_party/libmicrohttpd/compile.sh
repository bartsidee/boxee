#!/bin/bash
source ../common.sh
pushd libmicrohttpd
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
