#ifndef AFP_DIRECTORY_H
#define AFP_DIRECTORY_H

#include "system.h"

#ifdef HAS_AFP
#include "IDirectory.h"

class CAFP
{
public:
  CAFP(){m_initialized = false;};
  virtual ~CAFP(){};
  void Init();
  void Deinit();
private:
  bool m_initialized;
};

struct afp_url;

namespace DIRECTORY
{
class CAfpDirectory : public IDirectory
{
public:
  CAfpDirectory(void);
  virtual ~CAfpDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

  static void EjectUser(const CStdString& strPath);
  static bool GetResource(const CURI& path, CFileItem &item);

private:
  bool GetVolumes(const CURI& url, CFileItemList &items);
  static CStdString HandlePath(const CStdString& strPath, bool dontUseGuestCredentials = false);

};
}
#endif

#endif
