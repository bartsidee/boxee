
#include "UpdateSourceFile.h"
#include "BoxeeMediaSourceList.h"
#include "Settings.h"
#include "Util.h"
#include "Profile.h"
#include "Application.h"
#include "SystemInfo.h"
#include "GUISettings.h"
#include "SpecialProtocol.h"

using namespace BOXEE;

CUpdateSourceFile::CUpdateSourceFile()
{
  
}

CUpdateSourceFile::~CUpdateSourceFile()
{
  
}

//////////////////////
// Public functions //
//////////////////////

bool CUpdateSourceFile::UpdateProfilesSourceFile()
{
  // if we're on a Mac or if XBMC_PLATFORM_MODE is set, enable platform
  // specific directories.
  g_application.EnablePlatformDirectories();

  // if XBMC_DEFAULT_MODE is set, disable platform specific directories
  if (getenv("XBMC_DEFAULT_MODE"))
  {
    g_application.EnablePlatformDirectories(false);
  }
  
  g_guiSettings.Initialize();  // Initialize default Settings
  g_settings.Initialize();     //Initialize default AdvancedSettings
  g_application.InitDirectories();
    
  // Check if profiles was loaded. If no -> Try to load them.
  if (g_settings.m_vecProfiles.size() == 0)
  {
    // Load the profiles on this machine
    CStdString profilesFilePath = PROFILES_FILE;
    g_settings.LoadProfiles(profilesFilePath);
  }
  
  // If no profiles were loaded, there is no update to do
  if((g_settings.m_vecProfiles).size() == 0)
  {
    printf("CUSF::UpdateProfilesSourceFile - WARNING - There are no profiles on this machine\n");
    return false;
  }

  DeleteDuplicateApps();
  
  // Load the sources.xml.in.diff file
  TiXmlDocument xmlDoc;
  TiXmlElement* pRootElement = NULL;
#ifdef __APPLE__
  CStdString diffFilePath;
  if(CSysInfo::IsAppleTV())
  {
    diffFilePath = "special://xbmc/userdata/sources.xml.in.diff.appletv";
  }
  else
  {
    diffFilePath = "special://xbmc/userdata/sources.xml.in.diff.osx";
  }
#elif defined(HAS_EMBEDDED)
  CStdString diffFilePath = "special://xbmc/userdata/sources.xml.in.diff.embedded";
#elif defined(_LINUX)
  CStdString diffFilePath = "special://xbmc/userdata/sources.xml.in.diff.linux";
#elif defined(_WIN32)
  CStdString diffFilePath = "special://xbmc/userdata/sources.xml.in.diff.win";
#endif
  
  diffFilePath = _P(diffFilePath);
  
  if (!xmlDoc.LoadFile(diffFilePath))
  {
    printf("CUSF::UpdateProfilesSourceFile - ERROR - Failed to read diffFile [%s]\n",diffFilePath.c_str());    
    return false;
  }
  
  pRootElement = xmlDoc.RootElement();
  if (pRootElement == NULL)
  {
    printf("CUSF::UpdateProfilesSourceFile - ERROR - Failed to get root element from file [%s]\n",diffFilePath.c_str());    
    return false;
  }

  CStdString strValue = pRootElement->Value();
  if (strValue != "sources")
  {
    printf("CUSF::UpdateProfilesSourceFile - ERROR - The root tag in file [%s] is NOT <sources>\n",diffFilePath.c_str());    
    return false;
  }

  // First, delete all the obsolete shares
  const TiXmlNode* pTag = pRootElement->FirstChildElement("delete");
  if (pTag)
  {
    printf("CUSF::UpdateProfilesSourceFile - DEBUG - Found <delete> tag, deleting sources.\n");
    UpdateSourceFiles(pTag,UPDATE_DELETE);
  }

  // Add/update the new/existing ones
  pTag = pRootElement->FirstChildElement("add");
  if (pTag)
  {
    printf("CUSF::UpdateProfilesSourceFile - DEBUG - Found <add> tag, adding sources.\n");
    UpdateSourceFiles(pTag,UPDATE_ADD);
  }
  
  return true;
}

///////////////////////
// Private functions //
///////////////////////

