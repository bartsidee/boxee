#!/bin/bash
source ../common.sh
pushd avahi-0.6.28
./configure \
  --prefix=/opt/local \
  --enable-shared \
  --enable-static \
  --disable-glib \
  --disable-gobject \
  --disable-qt3 \
  --disable-qt4 \
  --disable-gtk \
  --disable-gtk3 \
  --with-xml=expat \
  --disable-dbm \
  --enable-gdbm \
  --enable-libdaemon \
  --disable-python \
  --disable-pygtk \
  --disable-python-dbus \
  --disable-mono \
  --disable-monodoc \
  --disable-doxygen-doc \
  --disable-doxygen-dot \
  --disable-doxygen-man \
  --disable-doxygen-rtf \
  --disable-doxygen-xml \
  --disable-doxygen-chm \
  --disable-doxygen-chi \
  --disable-doxygen-html \
  --disable-doxygen-ps \
  --disable-doxygen-pdf \
  --disable-xmltoman \
  --with-distro=none \
  --with-avahi-user=nobody \
  --with-avahi-group=nobody \
  --with-autoipd-user=nobody \
  --with-autoipd-group=nobody
make -j6
#sudo make install
popd
