
noinst_LIBRARIES=libtinyxpath.a
libtinyxpath_a_SOURCES = htmlutil.cpp \
   action_store.cpp lex_util.cpp node_set.cpp tokenlist.cpp xml_util.cpp \
   xpath_expression.cpp xpath_processor.cpp xpath_stack.cpp xpath_stream.cpp xpath_syntax.cpp xpath_static.cpp

# set the include path found by configure
INCLUDES= $(all_includes) -I../../../../guilib/tinyXML

AM_CXXFLAGS = -Wall -DTIXML_USE_STL

