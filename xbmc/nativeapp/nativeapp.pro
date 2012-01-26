include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../.. ../linux ../../guilib ../.. ../cores ../utils ../lib/libBoxee ../FileSystem ../cores/dvdplayer/Codecs ../cores/dvdplayer/Codecs/ffmpeg
DEFINES +=   __STDC_CONSTANT_MACROS
SOURCES += \
BXNativeApp.cpp \
NativeApplicationWindow.cpp \
NativeApplication.cpp \


HEADERS += \
BXNativeApp.h \
NativeApplicationWindow.h \
NativeApplication.h \


