include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib ../../../xbmc ../../utils
DEFINES += API_DEBUG
SOURCES += \
 \
dll_tracker.cpp \
ldt_keeper.c \
mmap_anon.c \
SoLoader.cpp \
dll_util.cpp \
dll.cpp \
LibraryLoader.cpp \
DllLoader.cpp \
DllLoaderContainer.cpp \
dll_tracker_library.cpp \
coff.cpp \
dll_tracker_file.cpp \


HEADERS += \
 \
dll_tracker.h \
ldt_keeper.c \
mmap_anon.c \
SoLoader.h \
dll_util.h \
dll.h \
LibraryLoader.h \
DllLoader.h \
DllLoaderContainer.h \
dll_tracker_library.h \
coff.h \
dll_tracker_file.h \


INCLUDEPATH += ../../../
