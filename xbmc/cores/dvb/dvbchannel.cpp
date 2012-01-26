#include "system.h"

#ifdef HAS_DVB

#include "Util.h"
#include "dvbchannel.h"

#include <json/value.h>

struct ChannelFreqTable
{
  int channel;
  float minMhz;
  float maxMhz;
};

static ChannelFreqTable QAM_CHANNELS[] =
{
    { 2,   55.2500, 59.7500 },
    { 3,   61.2500,   65.7500 },
    { 4,   67.2500,   71.7500 },
    { 5,   77.2500,   81.7500 },
    { 6,   83.2500,   87.7500 },
    { 14,  121.2625,  125.7625 },
    { 15,  127.2625,  131.7625 },
    { 16,  133.2625,  137.7625 },
    { 17,  139.2500,  143.7500 },
    { 18,  145.2500,  149.7500 },
    { 19,  151.2500,  155.7500 },
    { 20,  157.2500,  161.7500 },
    { 21,  163.2500,  167.7500 },
    { 22,  169.2500,  173.7500 },
    { 23,  217.2500,  221.7500 },
    { 24,  223.2500,  227.7500 },
    { 25,  229.2625,  233.7625 },
    { 26,  235.2625,  239.7625 },
    { 27,  241.2625,  245.7625 },
    { 28,  247.2625,  251.7625 },
    { 29,  253.2625,  257.7625 },
    { 30,  259.2625,  263.7625 },
    { 31,  265.2625,  269.7625 },
    { 32,  271.2625,  275.7625 },
    { 33,  277.2625,  281.7625 },
    { 34,  283.2625,  287.7625 },
    { 35,  289.2625,  293.7625 },
    { 36,  295.2625,  299.7625 },
    { 37,  301.2625,  305.7625 },
    { 38,  307.2625,  311.7625 },
    { 39,  313.2625,  317.7625 },
    { 40,  319.2625,  323.7625 },
    { 41,  325.2625,  329.7625 },
    { 42,  331.2750,  335.7750 },
    { 43,  337.2625,  341.7625 },
    { 44,  343.2625,  347.7625 },
    { 45,  349.2625,  353.7625 },
    { 46,  355.2625,  359.7625 },
    { 47,  361.2625,  365.7625 },
    { 48,  367.2625,  371.7625 },
    { 49,  373.2625,  377.7625 },
    { 50,  379.2625,  383.7625 },
    { 51,  385.2625,  389.7625 },
    { 52,  391.2625,  395.7625 },
    { 53,  397.2625,  401.7625 },
    { 54,  403.2500,  407.7500 },
    { 55,  409.2500,  413.7500 },
    { 56,  415.2500,  419.7500 },
    { 57,  421.2500,  425.7500 },
    { 58,  427.2500,  431.7500 },
    { 59,  433.2500,  437.7500 },
    { 60,  439.2500,  443.7500 },
    { 61,  445.2500,  449.7500 },
    { 62,  451.2500,  455.7500 },
    { 63,  457.2500,  461.7500 },
    { 64,  463.2500,  467.7500 },
    { 65,  469.2500,  473.7500 },
    { 66,  475.2500,  479.7500 },
    { 67,  481.2500,  485.7500 },
    { 68,  487.2500,  491.7500 },
    { 69,  493.2500,  497.7500 },
    { 70,  499.2500,  503.7500 },
    { 71,  505.2500,  509.7500 },
    { 72,  511.2500,  515.7500 },
    { 73,  517.2500,  521.7500 },
    { 74,  523.2500,  527.7500 },
    { 75,  529.2500,  533.7500 },
    { 76,  535.2500,  539.7500 },
    { 77,  541.2500,  545.7500 },
    { 78,  547.2500,  551.7500 },
    { 79,  553.2500,  557.7500 },
    { 80,  559.2500,  563.7500 },
    { 81,  565.2500,  569.7500 },
    { 82,  571.2500,  575.7500 },
    { 83,  577.2500,  581.7500 },
    { 84,  583.2500,  587.7500 },
    { 85,  589.2500,  593.7500 },
    { 86,  595.2500, 599.7500 },
    { 87,  601.2500,  605.7500 },
    { 88,  607.2500,  611.7500 },
    { 89,  613.2500,  617.7500 },
    { 90,  619.2500,  623.7500 },
    { 91,  625.2500,  629.7500 },
    { 92,  631.2500,  635.7500 },
    { 93,  637.2500,  641.7500 },
    { 94,  643.2500,  647.7500 },
    { 95,  91.2500,   95.7500 },
    { 96,  97.2500,   101.7500 },
    { 97,  103.2500,  107.7500 },
    { 98,  109.2750,  113.7750 },
    { 99,  115.2750,  119.7750 },
    { 100,   649.2500,  653.7500 },
    { 101,   655.2500,  659.7500 },
    { 102,   661.2500,  665.7500 },
    { 103,   667.2500,  671.7500 },
    { 104,   673.2500,  677.7500 },
    { 105,   679.2500,  683.7500 },
    { 106,   685.2500,  689.7500 },
    { 107,   691.2500,  695.7500 },
    { 108,   697.2500,  701.7500 },
    { 109,   703.2500,  707.7500 },
    { 110,   709.2500,  713.7500 },
    { 111,   715.2500,  719.7500 },
    { 112,   721.2500,  725.7500 },
    { 113,   727.2500,  731.7500 },
    { 114,   733.2500,  737.7500 },
    { 115,   739.2500,  743.7500 },
    { 116,   745.2500,  749.7500 },
    { 117,   751.2500,  755.7500 },
    { 118,   757.2500,  761.7500 },
    { 119,   763.2500,  767.7500 },
    { 120,   769.2500,  773.7500 },
    { 121,   775.2500,  779.7500 },
    { 122,   781.2500,  785.7500 },
    { 123,   787.2500,  791.7500 },
    { 124,   793.2500,  797.7500 },
    { 125,   799.2500,  803.7500 },
    { 126,   805.2500,  809.7500 },
    { 127,   811.2500,  815.7500 },
    { 128,   817.2500,  821.7500 },
    { 129,   823.2500,  827.7500 },
    { 130,   829.2500,  833.7500 },
    { 131,   835.2500,  839.7500 },
    { 132,   841.2500,  845.7500 },
    { 133,   847.2500,  851.7500 },
    { 134,   853.2500,  857.7500 },
    { 135,   859.2500,  863.7500 },
    { 136,   865.2500,  869.7500 },
    { 137,   871.2500,  875.7500 },
    { 138,   877.2500,  881.7500 },
    { 139,   883.2500,  887.7500 },
    { 140,   889.2500,  893.7500 },
    { 141,   895.2500,  899.7500 },
    { 142,   901.2500,  905.7500 },
    { 143,   907.2500,  911.7500 },
    { 144,   913.2500,  917.7500 },
    { 145,   919.2500,  923.7500 },
    { 146,   925.2500,  929.7500 },
    { 147,   931.2500,  935.7500 },
    { 148,   937.2500,  941.7500 },
    { 149,   943.2500,  947.7500 },
    { 150,   949.2500,  953.7500 },
    { 151,   955.2500,  959.7500 },
    { 152,   961.2500,  965.7500 },
    { 153,   967.2500,  971.7500 },
    { 154,   973.2500,  977.7500 },
    { 155,   979.2500,  983.7500 },
    { 156,   985.2500,  989.7500 },
    { 157,   991.2500,  995.7500 },
    { 158,   997.2500,  1001.7500 },
    { 0, 0, 0}
};

