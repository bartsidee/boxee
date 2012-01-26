#!/bin/bash
source ../common.sh
pushd libgcrypt-1.4.5
pth_config_prefix=${PREFIX} ./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
