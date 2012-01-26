INCLUDES=-I. -I../ -Iffmpeg -I../linux -I../../guilib -I../utils -Idvdplayer -I./flashplayer -I./dvdplayer/Codecs/ffmpeg

SRCS=DummyVideoPlayer.cpp \
     ssrc.cpp \
     dlgcache.cpp \
     IntelSMDGlobals.cpp

LIB=cores.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
