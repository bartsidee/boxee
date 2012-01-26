INCLUDES=-I. -I../ -Icommon -I../xbmc -I../xbmc/cores -I../xbmc/linux -I../xbmc/utils -ItinyXML -I../xbmc/lib/libsquish -I../xbmc/cores/dvdplayer/Codecs -I../xbmc/cores/dvdplayer/Codecs/ffmpeg

CXXFLAGS+= -D__STDC_CONSTANT_MACROS

SRCS=SDL_anigif.c \
     AudioContext.cpp \
     DirectXGraphics.cpp \
     DDSImage.cpp \
     GraphicContext.cpp \
     GUIAudioManager.cpp \
     GUIBaseContainer.cpp \
     GUIButtonControl.cpp \
     GUIButtonScroller.cpp \
     GUICheckMarkControl.cpp \
     GUIControl.cpp \
     GUIControlFactory.cpp \
     GUIControlGroup.cpp \
     GUIControlGroupList.cpp \
     GUIDialog.cpp \
     GUIEditControl.cpp \
     GUIMultilineEditControl.cpp \
     GUIFadeLabelControl.cpp \
     GUIFixedListContainer.cpp \
     GUIFont.cpp \
     GUIFontManager.cpp \
     GUIFontTTF.cpp \
     GUIImage.cpp \
     GUIIncludes.cpp \
     GUILabelControl.cpp \
     GUIListContainer.cpp \
     GUIListGroup.cpp \
     GUIListItem.cpp \
     GUIListItemLayout.cpp \
     GUIMessage.cpp \
     GUIMoverControl.cpp \
     GUIMultiImage.cpp \
     GUIPanelContainer.cpp \
     GUIProgressControl.cpp \
     GUIRadioButtonControl.cpp \
     GUIResizeControl.cpp \
     GUIRSSControl.cpp \
     GUIScrollBarControl.cpp \
     GUISelectButtonControl.cpp \
     GUISettingsSliderControl.cpp \
     GUISliderControl.cpp \
     GUISpinControl.cpp \
     GUISpinControlEx.cpp \
     GUIStandardWindow.cpp \
     GUITextBox.cpp \
     GUITexture.cpp \
     GUITextureGL.cpp \
     GUITextureGLES.cpp \
     GUIToggleButtonControl.cpp \
     GUIVideoControl.cpp \
     GUIVisualisationControl.cpp \
     GUIWindow.cpp \
     GUIWindowManager.cpp \
     GUIWrappingListContainer.cpp \
     IWindowManagerCallback.cpp \
     Key.cpp \
     LocalizeStrings.cpp \
     SkinInfo.cpp \
     TextureBundleXPR.cpp \
     TextureBundleXBT.cpp \
     TextureBundle.cpp \
     TextureManager.cpp \
     VisibleEffect.cpp \
     XMLUtils.cpp \
     GUISound.cpp \
     GUIColorManager.cpp \
     FrameBufferObject.cpp \
     Shader.cpp \
     GUIListLabel.cpp \
     GUIBorderedImage.cpp \
     GUITextLayout.cpp \
     GUIMultiSelectText.cpp \
     GUIInfoTypes.cpp \
     GUIFontTTFGL.cpp \
     GUIFontTTFGLES.cpp \
     Texture.cpp \
     TextureGL.cpp \
     GUIControlProfiler.cpp \
     GUIKeyboardControl.cpp \
     XBTF.cpp \
     XBTFReader.cpp \
     GUIWebControl.cpp

LIB=guilib.a

include ../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

try : try.o TextureBundle.o DirectXGraphics.o GUIFontTTF.o
	g++ -o try try.o TextureBundle.o DirectXGraphics.o GraphicContext.o GUIIncludes.o ../xbmc/utils/CriticalSection.o ../xbmc/XBVideoConfig.o SkinInfo.o tinyXML/tinyxml.a ../xbmc/linux/CriticalSection.o GUIFontTTF.o GUIFontBase.o GUIFontManager.o GUIFont.o XMLUtils.o GUIImage.o GUIControl.o TextureManager.o GUIMessage.o ../xbmc/utils/SingleLock.o VisibleEffect.o GUIWindowManager.o AnimatedGif.o -lSDL_image -lSDL_gfx -lSDL -llzo -lfreetype


