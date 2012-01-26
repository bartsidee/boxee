include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
DEFINES += -ISource/Shared PIC -fPIC -ISource/MACLib -pedantic __GNUC_IA32__ BUILD_CROSS_PLATFORM _LINUX
	DEFINES += 10.4
	DEFINES += -UHAVE_WCSCASECMP
	DEFINES += -w
	DEFINES += HAVE_WCSCASECMP
SOURCES += \


HEADERS += \


INCLUDEPATH += ../../../../
