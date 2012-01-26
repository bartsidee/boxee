#include "GUIDialogFirstTimeUseLang.h"

#ifdef HAS_EMBEDDED

#include "GUIWindowManager.h"
#include "GUISettings.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "FileItem.h"
#include "BoxeeUtils.h"
#include "log.h"
#include "SpecialProtocol.h"
#include "tinyXML/tinyxml.h"

#define AVAILABLE_LANG_FILE_PATH "special://xbmc/language/availablelangs.xml"

CGUIDialogFirstTimeUseLang::CGUIDialogFirstTimeUseLang() : CGUIDialogFirstTimeUseWithList(WINDOW_DIALOG_FTU_LANG,"ftu_lang.xml","CGUIDialogFirstTimeUseLang")
{

}

CGUIDialogFirstTimeUseLang::~CGUIDialogFirstTimeUseLang()
{

}

bool CGUIDialogFirstTimeUseLang::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"%s::OnMessage - GUI_MSG_CLICKED - [iControl=%d] (initbox)",m_name.c_str(),iControl);

    switch (iControl)
    {
    case LIST_CTRL:
    {
      // set the selected lang
      if (!CGUIDialogFirstTimeUseWithList::OnMessage(message))
      {
        CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseLang::OnMessage - LIST_CTRL - Call to CGUIDialogFirstTimeUseWithList::OnMessage returned FALSE (initbox)");
        return true;
      }

      if (UpdateLanguage())
      {
        m_actionChoseEnum = CActionChose::NEXT;
        Close();
      }
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseWithList::OnMessage(message);
}

bool CGUIDialogFirstTimeUseLang::HandleClickNext()
{
  return UpdateLanguage();
}

bool CGUIDialogFirstTimeUseLang::HandleClickBack()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseLang::HandleListChoice()
{
  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseLang::HandleListChoice - SelectedLang is [id=%s][displayName=%s][dirName=%s] (initbox)",m_selectedItem->GetProperty("lang_id").c_str(),m_selectedItem->GetProperty("lang_displayName").c_str(),m_selectedItem->GetProperty("lang_dirName").c_str());
  return true;
}

bool CGUIDialogFirstTimeUseLang::FillListOnInit()
{
  m_listItems.Clear();

  if (!BoxeeUtils::GetAvailableLanguages(m_listItems))
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseLang::FillList - FAILED to get available languages (initbox)");
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseLang::FillList - Get available Languages. [NumOfLanguages=%d] (initbox)",m_listItems.Size());

  CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), LIST_CTRL, 0, 0, &m_listItems);
  OnMessage(message);

  return true;
}

bool CGUIDialogFirstTimeUseLang::UpdateLanguage()
{
  ///////////////////////////////
  // update to chosen language //
  ///////////////////////////////

  bool succeeded = true;

  if (!m_selectedItem.get())
  {
    // no language was chosen
    CGUIDialogOK2::ShowAndGetInput(54615,54616);
    SET_CONTROL_FOCUS(LIST_CTRL, 0);
    succeeded = false;
  }
  else
  {
    CStdString currentLang = g_guiSettings.GetString("locale.language");
    CStdString choosenLang = m_selectedItem->GetProperty("lang_dirName");

    if (stricmp(currentLang.c_str(),choosenLang.c_str()) != 0)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseLang::UpdateLanguage - [currentLang=%s != %s=choosenLang] -> going to reload skin (initbox)",currentLang.c_str(),choosenLang.c_str());

      CGUIWindowSettingsCategory* pWindow = (CGUIWindowSettingsCategory*)g_windowManager.GetWindow(WINDOW_SETTINGS_MYPICTURES);
      if (pWindow)
      {
        pWindow->SetNewLanguage(choosenLang);
        if (!pWindow->LoadNewLanguage())
        {
          CGUIDialogOK2::ShowAndGetInput(54615,54617);
          succeeded = false;
      }
      }
      else
      {
        CGUIDialogOK2::ShowAndGetInput(54615,54617);
        succeeded = false;
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseLang::UpdateLanguage - [choosenLang=%s] is the [currentLang=%s] -> no need to reload (initbox)",choosenLang.c_str(),currentLang.c_str());
    }
  }

  return succeeded;
}

#endif

