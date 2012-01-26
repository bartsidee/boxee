#!/bin/bash
source ../common.sh
pushd libsndfile-1.0.23
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
