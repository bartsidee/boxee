include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../linux ../../ ../../../guilib ../../../visualisations
DEFINES += -O3 -g -fPIC
DEFINES += -fno-common HAS_SDL
SOURCES += \


HEADERS += \


INCLUDEPATH += ../../../
