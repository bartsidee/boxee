
 /*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "TextureManager.h"
#include "Texture.h"
#include "SDL_anigif.h"
#include "PackedTexture.h"
#include "GraphicContext.h"
#include "utils/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/log.h"
#ifdef _DEBUG
#include "utils/TimeUtils.h"
#endif
#include "../xbmc/Util.h"
#include "../xbmc/FileSystem/File.h"
#include "../xbmc/FileSystem/Directory.h"
#include "TimeUtils.h"
#include <assert.h>
#include "tinyXML/tinyxml.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"
#include "../xbmc/FileSystem/File.h"

using namespace std;

CGUITextureManager g_TextureManager;

class LoadFromFileJob : public BOXEE::BXBGJob
{
public:
  LoadFromFileJob(const CStdString &fileName, const CStdString &textureName, const int dstWidth, const int dstHeight) : BXBGJob("LoadFromFileJob")
  { 
    m_fileName    = fileName; 
    m_textureName = textureName; 
    m_dstWidth = dstWidth;
    m_dstHeight = dstHeight;
  }

  virtual void DoWork()
  {
    CBaseTexture *pTexture = new CTexture();    
    if (pTexture->LoadFromFile(m_fileName, 0, 0, false, NULL, NULL, m_dstWidth, m_dstHeight))
      g_TextureManager.TextureLoaded(m_fileName, m_textureName, pTexture);
    else
    {
      g_TextureManager.ReleaseTexture(m_fileName);
      delete pTexture;
  }
  }
private:
  CStdString m_fileName;
  CStdString m_textureName;
  int m_dstWidth;
  int m_dstHeight;
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
CTextureArray::CTextureArray(int width, int height, int loops,  bool texCoordsArePixels)
{
  m_width = width;
  m_height = height;
  m_initialWidth = width;
  m_initialHeight = height;
  m_loops = loops;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
  m_bPlaceHolder = false;
}

CTextureArray::CTextureArray()
{
  Reset();
}

CTextureArray::~CTextureArray()
{

}

unsigned int CTextureArray::size() const
{
  return m_textures.size();
}


void CTextureArray::Reset()
{
  m_textures.clear();
  m_delays.clear();
  m_width = 0;
  m_height = 0;
  m_loops = 0;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
  m_bPlaceHolder = false;
}

void CTextureArray::Add(CBaseTexture *texture, int delay)
{
  if (!texture)
    return;

  m_textures.push_back(texture);
  m_delays.push_back(delay ? delay * 2 : 100);

  m_texWidth = texture->GetTextureWidth();
  m_texHeight = texture->GetTextureHeight();
  m_texCoordsArePixels = false;
}

void CTextureArray::Set(CBaseTexture *texture, int width, int height)
{
  assert(!m_textures.size()); // don't try and set a texture if we already have one!
  m_width = width;
  m_height = height;
  m_orientation = texture ? texture->GetOrientation() : 0;
  Add(texture, 100);
}

void CTextureArray::Free()
{
  for (unsigned int i = 0; i < m_textures.size(); i++)
  {
    delete m_textures[i];
  }

  m_textures.clear();
  m_delays.clear();

  Reset();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

CTextureMap::CTextureMap()
{
  m_textureName = "";
  m_referenceCount = 0;
  m_memUsage = 0;
  m_bPlaceHolder = false;
}

CTextureMap::CTextureMap(const CStdString& textureName, int width, int height, int loops)
: m_texture(width, height, loops)
{
  m_textureName = textureName;
  m_referenceCount = 0;
  m_memUsage = 0;
  m_bPlaceHolder = false;
}

CTextureMap::~CTextureMap()
{
  FreeTexture();
}

void CTextureMap::SetName(const CStdString& textureName)
{
  m_textureName = textureName;
}

bool CTextureMap::IsPlaceHolder() const
{
  return m_bPlaceHolder;
}

void CTextureMap::SetPlaceHolder(bool bPlaceHolder)
{
  m_bPlaceHolder = bPlaceHolder;
  m_texture.m_bPlaceHolder = bPlaceHolder;
}

bool CTextureMap::Release()
{
  if (!m_texture.m_textures.size()) 
    return true;
  if (!m_referenceCount)
    return true;

  m_referenceCount--;
  if (!m_referenceCount)
  {
    return true;
  }
  return false;
}

const CStdString& CTextureMap::GetName() const
{
  return m_textureName;
}

const CTextureArray& CTextureMap::GetTexture()
{
  m_referenceCount++;
  return m_texture;
}

void CTextureMap::Dump() const
{
  if (!m_referenceCount)
    return;   // nothing to see here

  CStdString strLog;
  strLog.Format("  texture:%s has %i frames %i refcount\n", m_textureName.c_str(), m_texture.m_textures.size(), m_referenceCount);
  OutputDebugString(strLog.c_str());
}

unsigned int CTextureMap::GetMemoryUsage() const
{
  return m_memUsage;
}

void CTextureMap::Flush()
{
  if (!m_referenceCount)
    FreeTexture();
}

void CTextureMap::ResetTexture()
{
  m_texture.Reset();
}

void CTextureMap::FreeTexture()
{
  if (!m_bPlaceHolder)
  m_texture.Free();
}

bool CTextureMap::IsEmpty() const
{
  return m_texture.m_textures.size() == 0;
}

void CTextureMap::Add(CBaseTexture* texture, int delay)
{
  m_texture.Add(texture, delay);

  if (texture)
    m_memUsage += sizeof(CTexture) + (texture->GetTextureWidth() * texture->GetTextureHeight() * 4); 
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
CGUITextureManager::CGUITextureManager(void)
{
  m_pLoadingTexture = NULL;
  
  // we set the theme bundle to be the first bundle (thus prioritizing it)
  m_TexBundle[0].SetThemeBundle(true);
  m_bgProcessor.Start(2);
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
  m_bgProcessor.Stop();
}

const CTextureArray& CGUITextureManager::GetTexture(const CStdString& strTextureName)
{
  CSingleLock lock(m_lock);

  static CTextureArray emptyTexture;
  //  CLog::Log(LOGINFO, " refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      //CLog::Log(LOGDEBUG, "Total memusage %u", GetMemoryUsage());
      return pMap->GetTexture();
    }
  }
  return emptyTexture;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
bool CGUITextureManager::CanLoad(const CStdString &texturePath) const
{
  if (texturePath == "-")
    return false;

  if (!CURI::IsFullPath(texturePath))
    return true;  // assume we have it

  // we can't (or shouldn't) be loading from remote paths, so check these
  return CUtil::IsHD(texturePath);
}

bool CGUITextureManager::HasTexture(const CStdString &textureName, CStdString *path, int *bundle, int *size)
{
  // default values
  if (bundle) *bundle = -1;
  if (size) *size = 0;
  if (path) *path = textureName;

  if (!CanLoad(textureName))
    return false;

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(m_lock);

  // Check our loaded and bundled textures - we store in bundles using \\.

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == textureName)
    {
      if (size) *size = 1;
      return true;
    }
  }

  CStdString bundledName = CTextureBundle::Normalize(textureName);
  for (int i = 0; i < 2; i++)
  {
    if (m_TexBundle[i].HasFile(bundledName))
    {
      if (bundle) *bundle = i;
      return true;
    }
  }

  CStdString fullPath = GetTexturePath(textureName);
  if (path)
    *path = fullPath;

  return !fullPath.IsEmpty();
}

CTextureMap *CGUITextureManager::LoadGif(const CStdString& strTextureName, const CStdString& strPath, int bundle)
{
    CTextureMap* pMap;

    if (bundle >= 0)
    {
      CBaseTexture **pTextures;
      int nLoops = 0, width = 0, height = 0, initialHeight = 0, initialWidth = 0;
      int* Delay;
      int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &pTextures, width, initialWidth, height, initialHeight, nLoops, &Delay);
      if (!nImages)
      {
        CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
      return NULL;
      }

      pMap = new CTextureMap(strTextureName, width, height, nLoops);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        pMap->Add(pTextures[iImage], Delay[iImage]);
      }

      delete [] pTextures;
      delete [] Delay;
    }
    else
    {
      int gnAG = AG_LoadGIF(_P(strPath).c_str(), NULL, 0);
      if (gnAG == 0)
      {
        if (!strnicmp(strPath.c_str(), "special://home/skin/", 20) && !strnicmp(strPath.c_str(), "special://xbmc/skin/", 20))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return NULL;
      }

      AG_Frame* gpAG = new AG_Frame[gnAG];
      AG_LoadGIF(_P(strPath.c_str()), gpAG, gnAG);
      AG_NormalizeSurfacesToDisplayFormat(gpAG, gnAG);

      pMap = new CTextureMap(strTextureName, gpAG[0].surface->w, gpAG[0].surface->h, 0);

      for (int iImage = 0; iImage < gnAG; iImage++)
      {
        CTexture *glTexture = new CTexture();
        if (glTexture)
        {
          glTexture->LoadFromMemory(gpAG[iImage].surface->w, gpAG[iImage].surface->w,
              gpAG[iImage].surface->h, gpAG[iImage].surface->h,
              gpAG[iImage].surface->pitch, XB_FMT_B8G8R8A8, (unsigned char *)gpAG[iImage].surface->pixels);
          pMap->Add(glTexture, gpAG[iImage].delay);
        }
      }

      AG_FreeSurfaces(gpAG, gnAG);
    }

  return pMap;
}

int CGUITextureManager::Load(const CStdString& strTextureName, bool checkBundleOnly /*= false */, const CStdString& strLoadingTexture)
{
  CStdString strPath;
  int bundle = -1;
  int size = 0;
  if (!HasTexture(strTextureName, &strPath, &bundle, &size))
    return 0;

  if (size) // we found the texture
    return size;

  if (checkBundleOnly && bundle == -1)
    return 0;

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(m_lock);

  if (strPath.IsEmpty())
  {
    CLog::Log(LOGWARNING,"TextureManager - unable to load <%s>", strTextureName.c_str());
    return 0;
  }

#ifdef _DEBUG
  int64_t start;
  start = CurrentHostCounter();
#endif

  if (strPath.Right(4).ToLower() == ".gif")
  {
    CTextureMap* pMap = LoadGif(strTextureName, strPath, bundle);
    if (!pMap)
      return 0;
    
#ifdef _DEBUG
    int64_t end, freq;
    end = CurrentHostCounter();
    freq = CurrentHostFrequency();
    char temp[200];
    sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end - start) / freq, (bundle >= 0) ? " (bundled)" : "");
    OutputDebugString(temp);
