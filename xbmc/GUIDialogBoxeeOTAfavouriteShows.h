#pragma once

#include "GUIDialogBoxeeWizardBase.h"
#include "FileItem.h"

class CGUIDialogBoxeeOTAfavouriteShows : public CGUIDialogBoxeeWizardBase
{
public:

  CGUIDialogBoxeeOTAfavouriteShows();
  virtual ~CGUIDialogBoxeeOTAfavouriteShows();
  void OnInitWindow();
  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage& message);

private:
  bool m_bIsDirty;
  bool GetFavoriteShowsFromServer();
  void SetFavoriteShowsToServer();
  bool HandleSelectShow();
  CFileItemList m_favoriteShowsSuggestions;
};