static ChannelFreqTable VSB_CHANNELS[] =
{
    { 14, 470, 476 },
    { 15, 476, 482 },
    { 16, 482, 488 },
    { 17, 488, 494 },
    { 18, 494, 500 },
    { 19, 500, 506 },
    { 20, 506, 512 },
    { 21, 512, 518 },
    { 22, 518, 524 },
    { 23, 524, 530 },
    { 24, 530, 536 },
    { 25, 536, 542 },
    { 26, 542, 548 },
    { 27, 548, 554 },
    { 28, 554, 560 },
    { 29, 560, 566 },
    { 30, 566, 572 },
    { 31, 572, 578 },
    { 32, 578, 584 },
    { 33, 584, 590 },
    { 34, 590, 596 },
    { 35, 596, 602 },
    { 36, 602, 608 },
    { 37, 608, 614 },
    { 38, 614, 620 },
    { 39, 620, 626 },
    { 40, 626, 632 },
    { 41, 632, 638 },
    { 42, 638, 644 },
    { 43, 644, 650 },
    { 44, 650, 656 },
    { 45, 656, 662 },
    { 46, 662, 668 },
    { 47, 668, 674 },
    { 48, 674, 680 },
    { 49, 680, 686 },
    { 50, 686, 692 },
    { 51, 692, 698 },
    { 52, 698, 704 },
    { 53, 704, 710 },
    { 54, 710, 716 },
    { 55, 716, 722 },
    { 56, 722, 728 },
    { 57, 728, 734 },
    { 58, 734, 740 },
    { 59, 740, 746 },
    { 60, 746, 752 },
    { 61, 752, 758 },
    { 62, 758, 764 },
    { 63, 764, 770 },
    { 64, 770, 776 },
    { 65, 776, 782 },
    { 66, 782, 788 },
    { 67, 788, 794 },
    { 68, 794, 800 },
    { 69, 800, 806 },
    { 70, 806, 812 },
    { 71, 812, 818 },
    { 72, 818, 824 },
    { 73, 824, 830 },
    { 74, 830, 836 },
    { 75, 836, 842 },
    { 76, 842, 848 },
    { 77, 848, 854 },
    { 78, 854, 860 },
    { 79, 860, 866 },
    { 80, 866, 872 },
    { 81, 872, 878 },
    { 82, 878, 884 },
    { 0, 0, 0}
};

