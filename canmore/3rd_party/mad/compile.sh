#!/bin/bash
source ../common.sh
patch -p0 < pkg_config_support.patch
pushd libmad-0.15.1b
./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
#sudo cp mad.pc /opt/canmore/local/lib/pkgconfig/
popd
