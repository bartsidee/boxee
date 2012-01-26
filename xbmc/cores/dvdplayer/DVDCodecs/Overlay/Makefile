INCLUDES=-I. -I../../ -I../../../../ -I../../../../.. -I../../../../linux -I../../../../../guilib -I../../Codecs -I../../Codecs/ffmpeg

CXXFLAGS+= -D__STDC_CONSTANT_MACROS

SRCS=	DVDOverlayCodecCC.cpp \
	DVDOverlayCodecFFmpeg.cpp \
	DVDOverlayCodecText.cpp \
	DVDOverlayCodecSSA.cpp \
	DVDOverlayCodecTX3G.cpp \
	libspucc/cc_decoder.c \

LIB=	Overlay.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

