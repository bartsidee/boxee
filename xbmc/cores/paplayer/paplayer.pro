include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../../ ../../linux ../../../guilib ../../utils ../dvdplayer .. ../../..
  INCLUDEPATH += ../dvdplayer/Codecs ../dvdplayer/Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
TimidityCodec.cpp \
 \
AC3Codec.cpp \
ReplayGain.cpp \
ModuleCodec.cpp \
DTSCDDACodec.cpp \
SPCCodec.cpp \
ADPCMCodec.cpp \
OggCallback.cpp \
CodecFactory.cpp \
NSFCodec.cpp \
PAPlayer.cpp \
GYMCodec.cpp \
CDDAcodec.cpp \
YMCodec.cpp \
APEcodec.cpp \
AdplugCodec.cpp \
OGGcodec.cpp \
AIFFcodec.cpp \
WAVcodec.cpp \
ASAPCodec.cpp \
DVDPlayerCodec.cpp \
VGMCodec.cpp \
WAVPackcodec.cpp \
MP3codec.cpp \
AudioDecoder.cpp \
SIDCodec.cpp \
DTSCodec.cpp \
FLACcodec.cpp \
AC3CDDACodec.cpp \
SHNcodec.cpp \


HEADERS += \
TimidityCodec.h \
 \
AC3Codec.h \
ReplayGain.h \
ModuleCodec.h \
DTSCDDACodec.h \
SPCCodec.h \
ADPCMCodec.h \
OggCallback.h \
CodecFactory.h \
NSFCodec.h \
PAPlayer.h \
GYMCodec.h \
CDDAcodec.h \
YMCodec.h \
APEcodec.h \
AdplugCodec.h \
OGGcodec.h \
AIFFcodec.h \
WAVcodec.h \
ASAPCodec.h \
DVDPlayerCodec.h \
VGMCodec.h \
WAVPackcodec.h \
MP3codec.h \
AudioDecoder.h \
SIDCodec.h \
DTSCodec.h \
FLACcodec.h \
AC3CDDACodec.h \
SHNcodec.h \


INCLUDEPATH += ../../../
