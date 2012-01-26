#!/bin/sh

rm -rf bin/win32

mkdir -p bin/win32

#libdvdread
cd libdvdread
echo "***** Cleaning libdvdread *****"
make distclean
echo "***** Building libdvdread *****"
./configure2 \
      --prefix="/usr" \
      --disable-shared \
      --enable-static \
      --extra-cflags=" -D_XBMC -DNDEBUG -D_MSC_VER -I`pwd`/../includes" \
      --disable-debug
mkdir -p ../includes/dvdread
cp ../libdvdread/src/*.h ../includes/dvdread
make
cd ..

#libdvdnav
cd libdvdnav
echo "***** Cleaning libdvdnav *****"
make distclean
echo "***** Building libdvdnav *****"
./configure2 \
      --disable-shared \
      --enable-static \
      --extra-cflags="-D_XBMC -DNDEBUG -I`pwd`/../includes" \
      --with-dvdread-config="`pwd`/../libdvdread/obj/dvdread-config"
      --disable-debug
make
gcc \
      -shared \
      -o obj/libdvdnav.dll \
      ../libdvdread/obj/*.o obj/*.o  \
      -ldl \
      -Wl,--enable-auto-image-base \
      -Xlinker --enable-auto-import

strip -S obj/libdvdnav.dll
cd ..
cp libdvdnav/obj/libdvdnav.dll bin/win32
echo "***** Done *****"
