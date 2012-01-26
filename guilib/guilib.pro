include(../boxee.pri)

TEMPLATE = lib
CONFIG += lib_bundle staticlib
CONFIG -= app_bundle
QT += core gui opengl

MOC_DIR=build
OBJECTS_DIR=build
INCLUDEPATH += . ../ common ../xbmc ../xbmc/cores ../xbmc/linux ../xbmc/utils tinyXML ../xbmc/lib/libsquish ../xbmc/cores/dvdplayer/Codecs ../xbmc/cores/dvdplayer/Codecs/ffmpeg
DEFINES +=  __STDC_CONSTANT_MACROS
SOURCES += \
 \
GUIScrollBarControl.cpp \
GUIResizeControl.cpp \
GUIDialog.cpp \
GUIFontTTFGL.cpp \
Shader.cpp \
GUIControlFactory.cpp \
GUIMultiImage.cpp \
TextureBundleXBT.cpp \
GUITexture.cpp \
GUIListLabel.cpp \
XBTFReader.cpp \
GUIListContainer.cpp \
GUIIncludes.cpp \
GUISettingsSliderControl.cpp \
GUIFontTTF.cpp \
SDL_anigif.c \
GUIAudioManager.cpp \
Texture.cpp \
VisibleEffect.cpp \
GUISliderControl.cpp \
GUIWindowManager.cpp \
GUIProgressControl.cpp \
XBTF.cpp \
GUIStandardWindow.cpp \
GUIMultilineEditControl.cpp \
GUIFont.cpp \
GUICheckMarkControl.cpp \
GUISpinControl.cpp \
GUIControlGroup.cpp \
GUISound.cpp \
GUIVideoControl.cpp \
TextureGL.cpp \
GUITextureGL.cpp \
AudioContext.cpp \
GUITextBox.cpp \
GUIMessage.cpp \
GUIBaseContainer.cpp \
GUIButtonControl.cpp \
GUIInfoTypes.cpp \
IWindowManagerCallback.cpp \
GUIRadioButtonControl.cpp \
GUIBorderedImage.cpp \
GUIControlProfiler.cpp \
GUIListGroup.cpp \
GUIFontTTFGLES.cpp \
GUIWindow.cpp \
GUIFadeLabelControl.cpp \
GUIToggleButtonControl.cpp \
GUIControlGroupList.cpp \
GUIFontManager.cpp \
GUITextureGLES.cpp \
GUIListItem.cpp \
GUIButtonScroller.cpp \
GUIVisualisationControl.cpp \
GUITextLayout.cpp \
GUISelectButtonControl.cpp \
GUIPanelContainer.cpp \
GUIRSSControl.cpp \
TextureManager.cpp \
GUILabelControl.cpp \
XMLUtils.cpp \
TextureBundle.cpp \
GUIKeyboardControl.cpp \
GraphicContext.cpp \
GUIMoverControl.cpp \
GUIEditControl.cpp \
GUIControl.cpp \
GUISpinControlEx.cpp \
GUIMultiSelectText.cpp \
Key.cpp \
GUIFixedListContainer.cpp \
FrameBufferObject.cpp \
GUIWrappingListContainer.cpp \
DDSImage.cpp \
DirectXGraphics.cpp \
GUIColorManager.cpp \
SkinInfo.cpp \
GUIListItemLayout.cpp \
TextureBundleXPR.cpp \
LocalizeStrings.cpp \
GUIImage.cpp \


HEADERS += \
 \
GUIScrollBarControl.h \
GUIResizeControl.h \
GUIDialog.h \
GUIFontTTFGL.h \
Shader.h \
GUIControlFactory.h \
GUIMultiImage.h \
TextureBundleXBT.h \
GUITexture.h \
GUIListLabel.h \
XBTFReader.h \
GUIListContainer.h \
GUIIncludes.h \
GUISettingsSliderControl.h \
GUIFontTTF.h \
SDL_anigif.h \
GUIAudioManager.h \
Texture.h \
VisibleEffect.h \
GUISliderControl.h \
GUIWindowManager.h \
GUIProgressControl.h \
XBTF.h \
GUIStandardWindow.h \
GUIMultilineEditControl.h \
GUIFont.h \
GUICheckMarkControl.h \
GUISpinControl.h \
GUIControlGroup.h \
GUISound.h \
GUIVideoControl.h \
TextureGL.h \
GUITextureGL.h \
AudioContext.h \
GUITextBox.h \
GUIMessage.h \
GUIBaseContainer.h \
GUIButtonControl.h \
GUIInfoTypes.h \
IWindowManagerCallback.h \
GUIRadioButtonControl.h \
GUIBorderedImage.h \
GUIControlProfiler.h \
GUIListGroup.h \
GUIFontTTFGLES.h \
GUIWindow.h \
GUIFadeLabelControl.h \
GUIToggleButtonControl.h \
GUIControlGroupList.h \
GUIFontManager.h \
GUITextureGLES.h \
GUIListItem.h \
GUIButtonScroller.h \
GUIVisualisationControl.h \
GUITextLayout.h \
GUISelectButtonControl.h \
GUIPanelContainer.h \
GUIRSSControl.h \
TextureManager.h \
GUILabelControl.h \
XMLUtils.h \
TextureBundle.h \
GUIKeyboardControl.h \
GraphicContext.h \
GUIMoverControl.h \
GUIEditControl.h \
GUIControl.h \
GUISpinControlEx.h \
GUIMultiSelectText.h \
Key.h \
GUIFixedListContainer.h \
FrameBufferObject.h \
GUIWrappingListContainer.h \
DDSImage.h \
DirectXGraphics.h \
GUIColorManager.h \
SkinInfo.h \
GUIListItemLayout.h \
TextureBundleXPR.h \
LocalizeStrings.h \
GUIImage.h \


