#ifndef _FILEAFP_H
#define _FILEAFP_H

#include "system.h"

#ifdef HAS_AFP

#include "IFile.h"

extern "C" {
#include <afp.h>
}

namespace XFILE
{

class CFileAfp : public IFile
{
public:
  CFileAfp();
  virtual ~CFileAfp();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURI& url);
  virtual bool Exists(const CURI& url);
  virtual int Stat(const CURI& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();

protected:
  bool m_bOpened;
  IFile *m_file;
};
}

#endif

#endif
