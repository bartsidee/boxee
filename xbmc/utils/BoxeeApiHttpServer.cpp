#include "BoxeeApiHttpServer.h"
#include "log.h"
#include "StdString.h"
#include "lib/libjson/include/json/reader.h"
#include "lib/libjson/include/json/writer.h"
#include "Application.h"
#include "GUIDialogKeyboard.h"
#include "BoxeeUtils.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"
#include "BoxeeItemLauncher.h"
#include "ApplicationMessenger.h"
#include "GUIDialogBoxeePair.h"
#include "GUIUserMessages.h"
#include "Key.h"
#include "lib/libBoxee/bxutils.h"

using namespace Json;
using namespace BOXEE;

SDL_mutex* CBoxeeApiHttpServer::m_statusLock = SDL_CreateMutex();
SDL_cond* CBoxeeApiHttpServer::m_statusCond = SDL_CreateCond();

CBoxeeApiHttpServer::CBoxeeApiHttpServer()
{

}

CBoxeeApiHttpServer::~CBoxeeApiHttpServer()
{
  if (m_statusCond)
  {
    SDL_DestroyCond(m_statusCond);
  }

  if (m_statusLock)
  {
    SDL_DestroyMutex(m_statusLock);
  }
}

void CBoxeeApiHttpServer::GetChunkedResponse(const CStdString& deviceId, const CStdString& url, std::vector<unsigned char>& data)
{
  CJsonRpcMethod::JsonRpcMethodEnum methodEnum = GetMethod(url);

  if (methodEnum == CJsonRpcMethod::NONE)
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::GetChunkedResponse - FAILED to handle [url=%s] (hsrv)",url.c_str());
    data.clear();
    return;
  }

  SDL_LockMutex(m_statusLock);
  SDL_CondWait(m_statusCond, m_statusLock);
  SDL_UnlockMutex(m_statusLock);

  Json::Value inputroot;
  Json::Value outputroot;
  HandlePostRequestInternal(deviceId,methodEnum,inputroot,outputroot);
  Json::FastWriter writer;
  CStdString bodyStr = writer.write(outputroot);
  data.insert(data.begin(),bodyStr.begin(), bodyStr.end());
  data.insert(data.end(),'\0');
}

void CBoxeeApiHttpServer::HandleRequest(CHttpRequest& request, CHttpResponse& httpResponse)
{
  /*
   * Make sure we have X-Boxee-Device-ID in the header
   */
  IMAPHTTPHEADER it = request.GetHeader().find("X-Boxee-Device-ID");
  if (it == request.GetHeader().end())
  {
    httpResponse.SetCode(401);
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePostRequest - FAILED to get X-Boxee-Device-ID from header (hsrv)");
    return;
  }

  CStdString deviceId(it->second);

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePostRequest - got [X-Boxee-Device-ID=%s] from header (hsrv)",deviceId.c_str());

  /*
   * Parse the uri and make sure we can handle that method
   */
  char* uri = request.GetUri();
  if (!uri)
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePostRequest - enter with NULL [uri=%p] (hsrv)",uri);
    return;
  }

  CStdString uriStr(uri);

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePostRequest - enter function. [uri=%s][deviceid=%s][NumOfHeaderParams=%lu] (hsrv)",uri, deviceId.c_str(), request.GetHeader().size());

  CJsonRpcMethod::JsonRpcMethodEnum method = GetMethod(uriStr);
  if (method == CJsonRpcMethod::NONE)
  {
    httpResponse.SetCode(404);
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePostRequest - FAILED to get method (hsrv)");
    return;
  }

  // check if device is paired
  if ((method > CJsonRpcMethod::PAIRING_RESPONSE) && !g_application.GetBoxeeDeviceManager().IsPairDevice(deviceId))
  {
    httpResponse.SetCode(401);
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePostRequest - FAILED to handle method [%d] with [deviceId=%s] because deviceId ISN'T paired (hsrv)",method,deviceId.c_str());
    return;
  }

  // TEMPORARY!!
  if (method == CJsonRpcMethod::BOXEE_STATUS)
  {
    httpResponse.SetCode(200);
    httpResponse.SetChunked(true);
    httpResponse.SetChunkedCallback(this);
    return;
  }

  /*
   * Parse the request and handle it
   */
  Json::Value inputroot;
  Json::Value outputroot;
  Json::Reader reader;
  int retCode;

  CStdString requestBody(request.GetBody());

  if (!requestBody.IsEmpty() && (!reader.parse(requestBody, inputroot) || !inputroot.isObject()))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePostRequest - FAILED to parse [%s]. [isObject=%d] (hsrv)",requestBody.c_str(),inputroot.isObject());
    retCode = 400;
    SetErrorValue(outputroot,"failed to parse request body");
  }
  else
  {
    retCode = HandlePostRequestInternal(deviceId,method,inputroot,outputroot);
  }

  /*
   * Set the response
   */
  httpResponse.SetCode(retCode);
  Json::FastWriter writer;
  CStdString bodyStr = writer.write(outputroot);
  std::vector<unsigned char> body(bodyStr.begin(), bodyStr.end());
  httpResponse.SetBody(body);

  //CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePostRequest - create response body [%s] (hsrv)",bodyStr.c_str());
}

