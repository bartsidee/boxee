include(../../../../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../../../ ../../../linux ../../../.. ../../../../guilib ../../../utils ../../../cores ../../libBoxee ../../../cores ../../../../guilib/tinyXML ../../../cores/dvdplayer/Codecs ../../../cores/dvdplayer/Codecs/ffmpeg
DEFINES +=   __STDC_FORMAT_MACROS  __STDC_CONSTANT_MACROS
SOURCES += \
xbmcplugin.cpp \
controlfadelabel.cpp \
controllabel.cpp \
controlimage.cpp \
pyutil.cpp \
dialog.cpp \
listitem.cpp \
controlspin.cpp \
controllist.cpp \
xbmcmodule.cpp \
xbmcguimodule.cpp \
PythonSettings.cpp \
control.cpp \
window.cpp \
controlgroup.cpp \
infotagvideo.cpp \
GUIPythonWindowDialog.cpp \
player.cpp \
controlcheckmark.cpp \
GUIPythonWindowXML.cpp \
GUIPythonWindow.cpp \
winxmldialog.cpp \
GUIPythonWindowXMLDialog.cpp \
controlbutton.cpp \
language.cpp \
PythonPlayer.cpp \
keyboard.cpp \
action.cpp \
infotagmusic.cpp \
winxml.cpp \
pyplaylist.cpp \
controltextbox.cpp \
controlprogress.cpp \
controlradiobutton.cpp \


HEADERS += \
xbmcplugin.h \
controlfadelabel.h \
controllabel.h \
controlimage.h \
pyutil.h \
dialog.h \
listitem.h \
controlspin.h \
controllist.h \
xbmcmodule.h \
xbmcguimodule.h \
PythonSettings.h \
control.h \
window.h \
controlgroup.h \
infotagvideo.h \
GUIPythonWindowDialog.h \
player.h \
controlcheckmark.h \
GUIPythonWindowXML.h \
GUIPythonWindow.h \
winxmldialog.h \
GUIPythonWindowXMLDialog.h \
controlbutton.h \
language.h \
PythonPlayer.h \
keyboard.h \
action.h \
infotagmusic.h \
winxml.h \
pyplaylist.h \
controltextbox.h \
controlprogress.h \
controlradiobutton.h \


INCLUDEPATH += ../../../../
