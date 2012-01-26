// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxmessages
//
// Description: 
//
// this file contains the message types that can be communicated to the boxee server.
// 
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXMESSAGES_H
#define BOXEEBXMESSAGES_H

#include <string>
#include <map>
#include <vector>
#include "tinyXML/tinyxml.h"
#include "bxstringmap.h"
#include "bxobject.h"

#undef GetObject

namespace BOXEE {

// message types
#define MSG_OBJ_TYPE_MOVIE  "movie"
#define MSG_OBJ_TYPE_TV  	"tv_show"
#define MSG_OBJ_TYPE_AUDIO_TRACK "audio_track"
#define MSG_OBJ_TYPE_AUDIO_ALBUM "audio_album"
#define MSG_OBJ_TYPE_RADIO "radio"
#define MSG_OBJ_TYPE_STREAM "stream"
#define MSG_OBJ_TYPE_VIDEO_STREAM "stream_video"
#define MSG_OBJ_TYPE_AUDIO_STREAM "stream_audio"
#define MSG_OBJ_TYPE_PICTURE "picture"
#define MSG_OBJ_TYPE_PICTURES "pictures"
#define MSG_OBJ_TYPE_UNKNOWN_VIDEO "unknown_video"
#define MSG_OBJ_TYPE_UNKNOWN_AUDIO "unknown_audio"
#define MSG_OBJ_TYPE_UNKNOWN "unknown"
#define MSG_OBJ_TYPE_USER "user"
#define MSG_OBJ_TYPE_FLASH_STREAM_MOVIE  "stream_video_flash_movie"
#define MSG_OBJ_TYPE_FLASH_STREAM_TVSHOW "stream_video_flash_tv_show"
#define MSG_OBJ_TYPE_TVSHOW_SUBSCRIPTION "tvshow-subscription"
#define MSG_OBJ_TYPE_RSS_SUBSCRIPTION "rss-subscription"

#define MSG_OBJ_TYPE_STREAM_MOVIE  "stream_movie"
#define MSG_OBJ_TYPE_STREAM_TV_SHOW  "stream_tv_show"
#define MSG_OBJ_TYPE_APPLICATION      "application"

#define MSG_ACTION_TYPE_RECOMMEND                "recommend"
#define MSG_ACTION_TYPE_PROMOTION                "promotion"
#define MSG_ACTION_TYPE_LISTEN                   "listen"
#define MSG_ACTION_TYPE_WATCH                    "watch"
#define MSG_ACTION_TYPE_ADD_FRIEND               "friend"
#define MSG_ACTION_TYPE_RATE                     "rate"
#define MSG_ACTION_TYPE_SHARE                    "share"
#define MSG_ACTION_TYPE_QUEUE                    "queue"
#define MSG_ACTION_TYPE_DEQUEUE                  "dequeue"
#define MSG_ACTION_TYPE_INSTALL                  "install"
#define MSG_ACTION_TYPE_REMOVE                   "remove"
#define MSG_ACTION_TYPE_SERVICE                  "service"
#define MSG_ACTION_TYPE_SUBSCRIBE                "subscribe"
#define MSG_ACTION_TYPE_UNSUBSCRIBE              "unsubscribe"
#define MSG_ACTION_TYPE_FEATURED                 "featured"
#define MSG_ACTION_TYPE_LAUNCH                   "launch"
//
// every boxee message is actually a general string map.
// the user should query the map's values according to the message type.
//

// message keys
#define MSG_KEY_TIMESTAMP	"timestamp"
#define MSG_KEY_DESCRIPTION	"description"
#define MSG_KEY_NAME		"name"
#define MSG_KEY_TITLE		"title"
#define MSG_KEY_ARTIST		"artist"
#define MSG_KEY_ALBUM_ARTIST "album_artist"
#define MSG_KEY_SEASON		"season"
#define MSG_KEY_ALBUM		"album"
#define MSG_KEY_EPISODE		"episode"
#define MSG_KEY_EPISODE_NAME   "episode_name"
#define MSG_KEY_GENRE		"genre"
#define MSG_KEY_DIRECTOR	"director"
#define MSG_KEY_STUDIO		"studio"
#define MSG_KEY_FILE		"file"
#define MSG_KEY_IMDB_NUM	"imdb_num"
#define MSG_KEY_LIKE		"thumbs_up"
#define MSG_KEY_THUMB		"thumb"
#define MSG_KEY_THUMB_URL	"thumb_url"
#define MSG_KEY_VERSION		"version"
#define MSG_KEY_BDAY		"birth_date"
#define MSG_KEY_GENDER		"gender"
#define MSG_KEY_LOCATION	"location"
#define MSG_KEY_PROFILE		"profile"
#define MSG_KEY_PRESENCE	"presence"
#define MSG_KEY_FEED		"feed"
#define MSG_KEY_FRIENDS		"friends"
#define MSG_KEY_ACTIONS		"actions"
#define MSG_KEY_URL			"url"
#define MSG_KEY_STATION		"station"
#define MSG_KEY_TRAILER		"trailer"
#define MSG_KEY_EMAIL		"email"
#define MSG_KEY_RELEASE_DATE  "release_date"
#define MSG_KEY_RELEASE_YEAR  "year"
#define MSG_KEY_CONTENT_TYPE  "content_type"
#define MSG_KEY_SRC    "src"
#define MSG_KEY_ID     "id"
#define MSG_KEY_IN_BOXEE    "in_boxee"
#define MSG_KEY_USER_TEXT   "user_text"
#define MSG_KEY_BOXEE_ID    "boxee_id"
#define MSG_KEY_CLIENT_ID   "client_id"
#define MSG_KEY_SHOW_ID     "show_id"
#define MSG_KEY_SHOW_NAME   "show_name"
#define MSG_KEY_STREAM_TYPE  "stream_type"
#define MSG_KEY_APP_ID  "app_id"
#define MSG_KEY_RUN_TIME  "run_time"

#define MSG_KEY_PROVIDER                "provider"
#define MSG_KEY_PLAY_PROVIDER_LABEL     "label"
#define MSG_KEY_PLAY_PROVIDER_THUMB     "provider_thumb"
#define MSG_KEY_PLAY_PROVIDER_THUMB_ON  "provider_thumb_on"

#define MSG_KEY_ACTIVATED_FROM "activated_from"

#define MSG_PRESENCE_ONLINE "online"
#define MSG_PRESENCE_AWAY   "away"
#define MSG_PRESENCE_OFFLINE "offline"

#define MSG_KEY_ADULT "adult"
#define MSG_KEY_COUNTRY "country"
#define MSG_KEY_COUNTRY_ALLOW "country-allow"

class BXGeneralMessage : public BXStringMap {
public: 
	BXGeneralMessage();
	virtual ~BXGeneralMessage();
	
