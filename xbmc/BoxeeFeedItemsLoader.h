#pragma once

#include "BackgroundInfoLoader.h"

class CBoxeeFeedItemsLoader : public CBackgroundInfoLoader
{
public:
  CBoxeeFeedItemsLoader();
  virtual ~CBoxeeFeedItemsLoader();
  virtual bool LoadItem(CFileItem* pItem, bool bCanBlock);
  
  bool HandleAlbum(CFileItem* pItem, bool bCanBlock);

};

