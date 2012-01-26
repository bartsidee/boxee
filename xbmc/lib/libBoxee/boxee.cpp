#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#else
#include "../../win32/c_defs.h"
#endif

//#include <curl/curl.h>
#include "tinyXML/tinyxml.h"
#include "logger.h"
#include "boxee.h"
#include "bxcurl.h"
#include "bxrssreader.h"
#include "bxconfiguration.h"
#include "bxoemconfiguration.h"
#include "bxboxeefeed.h"
#include "bxutils.h"
#include "bxversion.h"
#include "GUISettings.h"


#include "../../utils/GUIInfoManager.h"

#ifdef USE_MINI_DUMPS
#include "MiniDumps.h"
#endif

#if defined (_LINUX) || defined(__APPLE__)
#define bx_stricmp strcasecmp
#else
#define bx_stricmp _stricmp
#endif

#ifndef _WIN32
using namespace std;
#endif
 
#define IPHONE_REMOTE_KEY "b0xeeRem0tE!"
#define SHOW_MOVIE_LIBARRY "show_movie_library"

namespace BOXEE 
{  

int Boxee::discoverySocketDescriptor;

Boxee::Boxee() : m_bVerbose(false), m_bStarted(false), m_thread(NULL), m_boxeeScheduleTaskManager("BoxeeSTM",500)
{
  mInOfflineMode = true;
  m_inScreenSaver = false;
  mDatabaseFolderPath = "";
  m_listenersGuard = SDL_CreateMutex();
	m_boxeeGuard = SDL_CreateMutex();
	m_timeStart = time(NULL);
	m_webServerPort = "";
	m_webserverPasswordEmpty = true;
	discoverySocketDescriptor = (-1);
	srand((unsigned)time(0));
  BXConfiguration::GetInstance().Load();

  m_bDuringLogin = false;

  m_credentialsGuard = SDL_CreateMutex();

  m_platform = "";

  BXCurl::Initialize();
  BXGeneralMessage::Initialize();
  BXFriend::Initialize();  
}

Boxee::~Boxee()
{
  Stop();
  BXCurl::DeInitialize();
  SDL_DestroyMutex(m_listenersGuard);
  SDL_DestroyMutex(m_boxeeGuard);
  SDL_DestroyMutex(m_credentialsGuard);
}
  
Boxee &Boxee::GetInstance()
{
  static Boxee g_instance;
  return g_instance;
}

void Boxee::SetCredentials(const BXCredentials &creds)
{
  SDL_LockMutex(m_credentialsGuard);

  m_credentials = creds;

  // save username in lowercase for server request
  std::string usernameInLower = m_credentials.GetUserName();
  BXUtils::StringToLower(usernameInLower);
  m_credentials.SetUserName(usernameInLower);

  SDL_UnlockMutex(m_credentialsGuard);
}

BXCredentials Boxee::GetCredentials()
{
  BXCredentials creds;

  SDL_LockMutex(m_credentialsGuard);
  creds = m_credentials;
  SDL_UnlockMutex(m_credentialsGuard);

  return creds;
}

bool Boxee::Start()
{
  if (m_bStarted)
  {
	  return true;
  }

  m_bStarted = true;
  m_backGroundProcessor.SetName("Boxee Feed Processor");
  m_backGroundProcessor.Start(1);
  //m_thread = SDL_CreateThread(BoxeeThread, NULL);
  m_discoveryThread = SDL_CreateThread(DiscoveryThread, NULL);
	
	//m_MetadataEngine.Start();

  bool succeeded = m_boxeeScheduleTaskManager.Start(10);
  if(!succeeded)
  {
    LOG(LOG_LEVEL_ERROR,"Boxee::Start - FAILED to start CBoxeeScheduleTaskManager (bcscm)(login)");
    return false;
  }

	return true;
}

bool Boxee::SignalStop()
{
  m_backGroundProcessor.SignalStop();
	return true;
}
  
bool Boxee::Stop()
{
  LOG(LOG_LEVEL_INFO, "Boxee: asked to stop");
  if (!m_bStarted) 
	{
    LOG(LOG_LEVEL_INFO, "Boxee: already stopped");
		return true;
  }
	
	m_bStarted = false;
	
	LOG(LOG_LEVEL_INFO, "Stopping boxee background processor");
	m_backGroundProcessor.Stop();
	
	LOG(LOG_LEVEL_INFO, "Stopping metadata engine");
	m_MetadataEngine.Stop();
	
	if (m_thread) 
	{
    SDL_WaitThread(m_thread, NULL);
	}

	if (m_discoveryThread) 
	{
    SDL_WaitThread(m_discoveryThread, NULL);
	}
	
	if(discoverySocketDescriptor != (-1))
	{
#if defined (_LINUX) || defined (__APPLE__)
  close(discoverySocketDescriptor);
#else
  closesocket(discoverySocketDescriptor);
#endif
	}
		
	m_thread = NULL;

  // Deinitialize CBoxeeClientServerComManager
  bool succeeded = m_boxeeCscManager.Deinitialize();
  if(!succeeded)
  {
    LOG(LOG_LEVEL_WARNING,"Boxee::Stop - FAILED to deinitialize CBoxeeClientServerComManager (logout)");
  }

  succeeded = m_boxeeScheduleTaskManager.Stop();
  if(!succeeded)
  {
    LOG(LOG_LEVEL_WARNING,"Boxee::Stop - FAILED to stop CBoxeeScheduleTaskManager (bcscm)(logout)");
  }

  return true;
}

bool Boxee::IsRunning() 
{
  return m_bStarted;
}

void Boxee::AddListener(BoxeeListener *pListener)
{
  SDL_LockMutex(m_listenersGuard);
	bool bFound=false;
	
	for (unsigned int i=0; i<m_listeners.size(); i++) 
	{
	  if (m_listeners[i] == pListener) 
    {
		  bFound = true;
			break;
    }
  }
	
  if (!bFound)
	{
    m_listeners.push_back(pListener);
  }
    
  SDL_UnlockMutex(m_listenersGuard);
}

void Boxee::RemoveListener(BoxeeListener *pListener)
{
  SDL_LockMutex(m_listenersGuard);
  std::vector<BoxeeListener *>::iterator iter = m_listeners.begin();
  while (iter != m_listeners.end())
	{
    if (*iter == pListener)
		{
      iter = m_listeners.erase(iter);
    }
    else
		{
      iter++;
    }
	}
	SDL_UnlockMutex(m_listenersGuard);
}

void Boxee::TriggerAllListeners(CallbackTrigger *pTrigger)
{
  if (!pTrigger)
	{
    return;
	}
    
	SDL_LockMutex(m_listenersGuard);
	for (unsigned int i=0; i<m_listeners.size(); i++)
	{
	  pTrigger->HandleListener(m_listeners[i]);
  }
	SDL_UnlockMutex(m_listenersGuard);
}

void Boxee::SetVerbose(bool bOn)
{
  m_bVerbose = bOn;
}

bool Boxee::IsVerbose()
{
  return m_bVerbose;
}

void Boxee::SetFieldIfNotNull(BXGeneralMessage &msg, const std::string &field, const std::string &value)
{
  if (!value.empty())
	{
	  msg.SetValue(field,value);
	}
}

std::string Boxee::StringMapToHttpRequest(const std::map<std::string,std::string> &source) 
{
  std::map<std::string,std::string>::const_iterator iter = source.begin();
  std::string strResult;
	
	while (iter != source.end())
	{
	  strResult += iter->first + "=" + iter->second;
		iter++;
		
		if (iter != source.end())
		{
		  strResult += "&";
		}
  }
	
	return strResult;
}

std::string Boxee::StringMapToXML(const std::map<std::string,std::string> &source, const std::string &strParentTag)
{
  std::map<std::string,std::string>::const_iterator iter = source.begin();
  std::string strResult = "<" + strParentTag + ">";
	
	while (iter != source.end())
	{
	  strResult += "<" + iter->first + ">" + iter->second + "</" + iter->first + ">";
		iter++;
	}
	
	strResult += "</" + strParentTag + ">";
	
	return strResult;
}

bool Boxee::GetBoxeeFeed(BXBoxeeFeed &messages)
{
  LOG(LOG_LEVEL_DEBUG,"Enter to Boxee::GetBoxeeFeed. [mInOfflineMode=%d] (feed)",mInOfflineMode);
  
  if(mInOfflineMode)
  {
    return false;
  }

  LOG(LOG_LEVEL_DEBUG,"Going to handle GetBoxeeFeed (feed)");

	SDL_LockMutex(m_boxeeGuard);
	bool feedWasLoaded = m_boxeeFeed.IsLoaded();
	SDL_UnlockMutex(m_boxeeGuard);
	
	while (!feedWasLoaded)
	{
	  LOG(LOG_LEVEL_DEBUG,"BoxeeFeed is not loaded yet. Going to try again in [1000ms] (feed)");

		SDL_Delay(1000);
		
		SDL_LockMutex(m_boxeeGuard);
		feedWasLoaded = m_boxeeFeed.IsLoaded();
		SDL_UnlockMutex(m_boxeeGuard);
		
		if (!m_bStarted)
		{
		  return false;
		}
  }
	
	SDL_LockMutex(m_boxeeGuard);
	messages = m_boxeeFeed;
	SDL_UnlockMutex(m_boxeeGuard);
	
	LOG(LOG_LEVEL_DEBUG,"Going to return BoxeeFeed [BoxeeFeedListSize=%d] (feed)",messages.GetNumOfActions());

	return true;
}

void Boxee::GetCurrentUser(BXObject &userObj)
{
  userObj = m_currentUserObj;
}

std::string Boxee::GetCurrentUserID()
{
  return m_currentUserObj.GetID();
}

bool Boxee::RetrieveUserActions(BXBoxeeFeed &list, const std::string &user)
{  
  if(mInOfflineMode)
  {    
    return false;
  }

  LOG(LOG_LEVEL_DEBUG,"Enter to Boxee::RetrieveUserActions with [user=%s] (ac)",user.c_str());
	
	std::string currentUser = m_credentials.GetUserName();
	
	if((user == "") || (user == currentUser))
	{
	  LOG(LOG_LEVEL_DEBUG,"User received [%s]. Going to get ActionsList of current user [%s] (ac)",user.c_str(),currentUser.c_str());
		
		SDL_LockMutex(m_boxeeGuard);
		bool actionsListWasLoaded = m_actionsList.IsLoaded();
		SDL_UnlockMutex(m_boxeeGuard);
		
		while (!actionsListWasLoaded)
		{
		  LOG(LOG_LEVEL_DEBUG,"ActionsList of current user [%s] is not loaded yet. Going to try again in [1000ms] (ac)",currentUser.c_str());
			
			SDL_Delay(1000);
			
			SDL_LockMutex(m_boxeeGuard);
			actionsListWasLoaded = m_actionsList.IsLoaded();
			SDL_UnlockMutex(m_boxeeGuard);
		}
		
		SDL_LockMutex(m_boxeeGuard);
		list = m_actionsList;
		SDL_UnlockMutex(m_boxeeGuard);
		
		LOG(LOG_LEVEL_DEBUG,"Going to return ActionsList [ActionsListSize=%d] (ac)",list.GetNumOfActions());
		
		return true;
	}
	else
	{
	  // retreive the ActionsList of a specific user
		
		LOG(LOG_LEVEL_DEBUG,"User received [%s]. Going to retrieve ActionsList of this user [%s] (ac)",user.c_str(),user.c_str());
		
		std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ActionsListUrl","http://app.boxee.tv/user/");
		strUrl += user;
		strUrl += "/actions";
		
		BXCredentials credentials;
		credentials.SetUserName(m_credentials.GetUserName());
		credentials.SetPassword(m_credentials.GetPassword());
		
		list.SetCredentials(credentials);
		list.SetVerbose(m_bVerbose);
		
		bool retVal = list.LoadFromURL(strUrl);
		
		LOG(LOG_LEVEL_DEBUG,"Call LoadFromURL with [%s] returned [ActionsListSize=%d][retVal=%d] (ac)",strUrl.c_str(),list.GetNumOfActions(),retVal);
		
		return (retVal);
	}
}

bool Boxee::AddFriend(const std::string &strFriendId, BXFriend &aFriend) 
{
  BXXMLDocument doc;
	std::string strPostData = "friend_id=" + strFriendId;
	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AddFriendUrl","http://app.boxee.tv/add_friend");
	
	ListHttpHeaders headers;
	headers.push_back("Content-Type: application/x-www-form-urlencoded");
	
	doc.SetCredentials(m_credentials);
	doc.SetVerbose(m_bVerbose);
	
  if (doc.LoadFromURL(strUrl,headers,strPostData))
    return aFriend.ParseFromXml(doc.m_doc.FirstChild());

	return false;
}

bool Boxee::SetUserAction(const BXGeneralMessage &action)
{
  std::string strXML = action.ToXML();
	
	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.SetUserActionUrl","http://app.boxee.tv/action/add");
	
	ListHttpHeaders headers;
	headers.push_back("Content-Type: text/xml");
	
	BXXMLDocument doc;
	doc.SetCredentials(m_credentials);
	
  LOG(LOG_LEVEL_DEBUG,"Boxee::SetUserAction - Going to call LoadFromURL with [%s]. [ActionType=%s] (sua)",strUrl.c_str(),action.GetMessageType().c_str());

	bool retVal = doc.LoadFromURL(strUrl,headers,strXML);
  LOG(LOG_LEVEL_DEBUG,"Boxee::SetUserAction - Call to  return LoadFromURL with [%s] returned [retVal=%d]. [ActionType=%s] (sua)",strUrl.c_str(),retVal,action.GetMessageType().c_str());

	return retVal;
}

bool Boxee::GetUserBoxeeFeed(BXBoxeeFeed &messages, const std::string strUrl)
{
  messages.SetCredentials(m_credentials);
	messages.SetVerbose(m_bVerbose);

	bool bResult = messages.LoadFromURL(strUrl);	
	if (bResult) 
	{
	  LOG(LOG_LEVEL_ERROR,"Boxee::GetUserBoxeeFeed, FEED, loaded boxee feed");
	}
	else 
	{
	  LOG(LOG_LEVEL_ERROR,"Boxee::GetUserBoxeeFeed, FEED, failed to load boxee feed");
	}
	
	return bResult;
}

bool Boxee::GetUserBoxeeFeed_bg(const std::string &strUserId, const std::string &strUrl)
{
  class GetUserFeedJob: public BXBGJob 
	{
	public:
    GetUserFeedJob() : BXBGJob("GetUserFeedJob") {}
	  virtual void DoWork()
		{
		  BXBoxeeFeed feed;
			bool bOk = Boxee::GetInstance().GetUserBoxeeFeed(feed,m_strUrl);
			if (!bOk)
			{
			  LOG(LOG_LEVEL_ERROR,"FEED, failed to get user's feed (%s)", m_strUrl.c_str());
			}
			else
			{
			  Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnUserFeedRetrieved(m_strUserId, feed);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
			}
		}
		
		std::string m_strUrl;
		std::string m_strUserId;
	};
	
	GetUserFeedJob *aJob = new GetUserFeedJob;
	aJob->m_strUrl = strUrl;
	aJob->m_strUserId = strUserId;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
	  delete aJob;
	}
	
