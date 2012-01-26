
#include "GUIDialogBoxeeRssFeedInfo.h"
#include "GUIButtonControl.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "GUIDialogOK2.h"
#include "BoxeeMediaSourceList.h"
#include "GUIRadioButtonControl.h"
#include "Util.h"
#include "URL.h"
#include "Application.h"
#include "LocalizeStrings.h"

#define CONTROL_LOCATION_BUTTON       51
#define CONTROL_VIDEO_BUTTON          61
#define CONTROL_MUSIC_BUTTON          62
#define CONTROL_PICTURE_BUTTON        63
#define CONTROL_OK_BUTTON             72                
#define CONTROL_CANCEL_BUTTON         71

CGUIDialogBoxeeRssFeedInfo::CGUIDialogBoxeeRssFeedInfo() : 
  CGUIDialog(WINDOW_DIALOG_BOXEE_RSS_FEED_INFO, "boxee_rss_source_info.xml")
{
}

CGUIDialogBoxeeRssFeedInfo::~CGUIDialogBoxeeRssFeedInfo() 
{
}

bool CGUIDialogBoxeeRssFeedInfo::OnAction(const CAction &action)
{
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }
  
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
     Close();
     return true;
  }

  return CGUIDialog::OnAction(action);
}


bool CGUIDialogBoxeeRssFeedInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {
      DWORD senderId = message.GetSenderId();
      if (senderId == CONTROL_OK_BUTTON)
      {
        CBoxeeMediaSource source;
        source.path = ((CGUIButtonControl*) GetControl(CONTROL_LOCATION_BUTTON))->GetLabel2();
        source.isVideo = ((CGUIRadioButtonControl*) GetControl(CONTROL_VIDEO_BUTTON))->IsSelected();
        source.isMusic = ((CGUIRadioButtonControl*) GetControl(CONTROL_MUSIC_BUTTON))->IsSelected();
        source.isPicture = ((CGUIRadioButtonControl*) GetControl(CONTROL_PICTURE_BUTTON))->IsSelected();        

        if (source.path == "")
        {
          CGUIDialogOK2::ShowAndGetInput(257, 51041);
          return true;
        }  
        
        // Make sure at least one content has been selected
        if (!source.isVideo && !source.isMusic && !source.isPicture)
        {
          CGUIDialogOK2::ShowAndGetInput(257, 51040);
          return true;
        }
        
        if (source.path.Find("://") == -1)
        {
          CStdString s = "rss://";
          s += source.path;
          source.path = s;
        }
        
        CURI url(source.path);
        GetRSSInfoBG* job = new GetRSSInfoBG(source.path);
        if (CUtil::RunInBG(job,false) == JOB_SUCCEEDED)
        {
          source.name = job->m_title;
          source.thumbPath = job->m_thumbnail;
          source.path.Replace("http://", "rss://");
          
          CBoxeeMediaSourceList sourceList;
          sourceList.addSource(source);
          
          delete job;

          CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(52039), g_localizeStrings.Get(52140));
        }  
        else
        {
          CGUIDialogOK2::ShowAndGetInput(257, 52020);
          return true;          
        }
        
        Close();
        return true;
      }
      else if (senderId == CONTROL_CANCEL_BUTTON)
      {
        Close();
        return true;
      }
      
      break;
    }
  };
  
  return CGUIDialog::OnMessage(message);
}
