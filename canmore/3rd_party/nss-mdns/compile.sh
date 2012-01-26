#!/bin/bash
source ../common.sh
pushd nss-mdns-0.10
./configure --host=${HOST} --prefix=${PREFIX} --localstatedir=/opt/local/var --sysconfdir=/opt/local/etc
make -j6
#sudo make install
popd
