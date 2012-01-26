#!/bin/bash
source ../common.sh
pushd pcre-8.12
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
