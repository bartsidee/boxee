INCLUDES+=-I../../FileSystem -I../../linux -I../../../guilib -I../../utils -I../../ -I.

SRCS=alac.c hairtunes.c socketlib.c shairport.c ao.cpp

LIB=libshairport.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