DvbChannel::DvbChannel()
{
  Clear();
}

DvbChannel::DvbChannel(int index)
{
  m_index = index;
  m_channelNumberCalculated = false;
}

DvbChannel::~DvbChannel()
{
}

CStdString DvbChannel::GetServiceName() const
{
  return m_serviceName;
}

CStdString DvbChannel::GetServiceProviderId() const
{
  return m_serviceProvider;
}

CStdString DvbChannel::GetFreq() const
{
  return m_freq;
}

void DvbChannel::Clear()
{
  m_serviceId = 0;
  m_originalNetworkId = 0;
  m_transportStreamId = 0;

  m_isEnabled = false;
  m_serviceName.clear();
  m_freq.clear();
  m_serverImage.clear();
  m_serverChannelName.clear();
  m_serverChannelNameShort.clear();
  m_serverId.clear();
}

int DvbChannel::GetVideoPid() const
{
  return m_videoPid;
}

int DvbChannel::GetServiceId() const
{
  return m_serviceId;
}

bool DvbChannel::IsEnabled() const
{
  return m_isEnabled;
}

void DvbChannel::SetEnabled(bool enabled)
{
  m_isEnabled = enabled;
}

CStdString DvbChannel::GetServerImage() const
{
  return m_serverImage;
}

CStdString DvbChannel::GetServerChannelName() const
{
  return m_serverChannelName;
}

CStdString DvbChannel::GetServerChannelNameShort() const
{
  return m_serverChannelNameShort;
}

CStdString DvbChannel::GetServerId() const
{
  return m_serverId;
}

bool DvbChannel::FromConfRecord(const CStdString &record)
{
  // parse a record from channels.conf and extract relevant info from it.
  // note that channels.conf is different between DVBT and ATSC but this should work on both
  Clear();
  CStdStringArray params;
  StringUtils::SplitString(record, ":", params);

  if (params.size() < 5)
    return false;

  if (params[0].Find(';') == -1)
  {
    m_serviceName = params[0];
    m_serviceProvider = "";
  }
  else
  {
    CStdStringArray params2;
    StringUtils::SplitString(params[0], ";", params2);
    m_serviceName = params2[0];
    m_serviceProvider = params2[1];
  }

  m_serverChannelName = m_serviceName;
  m_serverChannelNameShort = m_serviceName;

  m_freq = params[1];
  m_tuningParams = params[2];
  m_videoPid = atoi(params[3]);
  ParseAudioPids(params[4], false);
  ParseAudioPids(params[5], true);
  m_serviceId = atoi(params[6]);
  m_originalNetworkId = atoi(params[7]);
  m_transportStreamId = atoi(params[8]);
  m_virtualChannel = params[9];

  if (m_virtualChannel == "0.0")
    m_virtualChannel = "";

  //optional, only for development without server
  if (params.size() > 10)
    m_serverId = params[10];

  m_isEnabled = true;

  return true;
}

