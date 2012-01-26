#!/bin/bash
source ../common.sh
pushd fuse-exfat-0.9.3
DESTDIR=/opt/canmore/local/sbin scons CC=$CC CFLAGS="-I/opt/canmore/local/include -Wall -O2 -ggdb" LINKFLAGS="-L/opt/canmore/local/lib -liconv" install
popd