	return true;
}

/*
bool Boxee::RetrieveUserFriendsList_bg(const std::string &strUserId, const std::string &strUrl)
{
  class GetUserFriendsJob: public BXBGJob
	{
	public:
	  virtual void DoWork()
		{
		  BXFriendsList friends;
			bool bOk = Boxee::GetInstance().RetrieveUserFriendsList(friends,m_strUrl);
			if (!bOk)
			{
			  LOG(LOG_LEVEL_ERROR,"failed to get user friends (%s)", m_strUrl.c_str());
			}
			else
			{
			  friends.SetLoaded(true);
				Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnUserFriendsRetrieved(m_strUserId, friends);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
			}
		}
		
		std::string m_strUrl;
		std::string m_strUserId;
	};
	
	GetUserFriendsJob *aJob = new GetUserFriendsJob;
	aJob->m_strUrl = strUrl;
	aJob->m_strUserId = strUserId;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
	  delete aJob;
	}
	
	return true;
}
*/

bool Boxee::SetUserAction_bg(const BXGeneralMessage &action)
{
  class SetUserActionJob: public BXBGJob
	{
	public:
    SetUserActionJob() : BXBGJob("SetUserActionJob") {}
	  virtual void DoWork()
		{
		  bool bOk = Boxee::GetInstance().SetUserAction(action);
			if (!bOk)
			{
			  LOG(LOG_LEVEL_ERROR,"failed to run user action in BG");
			}
		}
		
		BXGeneralMessage action;
	};
	
	SetUserActionJob *aJob = new SetUserActionJob;
	aJob->action = action;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
	  delete aJob;
	}
	
	return true;
}

