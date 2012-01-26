
#include "SDL/SDL.h"
#ifdef _WIN32
#include "win32/PlatformDefs.h"
#else
#include "linux/PlatformDefs.h"
#endif
#include "Application.h"
#include "RepositoriesDirectory.h"
#include "Util.h"
#include "utils/log.h"
#include "Settings.h"
#include "URL.h"
#include "FileItem.h"
#include "VirtualDirectory.h"
#include "Directory.h"
#include "../lib/libBoxee/boxee.h"
#include "AppManager.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

CRepositoriesDirectory::CRepositoriesDirectory()
{

}

CRepositoriesDirectory::~CRepositoriesDirectory()
{

}

bool CRepositoriesDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CRepositoriesDirectory::GetDirectory - Enter function with [strPath=%s] (bapps)",strPath.c_str());

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.CompareNoCase("repositories") != 0)
  {
    CLog::Log(LOGERROR,"CRepositoriesDirectory::GetDirectory - The protocol of [strPath=%s] isn't [repositories] (bapps)",strPath.c_str());
    return false;
  }

  CStdString strType = url.GetHostName();

  // get repositories list
  GetRepositoriesList(items,strType);

  CLog::Log(LOGDEBUG,"CRepositoriesDirectory::GetDirectory - Exit function with [NumOfRepositories=%d] for [strPath=%s] (bapps)",items.Size(),strPath.c_str());

  return true;
}

void CRepositoriesDirectory::GetRepositoriesList(CFileItemList &items, const CStdString& type)
{
  CLog::Log(LOGDEBUG,"CRepositoriesDirectory::GetRepositoriesList - Enter function with [type=%s] (bapps)",type.c_str());

  CAppManager::GetInstance().GetRepositories().Load();
  const std::vector<CAppRepository> repositories = CAppManager::GetInstance().GetRepositories().Get();

  for (size_t i = 0; i < repositories.size(); i++)
  {
    const CAppRepository& repository = repositories[i];

    if ((type == "external") && (repository.GetID() == "tv.boxee"))
    {
      // don't return the boxee repository
      continue;
    }

    // Create new share FileItem
    CFileItemPtr share ( new CFileItem(repository.GetName()) );
    share->m_strPath = repository.GetURL();//repository.GetName();
    share->SetThumbnailImage(repository.GetThumbnail());
    share->SetProperty("description", repository.GetDescription());
    share->SetProperty("id", repository.GetID());
    share->SetProperty("repo-url", repository.GetURL());
    share->m_bIsFolder = true;

    items.Add(share);
  }
}

