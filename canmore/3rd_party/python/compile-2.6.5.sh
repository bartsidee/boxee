#!/bin/bash
source ../common.sh

# look at http://randomsplat.com/id5-cross-compiling-python-for-embedded-linux.html 
# wget http://randomsplat.com/wp-content/uploads/2010/04/Python-2.6.5-xcompile.patch
cd Python-2.6.5
patch -p1 < ../Python-2.6.5-xcompile.patch
./configure
python Parser/pgen
mv python hostpython
mv Parser/pgen Parser/hostpgen
make distclean

CFLAGS='-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64' OPT="-O3 -Wall -I/opt/canmore/IntelCE/include $CFLAGS" CC=i686-cm-linux-gcc CXX=i686-cm-linux-g++ AR=i686-cm-linux-ar RANLIB=i686-cm-linux-ranlib ./configure --host=i686-cm-linux --build=i686-pc-linux-gnu --prefix=/opt/canmore/local/

make HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes

make HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes libpython2.6.so

sudo PATH=$PATH:/opt/canmore/toolchains make -n install HOSTPYTHON=./hostpython BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes prefix=/opt/canmore/local

sudo cp libpython2.6.so /opt/canmore/local/lib
sudo chmod 755 /opt/canmore/local/lib/libpython2.6.so
