include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../.. ../../xbmc ../../xbmc/linux ../../xbmc/utils
SOURCES += \
IntelCERemote.cpp \
LinuxInputDevices.cpp \
LIRC.cpp \
SDLJoystick.cpp \


HEADERS += \
IntelCERemote.h \
LinuxInputDevices.h \
LIRC.h \
SDLJoystick.h \


