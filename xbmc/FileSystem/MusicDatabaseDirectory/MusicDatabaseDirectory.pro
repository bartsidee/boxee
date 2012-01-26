include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib
SOURCES += \
DirectoryNodeAlbumCompilations.cpp \
DirectoryNodeRoot.cpp \
DirectoryNodeArtist.cpp \
DirectoryNodeAlbumRecentlyPlayedSong.cpp \
DirectoryNodeAlbum.cpp \
DirectoryNodeAlbumCompilationsSongs.cpp \
DirectoryNodeAlbumRecentlyPlayed.cpp \
DirectoryNodeOverview.cpp \
DirectoryNodeAlbumRecentlyAdded.cpp \
DirectoryNodeGenre.cpp \
DirectoryNodeAlbumTop100.cpp \
DirectoryNodeSongTop100.cpp \
DirectoryNodeSingles.cpp \
DirectoryNodeAlbumTop100Song.cpp \
DirectoryNodeAlbumRecentlyAddedSong.cpp \
QueryParams.cpp \
DirectoryNodeYearSong.cpp \
DirectoryNode.cpp \
DirectoryNodeTop100.cpp \
DirectoryNodeYear.cpp \
DirectoryNodeYearAlbum.cpp \
DirectoryNodeSong.cpp \


HEADERS += \
DirectoryNodeAlbumCompilations.h \
DirectoryNodeRoot.h \
DirectoryNodeArtist.h \
DirectoryNodeAlbumRecentlyPlayedSong.h \
DirectoryNodeAlbum.h \
DirectoryNodeAlbumCompilationsSongs.h \
DirectoryNodeAlbumRecentlyPlayed.h \
DirectoryNodeOverview.h \
DirectoryNodeAlbumRecentlyAdded.h \
DirectoryNodeGenre.h \
DirectoryNodeAlbumTop100.h \
DirectoryNodeSongTop100.h \
DirectoryNodeSingles.h \
DirectoryNodeAlbumTop100Song.h \
DirectoryNodeAlbumRecentlyAddedSong.h \
QueryParams.h \
DirectoryNodeYearSong.h \
DirectoryNode.h \
DirectoryNodeTop100.h \
DirectoryNodeYear.h \
DirectoryNodeYearAlbum.h \
DirectoryNodeSong.h \


INCLUDEPATH += ../../../
