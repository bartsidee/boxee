include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../../ ../../../ ../../utils ../../FileSystem ../../linux ../../cores ../../../guilib ../libjson
SOURCES += \
AVPlayerOperations.cpp \
FileOperations.cpp \
JSONRPC.cpp \
FileItemHandler.cpp \
PlaylistOperations.cpp \
PlayerOperations.cpp \
AVPlaylistOperations.cpp \
TCPServer.cpp \
VideoLibrary.cpp \
PicturePlayerOperations.cpp \
SystemOperations.cpp \
AudioLibrary.cpp \
XBMCOperations.cpp \


HEADERS += \
AVPlayerOperations.h \
FileOperations.h \
JSONRPC.h \
FileItemHandler.h \
PlaylistOperations.h \
PlayerOperations.h \
AVPlaylistOperations.h \
TCPServer.h \
VideoLibrary.h \
PicturePlayerOperations.h \
SystemOperations.h \
AudioLibrary.h \
XBMCOperations.h \


INCLUDEPATH += ../../../
