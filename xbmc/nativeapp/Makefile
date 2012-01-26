INCLUDES=-I. -I../ -I../linux -I../../guilib -I../cores -I../utils -I../lib/libBoxee -I../FileSystem

SRCS=BXNativeApp.cpp NativeApplication.cpp NativeApplicationWindow.cpp

LIB=nativeapp.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
