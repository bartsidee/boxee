/*
 * bxboxeewebfavorites.cpp
 *
 *  Created on: Feb 10, 2011
 *      Author: shayyizhak
 */



//
// C++ Implementation: BXBoxeeWebFavorites
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeewebfavorites.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeWebFavorites::BXBoxeeWebFavorites() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeWebFavorites::~BXBoxeeWebFavorites()
{

}

void BXBoxeeWebFavorites::Clear()
{
  m_webFavorites.clear();
  m_bLoaded = false;
}

bool BXBoxeeWebFavorites::Parse()
{
  TiXmlHandle docHandle(&m_doc);

  TiXmlElement *pChild = docHandle.FirstChild("bookmarks").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeWebFavorites::Parse - FAILED to find <bookmark> node");
    return false;
  }

  m_webFavorites.clear();

  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("bookmark") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_webFavorites.push_back(obj);
      }
    }
  }

  return true;
}

unsigned long BXBoxeeWebFavorites::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeWebFavorites::GetNumOfWebFavorites() const
{
  return m_webFavorites.size();
}

BXObject BXBoxeeWebFavorites::GetWebFavorites(int index) const
{
  if (index >= GetNumOfWebFavorites())
  {
    return BXObject();
  }

  return m_webFavorites[index];
}

bool BXBoxeeWebFavorites::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeWebFavorites::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

