include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../../ ../../../linux ../../../utils ../../../../guilib
SOURCES += \
  emu_kernel32.cpp \
  exports_python_linux.o \
  wrapper.c \
  exports_python_linux.cpp \
  emu_msvcrt.cpp \
  emu_dummy.cpp \


HEADERS += \
  emu_kernel32.h \
  exports_python_linux.o \
  wrapper.c \
  exports_python_linux.h \
  emu_msvcrt.h \
  emu_dummy.h \


INCLUDEPATH += ../../../../
