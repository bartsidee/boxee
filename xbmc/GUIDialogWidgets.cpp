#include "stdafx.h"
#include "GUIDialogWidgets.h"
#include "guiImage.h"
#include "GUILabelControl.h"
#include "GUIFontManager.h"
#include "Util.h"

// WidgetEngine includes
#include "widgets/WidgetEngine.h"

CGUIDialogWidgets::CGUIDialogWidgets()
: CGUIDialog(WINDOW_DIALOG_WIDGETS_TEST, "Widgets.xml")
{
}

CGUIDialogWidgets::~CGUIDialogWidgets()
{
}

void CGUIDialogWidgets::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CLog::Log(LOGDEBUG, "-----------------  CGUIDialogWidgets::OnInitWindow");
  
  WidgetEngine * pWidgetEngine = new WidgetEngine();
    
  pWidgetEngine->LoadWidget(_P("special://xbmc/UserData/Dilbert Daily.kon"));
   
  pWidgetEngine->Init();
    
  pWidgetEngine->Run();

//  CLabelInfo info;
//  //info.align = XBFONT_CENTER_X | XBFONT_CENTER_Y;
//  info.font = g_fontManager.GetFont("font13");
//  info.textColor = 0xffffffff;
//
//  CGUILabelControl *pLabel = new CGUILabelControl(19000,100,48,48,
//      200,48,"Hello",info,false);
//  // pLabel->SetLabel("SDFSDFSDAFD");
//  pLabel->SetVisible(true);
//  //pLabel->SetVisibleCondition(1,true);
//  pLabel->AllocResources();
//  Add(pLabel);
  
  //  // Add static image
  CImage* pImage = new CImage();
  CGUIImage* pGUIImage = new CGUIImage(19000, 19001, 20, 20, 600, 208, *pImage);
  pGUIImage->SetFileName(_P("special://xbmc/UserData/dilbert1.gif"));
  pGUIImage->AllocResources();
  Add(pGUIImage);
  pGUIImage->SetVisible(true);


}

bool CGUIDialogWidgets::OnAction(const CAction &action)
{
  CLog::Log(LOGDEBUG, "CGUIDialogWidgets::OnAction");

  if ((action.wID == ACTION_CONTEXT_MENU || action.wID == ACTION_MOUSE_RIGHT_CLICK))
  {



    return false;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogWidgets::Render()
{
  CGUIDialog::Render();
}

bool CGUIDialogWidgets::OnMessage(CGUIMessage& message)
{
  CLog::Log(LOGDEBUG, "CGUIDialogWidgets::OnMessage");
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
    CGUIDialog::OnMessage(message);
    return true;
  }
  break;

  case GUI_MSG_WINDOW_INIT:
  {
    CLog::Log(LOGDEBUG, "CGUIDialogWidgets::OnMessage - GUI_MSG_WINDOW_INIT");
    return CGUIDialog::OnMessage(message);
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogWidgets::OnWindowLoaded()
{
  CLog::Log(LOGINFO, "CGUIDialogWidgets::OnWindowLoaded");
  CGUIDialog::OnWindowLoaded();

}

void CGUIDialogWidgets::OnWindowUnload()
{
  CLog::Log(LOGINFO, "CGUIDialogWidgets::OnWindowUnloaded");
  CGUIDialog::OnWindowUnload();
}
