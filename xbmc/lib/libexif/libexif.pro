include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
DEFINES += _LINUX _DLL 
QMAKE_CXXFLAGS += -fPIC -O2

SOURCES += libexif.cpp ExifParse.cpp IptcParse.cpp JpegParse.cpp
HEADERS += libexif.h ExifParse.h IptcParse.h JpegParse.h

INCLUDEPATH += ../../../