void CUpdateSourceFile::DeleteDuplicateApps()
{
  size_t numOfProfiles = g_settings.m_vecProfiles.size();
  
  // Run over each profile and make the necessary update
  for (size_t i = 0; i < numOfProfiles; i++)
  {
    CProfile profile = g_settings.m_vecProfiles[i];
    g_settings.m_iLastLoadedProfileIndex = i;
    CStdString profileName = profile.getName();
    
    // No need to update the "Master User" profile
    if (profileName == "Master user")
    {
      continue;      
    }

	printf("CUSF::UpdateSourceFiles - DEBUG - [%lu] Handling profile [%s] for duplicates\n",(unsigned long)i+1,profileName.c_str());
    
    // Update g_settings with the profile sources.xml
    CStdString profilePath = _P("special://home/profiles/");
    profilePath += profile.getID();
    CSpecialProtocol::SetProfilePath(profilePath);
    
    CStdString profileSourceFilePath = g_settings.GetSourcesFile();

    g_settings.m_videoSources.clear();
    g_settings.m_musicSources.clear();
    g_settings.m_pictureSources.clear();
    
    bool retVal = ReadSourcesFromFile(profileSourceFilePath, g_settings.m_videoSources, g_settings.m_musicSources, g_settings.m_pictureSources);
    if (retVal == false)
    {
	  printf("CUSF::UpdateSourceFiles - ERROR - [%lu] Failed to read sources from [%s] of profile is [%s] -> Continue\n",(unsigned long)i+1,profileSourceFilePath.c_str(),profileName.c_str());
      continue;
    }
    
    // First delete all the duplicate sources
    CBoxeeMediaSourceList sourceList(g_settings.m_videoSources, g_settings.m_musicSources, g_settings.m_pictureSources);
    sourceList.deleteDuplicateApps();    
  }
}

void CUpdateSourceFile::UpdateSourceFiles(const TiXmlNode* pAddTag, UPDATE_ACTION_TYPE updateActionType)
{
  size_t numOfProfiles = g_settings.m_vecProfiles.size();
  
  // Run over each profile and make the necessary update
  for (size_t i = 0; i < numOfProfiles; i++)
  {
    CProfile profile = g_settings.m_vecProfiles[i];
    g_settings.m_iLastLoadedProfileIndex = i;
    CStdString profileName = profile.getName();
    
    // No need to update the "Master User" profile
    if (profileName == "Master user")
    {
      continue;      
    }

	printf("CUSF::UpdateSourceFiles - DEBUG - [%lu] Handling profile [%s]\n",(unsigned long)i+1,profileName.c_str());

    // Update g_settings with the profile sources.xml
    CStdString profilePath = _P("special://home/profiles/");
    profilePath += profile.getID();
    CSpecialProtocol::SetProfilePath(profilePath);
    
    CStdString profileSourceFilePath = g_settings.GetSourcesFile();

    g_settings.m_videoSources.clear();
    g_settings.m_musicSources.clear();
    g_settings.m_pictureSources.clear();
    
    bool retVal = ReadSourcesFromFile(profileSourceFilePath, g_settings.m_videoSources, g_settings.m_musicSources, g_settings.m_pictureSources);
    
    if (retVal == false)
    {
	  printf("CUSF::UpdateSourceFiles - ERROR - [%lu] Failed to read sources from [%s] of profile is [%s] -> Continue\n",(unsigned long)i+1,profileSourceFilePath.c_str(),profileName.c_str());
      continue;
    }

    // Run over the <add> tag and add the sources
    const TiXmlNode* pTag = 0;
    while ((pTag = pAddTag->IterateChildren(pTag)))
    {
      if (pTag->ValueStr() == "video" || pTag->ValueStr() == "music" || pTag->ValueStr() == "pictures")
      {
        UpdateSourceFile(pTag, pTag->ValueStr(), updateActionType);
      }
    }

    g_settings.SaveSources();
  }  
}

