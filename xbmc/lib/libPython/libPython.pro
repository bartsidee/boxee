include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib ../../utils .
SOURCES += \
XBPython.cpp \
XBPyThread.cpp \
XBPythonDll.cpp \
XBPythonDllFuncs.S \
XBPyPersistentThread.cpp \


HEADERS += \
XBPython.h \
XBPyThread.h \
XBPythonDll.h \
XBPythonDllFuncs.S \
XBPyPersistentThread.h \


INCLUDEPATH += ../../../
