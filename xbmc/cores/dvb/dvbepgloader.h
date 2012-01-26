#pragma once

#include "system.h"

#ifdef HAS_DVB

#include <list>
#include "StdString.h"
#include "Thread.h"

struct DvbEpgEvent
{
  DvbEpgEvent() { startTime = 0; }

  int id;
  int64_t startTime;
  int duration;
  CStdString name;
  CStdString text;
  CStdString extra;
  CStdString rating;
};

struct DvbEpgInfo
{
  DvbEpgInfo() { serviceId = 0; networkId = 0; tsId = 0; }

  int serviceId;
  int networkId;
  int tsId;
  DvbEpgEvent current;
  std::list<DvbEpgEvent> events;
};

class IDvbEpgListener
{
public:
  virtual ~IDvbEpgListener() {}

  virtual void OnEpgNewInfo(const DvbEpgInfo& info) = 0;
};

struct dvbpsi_decoder_s;
struct EitData;

class DvbEpgLoader : public CThread
{
public:
  DvbEpgLoader();
  ~DvbEpgLoader();

  bool ProcessPacket(uint8_t* data);

  void AddListener(IDvbEpgListener *listener);
  void RemoveListener(IDvbEpgListener *listener);

  void OnEpgNewInfo(const DvbEpgInfo& info);

  virtual void Process();

  void OnNewEitData(void *p_cb_data, void *p_eit, bool b_current_following);
private:
  pthread_mutex_t             m_listenersLock;
  std::list<IDvbEpgListener*> m_listeners;
  struct dvbpsi_decoder_s *               m_dvbpsi;

  pthread_mutex_t             m_condLock;
  pthread_cond_t              m_newEitData;
  CCriticalSection            m_lock;
  std::list<EitData*>         m_eitData;
};

#endif