BXObject* Boxee::LoadObject(const std::string &strUrl, const BXObject* pObject)
{
  if (!pObject)
	{ 
	  return NULL;
	}
	
	BXObject* obj = new BXObject();
	obj->SetCredentials(Boxee::GetInstance().m_credentials);
	
  if (obj->LoadFromURL(strUrl, pObject->ToXML()))
  {
    return obj;
  }
	
	delete obj;
	return NULL;
}

bool Boxee::AsyncLoadObject(const std::string &strUrl)
{
  class LoadObjJob: public BXBGJob
	{
	public:
    LoadObjJob() : BXBGJob("LoadObjJob") {}
	  virtual void DoWork()
		{
		  if (m_url.empty())
			{
			  return;
			}
			
			BXObject obj;
			obj.SetCredentials(Boxee::GetInstance().m_credentials);
			
			bool bLoaded = obj.LoadFromURL(m_url);
			
			if (bLoaded)
			{
			  Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnObjectLoaded(obj);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
			}
      else
      {
			  Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnObjectLoadFailed(m_url);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
      }
		}
		
		std::string m_url;
	};
	
	LoadObjJob *aJob = new LoadObjJob;
	aJob->m_url = strUrl;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
    delete aJob;
	}
	
	return true;
}

bool Boxee::AsyncLoadUrl(const std::string &strUrl, const std::string &strLocalFile, const std::string &strTransId, void *pArg)
{
  class LoadUrlJob: public BXBGJob 
	{
	public:
    LoadUrlJob() : BXBGJob("LoadUrlJob") {}
	  virtual void DoWork()
		{
		  if (m_url.empty())
			{
			  return;
			}
			
			BXCurl request;
			request.SetCredentials(m_creds);
			std::string strPostData;
			
			if (request.HttpDownloadFile(m_url.c_str(), m_localFile.c_str(), strPostData))
			{
			  Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnFileDownloaded(m_localFile,m_url,m_transaction,m_pArg);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
			}
      else
			{
			  Boxee &boxee = Boxee::GetInstance();
				// alert on changes
				SDL_LockMutex(boxee.m_listenersGuard);
				for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
				{
				  boxee.m_listeners[i]->OnFileDownloadFailed(m_localFile,m_url,m_transaction,m_pArg);
				}
				SDL_UnlockMutex(boxee.m_listenersGuard);
			}
		}
		
		std::string m_url;
		std::string m_localFile;
		std::string m_transaction;
		BXCredentials m_creds;
		void *m_pArg;
	};
	
	LoadUrlJob *aJob = new LoadUrlJob;
	aJob->m_url = strUrl;
	aJob->m_localFile = strLocalFile;
	aJob->m_transaction = strTransId;
	aJob->m_creds = Boxee::GetInstance().m_credentials;
	aJob->m_pArg = pArg;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
	  delete aJob;
	}
	
	return true;
}

