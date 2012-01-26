#!/bin/bash 
rm -r .libs
make distclean

./configure \
--extra-cflags="-D_XBOX -fno-common" \
--disable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-zlib \
--enable-static \
--disable-altivec \
--disable-muxers \
--disable-encoders \
--disable-ipv6 \
--disable-debug && 
 
make -j3 && 
mkdir .libs &&
cp lib*/*.a .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll
