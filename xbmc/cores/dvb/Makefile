INCLUDES=-I. -I.. -I../.. -I../../../ -I../../../.. -I../../utils -I../../linux -I../../../guilib -I../../FileSystem -I../../lib/libdvbpsi/src -I../../lib/libjson/include -I../../lib/sqlite3pp/ -I../../lib/libBoxee

SRCS=dvbchannel.cpp dvbchannels.cpp dvbtuner.cpp v4lhelper.cpp dvbmanager.cpp dvbepgloader.cpp dvbpatloader.cpp epgstore.cpp epgserverloader.cpp

LIB=dvb.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
