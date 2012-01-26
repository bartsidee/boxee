INCLUDES=-I. 
SRCS=list.c range.c statusq.c
LIB=libnbtscan.a
DEFINES= -DTIME_WITH_SYS_TIME=1 -DHAVE_SNPRINTF=1 -DHAVE_INET_ATON=1 -DHAVE_SOCKET=1

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
