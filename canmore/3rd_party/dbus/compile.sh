#!/bin/bash
source ../common.sh
pushd dbus-1.4.1
./configure \
  --prefix=/opt/local \
  --disable-ansi \
  --disable-asserts \
  --disable-console-owner-file \
  --disable-doxygen-docs \
  --disable-gcov \
  --disable-selinux \
  --disable-tests \
  --disable-verbose-mode \
  --disable-xml-docs \
  --with-xml="expat" \
  --with-dbus-user=root \
  --without-x
make -j6
#sudo make install
popd
