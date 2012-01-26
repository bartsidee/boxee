#include "GUIWindowTestBadPixelsManager.h"
#include "Key.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIWindowTestBadPixels.h"
#include "log.h"

#define LIST_CTRL 240

//#define YELLOW_COLOR_VALUE 0xFFFFFF00
#define RED_COLOR_VALUE 0xFFFF0000
#define GREEN_COLOR_VALUE 0xFF00FF00
#define BLUE_COLOR_VALUE 0xFF0000FF
#define WHITE_COLOR_VALUE 0xFFFFFFFF
#define BLACK_COLOR_VALUE 0xFF000000

#define LOOP_ALL_COLOR_INDEX -1
#define RED_COLOR_INDEX 0
#define GREEN_COLOR_INDEX 1
#define BLUE_COLOR_INDEX 2
#define WHITE_COLOR_INDEX 3
#define BLACK_COLOR_INDEX 4

std::vector<color_t> CGUIWindowTestBadPixelsManager::m_testBadPixelsColorsVec;

CGUIWindowTestBadPixelsManager::CGUIWindowTestBadPixelsManager() : CGUIWindow(WINDOW_TEST_BAD_PIXELS_MANAGER,"test_bad_pixels_manager.xml")
{
  m_colorsListItems.Clear();
  m_selectedColorIndex = 0;

  m_testBadPixelsColorsVec.clear();
  m_testBadPixelsColorsVec.push_back(RED_COLOR_VALUE);
  m_testBadPixelsColorsVec.push_back(GREEN_COLOR_VALUE);
  m_testBadPixelsColorsVec.push_back(BLUE_COLOR_VALUE);
  m_testBadPixelsColorsVec.push_back(WHITE_COLOR_VALUE);
  m_testBadPixelsColorsVec.push_back(BLACK_COLOR_VALUE);
}

CGUIWindowTestBadPixelsManager::~CGUIWindowTestBadPixelsManager()
{

}

void CGUIWindowTestBadPixelsManager::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  if (FillColorsList())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowTestBadPixelsManager::OnInitWindow - After adding [NumOfColors=%d] to list (tbp)",m_colorsListItems.Size());
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowTestBadPixelsManager::OnInitWindow - FAILED to add colors to the list. [NumOfColors=%d] (tbp)",m_colorsListItems.Size());
    Close();
  }

  if (m_selectedColorIndex >= 0 && m_selectedColorIndex < m_colorsListItems.Size())
  {
    CONTROL_SELECT_ITEM(LIST_CTRL,m_selectedColorIndex);
  }
}

void CGUIWindowTestBadPixelsManager::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);

  if (nextWindowID != WINDOW_TEST_BAD_PIXELS)
  {
    m_selectedColorIndex = 0;
  }
}

bool CGUIWindowTestBadPixelsManager::OnAction(const CAction &action)
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

bool CGUIWindowTestBadPixelsManager::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowTestBadPixelsManager::OnMessage - GUI_MSG_CLICKED - [iControl=%d] (tbp)",iControl);

    switch (iControl)
    {
    case LIST_CTRL:
    {
      // Get selected index from the list
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), LIST_CTRL);
      OnMessage(msg);

      m_selectedColorIndex = msg.GetParam1();

      if (m_selectedColorIndex < 0 || m_selectedColorIndex > m_colorsListItems.Size() - 1)
      {
        CLog::Log(LOGERROR,"CGUIWindowTestBadPixelsManager::OnMessage - GUI_MSG_CLICKED - FAILED to get [ChoiceIndex=%d]. [colorsListItems=%d] (tbp)",m_selectedColorIndex,m_colorsListItems.Size());
        return true;
      }

      CFileItemPtr selectedColorItem = m_colorsListItems.Get(m_selectedColorIndex);

      if (!selectedColorItem.get())
      {
        CLog::Log(LOGERROR,"CGUIWindowTestBadPixelsManager::OnMessage - LIST_CTRL - FAILED to get selected list item (tbp)");
        return true;
      }

      // show window with the selected color

      CGUIWindowTestBadPixels* tbpWin = (CGUIWindowTestBadPixels*)g_windowManager.GetWindow(WINDOW_TEST_BAD_PIXELS);
      if (!tbpWin)
      {
        CLog::Log(LOGERROR,"CGUIWindowTestBadPixelsManager::OnMessage - LIST_CTRL - FAILED to get CGUIWindowTestBadPixels (tbp)");
        return true;
      }

      tbpWin->SetColorItem(selectedColorItem);
      g_windowManager.ActivateWindow(WINDOW_TEST_BAD_PIXELS);
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowTestBadPixelsManager::FillColorsList()
{
  CGUIMessage messageReset(GUI_MSG_LABEL_RESET, GetID(), LIST_CTRL);
  OnMessage(messageReset);

  m_colorsListItems.Clear();

  // add RED
  CFileItemPtr colorItemRed(new CFileItem(g_localizeStrings.Get(54855)));
  colorItemRed->SetProperty("color-index",RED_COLOR_INDEX);
  m_colorsListItems.Add(colorItemRed);

  // add Green
  CFileItemPtr colorItemGreen(new CFileItem(g_localizeStrings.Get(54856)));
  colorItemGreen->SetProperty("color-index",GREEN_COLOR_INDEX);
  m_colorsListItems.Add(colorItemGreen);

  // add Blue
  CFileItemPtr colorItemBlue(new CFileItem(g_localizeStrings.Get(54857)));
  colorItemBlue->SetProperty("color-index",BLUE_COLOR_INDEX);
  m_colorsListItems.Add(colorItemBlue);

  // add White
  CFileItemPtr colorItemWhite(new CFileItem(g_localizeStrings.Get(54858)));
  colorItemWhite->SetProperty("color-index",WHITE_COLOR_INDEX);
  m_colorsListItems.Add(colorItemWhite);

  // add Black
  CFileItemPtr colorItemBlack(new CFileItem(g_localizeStrings.Get(54859)));
  colorItemBlack->SetProperty("color-index",BLACK_COLOR_INDEX);
  m_colorsListItems.Add(colorItemBlack);

  // add loop all
  CFileItemPtr colorItemLoopAll(new CFileItem(g_localizeStrings.Get(54860)));
  m_colorsListItems.Add(colorItemLoopAll);

  CGUIMessage messageBind(GUI_MSG_LABEL_BIND, GetID(), LIST_CTRL, 0, 0, &m_colorsListItems);
  OnMessage(messageBind);

  return true;
}

color_t CGUIWindowTestBadPixelsManager::GetColor(int index)
{
  if (index < 0 || index > (int)m_testBadPixelsColorsVec.size() - 1)
  {
    CLog::Log(LOGERROR,"CGUIWindowTestBadPixelsManager::GetColor - Enter function with a bad [index=%d]. return color WHITE. [SizeOfColorsVec=%d] (tbp)",index,(int)m_testBadPixelsColorsVec.size());
    return WHITE_COLOR_VALUE;
  }

  return m_testBadPixelsColorsVec[index];
}

int CGUIWindowTestBadPixelsManager::GetNumOfColors()
{
  return (int)m_testBadPixelsColorsVec.size();
}

