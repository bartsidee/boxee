INCLUDES=-I. -I../ -I../linux -I../../guilib -I../cores -I../utils -I../lib/libBoxee -I../FileSystem

HEADERS = XAPP_App.h XAPP_Button.h XAPP_Control.h XAPP_Image.h XAPP_Label.h XAPP_List.h XAPP_ListItem.h XAPP_LocalConfig.h XAPP_ToggleButton.h XAPP_Window.h XAPP_MC.h XAPP_Player.h XAPP_PlayList.h AppException.h

SRCS=App_Python_Wrapper.cpp XAPP_App.cpp XAPP_Button.cpp XAPP_Control.cpp XAPP_Image.cpp XAPP_Label.cpp XAPP_List.cpp XAPP_ListItem.cpp XAPP_LocalConfig.cpp XAPP_MC.cpp XAPP_Player.cpp XAPP_PlayList.cpp XAPP_ToggleButton.cpp XAPP_Window.cpp XAPP_Edit.cpp XAPP_Http.cpp

LIB=app.a

all : mc_doc.i App_Python_Wrapper.cpp $(LIB)

App_Python_Wrapper.cpp: $(HEADERS) mc.i mc_doc.i fix_wrapper.sh
	swig -Wall -c++ -python mc.i
	./fix_wrapper.sh
	-rm mc_wrap.cxx
	mv mc.py ../../system/python/local

mc_doc.i : $(HEADERS) doxygen.conf doxygen2boxeedoc.php
	doxygen doxygen.conf
	python ../../tools/doxygen/doxy2swig.py xml/index.xml mc_doc.i
	xsltproc xml/combine.xslt xml/index.xml > all.xml
	php doxygen2boxeedoc.php > reference_doc.html

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
