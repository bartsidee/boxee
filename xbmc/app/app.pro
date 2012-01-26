include(../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ ../.. ../linux ../../guilib ../cores ../utils ../lib/libBoxee ../FileSystem ../lib/libPython

SOURCES +=  App_Python_Wrapper.cpp XAPP_App.cpp XAPP_Button.cpp XAPP_Control.cpp XAPP_Image.cpp XAPP_Label.cpp XAPP_List.cpp XAPP_ListItem.cpp XAPP_LocalConfig.cpp XAPP_MC.cpp XAPP_Player.cpp XAPP_PlayList.cpp XAPP_ToggleButton.cpp XAPP_Window.cpp XAPP_Edit.cpp XAPP_Http.cpp

HEADERS += XAPP_App.h XAPP_Button.h XAPP_Control.h XAPP_Image.h XAPP_Label.h XAPP_List.h XAPP_ListItem.h XAPP_LocalConfig.h XAPP_ToggleButton.h XAPP_Window.h XAPP_MC.h XAPP_Player.h XAPP_PlayList.h AppException.h

# add a build command
defineReplace( nc  ) { 
  return( $$escape_expand(\n\t)$$1    ) 
}

# add a silent build command
defineReplace( snc ) { 
  return( $$escape_expand(\n\t)"@"$$1 ) 
}

# add end of line
defineReplace( nl  ) { 
  return( $$escape_expand(\n)         ) 
}

wrapper.target=App_Python_Wrapper.cpp
wrapper.commands = swig  -Wall -c++ -python mc.i 
wrapper.commands +=$$nc( ./fix_wrapper.sh )
wrapper.commands +=$$nc( -rm mc_wrap.cxx )
wrapper.commands +=$$nc( mv mc.py ../../system/python/local )
wrapper.commands +=$$nc( patch -p0 < remove_loop.patch )
wrapper.commands +=$$nc( patch -p0 < reduce_exceptions.patch )
wrapper.depends = $$HEADERS mc.i mc_doc.i fix_wrapper.sh

mc_doc.target = mc_doc.i
mc_doc.depends = $$HEADERS doxygen.conf doxygen2boxeedoc.php
mc_doc.commands = doxygen doxygen.conf
mc_doc.commands += $$nc(python ../../tools/doxygen/doxy2swig.py xml/index.xml mc_doc.i)
mc_doc.commands += $$nc(xsltproc xml/combine.xslt xml/index.xml > all.xml)
mc_doc.commands += $$nc(php doxygen2boxeedoc.php > reference_doc.html)

QMAKE_EXTRA_TARGETS += wrapper mc_doc


