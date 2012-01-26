include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../cores ../linux ../../guilib ../lib/UnrarXLib ../utils
INCLUDEPATH += ../lib/libcdio/libcdio/include
INCLUDEPATH += ../lib/libBoxee
INCLUDEPATH += ../lib/libUPnP/Platinum/Source/Core \
DEFINES += __STDC_FORMAT_MACROS \
SOURCES += \
UPnPVirtualPathDirectory.cpp \
HTSPDirectory.cpp \
 \
UDFDirectory.cpp \
FileHD.cpp \
HTTPDirectory.cpp \
FileSmb.cpp \
DllLibCurl.cpp \
iso9660.cpp \
AppBoxDirectory.cpp \
FileFileReader.cpp \
DirectoryHistory.cpp \
PlaylistFileDirectory.cpp \
NptXbmcFile.cpp \
CMythSession.cpp \
DirectoryTuxBox.cpp \
VirtualDirectory.cpp \
SpecialProtocolDirectory.cpp \
MusicFileDirectory.cpp \
SmartPlaylistDirectory.cpp \
DAAPDirectory.cpp \
RarManager.cpp \
FileTuxBox.cpp \
FileRar.cpp \
VTPDirectory.cpp \
FileFactory.cpp \
MusicSearchDirectory.cpp \
FTPParse.cpp \
CDDADirectory.cpp \
FileCDDA.cpp \
ScriptDirectory.cpp \
PipesManager.cpp \
FileUDF.cpp \
cddb.cpp \
BoxeeUserActionsDirectory.cpp \
HTSPSession.cpp \
RTVDirectory.cpp \
PlaylistDirectory.cpp \
RingBuffer.cpp \
VideoDatabaseDirectory.cpp \
RepositoriesDirectory.cpp \
FileCurl.cpp \
BoxeeFriendsDirectory.cpp \
FactoryDirectory.cpp \
PluginDirectory.cpp \
MusicDatabaseDirectory.cpp \
SAPFile.cpp \
udf25.cpp \
BoxeeItemsHistoryDirectory.cpp \
IDirectory.cpp \
BoxeeShortcutsDirectory.cpp \
SpecialProtocol.cpp \
CacheStrategy.cpp \
FileRTV.cpp \
BoxeeFeedDirectory.cpp \
FileSpecialProtocol.cpp \
BoxeeDatabaseDirectory.cpp \
Directory.cpp \
MultiPathDirectory.cpp \
LastFMDirectory.cpp \
DirectoryCache.cpp \
FileDAAP.cpp \
SMBDirectory.cpp \
XBMSDirectory.cpp \
SAPDirectory.cpp \
FTPDirectory.cpp \
FileXBMSP.cpp \
HDHomeRun.cpp \
CMythDirectory.cpp \
ZeroconfDirectory.cpp \
FileLastFM.cpp \
FilePipe.cpp \
ShoutcastDirectory.cpp \
StackDirectory.cpp \
CacheMemBuffer.cpp \
AppsDirectory.cpp \
ASAPFileDirectory.cpp \
FileISO.cpp \
NSFFileDirectory.cpp \
BoxeeServerDirectory.cpp \
File.cpp \
FactoryFileDirectory.cpp \
SourcesDirectory.cpp \
ZipDirectory.cpp \
HDDirectory.cpp \
RSSDirectory.cpp \
FileCache.cpp \
VTPFile.cpp \
ShoutcastRipFile.cpp \
VTPSession.cpp \
ISO9660Directory.cpp \
FilePlaylist.cpp \
cdioSupport.cpp \
ZipManager.cpp \
UPnPDirectory.cpp \
RarDirectory.cpp \
NetworkDirectory.cpp \
OGGFileDirectory.cpp \
CMythFile.cpp \
SIDFileDirectory.cpp \
FileMusicDatabase.cpp \
FileZip.cpp \
FileUPnP.cpp \
IFile.cpp \
MultiPathFile.cpp \
FileShoutcast.cpp \
VirtualPathDirectory.cpp \


HEADERS += \
UPnPVirtualPathDirectory.h \
HTSPDirectory.h \
 \
UDFDirectory.h \
FileHD.h \
HTTPDirectory.h \
FileSmb.h \
DllLibCurl.h \
iso9660.h \
AppBoxDirectory.h \
FileFileReader.h \
DirectoryHistory.h \
PlaylistFileDirectory.h \
NptXbmcFile.h \
CMythSession.h \
DirectoryTuxBox.h \
VirtualDirectory.h \
SpecialProtocolDirectory.h \
MusicFileDirectory.h \
SmartPlaylistDirectory.h \
DAAPDirectory.h \
RarManager.h \
FileTuxBox.h \
FileRar.h \
VTPDirectory.h \
FileFactory.h \
MusicSearchDirectory.h \
FTPParse.h \
CDDADirectory.h \
FileCDDA.h \
ScriptDirectory.h \
PipesManager.h \
FileUDF.h \
cddb.h \
BoxeeUserActionsDirectory.h \
HTSPSession.h \
RTVDirectory.h \
PlaylistDirectory.h \
RingBuffer.h \
VideoDatabaseDirectory.h \
RepositoriesDirectory.h \
FileCurl.h \
BoxeeFriendsDirectory.h \
FactoryDirectory.h \
PluginDirectory.h \
MusicDatabaseDirectory.h \
SAPFile.h \
udf25.h \
BoxeeItemsHistoryDirectory.h \
IDirectory.h \
BoxeeShortcutsDirectory.h \
SpecialProtocol.h \
CacheStrategy.h \
FileRTV.h \
BoxeeFeedDirectory.h \
FileSpecialProtocol.h \
BoxeeDatabaseDirectory.h \
Directory.h \
MultiPathDirectory.h \
LastFMDirectory.h \
DirectoryCache.h \
FileDAAP.h \
SMBDirectory.h \
XBMSDirectory.h \
SAPDirectory.h \
FTPDirectory.h \
FileXBMSP.h \
HDHomeRun.h \
CMythDirectory.h \
ZeroconfDirectory.h \
FileLastFM.h \
FilePipe.h \
ShoutcastDirectory.h \
StackDirectory.h \
CacheMemBuffer.h \
AppsDirectory.h \
ASAPFileDirectory.h \
FileISO.h \
NSFFileDirectory.h \
BoxeeServerDirectory.h \
File.h \
FactoryFileDirectory.h \
SourcesDirectory.h \
ZipDirectory.h \
HDDirectory.h \
RSSDirectory.h \
FileCache.h \
VTPFile.h \
ShoutcastRipFile.h \
VTPSession.h \
ISO9660Directory.h \
FilePlaylist.h \
cdioSupport.h \
ZipManager.h \
UPnPDirectory.h \
RarDirectory.h \
NetworkDirectory.h \
OGGFileDirectory.h \
CMythFile.h \
SIDFileDirectory.h \
FileMusicDatabase.h \
FileZip.h \
FileUPnP.h \
IFile.h \
MultiPathFile.h \
FileShoutcast.h \
VirtualPathDirectory.h \


INCLUDEPATH += ../../
