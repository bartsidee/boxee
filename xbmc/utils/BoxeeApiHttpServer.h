#ifndef BOXEEAPIHTTPSERVER_H
#define BOXEEAPIHTTPSERVER_H

#include "HttpServer.h"
#include "lib/libjson/include/json/value.h"
#include <SDL/SDL_mutex.h>

class CGUIDialogKeyboard;

class CBoxeeApiHttpServer : public CHttpServer, public IHttpChunkedResponse
{
public:
  CBoxeeApiHttpServer();
  virtual ~CBoxeeApiHttpServer();
  virtual void HandleRequest(CHttpRequest& request, CHttpResponse& response);

  SDL_cond* GetStatusCond();

private:

  class CJsonRpcMethod
  {
  public:
    enum JsonRpcMethodEnum
    {
      PAIRING_CHALLENGE=0,
      PAIRING_RESPONSE=1,
      PAIRING_UNPAIR=2,
      BOXEE_STATUS=3,
      PLAYER_PLAY_URL=4,
      PLAYER_PLAY=5,
      PLAYER_PAUSE=6,
      PLAYER_PLAY_NEXT=7,
      PLAYER_PLAY_PREVIOUS=8,
      PLAYER_SEEK_TO_POS=9,
      PLAYER_SET_VOLUME=10,
      NAV_JUMP_TO_SECTION=11,
      TEXTFIELD_UPDATE=12,
      NAV_REMOTE_KEY=13,
      NAV_TRACKPAD_MOVE=14,
      NAV_TRACKPAD_CLICK=15,
      NAV_TRACKPAD_SCROLL=16,
      NONE = 17
    };
  };

  int HandlePostRequestInternal(const CStdString& deviceId, CJsonRpcMethod::JsonRpcMethodEnum method, const Json::Value& inputroot, Json::Value& outroot);

  CJsonRpcMethod::JsonRpcMethodEnum GetMethod(const CStdString& uri);

  int HandlePairingChallengeRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outroot);
  int HandlePairingResponseRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outroot);
  int HandlePairingUnpairRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outroot);
  int HandleBoxeeStatusRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerPlayUrlRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerPlayRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerPauseRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerPlayNextRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerPlayPreviousRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerSeekToPositionRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandlePlayerSetVolumeRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleNavJumpToSectionRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleTextfileldUpdateRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleNavRemoteKeyRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleNavTrackpadModeRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleNavTrackpadClickRequest(const Json::Value& inputroot, Json::Value& outroot);
  int HandleNavTrackpadScrollRequest(const Json::Value& inputroot, Json::Value& outroot);

  virtual void GetChunkedResponse(const CStdString& deviceId, const CStdString& url, std::vector<unsigned char>& data);

  void SetErrorValue(Json::Value& jValue, const char* errorMsg);

  CGUIDialogKeyboard* GetKeyboard();

  static SDL_mutex* m_statusLock;
  static SDL_cond* m_statusCond;
};

#endif // BOXEEAPIHTTPSERVER_H
