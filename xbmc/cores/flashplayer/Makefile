INCLUDES=-I. -I../ -I../../ -I../../linux -I../../../guilib -I../../utils  -I../dvdplayer/Codecs -I../dvdplayer/Codecs/ffmpeg -I../../lib/libjson/include
CXXFLAGS+= -D__STDC_CONSTANT_MACROS
SRCS=FlashVideoPlayer.cpp 

LIB=flashplayer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

