/*
 * GUIDialogBoxeeUpdateMessage.cpp
 *
 */


#include "GUIDialogBoxeeUpdateMessage.h"
#include "GUIDialogBoxeeUpdateProgress.h"
#include "GUIWindowManager.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogOK2.h"
#include "SystemInfo.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogButtonMenu.h"

#ifdef _LINUX
#include <unistd.h>
#endif

#define CONTROL_MORE_INFO_BACKGROUND        90
#define CONTROL_MORE_INFO_TEXTBOX           100
#define CONTROL_MORE_INFO_SCROLLBAR         110
#define CONTROL_UPDATE_LABEL                120
#define CONTROL_UPDATE_FORCE_NOTIFICATION   130
#define CONTROL_CANCEL                      210
#define CONTROL_MORE_INFO                   220
#define CONTROL_INSTALL_OR_OK               230
#define CONTROL_UPDATE_MESSAGE_LABEL        330

using namespace std;

CGUIDialogBoxeeUpdateMessage::CGUIDialogBoxeeUpdateMessage(void) : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_UPDATE_MESSAGE, "boxee_update_message.xml")
{
  m_inLoginScreen = true;  
  m_bConfirmed = false;
}

CGUIDialogBoxeeUpdateMessage::~CGUIDialogBoxeeUpdateMessage(void)
{
  
}

bool CGUIDialogBoxeeUpdateMessage::OnAction(const CAction &action)
{
  if ((action.id == ACTION_PREVIOUS_MENU) || (action.id == ACTION_PARENT_DIR))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnAction - Enter function with [wID=%d]. [ACTION_PREVIOUS_MENU=%d || ACTION_PARENT_DIR=%d] (update)",action.id,ACTION_PREVIOUS_MENU,ACTION_PARENT_DIR);

    const CGUIControl* pInfoCtrl = GetControl(CONTROL_MORE_INFO_BACKGROUND);
    const CGUIControl* pCancelButton = GetControl(CONTROL_CANCEL);

    if (pInfoCtrl && pInfoCtrl->IsVisible())
    {
      SET_CONTROL_HIDDEN(CONTROL_MORE_INFO_BACKGROUND);
      SET_CONTROL_FOCUS(CONTROL_MORE_INFO, 0);
    }
    else if (pCancelButton && pCancelButton->IsVisible())
    {
      Close();
    }

    return true;
  }

  return CGUIDialogBoxBase::OnAction(action);
}

bool CGUIDialogBoxeeUpdateMessage::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_INSTALL_OR_OK)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - CONTROL_INSTALL_OR_OK was clicked [id=%d]. (update)",iControl);

