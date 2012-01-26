#!/bin/bash
source ../common.sh
pushd xmlstarlet-1.0.4
./configure --host=${HOST} --prefix=${PREFIX} --with-libxml-prefix=/opt/canmore/local --with-libxslt-prefix=/opt/canmore/local
make -j6
#sudo make install
popd
