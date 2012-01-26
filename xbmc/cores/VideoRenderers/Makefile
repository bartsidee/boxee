INCLUDES=-I. -I.. -I../../ -I../../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer/Codecs -I../dvdplayer/Codecs/ffmpeg

CXXFLAGS +=  -D__STDC_CONSTANT_MACROS

SRCS=LinuxRenderer.cpp \
     RenderManager.cpp \
     LinuxRendererGL.cpp \
     LinuxRendererOMX.cpp \
     IntelSMDRenderer.cpp \
     OverlayRenderer.cpp \
     OverlayRendererGL.cpp \
     OverlayRendererUtil.cpp \
     BaseRenderer.cpp \
     NullRenderer.cpp \
     RenderCapture.cpp

LIB=VideoRenderer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
