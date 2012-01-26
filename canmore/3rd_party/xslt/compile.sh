#!/bin/bash
source ../common.sh
pushd libxslt-1.1.26
./configure --host=${HOST} --prefix=${PREFIX} --without-python --without-crypto
#make -j6
#sudo make install
popd
