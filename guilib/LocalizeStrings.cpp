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

#include "system.h"
#include "LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "FileSystem/SpecialProtocol.h"
#include "XMLUtils.h"
#include "SingleLock.h"
#include "GraphicContext.h"

extern CStdString g_LoadErrorStr;

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

CLocalizeStrings &CLocalizeStrings::GetInstance()
{
  static CLocalizeStrings localizeStrings ;
  return localizeStrings;
}

CLocalizeStrings &CLocalizeStrings::GetInstanceTemp()
{
  static CLocalizeStrings localizeStringsTemp ;
  return localizeStringsTemp;
}

CStdString CLocalizeStrings::ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.IsEmpty())
    return str;

  CStdString ret;
  g_charsetConverter.stringCharsetToUtf8(strEncoding, str, ret);
  return ret;
}

void CLocalizeStrings::ClearSkinStrings()
{
  // clear the skin strings
  Clear(31000, 31999);
}

bool CLocalizeStrings::LoadSkinStrings(const CStdString& path, const CStdString& fallbackPath)
{
  CSingleLock lock(g_graphicsContext);
  ClearSkinStrings();
  // load the skin strings in.
  CStdString encoding, error;
  if (!LoadXML(path, encoding, error))
  {
    if (path == fallbackPath) // no fallback, nothing to do
      return false;
  }

  // load the fallback
  if (path != fallbackPath)
    LoadXML(fallbackPath, encoding, error);

  return true;
}

bool CLocalizeStrings::LoadXML(const CStdString &filename, CStdString &encoding, CStdString &error, uint32_t offset /* = 0 */)
{
  CSingleLock lock(g_graphicsContext);
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(PTH_IC(filename)))
  {
    CLog::Log(LOGDEBUG, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    error.Format("Unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  XMLUtils::GetEncoding(&xmlDoc, encoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || 
       pRootElement->ValueStr()!=CStdString("strings"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <strings>", filename.c_str());
    error.Format("%s\nDoesnt start with <strings>", filename.c_str());
    return false;
  }

  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    // Load new style language file with id as attribute
    const char* attrId=pChild->Attribute("id");
    if (attrId && !pChild->NoChildren())
    {
      int id = atoi(attrId) + offset;
      if (m_strings.find(id) == m_strings.end())
        m_strings[id] = ToUTF8(encoding, pChild->FirstChild()->Value());
    }
    pChild = pChild->NextSiblingElement("string");
  }
  return true;
}

bool CLocalizeStrings::Load(const CStdString& strFileName, const CStdString& strFallbackFileName)
{
  CSingleLock lock(g_graphicsContext);

  CStdString strFile = _P(strFileName);
  CStdString strFallback = _P(strFallbackFileName);
  bool bLoadFallback = !strFile.Equals(strFallback);

  CStdString encoding, error;
  Clear();

  if (!LoadXML(strFile, encoding, error))
  {
    // try loading the fallback
    if (!bLoadFallback || !LoadXML(strFallback, encoding, error))
    {
      g_LoadErrorStr = error;
      return false;
    }
    bLoadFallback = false;
  }

  if (bLoadFallback)
    LoadXML(strFallback, encoding, error);

  // fill in the constant strings
  m_strings[20022] = "";

  g_charsetConverter.wToUTF8(L"\x0B0F", m_strings[20027]);
  g_charsetConverter.wToUTF8(L"\x0B0K", m_strings[20028]);
  g_charsetConverter.wToUTF8(L"\x0B0C", m_strings[20029]);
  g_charsetConverter.wToUTF8(L"\x0B0R", m_strings[20030]);
  g_charsetConverter.wToUTF8(L"\x0B0Ra", m_strings[20031]);
  g_charsetConverter.wToUTF8(L"\x0B0R", m_strings[20032]);
  g_charsetConverter.wToUTF8(L"\x0B0De", m_strings[20033]);
  g_charsetConverter.wToUTF8(L"\x0B0N", m_strings[20034]);
  g_charsetConverter.wToUTF8(L"\x0B0", m_strings[20035]);

  m_strings[20200] = ToUTF8(encoding, "km/h");
  m_strings[20201] = ToUTF8(encoding, "m/min");
  m_strings[20202] = ToUTF8(encoding, "m/s");
  m_strings[20203] = ToUTF8(encoding, "ft/h");
  m_strings[20204] = ToUTF8(encoding, "ft/min");
  m_strings[20205] = ToUTF8(encoding, "ft/s");
  m_strings[20206] = ToUTF8(encoding, "mph");
  m_strings[20207] = ToUTF8(encoding, "kts");
  m_strings[20208] = ToUTF8(encoding, "Beaufort");
  m_strings[20209] = ToUTF8(encoding, "inch/s");
  m_strings[20210] = ToUTF8(encoding, "yard/s");
  m_strings[20211] = ToUTF8(encoding, "Furlong/Fortnight");

  return true;
}

static CStdString szEmptyString = "";

const CStdString& CLocalizeStrings::Get(uint32_t dwCode) const
{
  ciStrings i = m_strings.find(dwCode);
  if (i == m_strings.end())
  {
    return szEmptyString;
  }
  return i->second;
}

void CLocalizeStrings::Clear()
{
  CSingleLock lock(g_graphicsContext);
  m_strings.clear();
}

void CLocalizeStrings::Clear(uint32_t start, uint32_t end)
{
  CSingleLock lock(g_graphicsContext);

  iStrings it = m_strings.begin();
  while (it != m_strings.end())
  {
    if (it->first >= start && it->first <= end)
      m_strings.erase(it++);
    else
      ++it;
  }
}

uint32_t CLocalizeStrings::LoadBlock(const CStdString &id, const CStdString &path, const CStdString &fallbackPath)
{
  CSingleLock lock(g_graphicsContext);

  iBlocks it = m_blocks.find(id);
  if (it != m_blocks.end())
    return it->second;  // already loaded

  // grab a new block
  uint32_t offset = block_start + m_blocks.size()*block_size;
  m_blocks.insert(make_pair(id, offset));

  // load the strings
  CStdString encoding, error;
  bool success = LoadXML(path, encoding, error, offset);
  if (!success)
  {
    if (path == fallbackPath) // no fallback, nothing to do
      return 0;
  }

  // load the fallback
  if (path != fallbackPath)
    success |= LoadXML(fallbackPath, encoding, error, offset);

  return success ? offset : 0;
}

void CLocalizeStrings::ClearBlock(const CStdString &id)
{
  CSingleLock lock(g_graphicsContext);

  iBlocks it = m_blocks.find(id);
  if (it == m_blocks.end())
  {
    CLog::Log(LOGERROR, "%s: Trying to clear non existent block %s", __FUNCTION__, id.c_str());
    return; // doesn't exist
  }
  
  // clear our block
  Clear(it->second, it->second + block_size);
  m_blocks.erase(it);
}
