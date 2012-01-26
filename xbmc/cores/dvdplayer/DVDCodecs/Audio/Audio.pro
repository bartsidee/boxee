include(../../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../.. ../../../../.. ../../../../ ../../../../utils ../../../../linux ../../../../../guilib ../../Codecs ../../Codecs/ffmpeg 
DEFINES += HAVE_MMX  __STDC_CONSTANT_MACROS
SOURCES += \
 \
DVDAudioCodecPcm.cpp \
DVDAudioCodecFFmpeg.cpp \
DVDAudioCodecLPcm.cpp \
DVDAudioCodecPassthrough.cpp \
DVDAudioCodecLiba52.cpp \
DVDAudioCodecLibDts.cpp \
DVDAudioCodecSMD.cpp \
DVDAudioCodecLibMad.cpp \
DVDAudioCodecLibFaad.cpp \


HEADERS += \
 \
DVDAudioCodecPcm.h \
DVDAudioCodecFFmpeg.h \
DVDAudioCodecLPcm.h \
DVDAudioCodecPassthrough.h \
DVDAudioCodecLiba52.h \
DVDAudioCodecLibDts.h \
DVDAudioCodecSMD.h \
DVDAudioCodecLibMad.h \
DVDAudioCodecLibFaad.h \


INCLUDEPATH += ../../../../../
