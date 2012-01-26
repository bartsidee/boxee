#!/bin/bash
source ../common.sh
cd util-linux-ng-2.18
export CFLAGS="$CFLAGS -I/opt/canmore/IntelCE/include/ncurses"
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
