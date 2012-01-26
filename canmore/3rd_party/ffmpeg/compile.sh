#!/bin/bash
source ../common.sh
pushd ffmpeg-export-2010-03-02
./configure \
    --extra-cflags="" \
    --target-os=linux \
    --arch=i686 \
    --cpu=i686 \
    --cross-prefix=i686-cm-linux- \
    --disable-muxers \
    --disable-encoders \
    --disable-decoder=mpeg_xvmc \
    --disable-devices \
    --disable-ffplay \
    --disable-ffserver \
    --disable-ffmpeg \
    --enable-shared \
    --disable-postproc \
    --enable-gpl \
    --enable-pthreads \
    --enable-libfaad \
    --prefix=/opt/canmore/local
make -j6
#sudo make install
popd
