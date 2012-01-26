#ifndef APPREPOSITORIES_H_
#define APPREPOSITORIES_H_

/*
 *      Copyright (C) 2005-2009 Team Boxee
 *      http://www.boxee.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include "AppRepository.h"
#include "FileItem.h"
#include "utils/Thread.h"

class CAppRepositories
{
public:
  CAppRepositories();
  virtual ~CAppRepositories();
  
  bool Load(bool force = false);
  bool Save();
  
  bool Add(CAppRepository& repository);
  bool Delete(const CStdString& id, bool reportToServer = true);
  std::vector<CAppRepository> Get();
  int Size();
  bool  GetDescriptorById(const CStdString id, CAppDescriptor& descriptor);
  CAppDescriptor::AppDescriptorsMap GetAvailableApps(bool force = false, bool boxeeAppsOnly = false);
  void GetAvailableApps(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, CFileItemList& availableAppsFileItems, const CStdString& mediaType = "");
  void ReloadAppDescriptors();
  
  bool UpdateServerOfRepositoriesFile();

private:

  int RemoveRepositoryInstalledApps(CAppRepository& repo);

  bool m_loaded;
  void Init();
  void LoadRepository(const TiXmlNode* node);
  CCriticalSection m_lock;
  std::vector<CAppRepository> m_repositories;  
  static CAppDescriptor::AppDescriptorsMap m_lastAvailableApps;
};

#endif /* APPREPOSITORIES_H_ */
