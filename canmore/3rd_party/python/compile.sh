#!/bin/bash

cd Python-2.4.6
./configure
make python Parser/pgen
mv python hostpython
mv Parser/pgen Parser/hostpgen
make distclean

ac_cv_lib_util_openpty=no ac_cv_func_openpty=no ac_cv_func_forkpty=no ac_cv_lib_util_forkpty=no OPT='-fno-strict-aliasing -g -Wall -I/opt/canmore/IntelCE/include -I/opt/canmore/IntelCE/include/ncurses ' CC=i686-cm-linux-gcc CXX=i686-cm-linux-g++ AR=i686-cm-linux-ar RANLIB=i686-cm-linux-ranlib ./configure --host=i686-cm-linux --build=i686-pc-linux-gnu --prefix=/opt/canmore/local/ --enable-unicode=ucs4

make HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes

#make HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes libpython2.4.so

sudo PATH=$PATH:/opt/canmore/toolchains make install HOSTPYTHON=./hostpython BLDSHARED="i686-cm-linux-gcc -shared -L/opt/canmore/IntelCE/lib" CROSS_COMPILE="i686-cm-linux-" CROSS_COMPILE_TARGET=yes prefix=/opt/canmore/local

sudo cp libpython2.4.a /opt/canmore/local/lib
#sudo cp libpython2.4.so /opt/canmore/local/lib
#sudo chmod 755 /opt/canmore/local/lib/libpython2.4.so