void CUpdateSourceFile::UpdateSourceFile(const TiXmlNode* pMediaTypeTag, const CStdString& mediaType, UPDATE_ACTION_TYPE updateActionType)
{
  bool retVal = false;
  
  const TiXmlNode* pSourceTag = 0;
  while ((pSourceTag = pMediaTypeTag->IterateChildren(pSourceTag)))
  {
    if (pSourceTag->ValueStr() == "source")
    {
      CMediaSource source;
      
      InitMediaSourceFromNode(pSourceTag, source, mediaType);
      bool nameExistsInSource = IsNameExistInSource(source.strName, mediaType);

      switch(updateActionType)
      {
        case UPDATE_ADD:
          if (!nameExistsInSource)
          {
            printf("    CUSF::UpdateSourceFile - DEBUG - Adding share: %s / %s\n", mediaType.c_str(), source.strName.c_str());
            retVal = g_settings.AddShare(mediaType,source);
          }
          else
          {         
            printf("    CUSF::UpdateSourceFile - DEBUG - Updating share: %s / %s\n", mediaType.c_str(), source.strName.c_str());
            retVal = g_settings.UpdateShare(mediaType,source.strName,source);
          }
          break;
        
        case UPDATE_DELETE:
          if (nameExistsInSource)
          { 
            retVal = g_settings.DeleteSource(mediaType, source.strName, source.strPath);
            printf("    CUSF::UpdateSourceFile - DEBUG - Deleting share: %s / %s. Result: %s\n", mediaType.c_str(), source.strName.c_str(), retVal ? "found" : "not found");
          }
          break;
        
        default:
          printf("    CUSF::UpdateSourceInFile - WARNING - For [name=%s][path=%s] wrong [updateType=%s] was requested", source.strName.c_str(), 
              source.strPath.c_str(), mediaType.c_str());
          break;
      }
    }
  }
}

void CUpdateSourceFile::InitMediaSourceFromNode(const TiXmlNode* pSourceTag,CMediaSource& source, const CStdString& mediaType)
{
  CBoxeeMediaSource boxeeMS;
  
  const TiXmlNode* pTag = 0;
  while ((pTag = pSourceTag->IterateChildren(pTag)))
  {
    const TiXmlNode *pValue = pTag->FirstChild();
    
    if(pTag->ValueStr() == "name")
    {
      boxeeMS.name = pValue->ValueStr();
    }
    else if(pTag->ValueStr() == "path")
    {
      boxeeMS.path = pValue->ValueStr();          
    }
    else if(pTag->ValueStr() == "thumbnail")
    {
      boxeeMS.thumbPath = pValue->ValueStr();         
    }
    else if(pTag->ValueStr() == "private")
    {
      boxeeMS.isPrivate = stricmp(pValue->ValueStr().c_str(), "true") == 0 ? true : false;
    }
    else if(pTag->ValueStr() == "adult")
    {
      boxeeMS.isAdult = stricmp(pValue->ValueStr().c_str(), "true") == 0 ? true : false;
    }        
    else if(pTag->ValueStr() == "country")
    {
      boxeeMS.country = pValue->ValueStr(); 
    }
    else if(pTag->ValueStr() == "country-allow")
    {
      boxeeMS.countryAllow = stricmp(pValue->ValueStr().c_str(), "true") == 0 ? true : false;
    }           
  }
  
  std::vector<CStdString> paths;
  paths.push_back(boxeeMS.path);
  source.m_iScanType = boxeeMS.scanType;
  source.m_strThumbnailImage = boxeeMS.thumbPath;
  source.FromNameAndPaths(mediaType, boxeeMS.name, paths);
  source.m_adult = boxeeMS.isAdult;
  source.m_country = boxeeMS.country;
  source.m_countryAllow = boxeeMS.countryAllow;
}

bool CUpdateSourceFile::IsNameExistInSource(const CStdString &strName, const CStdString &mediaType)
{
  VECSOURCES* sources = NULL;
  if (mediaType == "video") sources = &g_settings.m_videoSources;
  if (mediaType == "music") sources = &g_settings.m_musicSources;
  if (mediaType == "pictures") sources = &g_settings.m_pictureSources;
  
  if (!sources)
  {
    return false;
  }  
  
  for (size_t i = 0; i < sources->size(); i++)
  {
    if (strName.CompareNoCase((*sources)[i].strName) == 0)
    {
      return true;
    }
  }   
  
  return false;
}

