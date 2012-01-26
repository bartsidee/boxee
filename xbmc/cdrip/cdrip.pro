include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../.. ../linux ../../guilib
SOURCES += \
 \
EncoderVorbis.cpp \
CDDAReader.cpp \
EncoderWav.cpp \
CDDARipper.cpp \
Encoder.cpp \
EncoderLame.cpp \


HEADERS += \
 \
EncoderVorbis.h \
CDDAReader.h \
EncoderWav.h \
CDDARipper.h \
Encoder.h \
EncoderLame.h \


INCLUDEPATH += ../../
