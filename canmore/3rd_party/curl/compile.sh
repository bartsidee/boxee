#!/bin/bash
source ../common.sh
pushd curl-7.21.3
./configure --disable-debug --disable-ldap -disable-ldaps --disable-ipv6 --host=${HOST} --prefix=${PREFIX} --enable-ares=/opt/canmore/local --with-ca-bundle=/opt/local/share/curl/ca-bundle.crt
make -j6
#sudo make install
popd
