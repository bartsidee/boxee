#pragma once

#include "system.h"

#ifdef HAS_DVB

#include <list>
#include "StdString.h"

struct dvbpsi_decoder_s;

class DvbPatLoader
{
public:
  DvbPatLoader();
  ~DvbPatLoader() {}

  void Open();
  void Close();

  bool ProcessPacket(uint8_t* data);


  int m_fd;
private:
  struct dvbpsi_decoder_s *               m_dvbpsi;

};

#endif
