#ifndef APP_DESCRIPTOR_H_
#define APP_DESCRIPTOR_H_

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

#include "MediaSource.h"
#include "tinyXML/tinyxml.h"

#include <set>

class CAppDescriptor
{
public:
  typedef std::map<CStdString, CAppDescriptor> AppDescriptorsMap;
  
  CAppDescriptor(void);
  CAppDescriptor(const CStdString& url);
  CAppDescriptor(TiXmlElement* rootElement);
  virtual ~CAppDescriptor();

  void Load(const CStdString& url);
  static bool Exists(const CStdString& url);
  bool IsLoaded() const;
  const CStdString& GetId() const;
  const CStdString& GetThumb() const;
  const CStdString& GetName() const;
  const CStdString& GetMediaType() const;
  const CStdString& GetDescription() const;
  const CStdString& GetTagsStr() const;
  const CStdString& GetVersion() const;
  const CStdString& GetType() const;
  const CStdString& GetURL() const;
  const CStdString& GetReleaseDate() const;
  const CStdString& GetRepository() const;
  const CStdString& GetRepositoryId() const;
  const CStdString& GetMinVersion() const;
  const CStdString& GetMaxVersion() const;
  const CStdString& GetPlatform() const;
  const CStdString& GetLocalPath() const;
  const CStdString& GetStartWindow() const;
  const CStdString  GetSkinPath(CStdString& xmlName) const;
  const CStdString& GetMediaPath() const;
  const CStdString& GetAuthor() const;
  const CStdString& GetLocalURL() const;
  const CStdString& GetBackgroundImageURL() const;
  const CStdString& GetController() const;
  const CStdString& GetPartnerId() const;
  const CStdString& GetGlobalHandler() const;
  const CStdString& GetSignature() const;
  bool IsAdult() const;
  bool IsTestApp() const;
  void GetCountryRestrictions(CStdString& countries, bool& allow) const;
  bool IsAllowed() const;
  bool IsCountryAllowed() const;  
  bool IsBoxeeApp() const;
  bool MatchesPlatform();
  bool GenerateSignature();
  bool IsPersistent() const;
  const std::vector<CStdString>& GetAdditionalSharedLibraries() const;

protected:
  bool LoadDescriptorFile(void);
  bool LoadDescriptorFile(TiXmlElement* rootElement);
  void SetAdditionalFields(void);
  bool m_isLoaded;
  CStdString m_thumb;
  CStdString m_name;
  CStdString m_description;
  CStdString m_tagsStr;
  CStdString m_version;
  CStdString m_type;
  CStdString m_repository;
  CStdString m_repositoryId;
  CStdString m_localPath;
  CStdString m_skinPath;
  CStdString m_mediaPath;
  CStdString m_localUrl;
  CStdString m_url;
  CStdString m_startWindow;
  CStdString m_mediaType;
  CStdString m_id;
  CStdString m_releaseDate;
  CStdString m_minVersion;
  CStdString m_maxVersion;
  CStdString m_platform;
  CStdString m_author;
  CStdString m_backgroundImageURL;
  CStdString m_countries;
  CStdString m_controller;
  bool       m_countriesAllow;
  bool       m_adult;
  bool       m_testApp;
  CStdString m_partnerId;
  CStdString m_signature;
  CStdString m_globalHandler;
  CStdString m_boxeeSig;
  bool       m_persistent;
  std::vector<CStdString> m_additionalSharedLibraries;
};

#endif /*APP_DESCRIPTOR_H_*/
