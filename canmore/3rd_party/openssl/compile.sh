#!/bin/bash
pushd openssl-1.0.0c
AR=i686-cm-linux-ar RANLIB=i686-cm-linux-ranlib CC=i686-cm-linux-gcc ./Configure linux-generic32 shared --prefix=/opt/canmore/local
make
#sudo make install
popd
