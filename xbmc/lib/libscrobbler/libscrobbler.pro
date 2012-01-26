include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib ../../utils
INCLUDEPATH += ../../cores
SOURCES += \
lastfmscrobbler.cpp \
scrobbler.cpp \
librefmscrobbler.cpp \


HEADERS += \
lastfmscrobbler.h \
scrobbler.h \
librefmscrobbler.h \


INCLUDEPATH += ../../../
