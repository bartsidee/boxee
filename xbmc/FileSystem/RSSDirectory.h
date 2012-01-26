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

#ifndef CRSSDIRECTORY_H_
#define CRSSDIRECTORY_H_


#include "IDirectory.h"

#include "lib/libBoxee/boxee.h"

#include "RssSourceManager.h"
#include "CriticalSection.h"

namespace DIRECTORY
{
  class CRSSDirectory : public IDirectory, public BOXEE::BoxeeListener
  {
  public:
    CRSSDirectory();
    virtual ~CRSSDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool IsAllowed(const CStdString &strFile) const { return true; };
    virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
    virtual void OnFileDownloaded(const std::string &strLocalFile, 
         const std::string &strUrl, 
         const std::string &strTransactionId,
         void *pArg);

    virtual void OnFileDownloadFailed(const std::string &strLocalFile, 
                                  const std::string &strUrl, 
                                  const std::string &strTransactionId,
                                  void *pArg);
    virtual void OnForcedStop();

  private:
    void SignalOp();

    CStdString m_strUrl;
    CRssFeed m_feed;
    SDL_cond*  m_pOpFinishedCond;
    SDL_mutex* m_pOpFinishedMutex;
  };
}

#endif /*CRSSDIRECTORY_H_*/
