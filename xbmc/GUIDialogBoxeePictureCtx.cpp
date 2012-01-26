
#include "GUIDialogBoxeePictureCtx.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "GUIWindowSlideShow.h"
#include "PictureInfoLoader.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUIToggleButtonControl.h"

using namespace BOXEE;

#define MAX_ZOOM_FACTOR 10

#define BTN_MORE_INFO 9003

#define BTN_RATE      9006
#define BTN_RECOMMEND 9007


#define BTN_COMMENT   9008
#define BTN_LANG      9009
#define BTN_OPTIONS   9010
#define BTN_PRESET    9020

#define BTN_PREV      9101
#define BTN_PAUSE     9111
#define BTN_PLAY      9112

#define BTN_NEXT      9104
#define BTN_ZOOM      9105
#define BTN_ROTATE    9106
#define BTN_INFO      9107

#define BTN_ZOOM_OUT  9211
#define BTN_ZOOM_IN   9212
#define BTN_PAN       9213

#define BTN_PAN_UP    9201
#define BTN_PAN_DOWN  9202
#define BTN_PAN_LEFT  9203
#define BTN_PAN_RIGHT 9204

#define CONTROL_THUMB 19

#define INFO_HIDDEN_LIST 5000

CGUIDialogBoxeePictureCtx::CGUIDialogBoxeePictureCtx(void) : CGUIDialogBoxeeCtx(WINDOW_DIALOG_BOXEE_PICTURE_CTX, "boxee_picture_context.xml")
{
  m_bIsZooming = false;
  m_bWasPlaying = false;
  m_zoomInFactor = 0;
  
  m_numOfItemsInSlideshow = 0;
}

CGUIDialogBoxeePictureCtx::~CGUIDialogBoxeePictureCtx(void)
{

}

void CGUIDialogBoxeePictureCtx::Update()
{
//  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), INFO_HIDDEN_LIST, 0 );
//  OnMessage(msg);
//
//  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), INFO_HIDDEN_LIST, 0, 0, m_movieItem);
//  OnMessage(winmsg);
    
  CGUIDialogBoxeeCtx::Update();
}

bool CGUIDialogBoxeePictureCtx::OnAction(const CAction &action)
{
  CGUIWindowSlideShow* pWindow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pWindow)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePictureCtx::OnMessage, NEWUI, PICTURE, could not retreive slideshow window");
    return CGUIDialogBoxeeCtx::OnAction(action);
  }
  
  if (action.id == ACTION_PREVIOUS_MENU) 
  {
    if (m_bIsZooming) 
    {
      m_bIsZooming = false;
      // Reset the zoom to normal
      CAction newAction;
      newAction.id = ACTION_ZOOM_LEVEL_NORMAL;
      pWindow->OnAction(newAction);
      
      // Make the menu visible
      SET_CONTROL_VISIBLE(8000);
      SET_CONTROL_HIDDEN(9200);
      SET_CONTROL_FOCUS(BTN_ZOOM,0);
      
      if (m_bWasPlaying) 
      {
        m_bWasPlaying = false;
        CAction newAction;
        newAction.id = ACTION_PAUSE;
        pWindow->OnAction(newAction);
      }

      CGUIToggleButtonControl* pPanButton = (CGUIToggleButtonControl*) GetControl(BTN_PAN);
      pPanButton->SetSelected(false);
      SetProperty("is-panning", false);
      return true;
    }
    return CGUIDialogBoxeeCtx::OnAction(action);
  }
  
  if ((action.id == ACTION_MOVE_RIGHT && GetPropertyBOOL("is-panning")) ||
      (action.id == ACTION_MOVE_UP    && GetPropertyBOOL("is-panning")) ||
      (action.id == ACTION_MOVE_LEFT  && GetPropertyBOOL("is-panning")) ||
      (action.id == ACTION_MOVE_DOWN  && GetPropertyBOOL("is-panning")))
  {
    pWindow->OnAction(action);
    return true;
  }

  return CGUIDialogBoxeeCtx::OnAction(action);
}

bool CGUIDialogBoxeePictureCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {      
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::OnMessage, NEWUI, PICTURE, control = %d, sender = %d", message.GetControlId(), message.GetSenderId());
      // Pass the appropriate action to the slideshow window
      CGUIWindowSlideShow* pWindow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (!pWindow)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeePictureCtx::OnMessage, NEWUI, PICTURE, could not retreive slideshow window");
        return true;
      }
      int iControl = message.GetSenderId();
      CAction action;

      if (iControl == BTN_NEXT)
      {
        action.id = ACTION_NEXT_PICTURE;
      }
      else if (iControl == BTN_INFO)
      {
        pWindow->ToggleShowSlideInfo();
        return true;
      }
      else if (iControl == BTN_ZOOM)
      {
        m_zoomInFactor = 0;
        CONTROL_ENABLE(BTN_ZOOM_IN);
        CONTROL_DISABLE(BTN_ZOOM_OUT);
        CONTROL_DISABLE(BTN_PAN_UP);
        CONTROL_DISABLE(BTN_PAN_DOWN);
        CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::OnMessage - BTN_ZOOM was hit. [m_zoomInFactor=%d][BTN_ZOOM_IN=ENABLE][BTN_ZOOM_OUT=DISABLE][BTN_PAN_UP=DISABLE][BTN_PAN_DOWN=DISABLE] (zoom)",m_zoomInFactor);

        CGUIToggleButtonControl* pPanButton = (CGUIToggleButtonControl*) GetControl(BTN_PAN);
        pPanButton->SetSelected(false);
 
        m_bIsZooming = true;
        if (!pWindow->IsPaused())
        {
          action.id = ACTION_PAUSE;
          m_bWasPlaying = true;
        }
        else 
          return true;
      }
      else if (iControl == BTN_PREV)
      {
        action.id = ACTION_PREV_PICTURE;
      }
      else if (iControl == BTN_PLAY)
      {
        // Swap state to playing
        if (!pWindow->IsPaused()) 
        {
          //SET_CONTROL_VISIBLE(BTN_DUMMY_PLAY);
          //SET_CONTROL_HIDDEN(BTN_DUMMY_PAUSE);
        }
        else
        {
          SET_CONTROL_HIDDEN(BTN_PLAY);
          SET_CONTROL_VISIBLE(BTN_PAUSE);
          SET_CONTROL_FOCUS(BTN_PAUSE,0);
        }
     
        action.id = ACTION_PAUSE;
      }
      else if (iControl == BTN_PAUSE)
      {
        // Swap state to playing
        if (pWindow->IsPaused())
        {

        }
        else
        {
          SET_CONTROL_HIDDEN(BTN_PAUSE);
          SET_CONTROL_VISIBLE(BTN_PLAY);
          SET_CONTROL_FOCUS(BTN_PLAY,0);
        }

        action.id = ACTION_PAUSE;
      }
      else if (iControl == BTN_ZOOM_IN)
      {
        action.id = ACTION_ZOOM_IN;
        ZoomInActionUpdate();
      }
      else if (iControl == BTN_ZOOM_OUT)
      {
        action.id = ACTION_ZOOM_OUT;
        ZoomOutActionUpdate();
      }
      else if (iControl == BTN_PAN)
      {
        SetProperty("is-panning", !GetPropertyBOOL("is-panning"));
      }
      else if (iControl == BTN_PAN_DOWN)
      {
        action.id = ACTION_MOVE_DOWN;
      }
      else if (iControl == BTN_PAN_LEFT)
      {
        action.id = ACTION_MOVE_LEFT;
      }
      else if (iControl == BTN_PAN_RIGHT)
      {
        action.id = ACTION_MOVE_RIGHT;
      }
      else if (iControl == BTN_PAN_UP)
      {
        action.id = ACTION_MOVE_UP;
      }
      else if (iControl == BTN_ROTATE)
      {
        action.id = ACTION_ROTATE_PICTURE;
      }
      else
      {
        return CGUIDialogBoxeeCtx::OnMessage(message);
      }
      
      pWindow->OnAction(action);
    }
    break;
    case GUI_MSG_WINDOW_DEINIT:
    case GUI_MSG_VISUALISATION_UNLOADING:
    {

    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
    }
  }
  return CGUIDialogBoxeeCtx::OnMessage(message);
}

