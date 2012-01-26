INCLUDES=-I../../../../guilib/tinyXML/

SRCS=action_store.cpp htmlutil.cpp lex_util.cpp node_set.cpp tokenlist.cpp xml_util.cpp xpath_expression.cpp xpath_processor.cpp xpath_stack.cpp xpath_static.cpp xpath_stream.cpp xpath_syntax.cpp

LIB=libtinyxpath.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
