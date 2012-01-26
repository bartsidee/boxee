#include "GUIDialogFirstTimeUseWelcome.h"
#include "BoxeeUtils.h"

#ifdef HAS_EMBEDDED

CGUIDialogFirstTimeUseWelcome::CGUIDialogFirstTimeUseWelcome() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_WELCOME,"ftu_welcome.xml","CGUIDialogFirstTimeUseWelcome")
{

}

CGUIDialogFirstTimeUseWelcome::~CGUIDialogFirstTimeUseWelcome()
{

}

void CGUIDialogFirstTimeUseWelcome::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  g_settings.SetSkinString(g_settings.TranslateSkinString("IsDlink"),strcmpi(BoxeeUtils::GetPlatformStr(),"dlink.dsm380") == 0 ? "1" : "0");

  return;
}

bool CGUIDialogFirstTimeUseWelcome::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseWelcome::HandleClickBack()
{
  // nothing to do

  return true;
}

#endif

