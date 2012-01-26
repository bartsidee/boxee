include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../ ../../../ ../../linux ../../../guilib ../../utils ../dvdplayer/Codecs ../dvdplayer/Codecs/ffmpeg
DEFINES +=   __STDC_CONSTANT_MACROS
SOURCES += \
OverlayRenderer.cpp \
 \
BaseRenderer.cpp \
LinuxRenderer.cpp \
OverlayRendererGL.cpp \
IntelSMDRenderer.cpp \
LinuxRendererOMX.cpp \
RenderManager.cpp \
OverlayRendererUtil.cpp \
LinuxRendererGL.cpp \


HEADERS += \
OverlayRenderer.h \
 \
BaseRenderer.h \
LinuxRenderer.h \
OverlayRendererGL.h \
IntelSMDRenderer.h \
LinuxRendererOMX.h \
RenderManager.h \
OverlayRendererUtil.h \
LinuxRendererGL.h \


INCLUDEPATH += ../../../
