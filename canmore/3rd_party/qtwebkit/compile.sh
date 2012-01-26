#!/bin/bash
cd qtwebkit
export PATH=$PATH:/opt/canmore/local/qt/bin
export QMAKESPEC=/opt/canmore/local/qt/mkspecs/qws/linux-x86-intelce-g++/
export QTDIR=/opt/canmore/local/qt/
export CROSS_COMPILE=i686-cm-linux-
./WebKitTools/Scripts/build-webkit --qt --qmake=/opt/canmore/local/qt/bin/qmake -spec qws/linux-x86-intelce-g++ --makeargs="-j10" --video CONFIG+=embedded
