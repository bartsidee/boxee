INCLUDES=-I. -I../ -I../../ -I../../linux -I../../../guilib -I../../utils 

SRCS=FlashVideoPlayer.cpp 

LIB=flashplayer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

