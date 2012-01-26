#ifndef APPREPOSITORY_H_
#define APPREPOSITORY_H_

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

#include "AppDescriptor.h"

class CAppRepository
{
public:
  CAppRepository();
  CAppRepository(const CAppRepository& app);
  CAppRepository(const CStdString& url);
  CAppRepository(const CStdString& id, const CStdString& url, const CStdString& name, const CStdString& description, const CStdString& thumb);
  virtual ~CAppRepository();
  
  const CStdString& GetURL() const;
  const CStdString& GetName() const;
  const CStdString& GetDescription() const;
  const CStdString& GetID() const;
  const CStdString& GetThumbnail() const;
  bool IsValid();
  bool ReloadAppDescriptors(bool bDontWait = false , const CStdString& category="");
  CAppDescriptor::AppDescriptorsMap GetAvailableApps(const CStdString& category="");
  
  static CAppRepository LoadRepositoryFromXML(const TiXmlNode* rootElement);
  
private:
  bool LoadRepositoryFromURL(const CStdString& url);
  
  bool GetBoxeeApplications(CAppDescriptor::AppDescriptorsMap& result , const CStdString& category="");
  bool Get3rdPartyApplications(CAppDescriptor::AppDescriptorsMap& result, bool bDontWait);
  bool GetApps(CAppDescriptor::AppDescriptorsMap& result, TiXmlElement* rootElement, const CStdString& repositoryType);

  CStdString m_url;
  CStdString m_name;
  CStdString m_description;
  CStdString m_id;
  CStdString m_thumbnail;
  bool m_descriptorsLoaded;
  CAppDescriptor::AppDescriptorsMap m_cachedAvailableApps;
  unsigned int m_cachedAvailableAppsTimestamp;
  bool m_valid;
};

#endif /* APPREPOSITORY_H_ */
