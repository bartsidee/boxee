include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../.. ffmpeg ../linux ../../guilib ../utils dvdplayer ./flashplayer
SOURCES += \
 \
IntelSMDGlobals.cpp \
dlgcache.cpp \
ssrc.cpp \
DummyVideoPlayer.cpp \


HEADERS += \
 \
IntelSMDGlobals.h \
dlgcache.h \
ssrc.h \
DummyVideoPlayer.h \


INCLUDEPATH += ../../
