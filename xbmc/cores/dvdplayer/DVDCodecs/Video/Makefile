INCLUDES=-I. -I../../ -I../../../../ -I../../../ffmpeg -I../../../../linux -I../../../../../guilib

CFLAGS += -D__STDC_CONSTANT_MACROS

SRCS=	DVDVideoCodecFFmpeg.cpp \
	DVDVideoCodecLibMpeg2.cpp \
	DVDVideoPPFFmpeg.cpp \
	VDPAU.cpp \
	DVDVideoCodecVDA.cpp

LIB=Video.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

