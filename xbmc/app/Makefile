INCLUDES=-I. -I../ -I../linux -I../../guilib -I../cores -I../utils -I../lib/libBoxee -I../FileSystem -I../lib/libPython -I../lib/libjson/include

HEADERS = XAPP_App.h XAPP_Button.h XAPP_Control.h XAPP_Image.h XAPP_Label.h XAPP_List.h XAPP_ListItem.h XAPP_LocalConfig.h XAPP_ToggleButton.h XAPP_Window.h XAPP_MC.h XAPP_Player.h XAPP_PlayList.h AppException.h XAPP_Native.h XAPP_Textbox.h

SRCS=App_Python_Wrapper.cpp XAPP_App.cpp XAPP_Button.cpp XAPP_Control.cpp XAPP_Image.cpp XAPP_Label.cpp XAPP_List.cpp XAPP_ListItem.cpp XAPP_LocalConfig.cpp XAPP_MC.cpp XAPP_Player.cpp XAPP_PlayList.cpp XAPP_ToggleButton.cpp XAPP_Window.cpp XAPP_Edit.cpp XAPP_Http.cpp XAPP_Native.cpp XAPP_Textbox.cpp

LIB=app.a

all : mc_doc.i App_Python_Wrapper.cpp $(LIB)

App_Python_Wrapper.cpp: $(HEADERS) mc.i mc_doc.i fix_wrapper.sh
	swig  -Wall -c++ -python mc.i
	./fix_wrapper.sh mc_wrap.cxx App_Python_Wrapper.cpp
	-rm mc_wrap.cxx
	mv mc.py ../../system/python/local
	patch -p0 < remove_loop.patch
	patch -p0 < reduce_exceptions.patch
	patch -p0 < showdialogselect_fix.patch

mc_doc.i : $(HEADERS) doxygen.conf doxygen2boxeedoc.php
	doxygen doxygen.conf
	python ../../tools/doxygen/doxy2swig.py xml/index.xml mc_doc.i
	xsltproc xml/combine.xslt xml/index.xml > all.xml
	php doxygen2boxeedoc.php > reference_doc.html

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