bool Boxee::InternalGetBoxeeFeed(BXBoxeeFeed &messages, time_t listTimeStamp, const MessageFilter &filter)
{
  if (!m_bLoggedIn)
	{
	  return false;
	}
	
	messages.SetCredentials(m_credentials);
	messages.SetVerbose(m_bVerbose);
	
	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ActivityListUrl","http://app.boxee.tv/api/get_activity");
	
  return (messages.LoadFromURL(strUrl));
}

bool Boxee::InternalRetrieveFriendsList(BXFriendsList &list, time_t listTimeStamp)
{
  LOG(LOG_LEVEL_DEBUG,"In Boxee::InternalRetrieveFriendsList - Enter function [m_bLoggedIn=%d] (fr)",m_bLoggedIn);
	
	if (!m_bLoggedIn)
	{
	  return false;
	}
	
	list.SetCredentials(m_credentials);
	list.SetVerbose(m_bVerbose);
	
	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.FriendsListUrl","http://app.boxee.tv/user/");
	strUrl += m_credentials.GetUserName();
	strUrl += "/friends";
	
	bool retVal = list.LoadFromURL(strUrl);
	
	LOG(LOG_LEVEL_DEBUG,"Call LoadFromURL with [%s] returned [FriendsListSize=%d][retVal=%d] (fr)",strUrl.c_str(),list.GetCount(),retVal);
	
	return (retVal);
}

bool Boxee::InternalRetrieveActionsList(BXBoxeeFeed &list, time_t listTimeStamp)
{
  LOG(LOG_LEVEL_DEBUG,"In Boxee::InternalRetrieveActionsList - Enter function [m_bLoggedIn=%d] (ac)",m_bLoggedIn);
	
	if (!m_bLoggedIn)
	{
	  return false;
	}
	
	list.SetCredentials(m_credentials);
	list.SetVerbose(m_bVerbose);

	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ActionsListUrl","http://app.boxee.tv/user/");
	strUrl += m_credentials.GetUserName();
	strUrl += "/actions";
	
	bool retVal = list.LoadFromURL(strUrl);
	
	LOG(LOG_LEVEL_DEBUG,"Call LoadFromURL with [%s] returned [ActionsListSize=%d][retVal=%d] (ac)",strUrl.c_str(),list.GetNumOfActions(),retVal);
	
	return (retVal);
}

bool Boxee::InternalRetrieveRecommendationsList(BXBoxeeFeed &list, time_t listTimeStamp)
{
  LOG(LOG_LEVEL_DEBUG,"In Boxee::InternalRetrieveRecommendationsList - Enter function [m_bLoggedIn=%d] (rec)",m_bLoggedIn);
	
	if (!m_bLoggedIn)
	{
	  return false;
	}
	
	list.SetCredentials(m_credentials);
	list.SetVerbose(m_bVerbose);

	std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.RecommendationsListUrl","http://app.boxee.tv/api/get_recommendations");
	
	bool retVal = list.LoadFromURL(strUrl);
	
	LOG(LOG_LEVEL_DEBUG,"Call LoadFromURL with [%s] returned [RecommendationsListSize=%d][retVal=%d][timestamp=%lu] (rec)(recwtime)",strUrl.c_str(),list.GetNumOfActions(),retVal,list.GetTimeStamp());
	
	return (retVal);
}

