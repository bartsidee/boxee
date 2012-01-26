// Copyright © 2008 BOXEE. All rights reserved.
/*
* libBoxee
*
*
*
* --- property of boxee.tv
*
*/
#ifndef BOXEEBOXEE_H
#define BOXEEBOXEE_H

#define BOXEE_LIB_VER "1.0"
#define BOXEE_USER_AGENT ("Boxeelib Ver." BOXEE_LIB_VER)

#include <SDL/SDL.h>
#include <string>
#include <vector>
#include "bxcredentials.h"
#include "bxmessages.h"
#include "bxconstants.h"
#include "bxfriendslist.h"
#include "bxboxeefeed.h"
#include "bxbgprocess.h"
#include "bxmediadatabase.h"
#include "bxmetadataengine.h"
#include "bxcscmanager.h"
#include "bxscheduletaskmanager.h"

#include "HttpCacheManager.h"

namespace BOXEE {

#define DISCOVERY_SERVER_PORT 2562

#define DELAY_FOR_CHECK_FEED_LOADED 1000

//
// interface for callbacks
//

typedef enum { LOGIN_SUCCESS = 0, LOGIN_ERROR_ON_NETWORK, LOGIN_ERROR_ON_CREDENTIALS, LOGIN_GENERAL_ERROR } BXLoginStatus;

class BoxeeListener {
public:
	virtual ~BoxeeListener() {}
	virtual void OnLoginEnded(const BXCredentials &creds, BXLoginStatus eResult, const BXObject &userObj) {}
	virtual void OnBoxeeFeedChange(const BXBoxeeFeed &newFeed) {}
	virtual void OnUserFeedRetrieved(const std::string &strUserId, const BXBoxeeFeed &newFeed) {}
	virtual void OnUserFriendsRetrieved(const std::string &strUserId, const BXFriendsList &newFriends) {}
	virtual void OnObjectLoaded(const BXObject &obj) {}
	virtual void OnObjectLoadFailed(const std::string &url) {}
	virtual void OnFriendsListChange(const BXFriendsList &friends) {}
	virtual void OnActionsListChange(const BXBoxeeFeed &actionsList) {}

	virtual void OnNotification(const BXGeneralMessage &msg) {}
	virtual void OnConnectionDown() {}
	virtual void OnConnectionRestored() {}
	virtual void OnFileDownloaded(const std::string &strLocalFile,
								const std::string &strUrl,
								const std::string &strTransactionId,
								void *pArg) {}
	virtual void OnFileDownloadFailed(const std::string &strLocalFile,
                                const std::string &strUrl,
                                const std::string &strTransactionId,
                                void *pArg) {}
	virtual void OnBoxeeResponse(const BXGeneralMessage &msg,
							     const std::string &strUrl,
								 const std::string &strTransactionId,
								 void *pArg) {}
	virtual void OnBoxeeResponse(const char *pData,
								unsigned int nDataSize,
								const std::string &strUrl,
								const std::string &strTransactionId,
								void *pArg) {}
	virtual void OnForcedStop() {}
};

// interface for a callback trigger.
// a callback trigger is a class which wants to be able to trigger an action in all listeners.
// it will be called back with each listener.
class CallbackTrigger
{
public:
	virtual ~CallbackTrigger(){}
	virtual void HandleListener(BoxeeListener *pListener) = 0;
};

// currently - vector of message objects (copy the full message in and out).
// in case of performance penalty this should be switched to <BXGeneralMessage*> and
// free method should be added.
typedef std::vector<BOXEE::BXGeneralMessage> VecMessages;

/**
class for encapsulating various operations with boxee.tv servers
*/
class Boxee{
public:
    virtual ~Boxee();

	static Boxee &GetInstance();
	// login to boxee server. sync (blocking) transaction.
	// creds will be updated according to login (auth key).
	// userObj will return the details of the user logged in.
	BXLoginStatus Login(BXCredentials &creds, BXObject &userObj);

	// non-blocking login, result will call listener with OnLoginEnded
	bool Login_bg(const BXCredentials &creds);

	bool IsLoggedIn();

	bool IsDuringLogin();

	void GetCurrentUser(BXObject &userObj);
	std::string GetCurrentUserID();

	// logout the user - delete the cookies.
	void Logout();

	// remove all cookies - delete cookie jar
	void ResetCookies();

	void SetCredentials(const BXCredentials &creds);
	BXCredentials GetCredentials();

	// SetVerbose will toggle excessive logging (on/off)
	void SetVerbose(bool bOn);
	bool IsVerbose();

	bool Start();
	bool Stop();
	bool SignalStop();
	bool IsRunning();

	void RunTest();

	// actions
	bool GetBoxeeFeed(BXBoxeeFeed &messages);
	bool GetUserBoxeeFeed(BXBoxeeFeed &messages, const std::string strUrl);
	bool GetUserBoxeeFeed_bg(const std::string &strUserId, const std::string &strUrl);

	// retrieve the friends list.
	bool RetrieveFriendsList(BXFriendsList &list, const std::string &strUserId = "", time_t listTimeStamp=0);
	//bool RetrieveUserFriendsList(BXFriendsList &list, const std::string strUrl);
	bool RetrieveUserFriendsList_bg(const std::string &strUserId, const std::string &strUrl);

	bool RetrieveUserActions(BXBoxeeFeed &list, const std::string &user);

