#pragma once

#include "GUIWindow.h"
#include "FileItem.h"

class CGUIWindowTestBadPixelsManager : public CGUIWindow
{
public:
  
  CGUIWindowTestBadPixelsManager();
  virtual ~CGUIWindowTestBadPixelsManager();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

  static color_t GetColor(int index);
  static int GetNumOfColors();

protected:

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:

  bool FillColorsList();

  CFileItemList m_colorsListItems;
  int m_selectedColorIndex;

  static std::vector<color_t> m_testBadPixelsColorsVec;
};

