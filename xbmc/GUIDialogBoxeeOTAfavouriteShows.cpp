/* CGUIDialogBoxeeOTAFavoriteShows .cpp
 *
 */
#include "GUIDialogBoxeeOTAfavouriteShows.h"
#include "GUIWindowBoxeeBrowseTvEpisodes.h"
#include "GUIWindowManager.h"
#include "BoxeeOTAConfigurationManager.h"
#include "GUIDialogBoxeeOTALocationConfiguration.h"
#include "utils/log.h"
#include "bxconfiguration.h"
#include "bxutils.h"
#include "RssSourceManager.h"
#include "BoxeeUtils.h"


#define FAVOURITE_SUGGESTIONS_CONTROL_LIST 81
#define NEXT_CONTROL_BUTTON    1
#define CANCEL_CONTROL_BUTTON  2


CGUIDialogBoxeeOTAfavouriteShows::CGUIDialogBoxeeOTAfavouriteShows() :
CGUIDialogBoxeeWizardBase(WINDOW_OTA_FAVOURITE_SHOWES,"custom_boxee_livetv_setup_6a.xml","CGUIDialogBoxeeOTAfavouriteShows")
{
}

CGUIDialogBoxeeOTAfavouriteShows::~CGUIDialogBoxeeOTAfavouriteShows()
{

}

void CGUIDialogBoxeeOTAfavouriteShows::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if(m_favoriteShowsSuggestions.IsEmpty())
  {
    if(!GetFavoriteShowsFromServer())
    {
      return;
    }
  }

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), FAVOURITE_SUGGESTIONS_CONTROL_LIST);
  OnMessage(msgReset);

  CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), FAVOURITE_SUGGESTIONS_CONTROL_LIST, 0, 0, &m_favoriteShowsSuggestions);
  OnMessage(msgBind);
}

bool CGUIDialogBoxeeOTAfavouriteShows::OnAction(const CAction& action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    int iControl = GetFocusedControlID();

    if(iControl == FAVOURITE_SUGGESTIONS_CONTROL_LIST)
    {
      if(HandleSelectShow())
      {
        //mark the show as favourite in ui
      }
      else
      {
        //pop a msg or unmark the show
      }
      return true;
    }

    if(iControl == CANCEL_CONTROL_BUTTON)
    {
      m_bIsDirty = false;
      m_actionChoseEnum = CActionChoose::NEXT;
      Close();
      return true;
    }

    if(iControl == NEXT_CONTROL_BUTTON)
    {
      SetFavoriteShowsToServer();
      m_actionChoseEnum = CActionChoose::NEXT;
      Close();
      return true;
    }
  }

  return CGUIDialogBoxeeWizardBase::OnAction(action);
}

bool CGUIDialogBoxeeOTAfavouriteShows::OnMessage(CGUIMessage& message)
{
  return CGUIDialogBoxeeWizardBase::OnMessage(message);
}

//adds and removes from favourite
bool CGUIDialogBoxeeOTAfavouriteShows::HandleSelectShow()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), FAVOURITE_SUGGESTIONS_CONTROL_LIST);
  OnMessage(msg);

  int iShow = msg.GetParam1();

  if(iShow > 0  && iShow < m_favoriteShowsSuggestions.Size())
  {
    bool isSelected = m_favoriteShowsSuggestions[iShow]->GetPropertyBOOL("selectedAsFavourite");
    m_favoriteShowsSuggestions[iShow]->SetProperty("selectedAsFavourite",isSelected);

    bool changed = m_favoriteShowsSuggestions[iShow]->GetPropertyBOOL("selectAsFavouriteChanged");
    m_favoriteShowsSuggestions[iShow]->SetProperty("selectAsFavouriteChanged",!changed);
    m_bIsDirty = true;
    return true;
  }

  return false;
}

bool CGUIDialogBoxeeOTAfavouriteShows::GetFavoriteShowsFromServer()
{
  CStdString strUrl =  "http://app.boxee.tv/title/suggestions"; //"http://app.boxee.tv/title/suggestions";
  CRssFeed feed;
  feed.Init(strUrl,strUrl);
  feed.ReadFeed();
  feed.GetItemList(m_favoriteShowsSuggestions);

  if(m_favoriteShowsSuggestions.Size() == 0)
  {
    return false;
  }

  CStdString label;
  bool isSubscribed = false;

  for(int i = 0; i < m_favoriteShowsSuggestions.Size(); i++)
  {
    label = m_favoriteShowsSuggestions[i]->GetProperty("title");
    m_favoriteShowsSuggestions[i]->SetLabel(label);
    isSubscribed = BoxeeUtils::IsSubscribe(label);
    m_favoriteShowsSuggestions[i]->SetProperty("selectedAsFavourite",isSubscribed);
    m_favoriteShowsSuggestions[i]->SetProperty("selectAsFavouriteChanged",false);
  }

  return true;
}

void CGUIDialogBoxeeOTAfavouriteShows::SetFavoriteShowsToServer()
{
  if (!m_bIsDirty)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeOTAfavouriteShows::OnDeinitWindow - [dirty=%d] -> no need to update (cf)",m_bIsDirty);
    return;
  }

  CStdString strShowId;
  CStdString strShowTitle;
  bool subscribe;
  bool changed;
  for(int i = 0; i < m_favoriteShowsSuggestions.Size(); i++)
  {
    subscribe = m_favoriteShowsSuggestions[i]->GetPropertyBOOL("selectedAsFavourite");
    changed = m_favoriteShowsSuggestions[i]->GetPropertyBOOL("selectAsFavouriteChanged");

    if(changed)
    {
      strShowId = m_favoriteShowsSuggestions[i]->GetProperty("boxeeid");
      strShowTitle = m_favoriteShowsSuggestions[i]->GetLabel();

      SubscribeJob* job = new SubscribeJob(BOXEE::CSubscriptionType::TVSHOW_SUBSCRIPTION, strShowId, strShowTitle, subscribe);

      if (CUtil::RunInBG(job) != JOB_SUCCEEDED)
      {
        m_favoriteShowsSuggestions[i]->SetProperty("selectedAsFavourite",!subscribe);
        continue;
      }
    }
  }
}
