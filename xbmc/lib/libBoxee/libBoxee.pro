include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += ../../ ../../../guilib tinyxpath ../../../guilib/tinyXML ../../FileSystem ../../linux ../../utils
SOURCES += \
bxsourcesmanager.cpp \
bxstringmap.cpp \
bxmessages.cpp \
bxoemconfiguration.cpp \
bxfeedfactory.cpp \
bxmediadatabase.cpp \
bxboxeesubscriptions.cpp \
bxvideodatabase.cpp \
bxmediafile.cpp \
bxtrailersmanager.cpp \
bxuserprofiledatabase.cpp \
bxfriendslist.cpp \
bxmetadataengine.cpp \
bxservicesmanager.cpp \
bxaudiodatabase.cpp \
bxboxeeentitlements.cpp \
bxappboxpopularities.cpp \
boxee.cpp \
bxcscmanager.cpp \
bxfriend.cpp \
bxxmldocument.cpp \
bxwebfavoritesmanager.cpp \
bxappboxmanager.cpp \
bxbgprocess.cpp \
bxatomreader.cpp \
bxboxeeapplications.cpp \
bxscheduletaskmanager.cpp \
bxentitlementsmanager.cpp \
bxboxeeservices.cpp \
logger.cpp \
bxapplicationsmanager.cpp \
bxsubscriptionsmanager.cpp \
bxboxeefeed.cpp \
bxexceptions.cpp \
bximetadataresolver.cpp \
bxutils.cpp \
bxdatabase.cpp \
bxboxeesources.cpp \
bxmetadata.cpp \
bxboxeewebfavorites.cpp \
bxappboxapplications.cpp \
bxqueuemanager.cpp \
bxfriendsmanager.cpp \
bxpicturedatabase.cpp \
bxrssreader.cpp \
bxgenresmanager.cpp \
bxappboxrepositories.cpp \
bxconfiguration.cpp \
bxcurl.cpp \
bxrecommendationsmanager.cpp \
bxfeaturedmanager.cpp \
bxconstants.cpp \
boxee_md5.cpp \
bxobject.cpp \
bxcredentials.cpp \


HEADERS += \
bxsourcesmanager.h \
bxstringmap.h \
bxmessages.h \
bxoemconfiguration.h \
bxfeedfactory.h \
bxmediadatabase.h \
bxboxeesubscriptions.h \
bxvideodatabase.h \
bxmediafile.h \
bxtrailersmanager.h \
bxuserprofiledatabase.h \
bxfriendslist.h \
bxmetadataengine.h \
bxservicesmanager.h \
bxaudiodatabase.h \
bxboxeeentitlements.h \
bxappboxpopularities.h \
boxee.h \
bxcscmanager.h \
bxfriend.h \
bxxmldocument.h \
bxwebfavoritesmanager.h \
bxappboxmanager.h \
bxbgprocess.h \
bxatomreader.h \
bxboxeeapplications.h \
bxscheduletaskmanager.h \
bxentitlementsmanager.h \
bxboxeeservices.h \
logger.h \
bxapplicationsmanager.h \
bxsubscriptionsmanager.h \
bxboxeefeed.h \
bxexceptions.h \
bximetadataresolver.h \
bxutils.h \
bxdatabase.h \
bxboxeesources.h \
bxmetadata.h \
bxboxeewebfavorites.h \
bxappboxapplications.h \
bxqueuemanager.h \
bxfriendsmanager.h \
bxpicturedatabase.h \
bxrssreader.h \
bxgenresmanager.h \
bxappboxrepositories.h \
bxconfiguration.h \
bxcurl.h \
bxrecommendationsmanager.h \
bxfeaturedmanager.h \
bxconstants.h \
boxee_md5.h \
bxobject.h \
bxcredentials.h \


INCLUDEPATH += ../../../
