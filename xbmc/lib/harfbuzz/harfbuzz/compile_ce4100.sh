#!/bin/sh
export CFLAGS="-I/opt/canmore/local/include -I/opt/canmore/IntelCE/include -I/opt/canmore/IntelCE/usr/include -I/opt/canmore/IntelCE/usr/local/include"
export CXXFLAGS="$CFLAGS"
export CPPFLAGS="$CFLAGS"
export LIBS="-L/opt/canmore/local/lib -L/opt/canmore/IntelCE/lib -L/opt/canmore/IntelCE/usr/lib"
export LDFLAGS="-L/opt/canmore/local/lib -L/opt/canmore/IntelCE/lib -L/opt/canmore/IntelCE/usr/lib"
export CROSSBIN=/opt/canmore/toolchains/i686-cm-linux-
export CC=${CROSSBIN}gcc
export CXX=${CROSSBIN}g++
export LD=${CROSSBIN}ld
export AR=${CROSSBIN}ar
export RANLIB=${CROSSBIN}ranlib
export STRIP=${CROSSBIN}strip
export OBJDUMP=${CROSSBIN}objdump 
export PREFIX=/opt/canmore/local
export HOST=i686-cm-linux
export BUILD=i686-linux
export CXXCPP="$CXX -E"
export PKG_CONFIG_LIBDIR=/opt/canmore/local/lib/pkgconfig
export TARGETFS=/opt/canmore/targetfs
#export X11_INCLUDES=/opt/camore/usr/include
#export X11_LIBS=/opt/canmore/usr/lib

./autogen.sh
FREETYPE_CFLAGS="-I/opt/canmore/local/include/freetype2" FREETYPE_LIBS="-L/opt/canmore/local/lib -lfreetype" ac_cv_prog_have_icu=false GLIB_CFLAGS="-I/opt/canmore/local/lib/glib-2.0/include -I/opt/canmore/local/include/glib-2.0 -I/opt/canmore/local/include/glib-2.0/include" GLIB_LIBS="-L/opt/canmore/local/lib -lglib-2.0"  PKG_CONFIG="" ./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
