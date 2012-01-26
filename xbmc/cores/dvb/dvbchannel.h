#ifndef BXCHANNEL_H
#define BXCHANNEL_H

#include "StdString.h"
#include <boost/shared_ptr.hpp>
#include "json/value.h"
#include "FileItem.h"

struct DvbAudioInfo
{
  DvbAudioInfo() { isAC3 = false; }

  int pid;
  CStdString lang;
  bool isAC3;
};

class DvbChannel
{
public:
  DvbChannel();
  DvbChannel(int index);
  virtual ~DvbChannel();

  int GetIndex() const;
  void SetIndex(int index);

  /*
   * The following methods return the technical information
   * as retrieved by the dvb dongle by w_scan.
   */
  CStdString GetServiceName() const;
  CStdString GetServiceProviderId() const;
  CStdString GetFreq() const;
  CStdString GetTuningParams() const;
  int GetVideoPid() const;
  const std::vector<DvbAudioInfo>& GetAudioPids() const;
  int GetServiceId() const;
  int GetOriginalNetworkId() const;
  int GetTransportStreamId() const;
  CStdString GetDvbTriplet() const;
  CStdString GetVirtualChannel() const;

  /* Parse channels.conf */
  bool FromConfRecord(const CStdString &record);

  /* Additional information per channel returned by the server */
  CStdString GetServerImage() const;
  CStdString GetServerChannelName() const;
  CStdString GetServerChannelNameShort() const;
  CStdString GetServerId() const;

  /* The following is calculated either locally or from the server */
  CStdString GetChannelLabel() const;
  CStdString GetChannelNumber();
  CStdString GetChannelId() const;

  /* Manual editing of channel info */
  void SetChannelManualName(const CStdString& name);
  void SetChannelManualNumber(const CStdString& number);

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  bool FromJson(const Json::Value& rootValue);
  Json::Value ToJson();

protected:
  void Clear();
  void ParseAudioPids(const CStdString& confRecord, bool isAC3);
  void CalculateChannelNumber();

  /* local unique id per */
  int m_index;

  /* Data from w_scan */
  int m_serviceId;
  CStdString m_serviceName;
  CStdString m_serviceProvider;
  CStdString m_freq;
  CStdString m_tuningParams;
  int m_videoPid;
  std::vector<DvbAudioInfo> m_audioPids;
  int m_originalNetworkId;
  int m_transportStreamId;
  CStdString m_virtualChannel;
  CStdString m_calculatedChannelNumber;
  bool m_channelNumberCalculated;

  bool m_isEnabled;

  /* Data from server and user */
  CStdString m_serverId;
  CStdString m_serverImage;
  CStdString m_serverChannelName;
  CStdString m_serverChannelNameShort;

  CStdString m_manualChannelName;
  CStdString m_manualChannelNumber;

};

typedef boost::shared_ptr<DvbChannel> DvbChannelPtr;

#endif
