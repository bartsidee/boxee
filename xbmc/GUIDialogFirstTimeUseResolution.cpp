#include "GUIDialogFirstTimeUseResolution.h"

#ifdef HAS_EMBEDDED

#include "GUIWindowManager.h"
#include "GUIWindowSettingsCategory.h"
#include "Settings.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogOK2.h"
#include "log.h"

CGUIDialogFirstTimeUseResolution::CGUIDialogFirstTimeUseResolution() : CGUIDialogFirstTimeUseWithList(WINDOW_DIALOG_FTU_RES,"ftu_res.xml","CGUIDialogFirstTimeUseResolution")
{

}

CGUIDialogFirstTimeUseResolution::~CGUIDialogFirstTimeUseResolution()
{

}

void CGUIDialogFirstTimeUseResolution::OnInitWindow()
{
  CGUIDialogFirstTimeUseWithList::OnInitWindow();
}

/*
bool CGUIDialogFirstTimeUseResolution::IsFinishSetRes()
{
  return m_finishSetRes;
}
*/

/*
CStdString CGUIDialogFirstTimeUseResolution::GetSelectedResStr()
{
  return m_selectedResStr;
}
*/

bool CGUIDialogFirstTimeUseResolution::HandleClickNext()
{
  if (!m_selectedItem.get())
  {
    // no res was chosen

    CGUIDialogOK2::ShowAndGetInput(54628,54623);

    SET_CONTROL_FOCUS(LIST_CTRL, 0);

    return false;
  }

  bool okSelectedRes = TestNewRes();

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::HandleClickNext - Trying to set new res [label=%s][enum=%d] return [%d] (initbox)",m_selectedItem->GetLabel().c_str(),GetSelectedResolutionEnum());

  return okSelectedRes;
}

bool CGUIDialogFirstTimeUseResolution::HandleClickBack()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseResolution::HandleListChoice()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::HandleListChoice - Selected resolution is [%s] (initbox)",m_selectedItem->GetLabel().c_str());

  return true;
}

bool CGUIDialogFirstTimeUseResolution::FillListOnInit()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::FillListOnInit - Enter function. [ResInfoSize=%d] (initbox)",(int)g_settings.m_ResInfo.size());

  int counter = 0;
  int NumOfRes = (int)g_settings.m_ResInfo.size() - RES_DESKTOP;
  std::vector<RESOLUTION_INFO>::iterator it = g_settings.m_ResInfo.begin()+RES_DESKTOP;

  while(it != g_settings.m_ResInfo.end())
  {
    counter++;
    RESOLUTION_INFO resInfo = *it;

    CFileItemPtr resItem(new CFileItem(resInfo.strMode));

    m_listItems.Add(resItem);

    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::FillListOnInit - [%d/%d] - After adding item [label=%s] for resInfo [id=%s][mode=%s][output=%s] (initbox)",counter,NumOfRes,resItem->GetLabel().c_str(),resInfo.strId.c_str(),resInfo.strMode.c_str(),resInfo.strOutput.c_str());

    it++;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::FillListOnInit - Exit function. [ResListSize=%d] (initbox)",m_listItems.Size());

  return true;
}

bool CGUIDialogFirstTimeUseResolution::TestNewRes()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseResolution::TestNewRes - Handling selectedRes [label=%s][chooseIndex=%d] (initbox)",m_selectedItem->GetLabel().c_str(),m_selectedIndex);

  int NumOfRes = (int)g_settings.m_ResInfo.size() - RES_DESKTOP;
  if (m_selectedIndex < 0 || m_selectedIndex > (int)g_settings.m_ResInfo.size()-RES_DESKTOP)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseResolution::TestNewRes - FAILED to handle [SelectedIndex=%d] because it is out of bounds. [NumOfRes=%d] (initbox)",m_selectedIndex,NumOfRes);
    return false;
  }

  RESOLUTION selectedRes = GetSelectedResolutionEnum();

  CGUIWindowSettingsCategory* pWindow = (CGUIWindowSettingsCategory*)g_windowManager.GetWindow(WINDOW_SETTINGS_MYPICTURES);
  if (!pWindow)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseResolution::TestNewRes - FAILED to get GUIWindowSettingsCategory (initbox)");
    return false;
  }

  return (pWindow->TestAndSetNewResolution(selectedRes));
}

RESOLUTION CGUIDialogFirstTimeUseResolution::GetSelectedResolutionEnum()
{
  return (RESOLUTION)(RES_DESKTOP + m_selectedIndex);
}

#endif

