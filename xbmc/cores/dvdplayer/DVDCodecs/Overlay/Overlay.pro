include(../../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../../../ ../../../../.. ../../../../linux ../../../../../guilib ../../Codecs ../../Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
DVDOverlayCodecSSA.cpp \
 \
DVDOverlayCodecCC.cpp \
DVDOverlayCodecFFmpeg.cpp \
DVDOverlayCodecText.cpp \


HEADERS += \
DVDOverlayCodecSSA.h \
 \
DVDOverlayCodecCC.h \
DVDOverlayCodecFFmpeg.h \
DVDOverlayCodecText.h \


INCLUDEPATH += ../../../../../