int Boxee::DiscoveryThread(void *pParam)
{
#ifdef USE_MINI_DUMPS
  g_MiniDumps.Install();
#endif

  LOG(LOG_LEVEL_DEBUG,"Boxee discovery server thread starting.... (dis)");
	Boxee& boxee = Boxee::GetInstance();

	discoverySocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
  if (discoverySocketDescriptor < 0) 
  {
    LOG(LOG_LEVEL_ERROR, "Error: Opening socket for discovery server: %s (dis)", strerror(errno));
    return -1;
  }
  
  struct sockaddr_in server;
  int length = sizeof(server);
  bzero(&server, length);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(DISCOVERY_SERVER_PORT);
  if (bind(discoverySocketDescriptor, (struct sockaddr *)&server, length) < 0)
  {  
    LOG(LOG_LEVEL_ERROR, "Error: Binding to socket for discovery server: %s (dis)", strerror(errno));
    return -1;
  }
  
  socklen_t fromlen = sizeof(struct sockaddr_in);
  struct sockaddr_in from;
  char buf[2048];
  int n;
  fd_set socks;
  int readsocks;
  struct timeval timeout;
     
  while (boxee.m_bStarted)
	{
    FD_ZERO(&socks);
    FD_SET(discoverySocketDescriptor, &socks);
  	
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;	

    readsocks = select(discoverySocketDescriptor + 1, &socks, (fd_set *) NULL, (fd_set *) NULL, &timeout);
    if (readsocks < 0)
    {
      LOG(LOG_LEVEL_ERROR, "Error: waiting for input in server: %s (dis)", strerror(errno));
      return -1;		  
    }
    else if (FD_ISSET(discoverySocketDescriptor, &socks))
    {
      n = recvfrom(discoverySocketDescriptor, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);
      if (n > 0)
      {
        buf[n] = '\0';
        LOG(LOG_LEVEL_DEBUG, "Got discovery request: %s (dis)\n", buf);

        std::string response = GetInstance().GenerateDiscoveryResponse(buf);
        if (response.length() > 0)
        {
          sendto(discoverySocketDescriptor, response.c_str(), strlen(response.c_str()), 0, (struct sockaddr *)&from, fromlen);
	        LOG(LOG_LEVEL_DEBUG, "Sent discovery response: %s (dis)\n", response.c_str());
        }
      }
    }	  
  }
  
#if defined (_LINUX) || defined (__APPLE__)  
  close(discoverySocketDescriptor);
#else
  closesocket(discoverySocketDescriptor);
#endif
    
  return 0;
}

std::string Boxee::GenerateDiscoveryResponse(std::string request)
{
  std::string response;

  TiXmlDocument requestDoc;
  requestDoc.Parse(request.c_str());
  TiXmlElement* bdp = requestDoc.RootElement();

  if (bdp && bdp->Value() && bx_stricmp(bdp->Value(), "bdp1") == 0)
  {
    const char* cmd = bdp->Attribute("cmd");
    const char* application = bdp->Attribute("application");
    const char* challenge = bdp->Attribute("challenge");
    const char* signature = bdp->Attribute("signature");
    
    if (cmd && application && challenge && signature && bx_stricmp(cmd, "discover") == 0)
    {
      if (strcmp(application, "iphone_remote") == 0)
      {
        std::string expectedSignature = BXUtils::GetMD5Hex(challenge + std::string(IPHONE_REMOTE_KEY));        
        std::string receivedSignature = signature;
        BXUtils::StringToUpper(receivedSignature);
        if (expectedSignature == receivedSignature)
        {
          // Generate 16 chars random string
          int randomChallenge[4];
          srand(time(NULL));
          for (int i = 0; i < 4; i++)
          {
            randomChallenge[i] = rand();
          }
          std::string randomChallengeString = BXUtils::BytesToHexString((unsigned char*) randomChallenge, sizeof(randomChallenge));
          
          // Calculate the digest
          std::string responseSignature = BXUtils::GetMD5Hex(randomChallengeString + std::string(IPHONE_REMOTE_KEY));
          
          response = "<?xml version=\"1.0\"?>\n<BDP1 cmd=\"found\" application=\"boxee\" version=\"";
          response += BOXEE_VERSION;
          response += "\" name=\"";
          response += g_guiSettings.GetString("server.hostname");
          response += "\" httpPort=\"";
          response += m_webServerPort;
          response += "\" httpAuthRequired=\"";

          if(m_webserverPasswordEmpty)
          {
            response += "false";
          }
          else
          {
            response += "true";
          }
          response += "\" response=\"";
          
          response += randomChallengeString;
          response += "\" signature=\"";
          response += responseSignature;
          response += "\"/>";
          
          LOG(LOG_LEVEL_DEBUG, "After building response: %s (dis)\n", response.c_str());
        }
        else
        {
          LOG(LOG_LEVEL_ERROR, "Invalid signature received in request: %s (dis)\n", request.c_str());  
        }
      }
      else
      {
        LOG(LOG_LEVEL_ERROR, "Unknown application in discovery request: %s (dis)\n", application);  
      }
    }
    else
    {
      LOG(LOG_LEVEL_ERROR, "Malformed attributes in discovery request: %s (dis)\n", request.c_str());  
    }
  }
  else
  {
     LOG(LOG_LEVEL_ERROR, "Malformed discovery request: %s (dis)\n", request.c_str());  
  }

  return response;
}

void Boxee::SetWebServerPort(const std::string& port)
{
  m_webServerPort = port;
  
  LOG(LOG_LEVEL_DEBUG,"Boxee::SetWebServerPort - m_webServerPort was set to [%s] (dis)",m_webServerPort.c_str());
}

void Boxee::SetIsWebServerPasswordEmpty(bool webserverPasswordEmpty)
{
  m_webserverPasswordEmpty = webserverPasswordEmpty;
  
  LOG(LOG_LEVEL_DEBUG,"Boxee::SetIsWebServerPasswordEmpty - m_webserverPasswordEmpty was set to [%d] (dis)",m_webserverPasswordEmpty);
}