#endif

    m_vecTextures.push_back(pMap);
    return 1;
  } // of if (strPath.Right(4).ToLower()==".gif")

  CBaseTexture *pTexture = NULL;
  int width = 0, height = 0, initialWidth = 0, initialHeight = 0;
  if (bundle >= 0)
  {
    if (FAILED(m_TexBundle[bundle].LoadTexture(strTextureName, &pTexture, width, initialWidth, height, initialHeight)))
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
      return 0;
    }
  }
  else
  {
    // normal picture
    // convert from utf8
    CStdString texturePath;
    g_charsetConverter.utf8ToStringCharset(strPath, texturePath);

    //
    // skin items should be loaded immediately
    //
    if (!CURI::IsFullPath(strTextureName) || strTextureName.Find("special://xbmc/skin") == 0 || strTextureName.Find("special://xbmc/media") == 0)
    {
    pTexture = new CTexture();
      if(!pTexture->LoadFromFile(texturePath))
      {
        ReleaseTexture(texturePath);
        delete pTexture;
        return 0;
      }
    width = pTexture->GetWidth();
    height = pTexture->GetHeight();

      CTextureMap* pMap = new CTextureMap(strTextureName, width, height, 0);
      pMap->Add(pTexture, 100);
      m_vecTextures.push_back(pMap);
      
      return 1;
    }

    //
    // thumb loading - happens in bg
    //
    static const CStdString gLoadingIcon = "loading_placeholder.png"; // has to be pinned!

    CTextureMap* pMap = NULL;
    const CTextureArray &texture = GetTexture(strLoadingTexture.IsEmpty()?gLoadingIcon:strLoadingTexture);
    pMap = new CTextureMap(strTextureName, texture.m_width, texture.m_height, 0);
    if (!pMap)
      return 0;

    for (size_t i=0;i<texture.size();i++)
      pMap->Add(texture.m_textures[i], texture.m_delays[i] / 2);
    
    pMap->SetPlaceHolder(true);
    m_vecTextures.push_back(pMap);

    LoadFromFileJob *aJob = new LoadFromFileJob(texturePath, strTextureName, texture.m_width, texture.m_height);
    if (!m_bgProcessor.QueueFront(aJob))
    {
      delete aJob;
      if (pMap)
        delete pMap;
      return 0;
    }
    
    return pMap->GetTexture().size();
  }

  CTextureMap* pMap = new CTextureMap(strTextureName, width, height, 0);
  pMap->Add(pTexture, 100);
  m_vecTextures.push_back(pMap);

