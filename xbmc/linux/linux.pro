include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../guilib ../utils ../cores ../FileSystem
	INCLUDEPATH += ../osx
SOURCES += \
LinuxTimezone.cpp \
XMemUtils.cpp \
DBusMessage.cpp \
ConvUtils.cpp \
PosixMountProvider.cpp \
HALPowerSyscall.cpp \
NetworkLinux.cpp \
ConsoleDeviceKitPowerSyscall.cpp \
LinuxResourceCounter.cpp \
HALManager.cpp \
DeviceKitDisksProvider.cpp \
XRandR.cpp \
ZeroconfBrowserAvahi.cpp \
XFileUtils.cpp \
XHandle.cpp \
XLCDproc.cpp \
ZeroconfAvahi.cpp \
DBusUtil.cpp \
XThreadUtils.cpp \
XEventUtils.cpp \
HALProvider.cpp \
XCriticalSection.cpp \
XTimeUtils.cpp \
XSyncUtils.cpp \


HEADERS += \
LinuxTimezone.h \
XMemUtils.h \
DBusMessage.h \
ConvUtils.h \
PosixMountProvider.h \
HALPowerSyscall.h \
NetworkLinux.h \
ConsoleDeviceKitPowerSyscall.h \
LinuxResourceCounter.h \
HALManager.h \
DeviceKitDisksProvider.h \
XRandR.h \
ZeroconfBrowserAvahi.h \
XFileUtils.h \
XHandle.h \
XLCDproc.h \
ZeroconfAvahi.h \
DBusUtil.h \
XThreadUtils.h \
XEventUtils.h \
HALProvider.h \
XCriticalSection.h \
XTimeUtils.h \
XSyncUtils.h \


INCLUDEPATH += ../../
