#!/bin/bash
source ../common.sh
pushd harfbuzz.git
#./autogen.sh
FREETYPE_CFLAGS="-I/opt/canmore/local/include/freetype2" FREETYPE_LIBS="-L/opt/canmore/local/lib -lfreetype" ac_cv_prog_have_icu=false GLIB_CFLAGS="-I/opt/canmore/local/lib/glib-2.0/include -I/opt/canmore/local/include/glib-2.0 -I/opt/canmore/local/include/glib-2.0/include" GLIB_LIBS="-L/opt/canmore/local/lib -lglib-2.0"  PKG_CONFIG="" ./configure --host=${HOST} --prefix=${PREFIX}
make -j6
#sudo make install
popd
