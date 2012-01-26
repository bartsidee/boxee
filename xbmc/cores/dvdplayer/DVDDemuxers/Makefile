INCLUDES=-I. -I.. -I../.. -I../../../ -I../../../.. -I../../../utils -I../../../linux -I../../../../guilib -I../Codecs -I../Codecs/ffmpeg -I../../../lib/libjson/include -I../../../lib/libdvbpsi/src
CXXFLAGS+=-D__STDC_CONSTANT_MACROS \

SRCS=	DVDDemux.cpp \
	DVDDemuxFFmpeg.cpp \
	DVDDemuxShoutcast.cpp \
	DVDDemuxUtils.cpp \
	DVDFactoryDemuxer.cpp \
	DVDDemuxVobsub.cpp \
	DVDDemuxHTSP.cpp \
	DVDDemuxSMD.cpp \

LIB=	DVDDemuxers.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