#ifdef _DEBUG_TEXTURES
  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end - start) / freq, (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

  return 1;
}

void CGUITextureManager::TextureLoaded(const CStdString& strFileName, const CStdString& strTextureName, CBaseTexture *pTexture)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_vecTextures.size(); i++)
  {
    if (m_vecTextures[i]->GetName() == strTextureName)
    {
      delete m_vecTextures[i];

      CTextureMap* pMap = new CTextureMap(strTextureName, pTexture->GetWidth(), pTexture->GetHeight(), 0);
      pMap->Add(pTexture, 100);
      m_vecTextures[i] = pMap;
      break;
    }
  }
}

void CGUITextureManager::ReleaseTextureInternal(const CStdString& strTextureName)
{
  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    if (pMap->GetName() == strTextureName)
    {
      if (pMap->Release())
      {
        //CLog::Log(LOGINFO, "  cleanup:%s", strTextureName.c_str());
        // add to our textures to free
        m_unusedTextures.push_back(pMap);
        i = m_vecTextures.erase(i);
      }
      return;
    }
    ++i;
  }
  CLog::Log(LOGWARNING, "%s: Unable to release texture %s", __FUNCTION__, strTextureName.c_str());  
}

void CGUITextureManager::FreeUnusedTextures()
{
  CSingleLock lock(m_lock);
  for (ivecTextures i = m_unusedTextures.begin(); i != m_unusedTextures.end(); ++i)
    delete *i;
  m_unusedTextures.clear();
}