bool CUpdateSourceFile::ReadSourcesFromFile(const CStdString& profilesFilePath,VECSOURCES& videoSources,VECSOURCES& musicSources,VECSOURCES& pictureSources)
{  
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  
  if (!xmlDoc.LoadFile(profilesFilePath.c_str()))
  {
    printf("CUSF::ReadSourcesFromFile - ERROR - Failed to parse [%s] file\n",profilesFilePath.c_str());
    return false;    
  }
  
  pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    printf("CUSF::ReadSourcesFromFile - ERROR - Failed to get root element from file [%s]\n",profilesFilePath.c_str());     
    return false;      
  }    
  
  CStdString strValue = pRootElement->Value();  
  if (strValue != "sources")
  {
    printf("CUSF::ReadSourcesFromFile - ERROR - In file [%s] the root element is not <sources>\n",profilesFilePath.c_str());     
    return false;
  }
      
  CStdString tmpDefaultSource;
  
  // Get sources...
  g_settings.GetSources(pRootElement, "pictures", pictureSources, tmpDefaultSource);
  g_settings.GetSources(pRootElement, "music", musicSources, tmpDefaultSource);
  g_settings.GetSources(pRootElement, "video", videoSources, tmpDefaultSource);
      
  return true;
}

bool CUpdateSourceFile::UpgradeSettings()
{
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  CStdString guiSettingsPath = _P("special://home/guisettings.xml");
  bool modified = false;
  
  if (!xmlDoc.LoadFile(guiSettingsPath.c_str()))
  {
    printf("CUSF::UpgradeSettings - ERROR - Failed to parse [%s] file\n", guiSettingsPath.c_str());
    return false;    
  }
  
  pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    printf("CUSF::UpgradeSettings - ERROR - Failed to get root element from file [%s]\n", guiSettingsPath.c_str());     
    return false;      
  }
  
  // Transform projectM.vis -> ProjectM.vis
  if (pRootElement->FirstChildElement("mymusic") && pRootElement->FirstChildElement("mymusic")->FirstChildElement("visualisation"))
  {
    if (strcmp(pRootElement->FirstChildElement("mymusic")->FirstChildElement("visualisation")->GetText(), "projectM.vis") == 0)
    {
      pRootElement->FirstChildElement("mymusic")->FirstChildElement("visualisation")->FirstChild()->ToText()->SetValue("ProjectM.vis");
      modified |= true;
    }
  }

  // Transform Boxee Skin NG -> boxee
  if (pRootElement->FirstChildElement("lookandfeel") && pRootElement->FirstChildElement("lookandfeel")->FirstChildElement("skin"))
  {
    if (strcmp(pRootElement->FirstChildElement("lookandfeel")->FirstChildElement("skin")->GetText(), "Boxee Skin NG") == 0)
    {
      pRootElement->FirstChildElement("lookandfeel")->FirstChildElement("skin")->FirstChild()->ToText()->SetValue("boxee");
      modified |= true;
    }
  }

  // default vsync in windows should be VSYNC_VIDEO = 2 (i.e activate vsync only in video playback)
#ifdef _WIN32
  if (pRootElement->FirstChildElement("videoscreen") && pRootElement->FirstChildElement("videoscreen")->FirstChildElement("vsync"))
  {
      pRootElement->FirstChildElement("videoscreen")->FirstChildElement("vsync")->FirstChild()->ToText()->SetValue("1");
      modified |= true;
  }
#endif
  
  // Replace all Q:, T:, P:
  modified |= FixOldStyleDir(pRootElement->FirstChildElement("screensaver")->FirstChildElement("slideshowpath"));
  modified |= FixOldStyleDir(pRootElement->FirstChildElement("subtitles")->FirstChildElement("custompath"));
  modified |= FixOldStyleDir(pRootElement->FirstChildElement("system")->FirstChildElement("playlistspath"));
  
  if (modified)
  {
    xmlDoc.SaveFile(guiSettingsPath);
  }
  
  return true;
}

bool CUpdateSourceFile::FixOldStyleDir(TiXmlElement* element)
{
  CStdString current = element->FirstChild()->Value();

  if (current.length() > 2 && current[1] == ':')
  {
    printf("Replacing %s ", current.c_str());
    
    CStdString replacement = "";
    
    if (current[0] == 'q' || current[0] == 'Q') replacement = "special://xbmc";
    else if (current[0] == 't' || current[0] == 'T') replacement = "special://home";
    else if (current[0] == 'p' || current[0] == 'P') replacement = "special://home";
    else if (current[0] == 'z' || current[0] == 'Z') replacement = "special://temp";
    
    if (replacement.length() == 0)
    {
      return false;
    }
    
    current.Replace("\\", "/");
    replacement += current.substr(2);
    
    element->FirstChild()->ToText()->SetValue(replacement);

    printf("with %s\n", replacement.c_str());
    
    return true;
  }
  else
  {
    return false;
  }
}
