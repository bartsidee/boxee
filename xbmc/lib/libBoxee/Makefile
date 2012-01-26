INCLUDES=-I../../ -I../../../guilib -Itinyxpath -I../../../guilib/tinyXML -I../../FileSystem -I../../linux -I../../utils -I../../lib/libjson/include

SRCS=boxee.cpp bxaudiodatabase.cpp bxbgprocess.cpp bxboxeefeed.cpp bxconfiguration.cpp bxconstants.cpp bxcredentials.cpp bxcurl.cpp bxdatabase.cpp bxexceptions.cpp bxfriend.cpp bxfriendslist.cpp bximetadataresolver.cpp bxmediadatabase.cpp bxmediafile.cpp bxmessages.cpp bxmetadata.cpp bxmetadataengine.cpp bxobject.cpp bxpicturedatabase.cpp bxrssreader.cpp bxstringmap.cpp bxutils.cpp bxvideodatabase.cpp bxxmldocument.cpp logger.cpp boxee_md5.cpp bxfeedfactory.cpp bxatomreader.cpp bxscheduletaskmanager.cpp bxcscmanager.cpp bxboxeeapplications.cpp bxboxeesubscriptions.cpp bxboxeeservices.cpp bxrecommendationsmanager.cpp bxqueuemanager.cpp bxfriendsmanager.cpp bxapplicationsmanager.cpp bxsubscriptionsmanager.cpp bxservicesmanager.cpp bxfeaturedmanager.cpp bxappboxmanager.cpp bxappboxapplications.cpp bxappboxrepositories.cpp bxappboxpopularities.cpp bxtrailersmanager.cpp bxgenresmanager.cpp bxuserprofiledatabase.cpp bxoemconfiguration.cpp bxboxeeentitlements.cpp bxentitlementsmanager.cpp bxsourcesmanager.cpp bxboxeesources.cpp bxwebfavoritesmanager.cpp bxboxeewebfavorites.cpp bxcurrentlocation.cpp

LIB=libBoxee.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
