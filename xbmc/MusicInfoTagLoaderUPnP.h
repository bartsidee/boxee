/*
 * Copyright (c) 2010 BOXEE
 */

#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderUPnP: public IMusicInfoTagLoader
{
  public:
    CMusicInfoTagLoaderUPnP(void);
    virtual ~CMusicInfoTagLoaderUPnP();

    virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};

}
