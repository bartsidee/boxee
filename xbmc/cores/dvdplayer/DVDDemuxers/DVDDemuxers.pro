include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../.. ../../../ ../../../.. ../../../utils ../../../linux ../../../../guilib ../Codecs ../Codecs/ffmpeg
DEFINES += __STDC_CONSTANT_MACROS \
SOURCES += \
DVDDemuxHTSP.cpp \
DVDDemuxVobsub.cpp \
 \
DVDDemuxFFmpeg.cpp \
DVDDemux.cpp \
DVDFactoryDemuxer.cpp \
DVDDemuxUtils.cpp \
DVDDemuxShoutcast.cpp \


HEADERS += \
DVDDemuxHTSP.h \
DVDDemuxVobsub.h \
 \
DVDDemuxFFmpeg.h \
DVDDemux.h \
DVDFactoryDemuxer.h \
DVDDemuxUtils.h \
DVDDemuxShoutcast.h \


INCLUDEPATH += ../../../../
