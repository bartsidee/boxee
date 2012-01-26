INCLUDES=-I. -I../ -I../cores -I../linux -I../../guilib -I../lib/UnrarXLib -I../utils
INCLUDES+=-I../lib/libcdio/libcdio/include
INCLUDES+=-I../lib/libBoxee
INCLUDES+=-I../lib/libjson/include

CXXFLAGS+=-D__STDC_FORMAT_MACROS \

SRCS=AfpDirectory.cpp \
     ASAPFileDirectory.cpp \
     BMSDirectory.cpp \
     CacheMemBuffer.cpp \
     CacheStrategy.cpp \
     CDDADirectory.cpp \
     CIFSDirectory.cpp \
     cddb.cpp \
     cdioSupport.cpp \
     CMythDirectory.cpp \
     CMythFile.cpp \
     CMythSession.cpp \
     DAAPDirectory.cpp \
     DirectoryCache.cpp \
     Directory.cpp \
     DirectoryHistory.cpp \
     DirectoryTuxBox.cpp \
     DllLibCurl.cpp \
     DVBFile.cpp \
     FactoryDirectory.cpp \
     FactoryFileDirectory.cpp \
     FileAfp.cpp \
     FileCache.cpp \
     FileCDDA.cpp \
     File.cpp \
     FileBms.cpp \
     FileCurl.cpp \
     FileCifs.cpp \
     FileDAAP.cpp \
     FileFactory.cpp \
     FileFileReader.cpp \
     FileHD.cpp \
     FileISO.cpp \
     FileLastFM.cpp \
     FileMusicDatabase.cpp \
     FileRar.cpp \
     FileRTV.cpp \
     FileShoutcast.cpp \
     FileSpecialProtocol.cpp \
     FileSmb.cpp \
     FileTuxBox.cpp \
     FileUPnP.cpp \
     FileUPnPAv.cpp \
     FileXBMSP.cpp \
     FileZip.cpp \
     FilePlaylist.cpp \
     FTPDirectory.cpp \
     FTPParse.cpp \
     FileUDF.cpp \
     FileNfs.cpp \
     HTTPDirectory.cpp \
     HTSPDirectory.cpp \
     HTSPSession.cpp \
     HDDirectory.cpp \
     HDHomeRun.cpp \
     IDirectory.cpp \
     IFile.cpp \
     udf25.cpp \
     iso9660.cpp \
     ISO9660Directory.cpp \
     LastFMDirectory.cpp \
     MultiPathDirectory.cpp \
     MultiPathFile.cpp \
     MusicDatabaseDirectory.cpp \
     MusicFileDirectory.cpp \
     MusicSearchDirectory.cpp \
     NptXbmcFile.cpp \
     NfsDirectory.cpp \
     NSFFileDirectory.cpp \
     OGGFileDirectory.cpp \
     PlaylistDirectory.cpp \
     PlaylistFileDirectory.cpp \
     PluginDirectory.cpp \
     RarDirectory.cpp \
     RarManager.cpp \
     RSSDirectory.cpp \
     RTVDirectory.cpp \
     SAPDirectory.cpp \
     SAPFile.cpp \
     ShoutcastDirectory.cpp \
     ShoutcastRipFile.cpp \
     SIDFileDirectory.cpp \
     SmartPlaylistDirectory.cpp \
	 SMBUtils.cpp \
     SMBDirectory.cpp \
     SpecialProtocol.cpp \
     SpecialProtocolDirectory.cpp \
     StackDirectory.cpp \
     UDFDirectory.cpp \
     UPnPDirectory.cpp \
     UPnPAvDirectory.cpp \
     UPnPVirtualPathDirectory.cpp \
     VideoDatabaseDirectory.cpp \
     VirtualDirectory.cpp \
     VirtualPathDirectory.cpp \
     VTPDirectory.cpp \
     VTPSession.cpp \
     VTPFile.cpp \
     XBMSDirectory.cpp \
     ZeroconfDirectory.cpp \
     ZipDirectory.cpp \
     ZipManager.cpp

SRCS += RingBuffer.cpp AppsDirectory.cpp BoxeeDatabaseDirectory.cpp BoxeeFeedDirectory.cpp ScriptDirectory.cpp BoxeeFriendsDirectory.cpp BoxeeUserActionsDirectory.cpp BoxeeItemsHistoryDirectory.cpp SourcesDirectory.cpp BoxeeShortcutsDirectory.cpp BoxeeServerOTADirectory.cpp BoxeeServerDirectory.cpp RepositoriesDirectory.cpp NetworkDirectory.cpp AppBoxDirectory.cpp FilePipe.cpp PipesManager.cpp

SRCS += BoxeeBrowseMenuDirectory.cpp

INCLUDES+=-I../lib/libUPnP/Platinum/Source/Core \
          -I../lib/libUPnP/Platinum/Source/Platinum \
          -I../lib/libUPnP/Platinum/Source/Devices/MediaServer \
          -I../lib/libUPnP/Neptune/Source/System/Posix \
          -I../lib/libUPnP/Neptune/Source/Core

LIB=filesystem.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
