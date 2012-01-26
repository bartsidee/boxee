include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../../../ ../../../.. ../../../linux ../../../../guilib ../../../FileSystem/ ../DVDCodecs/Overlay/ ../Codecs ../Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
DVDSubtitleParserMicroDVD.cpp \
 \
DVDSubtitleParserSami.cpp \
DVDSubtitleParserSubrip.cpp \
DVDSubtitleLineCollection.cpp \
DVDSubtitleParserMPL2.cpp \
DVDSubtitleStream.cpp \
DVDSubtitlesLibass.cpp \
DVDSubtitleParserVplayer.cpp \
DVDFactorySubtitle.cpp \
SamiTagConvertor.cpp \
DVDSubtitleParserSSA.cpp \


HEADERS += \
DVDSubtitleParserMicroDVD.h \
 \
DVDSubtitleParserSami.h \
DVDSubtitleParserSubrip.h \
DVDSubtitleLineCollection.h \
DVDSubtitleParserMPL2.h \
DVDSubtitleStream.h \
DVDSubtitlesLibass.h \
DVDSubtitleParserVplayer.h \
DVDFactorySubtitle.h \
SamiTagConvertor.h \
DVDSubtitleParserSSA.h \


INCLUDEPATH += ../../../../
