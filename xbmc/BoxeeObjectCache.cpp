//
// C++ Implementation: BoxeeObjectCache
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include "BoxeeObjectCache.h"
#include "FileSystem/File.h"

using namespace XFILE;
using namespace BOXEE;

BoxeeObjectCache::BoxeeObjectCache()
{
}


BoxeeObjectCache::~BoxeeObjectCache()
{
}

std::string BoxeeObjectCache::GenerateCacheName(const std::string &strType, const std::string &strId)
{
  CStdString strFile("special://xbmc/boxee/cache/");
  strFile += strType + "_" + strId + ".xml";

  return strFile;
}

BOXEE::BXObject BoxeeObjectCache::LoadFromCache(const std::string &strType, const std::string &strId) 
{
  std::string strFileName = GenerateCacheName(strType, strId);
  if (!CFile::Exists(strFileName))
    return BXObject(false);

  BXObject obj;
  obj.LoadFromFile(strFileName);

  return obj;
}

bool BoxeeObjectCache::StoreInCache(BOXEE::BXObject obj)
{
  return obj.DumpToFile(GenerateCacheName(obj.GetType(), obj.GetID()));
}