	bool GetUserRecommendations(BXBoxeeFeed &recommendations);
	
	// add a friend.
	// returns the friend's details as output parameter
	bool AddFriend(const std::string &strFriendId, BXFriend &aFriend);

	// Set user action - this will apply the action to the sender
	bool SetUserAction(const BXGeneralMessage &action);
	bool SetUserAction_bg(const BXGeneralMessage &action); // to run in background

	BXObject* LoadObject(const std::string &strUrl, const BXObject* pObject);

	// async requests.
	bool AsyncLoadObject(const std::string &strUrl);
	bool AsyncLoadUrl(const std::string &strUrl, const std::string &strLocalFile,
	const std::string &strTransId="", void *pArg=0);

	// handle listeners
	void AddListener(BoxeeListener *pListener);
	void RemoveListener(BoxeeListener *pListener);
	void TriggerAllListeners(CallbackTrigger *pTrigger);

	// metadata engine
	BXMetadataEngine& GetMetadataEngine();

	unsigned int GetStartTime();
	void StartMetadataEngine();

	void setDatabaseFolderPath(const std::string& fDatabaseFolderPath);
	std::string getDatabaseFolderPath();
	
	bool IsInOfflineMode();
	void SetInOfflineMode(bool inOfflineMode);

	void SetTempFolderPath(const std::string& fTempFolderPath);
	std::string GetTempFolderPath();
	
	void SetWebServerPort(const std::string& port);
	void SetIsWebServerPasswordEmpty(bool webserverPasswordEmpty);
	
	void SetInScreenSaver(bool inScreenSaver);
  bool IsInScreenSaver();

  void SetHttpCacheManager(XBMC::CHttpCacheManager *cacheManager) { m_pHttpCache = cacheManager; }
  XBMC::CHttpCacheManager *GetHttpCacheManager() { return m_pHttpCache; }
  
  static const char* CBoxeeLoginStatusEnumAsString(BXLoginStatus boxeeLoginStatusEnum);

  CBoxeeClientServerComManager& GetBoxeeClientServerComManager();

  CBoxeeScheduleTaskManager& GetBoxeeScheduleTaskManager();

  void SetPlatform(const std::string& platform);
  std::string GetPlatform();

  bool ReportInstallRss(const std::string& strData);
  bool ReportInstallRss_bg(const std::string& strData);

  void PostLoginInitializations(const BXCredentials& creds, const BXObject& userObj);

protected:
  
  Boxee();
    
  bool ParseServerLoginResponse(BXXMLDocument& doc, BXObject& userObj);
  void ParseUserObjFromServerResponse(const TiXmlNode* pObjectTag, BXObject& userObj);

  std::string GenerateDiscoveryResponse(std::string request);

	// boxee thread - polling the server periodically getting updates
	static int BoxeeThread(void *pParam);
	static int DiscoveryThread(void *pParam);

	// server queries - run internaly by BoxeeThread
	bool InternalGetBoxeeFeed(BXBoxeeFeed &messages, time_t listTimeStamp, const MessageFilter &filter );
	bool InternalRetrieveFriendsList(BXFriendsList &list, time_t listTimeStamp=0);
	bool InternalRetrieveActionsList(BXBoxeeFeed &list, time_t listTimeStamp);
	
	bool InternalRetrieveRecommendationsList(BXBoxeeFeed &list, time_t listTimeStamp);

	// helper method to only set the field if the value is not null.
	// otherwise "HasValue" will return true (which is ok sometimes. depends on the use case).
	void SetFieldIfNotNull(BXGeneralMessage &msg, const std::string &field, const std::string &value);

	// helper for serializing "string-to-string" maps into http requests.
	std::string StringMapToHttpRequest(const std::map<std::string,std::string> &source);

	// helper for serializing "string-to-string" maps into xml.
	std::string StringMapToXML(const std::map<std::string,std::string> &source, const std::string &strParentTag);

	bool			m_bVerbose;
	bool  		m_bStarted;
	bool			m_bLoggedIn;
  bool      m_bDuringLogin;

	BXCredentials m_credentials;
	SDL_mutex*    m_credentialsGuard;

	BXBGProcess   m_backGroundProcessor;
	BXFriendsList	m_friendsList;
	BXBoxeeFeed   m_boxeeFeed;
	BXBoxeeFeed   m_actionsList;
	BXBoxeeFeed   m_recommendationsList;
	
	BXObject		m_currentUserObj;

	SDL_Thread						*m_thread;
	SDL_Thread						*m_discoveryThread;
	SDL_mutex						*m_boxeeGuard;
	SDL_mutex						*m_listenersGuard;
	std::vector<BoxeeListener *>	m_listeners;

	BXMetadataEngine m_MetadataEngine;

	time_t		 m_timeStart;

	std::string mDatabaseFolderPath;
	
	bool mInOfflineMode;
	std::string mTempFolderPath;
	
	static int discoverySocketDescriptor;
	std::string m_webServerPort;
	bool m_webserverPasswordEmpty;
	
	bool m_inScreenSaver;
  
  XBMC::CHttpCacheManager *m_pHttpCache;
  
  CBoxeeClientServerComManager m_boxeeCscManager;

  CBoxeeScheduleTaskManager m_boxeeScheduleTaskManager;

  std::string m_platform;
};

}

#endif