int Boxee::BoxeeThread(void *pParam)
{
  LOG(LOG_LEVEL_DEBUG,"Boxee thread starting....");
	
	BXConfiguration& config = BXConfiguration::GetInstance();
	Boxee& boxee = Boxee::GetInstance();
	
	time_t tmLastPoll = 0;
	time_t tmFeedTimestamp = 0;
	time_t tmFriendsTimestamp = 0;
	
	while (boxee.m_bStarted)
	{
	  if (!boxee.IsLoggedIn())
		{
		  tmLastPoll = 0;
		}
		
		time_t tmFromLastPoll = time(NULL) - tmLastPoll;
		
		if (!boxee.m_inScreenSaver && boxee.IsLoggedIn() && tmFromLastPoll > (config.GetIntParam("Boxee.PollFreq", 120) + (rand() % 60)))
		{		  
			bool retVal = false;
			
			/////////////////////////////
			// Get RecommendationsList //
			/////////////////////////////
			
			BXBoxeeFeed recommendations;
			
			retVal = boxee.InternalRetrieveRecommendationsList(recommendations, tmFriendsTimestamp);
			
			LOG(LOG_LEVEL_DEBUG,"Call to InternalRetrieveRecommendationsList returned [retVal=%d][RecommendationsListSize=%d] (rec)",retVal,recommendations.GetNumOfActions());
			
			if (retVal)
			{
			  SDL_LockMutex(boxee.m_boxeeGuard);
				boxee.m_recommendationsList = recommendations;
				boxee.m_recommendationsList.SetLoaded(true);
				SDL_UnlockMutex(boxee.m_boxeeGuard);
				
				LOG(LOG_LEVEL_DEBUG,"RecommendationsList datamember was set [RecommendationsListSize=%d] (rec)",boxee.m_recommendationsList.GetNumOfActions());
			}
			
			if (!Boxee::GetInstance().IsRunning())
			{
			  break;
			}

      //////////////
			// Get feed //
			//////////////
			
			MessageFilter filter;
			BXBoxeeFeed	  feed;
			
			retVal = boxee.InternalGetBoxeeFeed(feed, tmFeedTimestamp, filter);
			
			LOG(LOG_LEVEL_DEBUG,"Call to InternalGetBoxeeFeed returned [retVal=%d][feedSize=%d] (fd)",retVal,feed.GetNumOfActions());
			
			if (retVal)
			{
			  SDL_LockMutex(boxee.m_boxeeGuard);
				boxee.m_boxeeFeed = feed;
				boxee.m_boxeeFeed.SetLoaded(true);
				SDL_UnlockMutex(boxee.m_boxeeGuard);
				
				LOG(LOG_LEVEL_DEBUG,"Feed datamember was set [feedSize=%d] (fd)",boxee.m_boxeeFeed.GetNumOfActions());
			}
			
			if (!Boxee::GetInstance().IsRunning())
			{
			  break;
			}

			////////////////////////////////////////////////////
			// Get actionslist - NOTE: Currently doesn't send //
			////////////////////////////////////////////////////
			
			BXBoxeeFeed actions;

			//retVal = boxee.InternalRetrieveActionsList(actions, tmFriendsTimestamp);
			retVal = true; // 100509 - At the moment we don't want to ask the server for the user ActionList
			
			LOG(LOG_LEVEL_DEBUG,"Call to InternalRetrieveActionsList returned [retVal=%d][ActionsListSize=%d] (ac)",retVal,actions.GetNumOfActions());
			
			if (retVal)
			{
			  SDL_LockMutex(boxee.m_boxeeGuard);
				boxee.m_actionsList = actions;
				boxee.m_actionsList.SetLoaded(true);
				SDL_UnlockMutex(boxee.m_boxeeGuard);
				
				LOG(LOG_LEVEL_DEBUG,"ActionsList datamember was set [ActionsListSize=%d] (ac)",boxee.m_actionsList.GetNumOfActions());
			}
			
			if (!Boxee::GetInstance().IsRunning())
			{
			  break;
			}

			/////////////////////
			// Get friendslist //
			/////////////////////
			
			BXFriendsList friends;
			
			//retVal = boxee.InternalRetrieveFriendsList(friends, tmFriendsTimestamp);
			retVal = true; // 031109 - At the moment we don't want to ask the server for the user FriendList
			
			LOG(LOG_LEVEL_DEBUG,"Call to InternalRetrieveFriendsList returned [retVal=%d][FriendsListSize=%d] (fr)",retVal,friends.GetCount());
			
			if (retVal)
			{
			  SDL_LockMutex(boxee.m_boxeeGuard);
				boxee.m_friendsList = friends;
				boxee.m_friendsList.SetLoaded(true);
				SDL_UnlockMutex(boxee.m_boxeeGuard);
				
				LOG(LOG_LEVEL_DEBUG,"FriendsList datamember was set [FriendsListSize=%d] (fr)",boxee.m_friendsList.GetCount());
			}
      
      if (!Boxee::GetInstance().IsRunning())
			{
        break;
			}
				
			// alert on changes
			SDL_LockMutex(boxee.m_listenersGuard);
			for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
			{
			  boxee.m_listeners[i]->OnBoxeeFeedChange(feed);
				boxee.m_listeners[i]->OnFriendsListChange(friends);
				boxee.m_listeners[i]->OnActionsListChange(actions);
				boxee.m_listeners[i]->OnActionsListChange(recommendations);
			}
			SDL_UnlockMutex(boxee.m_listenersGuard);
			
			tmLastPoll = time(NULL);
		}
		
		if (!Boxee::GetInstance().IsRunning())
		{
		  break;
		}
		
		SDL_Delay(2000);
	}
	
	LOG(LOG_LEVEL_DEBUG,"Boxee thread terminating....");
	return 0;
}

unsigned int Boxee::GetStartTime()
{
  return m_timeStart;
}

BXMetadataEngine& Boxee::GetMetadataEngine()
{
  return m_MetadataEngine;
}

void Boxee::Logout()
{
  LOG(LOG_LEVEL_DEBUG,"Boxee::Logout - Enter function. Going to send LOGOUT to the server (logout)");

  std::string strLogoutUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.LogoutUrl","http://app.boxee.tv/api/logout");
  ListHttpHeaders headers;
  
  bool bLoaded = false;
  BXObject userObj;
  userObj.SetCredentials(m_credentials);

  bLoaded = userObj.LoadFromURL(strLogoutUrl.c_str(), headers);

  if (!bLoaded)
  {
    LOG(LOG_LEVEL_WARNING,"Boxee::Logout - LOGOUT from server FAILED (logout)");
  }
  
  SignalStop();

	m_bLoggedIn = false;
	m_boxeeFeed.Clear();
	m_recommendationsList.Clear();
	m_friendsList.Clear();
	m_actionsList.Clear();
	m_credentials.Clear();
	m_currentUserObj = BXObject();
	Stop();

	m_MetadataEngine.Reset();
	
  LOG(LOG_LEVEL_DEBUG,"Boxee::Logout - Exit function (logout)");
}

