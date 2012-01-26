include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../.. ../../../ ../../../linux ../../../../guilib ../../../lib/libRTMP ../Codecs ../Codecs/ffmpeg
INCLUDEPATH += ../../../utils ../../../lib/libbluray/src 
DEFINES +=  __STDC_FORMAT_MACROS  __STDC_CONSTANT_MACROS \
SOURCES += \
DVDInputStreamHTSP.cpp \
 \
DVDInputStreamMemory.cpp \
DVDFactoryInputStream.cpp \
DVDInputStreamFFmpeg.cpp \
DVDStateSerializer.cpp \
DVDInputStreamNavigator.cpp \
DVDInputStreamPlaylist.cpp \
DVDInputStreamHttp.cpp \
DVDInputStreamStack.cpp \
DVDInputStream.cpp \
DVDInputStreamBluray.cpp \
DVDInputStreamTV.cpp \
DVDInputStreamFile.cpp \
DVDInputStreamRTMP.cpp \
DVDInputStreamMMS.cpp \


HEADERS += \
DVDInputStreamHTSP.h \
 \
DVDInputStreamMemory.h \
DVDFactoryInputStream.h \
DVDInputStreamFFmpeg.h \
DVDStateSerializer.h \
DVDInputStreamNavigator.h \
DVDInputStreamPlaylist.h \
DVDInputStreamHttp.h \
DVDInputStreamStack.h \
DVDInputStream.h \
DVDInputStreamBluray.h \
DVDInputStreamTV.h \
DVDInputStreamFile.h \
DVDInputStreamRTMP.h \
DVDInputStreamMMS.h \


INCLUDEPATH += ../../../../
