#pragma once

#include "GUIDialog.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxboxeefeed.h"
#include "lib/libBoxee/bxfriendslist.h"
#include "utils/CriticalSection.h"
#include "PictureThumbLoader.h"

#include <vector>

class CGUIDialogBoxeeUserInfo : public CGUIWindow, public IBackgroundLoaderObserver // , public BOXEE::BoxeeListener,
{
public:
  
  class Friend {
  public:
      CStdString m_strId;
      CStdString m_strName;
      CStdString m_strBDay;
      CStdString m_strGender;
      CStdString m_strLocation;
      CStdString m_strDesc;
      CStdString m_strThumb;
  };
  
  CGUIDialogBoxeeUserInfo(void);
  virtual ~CGUIDialogBoxeeUserInfo(void);
  virtual bool OnMessage(CGUIMessage &message);
  
  void SetUserId(const CStdString& strUserId);

//  static bool Show(const BOXEE::BXObject &userObj);
//  static bool Show(const CFileItem* pUserItem);

  //virtual void OnObjectLoaded(const BOXEE::BXObject &obj);
  //virtual void OnUserFriendsRetrieved(const std::string &strUserId, const BOXEE::BXFriendsList &newFeed);
  //virtual void OnUserFeedRetrieved(const std::string &strUserId, const BOXEE::BXBoxeeFeed &newFriends);
  virtual void OnItemLoaded(CFileItem* pItem);

  void SetupScreen();
  //void SetupFeed();
  //void SetupFriends();
  //void SetupFriendsLabel(int iFriendCount);
  //void SetupActionsLabel(int iActionsCount);

  void RefreshThumbs();

  void Clear();
  //void ClearFeed();
  //void ClearFriends();

  void LoadFromBoxeeObj(const BOXEE::BXObject &userObj);
  void LoadFromFileItem(const CFileItem* pItem);

  virtual bool OnAction(const CAction &action);

protected:

  virtual void OnInitWindow();

  void OnFriendsListClick(int nAction);
  void OnActionsClick(int nAction);

  void OnBack();
  
  void RegisterContainers();

  void UpdateWindow();

  CCriticalSection m_lock;

//  CStdString m_strId;
//  CStdString m_strName;
//  CStdString m_strBDay;
//  CStdString m_strGender;
//  CStdString m_strLocation;
//  CStdString m_strDesc;
//  CStdString m_strThumb;
  
  std::vector<Friend> m_history;
  Friend m_currentFriend;

  bool leavingForGood;

  bool m_bUserThumbLoaded;
  //CPictureThumbLoader *m_friendsLoader;
  CPictureThumbLoader *m_userThumbLoader;
};