void Boxee::ResetCookies()
{
  unlink(BXCurl::GetCookieJar().c_str());
}

bool Boxee::IsLoggedIn()
{
  return m_bLoggedIn;
}

bool Boxee::IsDuringLogin()
{
  return m_bDuringLogin;
}

BXLoginStatus Boxee::Login(BXCredentials &creds, BXObject &userObj)
{
  m_bDuringLogin = true;

  Start(); // make sure we are running
  
  m_boxeeFeed.Clear();
  m_friendsList.Clear();
  m_actionsList.Clear();
  m_recommendationsList.Clear();
	
  BXXMLDocument doc;
  doc.SetCredentials(creds);

  LOG(LOG_LEVEL_DEBUG,"Boxee::Login - After set creds [userObjUserName=%s][userObjPass=%s]. [m_boxeeFeedSize=%d][m_friendsListSize=%d][m_actionsListSize=%d] (login)",userObj.GetCredentialsUserName().c_str(),userObj.GetCredentialsPassword().c_str(),m_boxeeFeed.GetNumOfActions(),m_friendsList.GetCount(),m_actionsList.GetNumOfActions());
	
  std::string strLoginUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.LoginUrl","http://app.boxee.tv/api/login");
	ListHttpHeaders headers;
	
	bool bLoaded = false;
  BXLoginStatus loginRet = LOGIN_ERROR_ON_NETWORK;

  bLoaded = doc.LoadFromURL(strLoginUrl.c_str(), headers);

  if (!bLoaded)
  {
    int nLastErr = doc.GetLastNetworkError();
    m_bLoggedIn = false;
    if (nLastErr == 401)
      loginRet = LOGIN_ERROR_ON_CREDENTIALS;

    m_bDuringLogin = false;

		return loginRet;
	}
	
	if (!ParseServerLoginResponse(doc,userObj))
	{
	  m_bLoggedIn = false;
	  m_bDuringLogin = false;
	  return LOGIN_GENERAL_ERROR;
	}
	
	creds.SetAuthKey(doc.GetRespHeader("cookie"));

	PostLoginInitializations(creds, userObj);

	m_bLoggedIn = true;
	loginRet = LOGIN_SUCCESS;
	
	m_bDuringLogin = false;

	return loginRet;
}

void Boxee::PostLoginInitializations(const BXCredentials& creds, const BXObject& userObj)
{
  // overrun internal credentials on successful login
  SetCredentials(creds);

  m_currentUserObj = userObj;
}

bool Boxee::ParseServerLoginResponse(BXXMLDocument& doc, BXObject& userObj)
{
  const TiXmlElement* pRootElement = doc.GetDocument().RootElement();

  if (!pRootElement)
  {
    LOG(LOG_LEVEL_ERROR,"Boxee::ParseServerLoginResponse - FAILED to get root element (login)");
    return false;
  }

  std::string showMovieLibraryStr = "";

  if (bx_stricmp(pRootElement->Value(),"object") == 0)
  {
    // old style server response -> only <object> element

    LOG(LOG_LEVEL_DEBUG,"CGUIWindowLoginScreen::ParseServerLoginResponse - Going to parse OLD style server response. only <object> element (login)");

    ParseUserObjFromServerResponse(pRootElement,userObj);

    if (userObj.HasValue(SHOW_MOVIE_LIBARRY))
    {
      showMovieLibraryStr = userObj.GetValue(SHOW_MOVIE_LIBARRY);
    }
  }
  else
  {
    // new style server response

    LOG(LOG_LEVEL_DEBUG,"CGUIWindowLoginScreen::ParseServerLoginResponse - Going to parse NEW style server response (login)");

    if (bx_stricmp(pRootElement->Value(),"boxee") != 0)
    {
      LOG(LOG_LEVEL_ERROR,"CGUIWindowLoginScreen::ParseServerLoginResponse - Root element isn't <boxee>. <%s> (login)",pRootElement->Value());
      return false;
    }

    bool foundUserObjectElement = false;

    const TiXmlNode* pTag = NULL;
    while ((pTag = pRootElement->IterateChildren(pTag)))
    {
      std::string elementName = pTag->ValueStr();

      if (elementName == "object")
      {
        ///////////////////////
        // case of <object> //
        ///////////////////////

        foundUserObjectElement = true;

        ParseUserObjFromServerResponse(pTag,userObj);
      }
      else if (elementName == SHOW_MOVIE_LIBARRY)
      {
        //////////////////////////////////
        // case of <show-movie-library> //
        //////////////////////////////////

        const TiXmlNode* pValue = pTag->FirstChild();

        if (pValue)
        {
          showMovieLibraryStr = pValue->ValueStr();
        }
        else
        {
          LOG(LOG_LEVEL_ERROR,"CGUIWindowLoginScreen::ParseServerLoginResponse - FAILED to get <show_movie_library> element value (login)");
        }
      }
    }

    if (!foundUserObjectElement)
    {
      LOG(LOG_LEVEL_ERROR,"CGUIWindowLoginScreen::ParseServerLoginResponse - Element <object> wasn't found (login)");
      return false;
    }
  }

  if (!userObj.IsValid())
  {
    LOG(LOG_LEVEL_ERROR,"CGUIWindowLoginScreen::ParseServerLoginResponse - User object is not valid (login)");
    return false;
  }

  if (!showMovieLibraryStr.empty())
  {
    LOG(LOG_LEVEL_DEBUG,"CGUIWindowLoginScreen::ParseServerLoginResponse - Read [showMovieLibraryStr=%s] (login)",showMovieLibraryStr.c_str());
    g_infoManager.SetShowMovieLibrary((bx_stricmp(showMovieLibraryStr.c_str(), "1") == 0 || bx_stricmp(showMovieLibraryStr.c_str(), "true") == 0) ? true : false);
  }

  return true;
}

