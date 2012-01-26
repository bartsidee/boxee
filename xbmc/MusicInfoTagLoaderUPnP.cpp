/*
 * Copyright (c) 2010 BOXEE
 */

#include "MusicInfoTagLoaderUPnP.h"
#include "MusicInfoTag.h"
#include "Settings.h"
#include "UPnPDirectory.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderUPnP::CMusicInfoTagLoaderUPnP(void)
{
}

CMusicInfoTagLoaderUPnP::~CMusicInfoTagLoaderUPnP()
{
}

bool CMusicInfoTagLoaderUPnP::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  DIRECTORY::CUPnPDirectory dir;
  CFileItemList list;
  if( dir.GetDirectory( strFileName, list ) )
  {
    CMusicInfoTag* t = list[0]->GetMusicInfoTag();
    tag = *t;
    return (t != NULL);
  }
  
  return false;
}
