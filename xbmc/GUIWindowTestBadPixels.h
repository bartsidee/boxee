#pragma once

#include "GUIWindow.h"

class CGUIWindowTestBadPixels : public CGUIWindow
{
public:

  CGUIWindowTestBadPixels();
  virtual ~CGUIWindowTestBadPixels();

  virtual bool OnAction(const CAction &action);

  virtual void Render();

  void SetColorItem(CFileItemPtr colorItem);

protected:

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual void DrawBadPixelsTest() = 0;

  bool SetInitializeColor();

  int m_colorIndex;
  CFileItemPtr m_colorItem;
  bool m_rotateColors;
  color_t m_color;
  int m_top;
  int m_bottom;
  int m_left;
  int m_right;

  unsigned long m_renderCounter;
};

class CGUIWindowTestBadPixelsGL : public CGUIWindowTestBadPixels
{
public:

  CGUIWindowTestBadPixelsGL();
  virtual ~CGUIWindowTestBadPixelsGL();

protected:

  virtual void DrawBadPixelsTest();
};

class CGUIWindowTestBadPixelsGLES : public CGUIWindowTestBadPixels
{
public:

  CGUIWindowTestBadPixelsGLES();
  virtual ~CGUIWindowTestBadPixelsGLES();

protected:

  virtual void DrawBadPixelsTest();
};

class CGUIWindowTestBadPixelsDX : public CGUIWindowTestBadPixels
{
public:

  CGUIWindowTestBadPixelsDX();
  virtual ~CGUIWindowTestBadPixelsDX();

protected:

  virtual void DrawBadPixelsTest();
};

