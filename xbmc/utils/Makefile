INCLUDES=-I. -I.. -I../../ -I../linux -I../cores -I../../guilib -I../../guilib/tinyXML -I../lib -I../lib/libjson/include -I../lib/libplist/include

SRCS=AlarmClock.cpp \
     Archive.cpp \
     Builtins.cpp \
     CharsetConverter.cpp \
     CriticalSection.cpp \
     Event.cpp \
     fstrcmp.cpp \
     GUIInfoManager.cpp \
     HTMLTable.cpp \
     HTMLUtil.cpp \
     HttpHeader.cpp \
     IMDB.cpp \
     InfoLoader.cpp \
     log.cpp \
     MusicAlbumInfo.cpp \
     MusicInfoScraper.cpp \
     RegExp.cpp \
     RssReader.cpp \
     ScraperParser.cpp \
     SingleLock.cpp \
     Splash.cpp \
     Stopwatch.cpp \
     SystemInfo.cpp \
     TuxBoxUtil.cpp \
     UdpClient.cpp \
     Weather.cpp \
     Thread.cpp \
     SharedSection.cpp \
     Win32Exception.cpp \
     CPUInfo.cpp \
     PCMAmplifier.cpp \
     PCMRemap.cpp \
     LabelFormatter.cpp \
     Network.cpp \
     BitstreamStats.cpp \
     PerformanceStats.cpp \
     PerformanceSample.cpp \
     LCDFactory.cpp \
     LCD.cpp \
     EventServer.cpp \
     EventPacket.cpp \
     EventClient.cpp \
     Socket.cpp \
     Fanart.cpp \
     ScraperUrl.cpp \
     MusicArtistInfo.cpp \
     Mutex.cpp \
     md5.cpp \
     ArabicShaping.cpp \
     AsyncFileCopy.cpp \
     DbusServer.cpp \
     Atomics.cpp \
     LockFree.cpp \
     StreamDetails.cpp \
     DBConnectionPool.cpp \
     SqliteConnectionPoolObject.cpp \
     SqlitePoolMngr.cpp \
     TimeUtils.cpp \
     JobManager.cpp \
     Teletext.cpp \
     fastmemcpy.cpp \
     AnnouncementManager.cpp \
     Variant.cpp \
     HttpParser.cpp \
     AirPlayServer.cpp \
     AirTunesServer.cpp \
     GLUtils.cpp

SRCS += Base64.cpp Clipboard.cpp IServer.cpp

LIB=utils.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
