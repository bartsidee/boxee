#include "GUIWindowTestBadPixels.h"
#include "GUIWindowTestBadPixelsManager.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "GUITexture.h"
#include "log.h"
#ifdef HAS_GLES
#include "GUITextureGLES.h"
#endif

#define SHOW_COLOR_IN_SEC 8

CGUIWindowTestBadPixels::CGUIWindowTestBadPixels() : CGUIWindow(WINDOW_TEST_BAD_PIXELS, "")
{
  m_top = 0;
  m_bottom = 0;
  m_left = 0;
  m_right = 0;

  m_color = 0;
}

CGUIWindowTestBadPixels::~CGUIWindowTestBadPixels()
{

}

void CGUIWindowTestBadPixels::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  m_top = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.top;
  m_bottom = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.bottom;
  m_left = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.left;
  m_right = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.right;

  if (!m_colorItem.get())
  {
    CLog::Log(LOGERROR,"CGUIWindowTestBadPixels::OnInitWindow - Color item WASN'T initialized (tbp)");
    Close();
  }

  m_colorIndex = -1;
  m_rotateColors = false;

  if (!SetInitializeColor())
  {
    CLog::Log(LOGERROR,"CGUIWindowTestBadPixels::OnInitWindow - FAILED to set color (tbp)");
    Close();
  }

  m_renderCounter = 0;
}

void CGUIWindowTestBadPixels::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);

  m_colorItem.reset();
  m_colorIndex = -1;
  m_rotateColors = false;
  m_renderCounter = 0;
}

bool CGUIWindowTestBadPixels::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_PARENT_DIR:
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  break;
  }

  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

void CGUIWindowTestBadPixels::Render()
{
  if (m_rotateColors)
  {
    m_renderCounter++;

    if (m_renderCounter % (SHOW_COLOR_IN_SEC*60) == 0)
    {
      m_renderCounter = 0;

      //////////////////////////////////////////
      // update color every SHOW_COLOR_IN_SEC //
      //////////////////////////////////////////

      m_colorIndex++;
      if (m_colorIndex >= CGUIWindowTestBadPixelsManager::GetNumOfColors())
      {
        m_colorIndex = 0;
      }

      m_color = CGUIWindowTestBadPixelsManager::GetColor(m_colorIndex);
    }
  }

  DrawBadPixelsTest();

  CGUIWindow::Render();
}

void CGUIWindowTestBadPixels::SetColorItem(CFileItemPtr colorItem)
{
  m_colorItem = colorItem;
}

bool CGUIWindowTestBadPixels::SetInitializeColor()
{
  bool succeeded = false;

  if (m_colorItem->HasProperty("color-index"))
  {
    int colorIndex = m_colorItem->GetPropertyInt("color-index");
    if (colorIndex < CGUIWindowTestBadPixelsManager::GetNumOfColors())
    {
      m_colorIndex = colorIndex;
      m_color = CGUIWindowTestBadPixelsManager::GetColor(m_colorIndex);
      succeeded = true;
    }
  }
  else if (CGUIWindowTestBadPixelsManager::GetNumOfColors() > 0)
  {
    m_rotateColors = true;
    m_colorIndex = 0;
    m_color = CGUIWindowTestBadPixelsManager::GetColor(m_colorIndex);
    succeeded = true;
  }

  return succeeded;
}

#ifdef HAS_GL
///////////////////////////////
// CGUIWindowTestBadPixelsGL //
///////////////////////////////

CGUIWindowTestBadPixelsGL::CGUIWindowTestBadPixelsGL() : CGUIWindowTestBadPixels()
{

}

CGUIWindowTestBadPixelsGL::~CGUIWindowTestBadPixelsGL()
{

}

void CGUIWindowTestBadPixelsGL::DrawBadPixelsTest()
{
  glDisable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CRect rect(m_left, m_top, m_right, m_bottom);
  CGUITexture::DrawQuad(rect, m_color);

  glEnable(GL_TEXTURE_2D);
}

#endif

#ifdef HAS_GLES
/////////////////////////////////
// CGUIWindowTestBadPixelsGLES //
/////////////////////////////////

CGUIWindowTestBadPixelsGLES::CGUIWindowTestBadPixelsGLES() : CGUIWindowTestBadPixels()
{

}

CGUIWindowTestBadPixelsGLES::~CGUIWindowTestBadPixelsGLES()
{

}

void CGUIWindowTestBadPixelsGLES::DrawBadPixelsTest()
{
  glClearColor(0, 0, 0, 255);
  glClear(GL_COLOR_BUFFER_BIT);

  g_graphicsContext.PushTransform(TransformMatrix(), true);

  CRect rect(m_left, m_top, m_right, m_bottom);
  CGUITextureGLES::DrawQuad(rect, m_color);

  glEnable(GL_TEXTURE_2D);
}
#endif

#ifdef HAS_DX
///////////////////////////////
// CGUIWindowTestBadPixelsDX //
///////////////////////////////

CGUIWindowTestBadPixelsDX::CGUIWindowTestBadPixelsDX() : CGUIWindowTestBadPixels()
{

}

CGUIWindowTestBadPixelsDX::~CGUIWindowTestBadPixelsDX()
{

}

void CGUIWindowTestBadPixelsDX::DrawBadPixelsTest()
{
  // TODO: write dx code
}
#endif