void CGUIDialogBoxeePictureCtx::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  SET_CONTROL_VISIBLE(BTN_MORE_INFO);

  CGUIWindowSlideShow* pWindow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  
  m_numOfItemsInSlideshow = pWindow->NumSlides();
  
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::OnInitWindow - Enter function. m_numOfItemsInSlideshow was set to [%d] (nslide)",m_numOfItemsInSlideshow);

  if(m_numOfItemsInSlideshow > 1)
  {
    if (pWindow && pWindow->IsPaused())
    {
      SET_CONTROL_VISIBLE(BTN_PLAY);
      SET_CONTROL_HIDDEN(BTN_PAUSE);
      SET_CONTROL_FOCUS(BTN_PLAY,0);
    }
    else 
    {
      SET_CONTROL_VISIBLE(BTN_PAUSE);
      SET_CONTROL_HIDDEN(BTN_PLAY);
      SET_CONTROL_FOCUS(BTN_PAUSE,0);
    }
  }
  else
  {
    CONTROL_DISABLE(BTN_PLAY);
    CONTROL_DISABLE(BTN_NEXT);
    CONTROL_DISABLE(BTN_PREV);
    SET_CONTROL_FOCUS(BTN_ZOOM,0);
  }

  if (m_item.IsInternetStream()) 
  {
    SET_CONTROL_VISIBLE(BTN_RATE);
    SET_CONTROL_VISIBLE(BTN_RECOMMEND);

  }
  else 
  {
    SET_CONTROL_HIDDEN(BTN_RATE);
    SET_CONTROL_HIDDEN(BTN_RECOMMEND);
  }

  if (m_item.IsPicture()) 
  {
    m_item.SetProperty("ispicture", 1);
  }

  SetProperty("is-panning", false);

  CGUIToggleButtonControl* pPanButton = (CGUIToggleButtonControl*) GetControl(BTN_PAN);
  pPanButton->SetSelected(false);
}

void CGUIDialogBoxeePictureCtx::OnMoreInfo()
{
  // This functionality is disabled, there should be no
  // more info in current implementation
  return;

  CPictureInfoLoader loader;
  loader.LoadItem(&m_item, true);
  
  m_item.Dump();
  
  g_windowManager.CloseDialogs(true);
  CGUIWindowBoxeeMediaInfo::Show(&m_item);
}

void CGUIDialogBoxeePictureCtx::ZoomInActionUpdate()
{
  m_zoomInFactor++;
  
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomInWasMade - [m_zoomInFactor=%d] (zoom)",m_zoomInFactor);

  if(m_zoomInFactor > 0)
  {
    CONTROL_ENABLE(BTN_PAN_UP);
    CONTROL_ENABLE(BTN_PAN_DOWN);    
    CONTROL_ENABLE(BTN_ZOOM_OUT);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomInWasMade - [m_zoomInFactor=%d] -> BTN_ZOOM_OUT was enabled (zoom)",m_zoomInFactor);
  }
  
  if(m_zoomInFactor == (MAX_ZOOM_FACTOR-1))
  {
    CONTROL_DISABLE(BTN_ZOOM_IN);
    SET_CONTROL_FOCUS(BTN_ZOOM_OUT,0);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomInWasMade - [m_zoomInFactor=%d=(MAX_ZOOM_FACTOR-1)=%d] -> BTN_ZOOM_IN was disabled and select BTN_ZOOM_OUT (zoom)",m_zoomInFactor,(MAX_ZOOM_FACTOR-1));
  }
}

void CGUIDialogBoxeePictureCtx::ZoomOutActionUpdate()
{
  m_zoomInFactor--;
  
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomOutWasMade - [m_zoomInFactor=%d] (zoom)",m_zoomInFactor);

  if(m_zoomInFactor == 0)
  {
    CONTROL_DISABLE(BTN_PAN_UP);
    CONTROL_DISABLE(BTN_PAN_DOWN);
    CONTROL_DISABLE(BTN_ZOOM_OUT);
    SET_CONTROL_FOCUS(BTN_ZOOM_IN,0);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomOutWasMade - [m_zoomInFactor=%d] -> BTN_ZOOM_OUT was disabled and select BTN_ZOOM_IN (zoom)",m_zoomInFactor);
  }
  
  if(m_zoomInFactor < MAX_ZOOM_FACTOR)
  {
    CONTROL_ENABLE(BTN_ZOOM_IN);
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeePictureCtx::ZoomOutWasMade - [m_zoomInFactor=%d<MAX_ZOOM_FACTOR=%d] -> BTN_ZOOM_IN was enabled (zoom)",m_zoomInFactor,MAX_ZOOM_FACTOR);
  }
}

