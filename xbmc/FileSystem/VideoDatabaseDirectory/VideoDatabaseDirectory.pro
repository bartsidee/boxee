include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib
SOURCES += \
DirectoryNodeMusicVideosOverview.cpp \
DirectoryNodeRoot.cpp \
DirectoryNodeEpisodes.cpp \
 \
DirectoryNodeDirector.cpp \
DirectoryNodeRecentlyAddedEpisodes.cpp \
DirectoryNodeTitleMovies.cpp \
DirectoryNodeOverview.cpp \
DirectoryNodeGenre.cpp \
QueryParams.cpp \
DirectoryNodeTitleTvShows.cpp \
DirectoryNodeActor.cpp \
DirectoryNodeSeasons.cpp \
DirectoryNodeSets.cpp \
DirectoryNodeTitleMusicVideos.cpp \
DirectoryNode.cpp \
DirectoryNodeMoviesOverview.cpp \
DirectoryNodeRecentlyAddedMovies.cpp \
DirectoryNodeYear.cpp \
DirectoryNodeRecentlyAddedMusicVideos.cpp \
DirectoryNodeStudio.cpp \
DirectoryNodeMusicVideoAlbum.cpp \
DirectoryNodeTvShowsOverview.cpp \


HEADERS += \
DirectoryNodeMusicVideosOverview.h \
DirectoryNodeRoot.h \
DirectoryNodeEpisodes.h \
 \
DirectoryNodeDirector.h \
DirectoryNodeRecentlyAddedEpisodes.h \
DirectoryNodeTitleMovies.h \
DirectoryNodeOverview.h \
DirectoryNodeGenre.h \
QueryParams.h \
DirectoryNodeTitleTvShows.h \
DirectoryNodeActor.h \
DirectoryNodeSeasons.h \
DirectoryNodeSets.h \
DirectoryNodeTitleMusicVideos.h \
DirectoryNode.h \
DirectoryNodeMoviesOverview.h \
DirectoryNodeRecentlyAddedMovies.h \
DirectoryNodeYear.h \
DirectoryNodeRecentlyAddedMusicVideos.h \
DirectoryNodeStudio.h \
DirectoryNodeMusicVideoAlbum.h \
DirectoryNodeTvShowsOverview.h \


INCLUDEPATH += ../../../
