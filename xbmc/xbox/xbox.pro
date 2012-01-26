include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../linux ../../guilib
INCLUDEPATH += ../lib/libcdio/libcdio/include
SOURCES += \
IoSupport.cpp \
XKGeneral.cpp \


HEADERS += \
IoSupport.h \
XKGeneral.h \


INCLUDEPATH += ../../