	std::string	GetMessageType() const;
	void		SetMessageType(const std::string &strType);

	int 		GetTimestamp() const;
	void		SetTimestamp(int nTimestamp);
	void 		SetTimestamp(const std::string  &strTimestamp);

	int 	 GetObjectCount() const;
	BXObject GetObject(int nIndex) const;
	BXObject GetObjectByType(const std::string &strType) const;
	BXObject GetObjectByID(const std::string &strID) const;

  void SetReferral(const std::string &strReferral);
  std::string GetReferral() const;

  void SetSource(const std::string &strSource);
  std::string GetSource() const;

	void AddObject(const BXObject &obj);

	virtual bool ParseFromActionNode(const TiXmlNode *pActionNode);
	virtual std::string ToXML() const;

	// need to be called once on startup to initialize static lookup table.
	static void Initialize();

protected:

	std::string  m_type;
	std::string  m_referral;
	std::string  m_source;
	std::vector<BXObject> m_objects;

	// lookup table to translate xml tag value to message constants.
	// values that can't be found will be copied as is
	static	std::map<std::string,std::string> m_xmlLookupTable;
} ;

//
// message filter is yet another string map. the values are the fields that the filter will apply
// to. 
// 
typedef std::map<std::string, std::string> MessageFilter;

}

#endif
