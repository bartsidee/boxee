INCLUDES=-I. -I../ -I../linux -I../../guilib -I../.. -I../cores -I../utils -I../lib/libBoxee -I../FileSystem -I../cores/dvdplayer/Codecs -I../cores/dvdplayer/Codecs/ffmpeg

CXXFLAGS+=  -D__STDC_CONSTANT_MACROS

SRCS=BXNativeApp.cpp NativeApplication.cpp NativeApplicationWindow.cpp

LIB=nativeapp.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
