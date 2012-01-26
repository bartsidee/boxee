include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib
SOURCES += \
sqlitedataset.cpp \
qry_dat.cpp \
dataset.cpp \


HEADERS += \
sqlitedataset.h \
qry_dat.h \
dataset.h \


INCLUDEPATH += ../../../
