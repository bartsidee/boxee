include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../ ../../../ ../../../linux ../../../../guilib ../../dvdplayer/Codecs ../../dvdplayer/Codecs/ffmpeg
SOURCES += \
VideoFilterShader.cpp \
YUV2RGBShader.cpp \


HEADERS += \
VideoFilterShader.h \
YUV2RGBShader.h \


INCLUDEPATH += ../../../../
