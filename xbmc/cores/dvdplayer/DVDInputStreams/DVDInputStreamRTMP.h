#pragma once

#include "DVDInputStream.h"
#include "lib/libRTMP/rtmp.h"

class CDVDInputStreamRTMP : public CDVDInputStream
{
public:
  CDVDInputStreamRTMP();
  virtual ~CDVDInputStreamRTMP();
  virtual bool    Open(const char* strFile, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  bool            SeekTime(int iTimeInMsec);
  virtual bool Pause(double dTime);
  virtual bool    IsEOF();
  virtual __int64 GetLength();

  virtual bool    NextStream();

  CCriticalSection m_RTMPSection;

protected:
  bool   m_eof;
  bool   m_bPaused;
  RTMP_LIB::CRTMP  *m_rtmp;
  int          m_prevTagSize;
  bool         m_bSentHeader;
  char         *m_leftOver;
  char*        m_sStreamPlaying;
  unsigned int m_leftOverSize;
  unsigned int m_leftOverConsumed;
};

