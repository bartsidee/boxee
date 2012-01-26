include(../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . .. ../../linux ../../  ../../../guilib
DEFINES += -O3 -fPIC
		$(CXXFLAGS) -o $(SLIB) $(OBJS)
	$(CXX) -shared $(CXXFLAGS) -o $(SLIB) $(OBJS)
SOURCES += \


HEADERS += \


INCLUDEPATH += ../../../
