INCLUDES+=-I. -I../libjson -I../../ -I../../linux -I../../../guilib
SRCS=BoxeeHalClient.cpp Functions.cpp JsonWrapper.cpp
LIB=libBoxeeHalClient.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

