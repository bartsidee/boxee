GXX=g++

FLAGS=-g -Wall -W -Werror -Wno-unused
JS_FLAGS=

.PHONY: all

all: jsgen-osx

jsgen-osx: jsgen.cpp jsgenio.cpp jsgenjs.cpp
	$(GXX) jsgen.cpp -o $@ -g -Wall