int CBoxeeApiHttpServer::HandlePostRequestInternal(const CStdString& deviceId, CJsonRpcMethod::JsonRpcMethodEnum method, const Json::Value& inputroot, Json::Value& outputroot)
{
  int retCode = 200;

  switch(method)
  {
  case CJsonRpcMethod::PAIRING_CHALLENGE:
  {
    retCode = HandlePairingChallengeRequest(deviceId,inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PAIRING_RESPONSE:
  {
    retCode = HandlePairingResponseRequest(deviceId,inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PAIRING_UNPAIR:
  {
    retCode = HandlePairingUnpairRequest(deviceId,inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::BOXEE_STATUS:
  {
    retCode = HandleBoxeeStatusRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_PLAY_URL:
  {
    retCode = HandlePlayerPlayUrlRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_PLAY:
  {
    retCode = HandlePlayerPlayRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_PAUSE:
  {
    retCode = HandlePlayerPauseRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_PLAY_NEXT:
  {
    retCode = HandlePlayerPlayNextRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_PLAY_PREVIOUS:
  {
    retCode = HandlePlayerPlayPreviousRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_SEEK_TO_POS:
  {
    retCode = HandlePlayerSeekToPositionRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::PLAYER_SET_VOLUME:
  {
    retCode = HandlePlayerSetVolumeRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::NAV_JUMP_TO_SECTION:
  {
    retCode = HandleNavJumpToSectionRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::TEXTFIELD_UPDATE:
  {
    retCode = HandleTextfileldUpdateRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::NAV_REMOTE_KEY:
  {
    retCode = HandleNavRemoteKeyRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::NAV_TRACKPAD_MOVE:
  {
    retCode = HandleNavTrackpadModeRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::NAV_TRACKPAD_CLICK:
  {
    retCode = HandleNavTrackpadClickRequest(inputroot,outputroot);
  }
  break;
  case CJsonRpcMethod::NAV_TRACKPAD_SCROLL:
  {
    retCode = HandleNavTrackpadScrollRequest(inputroot,outputroot);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CHttpJsonRpcHandler::HandlePostRequest - FAILED to handle [jRpcMethod=%d] (hsrv)",method);
    retCode = 500;
  }
  break;
  }

  return retCode;
}

CBoxeeApiHttpServer::CJsonRpcMethod::JsonRpcMethodEnum CBoxeeApiHttpServer::GetMethod(const CStdString& uri)
{
  size_t lastSlashPos = uri.find_last_of('/');

  if (lastSlashPos == std::string::npos)
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::GetMethod - FAILED to find [uri=%s] (hsrv)",uri.c_str());
    return CJsonRpcMethod::NONE;
  }

  CStdString methodStr = uri.substr(lastSlashPos+1);
  methodStr.ToLower();
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::GetMethod - got [method=%s] from [uri=%s] (hsrv)",methodStr.c_str(),uri.c_str());

  if (methodStr == "pairing.challenge")
  {
    return CJsonRpcMethod::PAIRING_CHALLENGE;
  }
  else if (methodStr == "pairing.response")
  {
    return CJsonRpcMethod::PAIRING_RESPONSE;
  }
  else if (methodStr == "pairing.unpair")
  {
    return CJsonRpcMethod::PAIRING_UNPAIR;
  }
  else if (methodStr == "boxee.status")
  {
    return CJsonRpcMethod::BOXEE_STATUS;
  }
  else if (methodStr == "player.playurl")
  {
    return CJsonRpcMethod::PLAYER_PLAY_URL;
  }
  else if (methodStr == "player.play")
  {
    return CJsonRpcMethod::PLAYER_PLAY;
  }
  else if (methodStr == "player.pause")
  {
    return CJsonRpcMethod::PLAYER_PAUSE;
  }
  else if (methodStr == "player.playnext")
  {
    return CJsonRpcMethod::PLAYER_PLAY_NEXT;
  }
  else if (methodStr == "player.playprevious")
  {
    return CJsonRpcMethod::PLAYER_PLAY_PREVIOUS;
  }
  else if (methodStr == "player.seektoposition")
  {
    return CJsonRpcMethod::PLAYER_SEEK_TO_POS;
  }
  else if (methodStr == "player.setvolume")
  {
    return CJsonRpcMethod::PLAYER_SET_VOLUME;
  }
  else if (methodStr == "nav.jumptosection")
  {
    return CJsonRpcMethod::NAV_JUMP_TO_SECTION;
  }
  else if (methodStr == "textfield.update")
  {
    return CJsonRpcMethod::TEXTFIELD_UPDATE;
  }
  else if (methodStr == "nav.remotekey")
  {
    return CJsonRpcMethod::NAV_REMOTE_KEY;
  }
  else if (methodStr == "nav.trackpadmove")
  {
    return CJsonRpcMethod::NAV_TRACKPAD_MOVE;
  }
  else if (methodStr == "nav.trackpadclick")
  {
    return CJsonRpcMethod::NAV_TRACKPAD_CLICK;
  }
  else if (methodStr == "nav.trackpadscroll")
  {
    return CJsonRpcMethod::NAV_TRACKPAD_SCROLL;
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::GetMethod - FAILED to handle unknown [method=%s]. [uri=%s] (hsrv)",methodStr.c_str(),uri.c_str());
    return CJsonRpcMethod::NONE;
  }
}

int CBoxeeApiHttpServer::HandlePairingChallengeRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - enter function (hsrv)");
  int retCode = 500;

  if (deviceId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - enter function with EMPTY [deviceId=%s] in request (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no deviceId parameter in request");
    return retCode;
  }

  if (g_application.GetBoxeeDeviceManager().IsPairDevice(deviceId))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to pair device because deviceId already paired. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"failed to pair device because deviceId already paired");
    return retCode;
  }

  CGUIDialogBoxeePair* pairDlg = (CGUIDialogBoxeePair*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAIR);
  if (pairDlg && pairDlg->IsActive())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to handle request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"failed to handle request");
    return retCode;
  }

  if (!inputroot.isMember("label"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to find [label] in request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no label parameter in request");
    return retCode;
  }
  CStdString deviceLabel = inputroot.get("label","").asString();

  if (!inputroot.isMember("device_type"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to find [device_type] in request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no device_type parameter in request");
    return retCode;
  }
  CStdString deviceType = inputroot.get("device_type","").asString();

  if (!inputroot.isMember("device_icon"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to find [device_icon] in request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no device_icon parameter in request");
    return retCode;
  }
  CStdString deviceIcon = inputroot.get("device_icon","").asString();

  if (!inputroot.isMember("application_id"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to find [application_id] in request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no application_id parameter in request");
    return retCode;
  }
  CStdString deviceApplicationId = inputroot.get("application_id","").asString();

  CBoxeeDeviceItem* pDeviceItem = new CBoxeeDeviceItem();
  if (!pDeviceItem || !pDeviceItem->Initialize(deviceId, deviceLabel, deviceType, deviceIcon, deviceApplicationId))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to initialize BoxeeDeviceItem. [deviceItem=%p][deviceId=%s] (hsrv)",pDeviceItem,deviceId.c_str());
    SetErrorValue(outputroot,"failed to initialize pairing");
    return retCode;
  }

  ThreadMessage tMsg = {TMSG_SHOW_BOXEE_DEVICE_PAIR_DIALOG};
  tMsg.lpVoid = pDeviceItem;
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

  retCode = 200;
  outputroot["success"] = "true";

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - exit function. [retCode=%d] (hsrv)",retCode);

  return retCode;
}

int CBoxeeApiHttpServer::HandlePairingResponseRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingResponseRequest - enter function (hsrv)");
  int retCode = 500;

  if (deviceId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - enter function with EMPTY [deviceId=%s] in request (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no deviceId parameter in request");
    return retCode;
  }

  if (!inputroot.isMember("code"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - FAILED to find [code] in request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no device_icon parameter in request");
    return retCode;
  }
  CStdString code = inputroot.get("code","").asString();

  CGUIDialogBoxeePair* pairDlg = (CGUIDialogBoxeePair*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAIR);
  if (!pairDlg || !pairDlg->IsActive())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - FAILED to handle request. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"failed to handle request");
    return retCode;
  }

  if (!pairDlg->PairDevice(deviceId, code))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - FAILED to pair device. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"failed to pair device");
    return retCode;
  }

  CGUIMessage refreshWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_SETTINGS_DEVICES, 0);
  g_windowManager.SendThreadMessage(refreshWinMsg);

  retCode = 200;
  outputroot["success"] = "true";

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingResponseRequest - exit function. [retCode=%d] (hsrv)",retCode);

  return retCode;
}

int CBoxeeApiHttpServer::HandlePairingUnpairRequest(const CStdString& deviceId, const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingUnpairRequest - enter function (hsrv)");
  int retCode = 500;

  if (deviceId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingUnpairRequest - enter function with EMPTY [deviceId=%s] in request (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"no deviceId parameter in request");
    return retCode;
  }

  if (!g_application.GetBoxeeDeviceManager().UnPairDevice(deviceId))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - FAILED to unpair device. [deviceId=%s] (hsrv)",deviceId.c_str());
    SetErrorValue(outputroot,"failed to unpair device");
    return retCode;
  }

  CGUIMessage refreshWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_SETTINGS_DEVICES, 0);
  g_windowManager.SendThreadMessage(refreshWinMsg);

  retCode = 200;
  outputroot["success"] = "true";

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingUnpairRequest - exit function. [retCode=%d] (hsrv)",retCode);

  return retCode;
}

int CBoxeeApiHttpServer::HandleBoxeeStatusRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleBoxeeStatusRequest - enter function (hsrv)");
  int retCode = 500;

  outputroot["success"] = "true";

  Json::Value status;
  CStdString keyboardMode = "none";
  CStdString keyboardText = "";
  CGUIDialogKeyboard* pKeyboard = GetKeyboard();
  if (pKeyboard->IsActive())
  {
    keyboardMode = pKeyboard->IsHiddenInput() ? "password" : "regular";
    keyboardText = pKeyboard->GetText();
  }
  status["keyboard"]["current-value"] = keyboardText;
  status["keyboard"]["mode"] = keyboardMode;

  CStdString navigationMode = "regular";
  if(g_application.m_pPlayer && g_application.m_pPlayer->MouseRenderingEnabled() && g_application.m_pPlayer->ForceMouseRendering())
  {
    navigationMode = "Trackpad";
  }
  status["navigation"]["mode"] = navigationMode;

  bool isPlaying = g_application.IsPlaying();
  if (isPlaying)
  {
    if (!g_application.IsPaused())
    {
      status["playback"]["status"] = "playing";
    }
    else
    {
      status["playback"]["status"] = "paused";
    }

    CStdString CurrentFilePath = g_application.CurrentFileItem().m_strPath;
    status["playback"]["stream"] = CurrentFilePath;
    CStdString CurrentFileLabel = g_application.CurrentFileItem().GetLabel();
    status["playback"]["name"] = CurrentFilePath;
    bool canSeek = g_application.m_pPlayer->CanSeek();
    status["playback"]["seekable"] = canSeek ? "true" : "false";

    char buff[10];

    int totalTime = g_application.m_pPlayer->GetTotalTime();
    sprintf(buff,"%d",totalTime);
    status["playback"]["length"] = buff;

    memset(buff,0, 10);
    int playbackPos = g_application.m_pPlayer->GetTime();
    sprintf(buff,"%d",playbackPos);
    status["playback"]["position"] = buff;

    memset(buff,0, 10);
    int volume = g_application.GetVolume();
    sprintf(buff,"%d",volume);
    status["playback"]["volume"] = buff;

    status["playback"]["can-play-next"] = g_application.CurrentFileItem().GetNextItem().get() ? "true" : "false";
    status["playback"]["can-play-previous"] = g_application.CurrentFileItem().GetPrevItem().get() ? "true" : "false";
  }
  else
  {
    status["playback"]["status"] = "no playback";
  }

  outputroot["status"] = status;

  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerPlayUrlRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayUrlRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("url"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerPlayUrlRequest - FAILED to find [url] in request (hsrv)");
    SetErrorValue(outputroot,"no url parameter in request");
    return retCode;
  }

  CStdString url = inputroot.get("url","").asString();
  CStdString excuteLine = "playmedia(";
  excuteLine += url;
  excuteLine += ")";

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayUrlRequest - going to call Execute with [%s] (hsrv)",excuteLine.c_str());
  g_application.getApplicationMessenger().ExecBuiltIn(excuteLine);
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerPlayRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayRequest - enter function (hsrv)");
  int retCode = 500;

  if (!g_application.m_pPlayer->IsPaused())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerPlayRequest - FAILED to find [url] in request (hsrv)");
    SetErrorValue(outputroot,"player already in play mode");
    return retCode;
  }

  CStdString excuteLine = "playercontrol(play)";
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayRequest - going to call Execute with [%s] (hsrv)",excuteLine.c_str());
  g_application.getApplicationMessenger().ExecBuiltIn(excuteLine);
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerPauseRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPauseRequest - enter function (hsrv)");
  int retCode = 500;

  if (g_application.m_pPlayer->IsPaused())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerPauseRequest - FAILED to find [url] in request (hsrv)");
    SetErrorValue(outputroot,"player already in pause mode");
    return retCode;
  }

  CStdString excuteLine = "playercontrol(play)";
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPauseRequest - going to call Execute with [%s] (hsrv)",excuteLine.c_str());
  g_application.getApplicationMessenger().ExecBuiltIn(excuteLine);
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerPlayNextRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayNextRequest - enter function (hsrv)");
  int retCode = 500;

  if (!g_application.CurrentFileItem().GetNextItem().get())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerPlayNextRequest - Item doesn't have a next item (hsrv)");
    SetErrorValue(outputroot,"player does not have a next item");
    return retCode;
  }

  //CBoxeeItemLauncher::Launch(*(g_application.CurrentFileItem().GetNextItem().get()));
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerPlayPreviousRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerPlayPreviousRequest - enter function (hsrv)");
  int retCode = 500;

  if (!g_application.CurrentFileItem().GetPrevItem().get())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerPlayPreviousRequest - Item doesn't have a previous item (hsrv)");
    SetErrorValue(outputroot,"player does not have a previous item");
    return retCode;
  }

  //CBoxeeItemLauncher::Launch(*(g_application.CurrentFileItem().GetPrevItem().get()));
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerSeekToPositionRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerSeekToPositionRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("position"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerSeekToPositionRequest - FAILED to find [position] in request (hsrv)");
    SetErrorValue(outputroot,"no position parameter in request");
    return retCode;
  }

  int pos = inputroot.get("position","0").asInt();
  g_application.SeekTime(pos);

  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandlePlayerSetVolumeRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerSetVolumeRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("volume"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePlayerSetVolumeRequest - FAILED to find [volume] in request (hsrv)");
    SetErrorValue(outputroot,"no volume parameter in request");
    return retCode;
  }

  int volume = inputroot.get("volume","0").asInt();
  CStdString excuteLine = "setvolume(";
  excuteLine += volume;
  excuteLine += ")";
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePlayerSetVolumeRequest - going to call Execute with [%s] (hsrv)",excuteLine.c_str());
  g_application.getApplicationMessenger().ExecBuiltIn(excuteLine);
  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandleNavJumpToSectionRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleNavJumpToSectionRequest - enter function (hsrv)");
  int retCode = 500;
  return retCode;
}

int CBoxeeApiHttpServer::HandleTextfileldUpdateRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleTextfileldUpdateRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("text"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleTextfileldUpdateRequest - FAILED to find [text] in request (hsrv)");
    SetErrorValue(outputroot,"no text parameter in request");
    return retCode;
  }

  CStdString text = inputroot.get("text","").asString();

  CGUIDialogKeyboard* pKeyboard = GetKeyboard();
  if (!pKeyboard)
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleTextfileldUpdateRequest - FAILED to get keyboard (hsrv)");
    SetErrorValue(outputroot,"failed to get keyboard");
    return retCode;
  }
  else if (!pKeyboard->IsActive())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleTextfileldUpdateRequest - keyboard is not active (hsrv)");
    SetErrorValue(outputroot,"keyboard is not active");
    return retCode;
  }

  CGUIMessage message(GUI_MSG_LABEL2_SET, WINDOW_DIALOG_KEYBOARD, CTL_LABEL_EDIT);
  message.SetLabel(text);
  g_windowManager.SendThreadMessage(message);

  retCode = 200;
  outputroot["success"] = "true";
  return retCode;
}

int CBoxeeApiHttpServer::HandleNavRemoteKeyRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleNavRemoteKeyRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("key"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleNavRemoteKeyRequest - FAILED to find [key] in request (hsrv)");
    SetErrorValue(outputroot,"no key parameter in request");
    return retCode;
  }

  int buttonCode = 0;

  CStdString keyStr = inputroot.get("key","").asString();
  keyStr.ToLower();

  if (keyStr == "up")
  {
    buttonCode = KEY_BUTTON_DPAD_UP;
  }
  else if (keyStr == "down")
  {
    buttonCode = KEY_BUTTON_DPAD_DOWN;
  }
  else if (keyStr == "left")
  {
    buttonCode = KEY_BUTTON_DPAD_LEFT;
  }
  else if (keyStr == "right")
  {
    buttonCode = KEY_BUTTON_DPAD_RIGHT;
  }
  else if (keyStr == "select")
  {
    buttonCode = KEY_BUTTON_A;
  }
  else if (keyStr == "back")
  {
    buttonCode = KEY_BUTTON_BACK;
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleNavRemoteKeyRequest - FAILED to handle [key=%s] in request (hsrv)",keyStr.c_str());
    SetErrorValue(outputroot,"failed to handle key parameter in request");
    return retCode;
  }

  ThreadMessage tMsg = {TMSG_SEND_KEY,buttonCode};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandleNavTrackpadModeRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleNavTrackpadModeRequest - enter function (hsrv)");
  int retCode = 500;

  if (!inputroot.isMember("delta_x"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleNavTrackpadModeRequest - FAILED to handle [delta_x] in request (hsrv)");
    SetErrorValue(outputroot,"failed to handle delta_x parameter");
    return retCode;
  }

  if (!inputroot.isMember("delta_y"))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleNavTrackpadModeRequest - FAILED to handle [delta_y] in request (hsrv)");
    SetErrorValue(outputroot,"failed to handle delta_y parameter");
    return retCode;
  }

  Value delta_x = inputroot.get("delta_x","");
  Value delta_y = inputroot.get("delta_y","");

  int iDelta_x;
  int iDelta_y;

  if (delta_x.isInt() && delta_y.isInt())
  {
    iDelta_x = delta_x.asInt();
    iDelta_y = delta_y.asInt();
  }
  else if (delta_x.isString() && delta_y.isString())
  {
    iDelta_x = BOXEE::BXUtils::StringToInt(delta_x.asString());
    iDelta_y = BOXEE::BXUtils::StringToInt(delta_y.asString());
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandleNavTrackpadModeRequest - FAILED to handle [delta_x] and [delta_y] in request (hsrv)");
    SetErrorValue(outputroot,"failed to handle delta_x and delta_y parameter");
    return retCode;
  }

  ThreadMessage tMsg = {TMSG_SEND_MOVE,iDelta_x,iDelta_y};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

  retCode = 200;
  outputroot["success"] = "true";

  return retCode;
}

int CBoxeeApiHttpServer::HandleNavTrackpadClickRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleNavTrackpadClickRequest - enter function (hsrv)");
  int retCode = 500;
  return retCode;
}

int CBoxeeApiHttpServer::HandleNavTrackpadScrollRequest(const Json::Value& inputroot, Json::Value& outputroot)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleNavTrackpadScrollRequest - enter function (hsrv)");
  int retCode = 500;
  return retCode;
}

void CBoxeeApiHttpServer::SetErrorValue(Json::Value& jValue, const char* errorMsg)
{
  jValue["success"] = "false";
  jValue["error_msg"] = errorMsg;
}

CGUIDialogKeyboard* CBoxeeApiHttpServer::GetKeyboard()
{
  CGUIDialogKeyboard* pKeyboard;

  pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SEARCH);

  if (pKeyboard && pKeyboard->IsActive())
  {
    return pKeyboard;
  }

  pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (pKeyboard)
  {
    return pKeyboard;
  }

  return NULL;
}

SDL_cond* CBoxeeApiHttpServer::GetStatusCond()
{
  return m_statusCond;
}
