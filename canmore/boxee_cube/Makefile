CXX=i686-cm-linux-g++
CC=i686-cm-linux-gcc
LDFLAGS=-L/opt/canmore/local/lib -lgdl -lEGL -ljpeg -lGLESv2
CFLAGS=-I/opt/canmore/local/include -I/opt/canmore/IntelCE/include -I/opt/canmore/IntelCE/include/linux_user -DHAS_EMBEDDED -D__STDC_CONSTANT_MACROS

all: boxee_cube

boxee_cube: boxee_cube.cpp cube2.o matrix.o readjpeg.o
	$(CXX) $(CFLAGS) -o boxee_cube boxee_cube.cpp  cube2.o matrix.o readjpeg.o $(LDFLAGS)
