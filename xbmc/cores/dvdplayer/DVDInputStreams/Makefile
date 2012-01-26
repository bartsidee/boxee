INCLUDES=-I. -I.. -I../.. -I../../../ -I../../../linux -I../../../../guilib -I../../../lib/libRTMP -I../Codecs -I../Codecs/ffmpeg
INCLUDES+=-I../../../utils -I../../../lib/libbluray/src -I../../../lib/libjson/include

CXXFLAGS += -D__STDC_FORMAT_MACROS  -D__STDC_CONSTANT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \

SRCS=	DVDFactoryInputStream.cpp \
	DVDInputStream.cpp \
	DVDInputStreamFile.cpp \
	DVDInputStreamHttp.cpp \
	DVDInputStreamMemory.cpp \
	DVDInputStreamNavigator.cpp \
	DVDStateSerializer.cpp \
	DVDInputStreamFFmpeg.cpp \
	DVDInputStreamTV.cpp \
	DVDInputStreamRTMP.cpp \
	DVDInputStreamStack.cpp \
	DVDInputStreamHTSP.cpp \
        DVDInputStreamMMS.cpp \
        DVDInputStreamPlaylist.cpp \
	DVDInputStreamBluray.cpp \

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

