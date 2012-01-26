include(../../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../../../ ../../../../.. ../../../../linux ../../../../../guilib ../../../ ../../Codecs/ffmpeg ../../Codecs ../../../ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
DVDVideoCodecFFmpeg.cpp \
DVDVideoCodecSMD.cpp \
DVDVideoCodecOmx.cpp \
VDPAU.cpp \
../DVDOmxUtils.cpp \
IntelSMDVideo.cpp \
DVDVideoPPFFmpeg.cpp \
DVDVideoCodecLibMpeg2.cpp \
DVDVideoCodecVDA.cpp \


HEADERS += \
DVDVideoCodecFFmpeg.h \
 \
.. \
DVDVideoCodecSMD.h \
DVDVideoCodecOmx.h \
VDPAU.h \
../DVDOmxUtils.h \
IntelSMDVideo.h \
DVDVideoPPFFmpeg.h \
DVDVideoCodecLibMpeg2.h \
DVDVideoCodecVDA.h \


INCLUDEPATH += ../../../../../
