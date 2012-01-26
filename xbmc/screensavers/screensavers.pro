include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../linux ../../guilib ../utils
INCLUDEPATH +=  /opt/local/include
QMAKE_CXXFLAGS += -fPIC

SOURCES += \
ScreenSaverFactory.cpp \
ScreenSaver.cpp \


HEADERS += \
ScreenSaverFactory.h \
ScreenSaver.h \

INCLUDEPATH += ../../

