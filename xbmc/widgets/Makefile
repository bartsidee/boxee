INCLUDES=-I. -I../linux -I../cores -I../../guilib -I../utils
INCLUDES+=-I../lib/libBoxee -I../lib/libBoxee/tinyxml -Ispidermonkey
INCLUDES+=-I../lib/libUPnP/Platinum/ThirdParty/Neptune/Source/Core -I../lib/libUPnP/Platinum/Source/Core -I../lib/libUPnP/Platinum/Source/Devices/MediaServer -I../lib/libUPnP/Platinum/ThirdParty/Neptune/Source/System/Posix

PCH=stdafx.h

SRCS=WidgetEngine.cpp WidgetParser.cpp WidgetElementFactory.cpp Exception.cpp MonkeyEngine.cpp XmlUtils.cpp
SRCS += WidgetElement.cpp Action.cpp Image.cpp URL.cpp Text.cpp Window.cpp Timer.cpp Widget.cpp
SRCS += JSText.cpp JSWindow.cpp JSWidget.cpp JSImage.cpp JSWidgetElement.cpp      

LIB=widgets.a 


include ../../Makefile.include
-include Makefile.depend
