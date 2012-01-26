include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../ ../linux ../cores ../../guilib ../../guilib/tinyXML ../lib ../lib/libjson
SOURCES += \
 \
StreamDetails.cpp \
CriticalSection.cpp \
Atomics.cpp \
Stopwatch.cpp \
JobManager.cpp \
LCDFactory.cpp \
LockFree.cpp \
Builtins.cpp \
UdpClient.cpp \
CharsetConverter.cpp \
Clipboard.cpp \
BitstreamStats.cpp \
PerformanceStats.cpp \
MusicInfoScraper.cpp \
HTMLUtil.cpp \
BoxeeApiHttpServer.cpp \
LCD.cpp \
IMDB.cpp \
GUIInfoManager.cpp \
RegExp.cpp \
Thread.cpp \
SystemInfo.cpp \
Event.cpp \
MusicArtistInfo.cpp \
fstrcmp.cpp \
Variant.cpp \
Teletext.cpp \
SqliteConnectionPoolObject.cpp \
Fanart.cpp \
LabelFormatter.cpp \
md5.cpp \
HttpParser.cpp \
Mutex.cpp \
IServer.cpp \
TuxBoxUtil.cpp \
Socket.cpp \
Splash.cpp \
Weather.cpp \
HTMLTable.cpp \
CPUInfo.cpp \
fastmemcpy.cpp \
TimeUtils.cpp \
DbusServer.cpp \
MusicAlbumInfo.cpp \
PCMAmplifier.cpp \
HttpHeader.cpp \
HttpServer.cpp \
SharedSection.cpp \
DBConnectionPool.cpp \
PerformanceSample.cpp \
Archive.cpp \
ScraperParser.cpp \
AlarmClock.cpp \
EventServer.cpp \
EventClient.cpp \
SqlitePoolMngr.cpp \
log.cpp \
ArabicShaping.cpp \
AirPlayServer.cpp \
SingleLock.cpp \
InfoLoader.cpp \
Win32Exception.cpp \
EventPacket.cpp \
Base64.cpp \
AsyncFileCopy.cpp \
Network.cpp \
RssReader.cpp \
AnnouncementManager.cpp \
PCMRemap.cpp \
ScraperUrl.cpp \


HEADERS += \
 \
StreamDetails.h \
CriticalSection.h \
Atomics.h \
Stopwatch.h \
JobManager.h \
LCDFactory.h \
LockFree.h \
Builtins.h \
UdpClient.h \
CharsetConverter.h \
Clipboard.h \
BitstreamStats.h \
PerformanceStats.h \
MusicInfoScraper.h \
HTMLUtil.h \
BoxeeApiHttpServer.h \
LCD.h \
IMDB.h \
GUIInfoManager.h \
RegExp.h \
Thread.h \
SystemInfo.h \
Event.h \
MusicArtistInfo.h \
fstrcmp.h \
Variant.h \
Teletext.h \
SqliteConnectionPoolObject.h \
Fanart.h \
LabelFormatter.h \
md5.h \
HttpParser.h \
Mutex.h \
IServer.h \
TuxBoxUtil.h \
Socket.h \
Splash.h \
Weather.h \
HTMLTable.h \
CPUInfo.h \
fastmemcpy.h \
TimeUtils.h \
DbusServer.h \
MusicAlbumInfo.h \
PCMAmplifier.h \
HttpHeader.h \
HttpServer.h \
SharedSection.h \
DBConnectionPool.h \
PerformanceSample.h \
Archive.h \
ScraperParser.h \
AlarmClock.h \
EventServer.h \
EventClient.h \
SqlitePoolMngr.h \
log.h \
ArabicShaping.h \
AirPlayServer.h \
SingleLock.h \
InfoLoader.h \
Win32Exception.h \
EventPacket.h \
Base64.h \
AsyncFileCopy.h \
Network.h \
RssReader.h \
AnnouncementManager.h \
PCMRemap.h \
ScraperUrl.h \


INCLUDEPATH += ../../
