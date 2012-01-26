INCLUDES=-I. -I../../ -I../../.. -I../../../../.. -I../../../../ -I../../../../utils -I../../../../linux -I../../../../../guilib -I../../Codecs -I../../Codecs/ffmpeg 
CXXFLAGS+=-DHAVE_MMX  -D__STDC_CONSTANT_MACROS

SRCS=	DVDAudioCodecFFmpeg.cpp \
	DVDAudioCodecLiba52.cpp \
	DVDAudioCodecLibDts.cpp \
	DVDAudioCodecLibFaad.cpp \
	DVDAudioCodecLibMad.cpp \
	DVDAudioCodecLPcm.cpp \
	DVDAudioCodecPassthrough.cpp \
	DVDAudioCodecPcm.cpp \
	DVDAudioCodecSMD.cpp \

LIB=Audio.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

