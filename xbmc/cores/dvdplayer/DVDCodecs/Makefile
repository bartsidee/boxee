INCLUDES=-I. -I../ -I../../ -I../../../ -I../../../.. -I../../../linux -I../../../../guilib -I../Codecs -I../Codecs/ffmpeg -I../../ffmpeg

CXXFLAGS+=  -D__STDC_CONSTANT_MACROS
SRCS=	DVDCodecUtils.cpp \
	DVDFactoryCodec.cpp \

LIB=	DVDCodecs.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

