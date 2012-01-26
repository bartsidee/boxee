#!/bin/bash
DEBUG_OPTIONS="--enable-optimizations --disable-debug"
export CANMORE_HOME=/opt/canmore

while [ "$1" ] 
do
  case "$1" in
  -d) DEBUG_OPTIONS="--disable-optimizations"
      ;;
  -c) export CANMORE_HOME="$2"
      shift
      ;;
  esac
  shift
done

export CONFIG_SITE=./canmore/canmore.config 
export PKG_CONFIG_PATH=$CANMORE_HOME/local/lib/pkgconfig

echo "export CANMORE_HOME=$CANMORE_HOME" > cross_env
chmod 755 cross_env

./configure $DEBUG_OPTIONS --host=i686-cm-linux  --disable-dvd-drive --disable-mysqlclient --disable-vdpau --disable-ss-rsxs --enable-external-python --enable-boxee-hal
