INCLUDES=-I. -I../ -I../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer -I../paplayer -I../../.. -I../dvdplayer/Codecs -I../dvdplayer/Codecs/ffmpeg
CXXFLAGS+= -D__STDC_CONSTANT_MACROS

SRCS=PlayerCoreFactory.cpp \
     PlayerSelectionRule.cpp

LIB=playercorefactory.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

