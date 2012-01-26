include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib ../../utils
SOURCES += \
 \
rtmp.cpp \
AMFObject.cpp \
rtmppacket.cpp \


HEADERS += \
 \
rtmp.h \
AMFObject.h \
rtmppacket.h \


INCLUDEPATH += ../../../
