include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build

DEFINES += SQUISH_USE_SSE=2 
QMAKE_CXXFLAGS += -msse2

SOURCES += \
alpha.cpp \
squish.cpp \
colourblock.cpp \
colourset.cpp \
colourfit.cpp \
singlecolourfit.cpp \
rangefit.cpp \
clusterfit.cpp \
maths.cpp \

HEADERS += \
alpha.h \
squish.h \
colourblock.h \
colourset.h \
colourfit.h \
singlecolourfit.h \
rangefit.h \
clusterfit.h \
maths.h \


INCLUDEPATH += . ../../../
