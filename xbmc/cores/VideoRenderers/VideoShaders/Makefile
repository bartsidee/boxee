INCLUDES=-I. -I.. -I../../ -I../../../ -I../../../linux -I../../../../guilib -I../../dvdplayer/Codecs -I../../dvdplayer/Codecs/ffmpeg
SRCS=YUV2RGBShader.cpp VideoFilterShader.cpp ConvolutionKernels.cpp 

LIB=VideoShaders.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
