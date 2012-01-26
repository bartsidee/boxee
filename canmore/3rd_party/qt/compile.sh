# copy linux-x86-intelce-linux under qt-everywhere*/mkspecs/qws

pushd qt-everywhere*
PKG_CONFIG_PATH=/opt/canmore/local/lib/pkgconfig:/opt/canmore/IntelCE/usr/local/lib/pkgconfig ./configure -embedded x86 -xplatform qws/linux-x86-intelce-g++ -force-pkg-config -opengl yes -openvg yes -plugin-gfx-directfb -qt-kbd-linuxinput -qt-mouse-linuxinput -no-gfx-linuxfb -no-gfx-multiscreen -no-kbd-tty -no-mouse-pc -no-mouse-linuxtp -no-mouse-pc -system-sqlite -no-libmng -no-webkit -no-svg -prefix /opt/local/qt -no-javascript-jit -opensource -confirm-license
# make install
cd tools/designer/src/uitools
make
# sudo make install
# cd /opt/local
# sudo tar cf - qt | ( cd /opt/canmore/local; sudo tar xvfp - )
