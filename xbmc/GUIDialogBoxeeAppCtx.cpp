
#include "GUIDialogBoxeeAppCtx.h"
#include "GUIWindowSettingsCategory.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "utils/GUIInfoManager.h"
#include "bxobject.h"
#include "PluginSettings.h"
#include "GUIDialogPluginSettings.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "GUIWindowManager.h"

#define CONTROL_SETTINGS 9011

CGUIDialogBoxeeAppCtx::CGUIDialogBoxeeAppCtx(void) : 
  CGUIDialogBoxeeCtx(WINDOW_DIALOG_BOXEE_APP_CTX, "boxee_application_context.xml")
{
}

CGUIDialogBoxeeAppCtx::~CGUIDialogBoxeeAppCtx(void)
{
}

void CGUIDialogBoxeeAppCtx::Update()
{
  CGUIDialogBoxeeCtx::Update();
}

bool CGUIDialogBoxeeAppCtx::OnMessage(CGUIMessage &message)
{
   switch (message.GetMessage())
   {
      case GUI_MSG_CLICKED:
      {
         int iControl = message.GetSenderId();
         
         if (iControl == CONTROL_SETTINGS && m_item.IsPlugin() && CPluginSettings::SettingsExist(m_item.m_strPath))
         {
            CURI url(m_item.m_strPath);
            CGUIDialogPluginSettings::ShowAndGetInput(url);
            return true;
         }
         else if (iControl == CONTROL_SETTINGS && m_item.IsLastFM())
         {
            ShowLastFMSettings();
         }
      }
   }
  
   return CGUIDialogBoxeeCtx::OnMessage(message);
}

void CGUIDialogBoxeeAppCtx::OnInitWindow()
{
  CGUIDialogBoxeeCtx::OnInitWindow();
}

void CGUIDialogBoxeeAppCtx::OnMoreInfo() 
{
}

void CGUIDialogBoxeeAppCtx::ShowLastFMSettings(void)
{
  CPluginSettings settings;
  if (!settings.Load(CStdString("<settings>"
                          "<setting id=\"username\" type=\"text\" label=\"15202\"/>" 
                          "<setting id=\"password\" type=\"text\" label=\"15203\"/>"
                          "<setting id=\"enable\" type=\"bool\" label=\"15201\" default=\"false\"/>"
                          "<setting id=\"recordtoprofile\" type=\"bool\" label=\"15250\" />"
                     "</settings>")))
     return;
  
  settings.Set("username", g_guiSettings.GetString("scrobbler.lastfmusername"));
  settings.Set("password", g_guiSettings.GetString("scrobbler.lastfmpassword"));
  settings.Set("enable", g_guiSettings.GetBool("lastfm.enable") ? "true" : "false");
  settings.Set("recordtoprofile", g_guiSettings.GetBool("lastfm.recordtoprofile") ? "true" : "false");
  
  // Create the dialog
  CGUIDialogPluginSettings* pDialog = (CGUIDialogPluginSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
  pDialog->SetSettings(settings);
  pDialog->DoModal();
  if (pDialog->IsConfirmed())
  {
     CLastfmScrobbler::GetInstance()->Term();
     g_guiSettings.SetString("scrobbler.lastfmusername", pDialog->GetValue("username"));
     g_guiSettings.SetString("scrobbler.lastfmpassword", pDialog->GetValue("password"));
     g_guiSettings.SetBool("lastfm.enable", pDialog->GetValue("enable") == "true");
     g_guiSettings.SetBool("lastfm.recordtoprofile", pDialog->GetValue("recordtoprofile") == "true");
     g_settings.Save();
     CLastfmScrobbler::GetInstance()->Init();
  }
}

