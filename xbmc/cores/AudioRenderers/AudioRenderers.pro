include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib ../../utils ../dvdplayer .. ../dvdplayer/Codecs ../dvdplayer/Codecs/ffmpeg
SOURCES += \
ALSADirectSound.cpp \
IntelSMDAudioRenderer.cpp \
AudioRendererFactory.cpp \
AudioUtils.cpp \
CoreAudioRenderer.cpp \
NullDirectSound.cpp \

HEADERS += \
ALSADirectSound.h \
IntelSMDAudioRenderer.h \
AudioRendererFactory.h \
AudioUtils.h \
CoreAudioRenderer.h \
NullDirectSound.h \

mac {
SOURCES += AudioUtilsHelperApple.cpp
HEADERS += AudioUtilsHelperApple.h 
}

unix:!mac {
SOURCES += AudioUtilsHelperLinux.cpp
HEADERS += AudioUtilsHelperLinux.h 
}

INCLUDEPATH += ../../../
