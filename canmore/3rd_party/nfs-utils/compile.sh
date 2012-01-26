#!/bin/bash
source ../common.sh
pushd nfs-utils-1.2.3
./configure --host=${HOST} --prefix=${PREFIX} --disable-tirpc --without-tcp-wrappers --disable-nfsv4 --disable-uuid --disable-gss
make -j6
#sudo make install
popd
