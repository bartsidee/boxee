cd ppp-2.4.5
patch -p0 < ../configure.diff
./configure --prefix=/opt/canmore/local
make CC=i686-cm-linux-gcc
# make -n install
