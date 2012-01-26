#!/bin/bash
source ../common.sh
pushd fontconfig-2.7.3
./configure --build=${BUILD} --host=${HOST} --prefix=${PREFIX}  --disable-docs --without-add-fonts --with-arch=i686 --with-cache-dir=/opt/local/var/cache/fontconfig --with-confdir=/opt/local/etc/fonts

# remove CFLAGS from fc-case fc-lang fc-glyphname fc-arch doc
#make -j6
#sudo make install
popd