#if defined(_LINUX) && defined(__APPLE__)

      ///////////////////////////////////////////////////////////////////////////////////////
      // Under OSX we need to get the user password and check it before closing the dialog //
      ///////////////////////////////////////////////////////////////////////////////////////

      if(CSysInfo::IsAppleTV())
      {
        /////////////////////////////////////////////////////////
        // Under AppleTV -> NO Need to check the user password //
        /////////////////////////////////////////////////////////
        
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - Under AppleTV -> Going to close the dialog (update)");
        
        m_bConfirmed = true;
        Close();
        return true;
      }
      else
      {
        ////////////////////////////////////////
        // Under OSX -> Check the OSX version //
        ////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////
        // Under OSX -> Need to check the root password (only if there is one) //
        /////////////////////////////////////////////////////////////////////////

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - Under OSX -> Going to open CGUIDialogKeyboard to get UserPassword (update)");
      
        CStdString userPassword;

        if (CGUIDialogKeyboard::ShowAndGetInput(userPassword, g_localizeStrings.Get(53200), true, true))
        {
          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - The user entered [UserPassword=%s] (update)",userPassword.c_str());
          
          /////////////////////////////////////
          // Check if root password is valid //
          /////////////////////////////////////

          CStdString checkUserPasswordCommand = "/usr/bin/dscl . -authonly ";
          checkUserPasswordCommand += getenv("USER");
          checkUserPasswordCommand += " '";
          checkUserPasswordCommand += userPassword;
          checkUserPasswordCommand += "'";
          
          char readBuf[256] = "";
          FILE* readPipe = (FILE*)popen(checkUserPasswordCommand.c_str(),"r");
                
          if (!readPipe)
          { 
            CLog::Log(LOGERROR,"CGUIDialogBoxeeUpdateMessage::OnMessage - FAILED to create pipe for execute [%s]. [errno=%d] (update)",checkUserPasswordCommand.c_str(),errno);
            return true;
          }

          fgets(readBuf, sizeof readBuf, readPipe);

          pclose(readPipe);

          bool validPass = false;

          if (strlen(readBuf) == 0)
          {
            validPass = true;
            CLog::Log(LOGERROR,"CGUIDialogBoxeeUpdateMessage::OnMessage - Execute [%s] returned EmptyString [%s] -> [validPass=%d] (update)",checkUserPasswordCommand.c_str(),readBuf,validPass);
          }

          if(!validPass)
          { 
            ////////////////////////////////////////////////////
            // UserPassword is invalid. Show an error message //
            ////////////////////////////////////////////////////

            CLog::Log(LOGERROR,"CGUIDialogBoxeeUpdateMessage::OnMessage - Entered password is INVALID. Execute [%s] FAILED. [validPass=%d][errno=%d] (update)",checkUserPasswordCommand.c_str(),validPass,errno);
            
            CStdString heading = g_localizeStrings.Get(53240);
            CStdString line = g_localizeStrings.Get(53212);

            CGUIDialogOK2::ShowAndGetInput(heading,line);
          }
          else
          {
            ///////////////////////////
            // UserPassword is valid //
            ///////////////////////////

            CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - Entered password is VALID. Execute [%s] succeeded. [validPass=%d] (update)",checkUserPasswordCommand.c_str(),validPass);
            
            g_boxeeVersionUpdateManager.SetUserPassword(userPassword);
            
            CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - CBoxeeVersionUpdateManager was set with it [%s]. Going to close the dialog (update)",(g_boxeeVersionUpdateManager.GetUserPassword()).c_str());
            
            m_bConfirmed = true;
            Close();
          }
        }
              
        return true;
      }

#else

      ////////////////////////////////////////
      // Under No Apple we close the dialog //
      ////////////////////////////////////////

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - Not under OSX ->Going to close the dialog (update)");
      
      m_bConfirmed = true;
      Close();
      return true;
      
#endif

    }
    else if (iControl == CONTROL_MORE_INFO) 
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - CONTROL_MORE_INFO was clicked [id=%d] (update)",iControl);
    }
    else if (iControl == CONTROL_CANCEL) 
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnMessage - CONTROL_CANCEL was clicked [id=%d]. Going to close the dialog (update)",iControl);

      m_bConfirmed = false;
      Close();
      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIDialogBoxeeUpdateMessage::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnInitWindow - Enter function (update)");
  
  CGUIDialogBoxBase::OnInitWindow();

  m_bConfirmed = false;
  
  CVersionUpdateInfo VersionUpdateInfo = ((g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob())).GetVersionUpdateInfo();
  
  //////////////////////////////////////
  // Set the buttons and update label //
  //////////////////////////////////////

  SetDialogUpdateLabelAndButtons(m_inLoginScreen,VersionUpdateInfo.GetVersionUpdateForce());
  
  //////////////////////////
  // Set the textbox text //
  //////////////////////////
  
  CStdString textboxStr = VersionUpdateInfo.m_UpdateNotesFileText;
  if(textboxStr.IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnInitWindow - The releaseNotesText parameter is empty (update)");
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeUpdateMessage::OnInitWindow - The releaseNotesText parameter is NOT empty. Going to set it (update)");

    SET_CONTROL_LABEL(CONTROL_MORE_INFO_TEXTBOX,textboxStr);   
  }
}

void CGUIDialogBoxeeUpdateMessage::InLoginScreen(bool inLoginScreen)
{
  m_inLoginScreen = inLoginScreen;
}