void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName)
{
  CSingleLock lock(m_lock);

  // Never delete pinned textures
  if (m_pinnedTextures.find(strTextureName) != m_pinnedTextures.end())
  {
    return;
  }

  ReleaseTextureInternal(strTextureName);
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(m_lock);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    CLog::Log(LOGDEBUG, "%s: Having to cleanup texture %s", __FUNCTION__, pMap->GetName().c_str());
    delete pMap;
    i = m_vecTextures.erase(i);
  }
  for (int i = 0; i < 2; i++)
    m_TexBundle[i].Cleanup();
  FreeUnusedTextures();
}

void CGUITextureManager::Dump() const
{
  CSingleLock lock(m_lock);

  CStdString strLog;
  strLog.Format("total texturemaps size:%i\n", m_vecTextures.size());
  OutputDebugString(strLog.c_str());

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    const CTextureMap* pMap = m_vecTextures[i];
    if (!pMap->IsEmpty())
      pMap->Dump();
  }
}

void CGUITextureManager::Flush()
{
  CSingleLock lock(m_lock);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    pMap->Flush();
    if (pMap->IsEmpty() )
    {
      delete pMap;
      i = m_vecTextures.erase(i);
    }
    else
    {
      ++i;
    }
  }
}

uint32_t CGUITextureManager::GetNumOfTextures() const
{
  CSingleLock lock(m_lock);
  return m_vecTextures.size();
}

uint32_t CGUITextureManager::GetMemoryUsage() const
{
  CSingleLock lock(m_lock);

  unsigned int memUsage = 0;
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    memUsage += m_vecTextures[i]->GetMemoryUsage();
  }
  return memUsage;
}

void CGUITextureManager::SetTexturePath(const CStdString &texturePath)
{
  CSingleLock lock(m_lock);

  m_texturePaths.clear();
  AddTexturePath(texturePath);
}

void CGUITextureManager::AddTexturePath(const CStdString &texturePath)
{
  CSingleLock lock(m_lock);

  if (!texturePath.IsEmpty())
    m_texturePaths.push_back(texturePath);
}

void CGUITextureManager::RemoveTexturePath(const CStdString &texturePath)
{
  CSingleLock lock(m_lock);

  for (vector<CStdString>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
  {
    if (*it == texturePath)
    {
      m_texturePaths.erase(it);
      return;
    }
  }
}

CStdString CGUITextureManager::GetTexturePath(const CStdString &textureName, bool directory /* = false */)
{
  CSingleLock lock(m_lock);

  if (CURI::IsFullPath(textureName))
  {
    if (XFILE::CFile::Exists(textureName))
    {
    return textureName;
    }
    return "";
  }
  else
  { // texture doesn't include the full path, so check all fallbacks
    for (vector<CStdString>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
    {
      CStdString path = CUtil::AddFileToFolder(it->c_str(), "media");
      path = CUtil::AddFileToFolder(path, textureName);
      if (directory)
      {
        if (DIRECTORY::CDirectory::Exists(path))
          return path;
      }
      else
      {
        if (XFILE::CFile::Exists(path))
          return path;
      }
    }
  }
  return "";
}

void CGUITextureManager::GetBundledTexturesFromPath(const CStdString& texturePath, std::vector<CStdString> &items)
{
  CSingleLock lock(m_lock);

  m_TexBundle[0].GetTexturesFromPath(texturePath, items);
  if (items.empty())
    m_TexBundle[1].GetTexturesFromPath(texturePath, items);
}

void CGUITextureManager::PinTexture(const CStdString& textureName)
{
  if (Load(textureName) != 0)
  {
    m_pinnedTextures.insert(textureName);
  }
}

void CGUITextureManager::LoadPinnedTextures()
{
  CStdString strFileName = CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media/textures.xml");
  strFileName = PTH_IC(strFileName);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strFileName))
  {
    CLog::Log(LOGINFO, "unable to load:%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return;
  }

  // Root element should be <textures>
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (!strValue.Equals("textures"))
  {
    CLog::Log(LOGERROR, "file :%s doesnt contain <textures>", strFileName.c_str());
    return;
  }

  TiXmlElement* pTexture = pRootElement->FirstChildElement("texture");
  while (pTexture)
  {
    const char* name = pTexture->Attribute("name");
    if (!name) continue;

    CStdString nameStr(name);
    const char* pinned = pTexture->Attribute("pinned");
    if (stricmp(pinned, "true") == 0) PinTexture(nameStr);
    pTexture = pTexture->NextSiblingElement("texture");
  }
}