void DvbChannel::ParseAudioPids(const CStdString& confRecord, bool isAC3)
{
  if (confRecord.size() == 0)
    return;

  CStdStringArray params;
  StringUtils::SplitString(confRecord, ",", params);

  for (size_t i = 0; i < params.size(); i++)
  {
    CStdStringArray p;
    StringUtils::SplitString(params[i], "=", p);

    DvbAudioInfo info;
    info.isAC3 = isAC3;
    info.pid = atoi(p[0]);
    if (p.size() > 1)
    {
      bool valid = true;

      for (size_t i = 0; i < p[1].length(); i++)
      {
        if (p[1][i] < 32)
        {
          valid = false;
          break;
        }
      }

      if (valid)
        info.lang = p[1];
    }

    m_audioPids.push_back(info);
  }
}

CStdString DvbChannel::GetVirtualChannel() const
{
  return m_virtualChannel;
}

int DvbChannel::GetOriginalNetworkId() const
{
  return m_originalNetworkId;
}

int DvbChannel::GetTransportStreamId() const
{
  return m_transportStreamId;
}

CStdString DvbChannel::GetDvbTriplet() const
{
  // From GraceNote docs:
  // A DVB triplet has the format dvb://aaa.bbb.ccc and consists of these parts:
  // <ONID>: the Original network ID (the first number). <TSID>: the Transport stream ID (the second number).
  // <SID>: the Service ID (the third number).
  CStdString result;
  if (m_transportStreamId == 0 || m_originalNetworkId == 0)
    return result;

  result.Format("%d.%d.%d", m_originalNetworkId, m_transportStreamId, m_serviceId);
  return result;
}

int DvbChannel::GetIndex() const
{
  return m_index;
}

void DvbChannel::SetIndex(int index)
{
  m_index = index;
}

CStdString DvbChannel::GetTuningParams() const
{
  return m_tuningParams;
}

const std::vector<DvbAudioInfo>& DvbChannel::GetAudioPids() const
{
  return m_audioPids;
}

CStdString DvbChannel::GetChannelLabel() const
{
  if (!m_manualChannelName.IsEmpty())
    return m_manualChannelName;

  if (!m_serverChannelNameShort.IsEmpty())
    return m_serverChannelNameShort;

  if (!m_serviceName.IsEmpty())
    return m_serviceName;

  return "";
}

void DvbChannel::CalculateChannelNumber()
{
  if (m_tuningParams == "QAM_256")
  {
    float freq = ((float) atoi(m_freq)) / 1000000;
    int i = 0;
    while (QAM_CHANNELS[i].channel != 0)
    {
      if (freq >= QAM_CHANNELS[i].minMhz && freq <= QAM_CHANNELS[i].maxMhz)
      {
        m_calculatedChannelNumber.Format("%d.%d", QAM_CHANNELS[i].channel, m_serviceId % 1000);
//        printf("********** QAM: %s => %s\n", m_freq.c_str(), m_calculatedChannelNumber.c_str());
        break;
      }
      ++i;
    }
  }

  else if (m_tuningParams == "8VSB")
  {
    float freq = ((float) atoi(m_freq)) / 1000000;
    int i = 0;
    while (VSB_CHANNELS[i].channel != 0)
    {
      if (freq >= VSB_CHANNELS[i].minMhz && freq <= VSB_CHANNELS[i].maxMhz)
      {
        m_calculatedChannelNumber.Format("%d.%d", VSB_CHANNELS[i].channel, m_serviceId % 1000);
//        printf("********** VSB: %s => %s\n", m_freq.c_str(), m_calculatedChannelNumber.c_str());
        break;
      }
      ++i;
    }
  }

  m_channelNumberCalculated = true;
}

CStdString DvbChannel::GetChannelNumber()
{
  if (!m_manualChannelNumber.IsEmpty())
    return m_manualChannelNumber;

  if (!m_virtualChannel.IsEmpty())
    return m_virtualChannel;

  if (!m_channelNumberCalculated)
  {
    CalculateChannelNumber();
  }

  return m_calculatedChannelNumber;
}

CStdString DvbChannel::GetChannelId() const
{
  if (!m_serverId.IsEmpty())
    return m_serverId;

  if (!m_virtualChannel.IsEmpty())
    return m_virtualChannel;

  if (!m_serviceName.IsEmpty())
    return m_serviceName;

  return "";
}

