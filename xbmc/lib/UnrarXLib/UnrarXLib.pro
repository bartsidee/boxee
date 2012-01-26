include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../ ../../linux ../../../guilib
DEFINES += SILENT
SOURCES += \
unicode.cpp \
errhnd.cpp \
filefn.cpp \
system.cpp \
rijndael.cpp \
strfn.cpp \
cmddata.cpp \
filestr.cpp \
rdwrfn.cpp \
file.cpp \
filcreat.cpp \
crypt.cpp \
unpack.cpp \
rarvm.cpp \
extract.cpp \
isnt.cpp \
int64.cpp \
rawread.cpp \
arcread.cpp \
encname.cpp \
scantree.cpp \
crc.cpp \
ulinks.cpp \
savepos.cpp \
options.cpp \
timefn.cpp \
getbits.cpp \
recvol.cpp \
find.cpp \
archive.cpp \
match.cpp \
log.cpp \
rs.cpp \
extinfo.cpp \
sha1.cpp \
global.cpp \
strlist.cpp \
volume.cpp \
resource.cpp \
rar.cpp \
consio.cpp \
pathfn.cpp \


HEADERS += \
unicode.h \
errhnd.h \
filefn.h \
system.h \
rijndael.h \
strfn.h \
cmddata.h \
filestr.h \
rdwrfn.h \
file.h \
filcreat.h \
crypt.h \
unpack.h \
rarvm.h \
extract.h \
isnt.h \
int64.h \
rawread.h \
arcread.h \
encname.h \
scantree.h \
crc.h \
ulinks.h \
savepos.h \
options.h \
timefn.h \
getbits.h \
recvol.h \
find.h \
archive.h \
match.h \
log.h \
rs.h \
extinfo.h \
sha1.h \
global.h \
strlist.h \
volume.h \
resource.h \
rar.h \
consio.h \
pathfn.h \


INCLUDEPATH += ../../../