void Boxee::ParseUserObjFromServerResponse(const TiXmlNode* pObjectTag, BXObject& userObj)
{
  if (!pObjectTag)
  {
    LOG(LOG_LEVEL_ERROR,"CGUIWindowLoginScreen::ParseServerLoginResponse - Element <object> wasn't found (login)");
    return;
  }

  userObj.FromXML(pObjectTag);
  userObj.SetValid(true);
}

void Boxee::RunTest()
{
  printf("Running Boxee test...\n");
	return;
}

void Boxee::StartMetadataEngine()
{
  m_MetadataEngine.Start();
}

bool Boxee::Login_bg(const BXCredentials &creds)
{
  Start(); // make sure we are running
	
	class BGLogin: public BXBGJob
	{
	public:
	  BGLogin() : BXBGJob("BGLogin") {}
	  virtual void DoWork()
		{
		  BXObject userObj(false);
			Boxee &boxee = Boxee::GetInstance();
			BXLoginStatus eLoginStat = boxee.Login(m_creds, userObj);
			
			// alert listeners
			SDL_LockMutex(boxee.m_listenersGuard);
			for (unsigned int i=0; i<boxee.m_listeners.size(); i++)
			{
			  boxee.m_listeners[i]->OnLoginEnded(m_creds, eLoginStat, userObj);
			}
			SDL_UnlockMutex(boxee.m_listenersGuard);
		}
		
		BXCredentials m_creds;
	};
	
	BGLogin *aJob = new BGLogin;
	aJob->m_creds = creds;
	
	if (!m_backGroundProcessor.QueueJob(aJob))
	{
	  delete aJob;
	}
	
	return true;
}

void Boxee::setDatabaseFolderPath(const std::string& fDatabaseFolderPath)
{
  mDatabaseFolderPath = fDatabaseFolderPath;
	LOG(LOG_LEVEL_DEBUG, "mDatabaseFolderPath was set to [%s]", mDatabaseFolderPath.c_str());
}

std::string Boxee::getDatabaseFolderPath()
{
  return mDatabaseFolderPath;
}

void Boxee::SetInOfflineMode(bool inOfflineMode)
{
  mInOfflineMode = inOfflineMode;

  LOG(LOG_LEVEL_DEBUG,"mInOfflineMode was set to [%d] (bf)(feed)(rec)(fr)",mInOfflineMode);
}

bool Boxee::IsInOfflineMode()
{
  return mInOfflineMode;
}

void Boxee::SetTempFolderPath(const std::string& fTempFolderPath)
{
  mTempFolderPath = fTempFolderPath;
	LOG(LOG_LEVEL_DEBUG, "mTempFolderPath was set to [%s]", mTempFolderPath.c_str());
}

std::string Boxee::GetTempFolderPath()
{
  return mTempFolderPath;
}

void Boxee::SetInScreenSaver(bool inScreenSaver)
{
  m_inScreenSaver = inScreenSaver;
  
  LOG(LOG_LEVEL_DEBUG,"m_inScreenSaver was set to [%d] (ssm)",m_inScreenSaver);
}

bool Boxee::IsInScreenSaver()
{
  return m_inScreenSaver;
}

CBoxeeClientServerComManager& Boxee::GetBoxeeClientServerComManager()
{
  return m_boxeeCscManager;
}

CBoxeeScheduleTaskManager& Boxee::GetBoxeeScheduleTaskManager()
{
  return m_boxeeScheduleTaskManager;
}

void Boxee::SetPlatform(const std::string& platform)
{
  m_platform = platform;
}

std::string Boxee::GetPlatform()
{
  return m_platform;
}

const char* Boxee::CBoxeeLoginStatusEnumAsString(BXLoginStatus boxeeLoginStatusEnum)
{
  switch(boxeeLoginStatusEnum)
  {
  case LOGIN_SUCCESS:
    return "LOGIN_SUCCESS";
  case LOGIN_ERROR_ON_NETWORK:
    return "LOGIN_ERROR_ON_NETWORK";
  case LOGIN_ERROR_ON_CREDENTIALS:
    return "LOGIN_ERROR_ON_CREDENTIALS";
  case LOGIN_GENERAL_ERROR:
    return "LOGIN_GENERAL_ERROR";
  default:
    LOG(LOG_LEVEL_DEBUG,"Boxee::CBoxeeLoginStatusEnumAsString - Failed to convert enum [%d] to string BoxeeLoginStatusEnum. Return LOGIN_STATUS_NONE (login)",boxeeLoginStatusEnum);
    return "LOGIN_STATUS_NONE";
  }
}

bool Boxee::ReportInstallRss(const std::string& strData)
{
  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ReportInstallRssUrl","http://app.boxee.tv/rss/subscribe_multi");

  BXCurl report;
  report.SetCredentials(m_credentials);

  ListHttpHeaders headers;
  headers.push_back("Content-Type: text/xml");
  report.HttpSetHeaders(headers);

  std::string response = report.HttpPostString(strUrl.c_str(),strData);

  LOG(LOG_LEVEL_DEBUG,"Boxee::ReportInstallRss - Report to server returned [ResSize=%d] (rtsa)",(int)response.length());

  return true;
}

bool Boxee::ReportInstallRss_bg(const std::string& strData)
{
  class ReportInstallRssJob: public BXBGJob
  {
  public:
    ReportInstallRssJob() : BXBGJob("ReportInstallRssJob") {}
    virtual void DoWork()
    {
      bool bOk = Boxee::GetInstance().ReportInstallRss(m_strData);
      if (!bOk)
      {
        LOG(LOG_LEVEL_ERROR,"failed to run user action in BG");
      }
    }

    std::string m_strData;
  };

  ReportInstallRssJob *aJob = new ReportInstallRssJob;
  aJob->m_strData = strData;

  if (!m_backGroundProcessor.QueueJob(aJob))
  {
    delete aJob;
  }

  return true;
}

} // namespace BOXEE

