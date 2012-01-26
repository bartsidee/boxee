include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../../ ../../../ ../../../.. ../../../linux ../../../../guilib ../Codecs ../Codecs/ffmpeg ../../ffmpeg
DEFINES +=   __STDC_CONSTANT_MACROS
SOURCES += \
DVDCodecUtils.cpp \
DVDFactoryCodec.cpp \


HEADERS += \
DVDCodecUtils.h \
DVDFactoryCodec.h \


INCLUDEPATH += ../../../../
