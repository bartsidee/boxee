include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../../ ../../linux ../../../guilib ../../utils  ../dvdplayer/Codecs ../dvdplayer/Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
 \
FlashVideoPlayer.cpp \


HEADERS += \
 \
FlashVideoPlayer.h \


INCLUDEPATH += ../../../
