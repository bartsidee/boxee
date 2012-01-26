INCLUDES=-I. -I../../ -I../../../../ -I../../../../.. -I../../../../linux -I../../../../../guilib -I../../../ -I../../Codecs/ffmpeg -I../../Codecs -I../../../ffmpeg

CFLAGS += -D__STDC_CONSTANT_MACROS
CXXFLAGS += -D__STDC_CONSTANT_MACROS

SRCS=	DVDVideoCodecFFmpeg.cpp \
	DVDVideoCodecLibMpeg2.cpp \
	DVDVideoPPFFmpeg.cpp \
	VDPAU.cpp \
	DVDVideoCodecOmx.cpp \
	DVDVideoCodecSMD.cpp \
	IntelSMDVideo.cpp \
	../DVDOmxUtils.cpp \
	DVDVideoCodecVDA.cpp

LIB=Video.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