bool DvbChannel::FromJson(const Json::Value& rootValue)
{
  if (rootValue["frequency"].isNull() ||
      rootValue["tune_flags"].isNull() ||
      !rootValue["video"].isObject() ||
      (rootValue["video"].isObject() && !rootValue["video"]["pid"].isInt()) ||
      !rootValue["audio"].isArray())
  {
    return false;
  }

  m_serviceId = 0;
  if (rootValue["program_id"].isInt())
    m_serviceId = rootValue["program_id"].asInt();

  if (m_serviceId == 0 && rootValue["service_id"].isInt())
    m_serviceId = rootValue["service_id"].asInt();

  if (m_serviceId == 0)
    return false;

  m_serviceName = rootValue["service_name"].asString();
  m_manualChannelName = rootValue["manual_name"].asString();
  m_manualChannelNumber = rootValue["manual_channel_id"].asString();

  m_serviceProvider = rootValue["provider_name"].asString();
  m_freq = rootValue["frequency"].asString();
  m_virtualChannel = rootValue["virtual_channel_id"].asString();

  if (atof(m_virtualChannel.c_str()) > 99.0 || m_virtualChannel == "0.0")
    m_virtualChannel = "";

  m_tuningParams = rootValue["tune_flags"].asString();

  if (!rootValue["ts_id"].isNull())
    m_transportStreamId = rootValue["ts_id"].asInt();
  else
    m_transportStreamId = 0;

  if (!rootValue["network_id"].isNull())
    m_originalNetworkId = rootValue["network_id"].asInt();
  else
    m_originalNetworkId = 0;

  m_videoPid = rootValue["video"]["pid"].asInt();

  Json::Value audio = rootValue["audio"];
  for (Json::ArrayIndex i = 0; i < audio.size(); i++)
  {
    Json::Value audioItem = audio[i];
    if (!audioItem["pid"].isInt())
      return false;

    DvbAudioInfo audioInfo;
    audioInfo.pid = audioItem["pid"].asInt();
    if (audioItem["is_ac3"].isBool())
      audioInfo.isAC3 = audioItem["is_ac3"].asBool();
    if (!audioItem["lang"].isNull())
      audioInfo.lang = audioItem["lang"].asString();

    m_audioPids.push_back(audioInfo);
  }

  m_isEnabled = !rootValue["is_filtered"].asBool();

  if (rootValue["channel"].isObject())
  {
    Json::Value channel = rootValue["channel"];
    m_serverId = channel["id"].asString();
    m_serverChannelName = channel["name"].asString();
    m_serverChannelNameShort = channel["name_short"].asString();
  }

  return true;
}

Json::Value DvbChannel::ToJson()
{
  Json::Value response;

  response["program_id"] = m_serviceId;
  response["service_id"] = m_serviceId;
  response["service_name"] = m_serviceName;
  if (!m_manualChannelName.IsEmpty())
    response["manual_name"] = m_manualChannelName;
  if (!m_manualChannelNumber.IsEmpty())
    response["manual_channel_id"] = m_manualChannelNumber;
  response["provider_name"] = m_serviceProvider;
  response["frequency"] = m_freq;
  response["virtual_channel_id"] = m_virtualChannel;
  response["tune_flags"] = m_tuningParams;
  if (m_transportStreamId)
    response["ts_id"] = m_transportStreamId;
  if (m_originalNetworkId)
    response["network_id"] = m_originalNetworkId;

  Json::Value video;
  video["pid"] = m_videoPid;
  response["video"] = video;

  Json::Value audio;
  for (size_t i = 0; i < m_audioPids.size(); i++)
  {
    Json::Value audioItem;
    audioItem["pid"] = m_audioPids[i].pid;
    audioItem["lang"] = m_audioPids[i].lang;
    audioItem["is_ac3"] = m_audioPids[i].isAC3;
    audio.append(audioItem);
  }
  response["audio"] = audio;

  response["is_filtered"] = !m_isEnabled;

  return response;
}

void DvbChannel::SetChannelManualName(const CStdString& name)
{
  m_manualChannelName = name;
}

void DvbChannel::SetChannelManualNumber(const CStdString& number)
{
  m_manualChannelNumber = number;
}

#endif
