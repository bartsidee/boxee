include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../.. ../../ ../../linux ../../utils ../../../guilib ../../../guilib/tinyXML ../../FileSystem Codecs Codecs/ffmpeg
DEFINES += __STDC_FORMAT_MACROS __STDC_CONSTANT_MACROS
SOURCES += \
 \
DVDStreamInfo.cpp \
DVDPlayerVideo.cpp \
DVDPlayerAudio.cpp \
DVDOverlayRenderer.cpp \
DVDClock.cpp \
DVDPerformanceCounter.cpp \
DVDPlayer.cpp \
DVDMessageTracker.cpp \
DVDPlayerTeletext.cpp \
DVDPlayerSubtitle.cpp \
DVDDemuxSPU.cpp \
DVDMessage.cpp \
DVDAudio.cpp \
DVDPlayerAudioResampler.cpp \
DVDFileInfo.cpp \
DVDTSCorrection.cpp \
DVDMessageQueue.cpp \
Edl.cpp \
DVDOverlayContainer.cpp \


HEADERS += \
 \
DVDStreamInfo.h \
DVDPlayerVideo.h \
DVDPlayerAudio.h \
DVDOverlayRenderer.h \
DVDClock.h \
DVDPerformanceCounter.h \
DVDPlayer.h \
DVDMessageTracker.h \
DVDPlayerTeletext.h \
DVDPlayerSubtitle.h \
DVDDemuxSPU.h \
DVDMessage.h \
DVDAudio.h \
DVDPlayerAudioResampler.h \
DVDFileInfo.h \
DVDTSCorrection.h \
DVDMessageQueue.h \
Edl.h \
DVDOverlayContainer.h \


INCLUDEPATH += ../../../