void CGUIDialogBoxeeUpdateMessage::SetDialogUpdateLabelAndButtons(bool inLoginScreen,VERSION_UPDATE_FORCE isVersionUpdateForce)
{
  CStdString updateButtonLabel = "";

  if(m_inLoginScreen)
  {
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Case we are in login screen (if we got here in login, we assume that we are on OSX or WINDOWS) //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    SET_CONTROL_HIDDEN(CONTROL_CANCEL);
    SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53247));

    if(isVersionUpdateForce == VUF_YES)
    {
      SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,g_localizeStrings.Get(53221));
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,g_localizeStrings.Get(53219));
    }

    SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,53260);
    updateButtonLabel = g_localizeStrings.Get(53260);
  }
  else
  {
    //////////////////////////////////////////////////////
    // Case we are NOT in login screen (in home screen) //
    //////////////////////////////////////////////////////

#ifdef _LINUX
          
    /////////////////////////////
    // APPLE and LINUX section //
    /////////////////////////////

#if defined(_LINUX) && !defined(__APPLE__)

    ///////////////////
    // LINUX section //
    ///////////////////

#ifdef HAS_EMBEDDED
    SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,53260);
    updateButtonLabel = g_localizeStrings.Get(53260);
    SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53248));

    if(isVersionUpdateForce == VUF_YES)
    {
      CStdString forceNotificationText = g_localizeStrings.Get(53220);
      if (g_windowManager.IsWindowActive(WINDOW_DIALOG_BUTTON_MENU))
      {
        // was open from shutdown dialog
        forceNotificationText = g_localizeStrings.Get(53221);
      }

      SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,forceNotificationText);
      SET_CONTROL_VISIBLE(CONTROL_UPDATE_FORCE_NOTIFICATION);
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,g_localizeStrings.Get(53219));
      SET_CONTROL_VISIBLE(CONTROL_UPDATE_FORCE_NOTIFICATION);
    }
#else
    SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,186);
    updateButtonLabel = g_localizeStrings.Get(186);
    SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53245));
    SET_CONTROL_HIDDEN(CONTROL_UPDATE_FORCE_NOTIFICATION);
#endif

#elif defined(__APPLE__)
    
    ///////////////////
    // APPLE section //
    ///////////////////

    if(CSysInfo::IsAppleTV())
    {
      SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,186);
      updateButtonLabel = g_localizeStrings.Get(53260);
      SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53246));
      SET_CONTROL_HIDDEN(CONTROL_UPDATE_FORCE_NOTIFICATION);
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,53260);
      updateButtonLabel = g_localizeStrings.Get(53260);
      SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53243));    
      
      if(isVersionUpdateForce == VUF_YES)
      {
        SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,g_localizeStrings.Get(53244));
        SET_CONTROL_VISIBLE(CONTROL_UPDATE_FORCE_NOTIFICATION);
      }
      else
      {
        SET_CONTROL_HIDDEN(CONTROL_UPDATE_FORCE_NOTIFICATION);        
      }
    }

#endif

#else

    /////////////////////
    // WINDOWS section //
    /////////////////////

    SET_CONTROL_LABEL(CONTROL_INSTALL_OR_OK,53260);
    updateButtonLabel = g_localizeStrings.Get(53260);
    SET_CONTROL_LABEL(CONTROL_UPDATE_LABEL,g_localizeStrings.Get(53243));        
    
    if(isVersionUpdateForce == VUF_YES)
    {
      SET_CONTROL_LABEL(CONTROL_UPDATE_FORCE_NOTIFICATION,g_localizeStrings.Get(53244));
      SET_CONTROL_VISIBLE(CONTROL_UPDATE_FORCE_NOTIFICATION);
    }
    else
    {
      SET_CONTROL_HIDDEN(CONTROL_UPDATE_FORCE_NOTIFICATION);        
    }

#endif

    SET_CONTROL_VISIBLE(CONTROL_CANCEL);
  }
  
  SET_CONTROL_VISIBLE(CONTROL_MORE_INFO);
  SET_CONTROL_VISIBLE(CONTROL_INSTALL_OR_OK);
  SET_CONTROL_FOCUS(CONTROL_INSTALL_OR_OK, 0);

  CStdString message = "";
  message.Format(g_localizeStrings.Get(53256).c_str(),updateButtonLabel);
  SET_CONTROL_LABEL(CONTROL_UPDATE_MESSAGE_LABEL,message);
}

