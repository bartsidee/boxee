#pragma once

#include "BackgroundInfoLoader.h"

class CBoxeeServerItemsLoader : public CBackgroundInfoLoader
{
public:
  CBoxeeServerItemsLoader();
  virtual ~CBoxeeServerItemsLoader();
  virtual bool LoadItem(CFileItem* pItem, bool bCanBlock);
  
};

