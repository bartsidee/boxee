#!/bin/bash
source ../common.sh
cd ntfs-3g-2011.1.15
./configure --host=${HOST} --prefix=${PREFIX} --with-fuse=external
make -j6
#sudo make install
