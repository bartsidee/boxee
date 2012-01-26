/*
 * bxwebfavoritesmanager.h
 *
 *  Created on: Feb 10, 2011
 *      Author: shayyizhak
 */

#ifndef BXWEBFAVORITESMANAGER_H_
#define BXWEBFAVORITESMANAGER_H_

#include "bxscheduletaskmanager.h"
#include "bxboxeewebfavorites.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXWebFavoritesItem : public BXItem
{
public:
  BXWebFavoritesItem(const std::string& type, const std::string& id, const std::string& name, const std::string src);
  BXWebFavoritesItem(const std::string& id, const std::string& name, const std::string src);
  virtual ~BXWebFavoritesItem(){};

  std::string GetSrc();

private:

  std::string m_src;
};

class BXWebFavoritesManager
{
public:
  BXWebFavoritesManager();
  virtual ~BXWebFavoritesManager();

  bool Initialize();

  bool UpdateWebFavoritesList(unsigned long executionDelayInMS, bool repeat);
  bool GetWebFavorites(BXBoxeeWebFavorites& webFavoritesList);
  bool GetWebFavorites(std::vector<BXObject>& webFavoritesVec);

  bool IsInWebFavorites(const std::string& src);

private:

  void LockWebFavoritesList();
  void UnLockWebFavoritesList();
  void CopyWebFavoritesList(const BXBoxeeWebFavorites& webFavoritesList);
  void SetWebFavoritesListIsLoaded(bool isLoaded);

  SDL_mutex* m_webFavoritesListGuard;
  BXBoxeeWebFavorites m_webFavoritesList;

  class RequestWebFavoritesListFromServerTask : public BoxeeScheduleTask
  {
  public:

    RequestWebFavoritesListFromServerTask(BXWebFavoritesManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestWebFavoritesListFromServerTask();
    virtual void DoWork();

  private:

    BXWebFavoritesManager* m_taskHandler;
  };
};

}


#endif /* BXWEBFAVORITESMANAGER_H_ */
