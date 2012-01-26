include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../linux ../../guilib ../cores ../utils ../.. ../cores/dvdplayer/Codecs ../cores/dvdplayer/Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
Cdg.cpp \
 \
GUIWindowKaraokeLyrics.cpp \
karaokelyricstextlrc.cpp \
karaokelyricstextkar.cpp \
karaokelyricsfactory.cpp \
karaokewindowbackground.cpp \
karaokelyricscdg.cpp \
GUIDialogKaraokeSongSelector.cpp \
karaokelyricsmanager.cpp \
karaokelyrics.cpp \
karaokelyricstextustar.cpp \
karaokelyricstext.cpp \


HEADERS += \
Cdg.h \
 \
GUIWindowKaraokeLyrics.h \
karaokelyricstextlrc.h \
karaokelyricstextkar.h \
karaokelyricsfactory.h \
karaokewindowbackground.h \
karaokelyricscdg.h \
GUIDialogKaraokeSongSelector.h \
karaokelyricsmanager.h \
karaokelyrics.h \
karaokelyricstextustar.h \
karaokelyricstext.h \


INCLUDEPATH += ../../
