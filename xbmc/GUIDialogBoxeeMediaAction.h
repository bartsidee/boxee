/*
 * CGUIDialogBoxeeMediaAction.h
 *
 *  The purpose of this dialog is to present a choice of actions available for a media item
 */

#ifndef CGUIDIALOGBOXEEMEDIAACTION_H_
#define CGUIDIALOGBOXEEMEDIAACTION_H_

#pragma once
#include "GUIDialog.h"
#include "FileItem.h"
#include "BoxeeShortcut.h"
#include <list>

class CPlaylistSourceType
{
public:
  enum PlaylistSourceTypeEnums
  {
    NONE=0,
    DVD=1,
    STACK=2
  };
};

class CFileMarkOptions
{
public:
  enum FileMarkOptionsEnums
  {
    CANNOT_BE_MARKED=0,
    MARKED_AS_SEEN=1,
    MARKED_AS_UNSEEN=2,
    NUM_OF_MARK_OPTIONS=3
  };
};

class CDialogButtons
{
public:
  enum DialogButtonsEnums
  {
    BTN_READ_MORE=0,
    BTN_ADD_TO_QUEUE=1,
    BTN_REMOVE_FROM_QUEUE=2,
    BTN_MARK_AS_SEEN=3,
    BTN_MARK_AS_UNSEEN=4,
    BTN_ADD_SHORTCUT=5,
    BTN_REMOVE_FROM_SHORTCUT=6,
    BTN_GOTO_SHOW=7,
    BTN_TRAILER=8,
    BTN_SHARE=9,
    BTN_REMOVE_FROM_HISTORY=10,
    BTN_EJECT=11,
    BTN_BROWSE=12,
    BTN_RESOLVE=13,
    BTN_ADD_TO_FAVORITE=14,
    BTN_REMOVE_FROM_FAVORITE=15,
    NUM_OF_BUTTONS=16,
    BTN_UNKNOWN=17
  };
};

class CGUIDialogBoxeeMediaAction :
      public CGUIDialog
{
public:
	CGUIDialogBoxeeMediaAction(void);
  virtual ~CGUIDialogBoxeeMediaAction(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  CStdString GetLabel(const CFileItem& item);
  
  static CFileMarkOptions::FileMarkOptionsEnums GetFileMarkStatus(const CFileItem& item);

  static bool CanAddAsShortcut(const CFileItem& item);
  static bool CanGotoShowScreen(const CFileItem& item);

  static bool OnPlay(const CFileItem& pItem);
  static bool OnResume(const CFileItem& pItem);
  static bool OnBrowse(const CFileItem& pItem);
  static bool OnPlayMedia(const CFileItem& pItem);
  static bool OnPlayAudioFromFolder(const CFileItem& item);
  static bool IsDVDPlaying();
  static bool ChooseVideoQuality(const CFileItem& item, int  &chosenItem);
  static bool GetPreferredQuality(const CFileItem& item, int &chosenItem);

  static bool CreatePlaylistAndPlay(const CFileItem& dvdFolderItem,CPlaylistSourceType::PlaylistSourceTypeEnums PlaylistSourceType);

  static void PlayTrailer(const CFileItem& item);
  static void RunSlideshow(const CStdString& folderPath, const CStdString& itemPath);

  static bool ShowAndGetInput(const CFileItem* pItem);

  static void OpenDvdDialog();

protected:

  void InitAppItem();
  bool BuildAppFileItemLinksList();

  bool HandleClickOnLuanchApp();
  bool HandleClickOnAddApp();
  bool HandleClickOnRemoveApp();

  void InitNotAppItem();
  bool AddFileItemToList(std::map<CStdString, CFileItemPtr>& addedLinksMap, std::list<CFileItemPtr>& pathsList);
  bool BuildLinksFileItemList();

  static void AddItemToHistory(const CFileItem& item);
  static const char* GetPlaylistSourceTypeEnumAsString(CPlaylistSourceType::PlaylistSourceTypeEnums PlaylistSourceTypeEnum);

  CStdString GetLinkPathToShowInDialog(const CFileItem& linkItem);
  void SetLinkPathAsWindowProperty(const CFileItem* linkPath, bool shortenPath = true);
  void SetLinkPathAsWindowProperty(const CStdString& _path, bool shortenPath = true);

  bool OnClick(CGUIMessage& message);

  bool HandleClickOnItemList();
  bool HandleClickOnButtonList(bool& needToCloseDialog);
  bool HandleClickOnMovieAdditinalButtonList();
  bool HandleClickOnReadMore();
  bool HandleClickOnTrailer();
  bool HandleClickOnShare();
  bool HandleClickOnAddToQueue(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnRemoveFromQueue(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnMarkAsSeen(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnMarkAsUnseen(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnAddShortcut(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnRemoveFromShortcut(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnGotoShow();
  bool HandleClickOnRemoveFromHistory();
  bool HandleClickOnEject();
  bool HandleClickOnBrowse();
  bool HandleClickOnResolve();
  bool HandleClickOnAddToFavorite(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);
  bool HandleClickOnRemoveFromFavorite(CStdString& newLabelToUpdate, CStdString& newThumbToUpdate);

  bool AddShortcut(const CBoxeeShortcut& shortcut);
  bool RemoveShortcut(const CBoxeeShortcut& shortcut);

  void AddDequeueButton(CFileItemList& buttonsList, const CStdString& referral);

  int FillDialogButtons(bool itemHasLinks);
  int FillMovieAdditionalDialogButtons();

  bool NeedToSubscribe();
  bool OnSubscription();

  void UpdateItemWithLinkData(const CFileItemPtr linkItem);
  static void CopyItemContnetProperties(CFileItem &dstItem, const CFileItemPtr linkItem);

  static CDialogButtons::DialogButtonsEnums GetButtonAsEnum(const CStdString& buttonLabel);
  static CDialogButtons::DialogButtonsEnums GetButtonActionPropertyAsEnum(const CStdString& buttonActionProperty);

  bool SetupTrailer(int& linksAddedCounter);

  void InitCastPanel();

  int GetVisibleButtonListControlId();

  bool m_bConfirmed;
  CFileItem m_item;
  CFileItemPtr m_trailerLinkItem;

  std::vector<CFileItemPtr> m_linksFileItemList;
  bool m_listContainServerLinks;

  bool m_tarilerWasAddedToLinkList;

  int m_numOfButtons;

  int m_visibleButtonsListId;

  bool m_refreshActiveWindow;
};

#endif /* CGUIDIALOGBOXEEMEDIAACTION_H_ */
