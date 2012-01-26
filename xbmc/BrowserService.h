#ifndef BROWSER_SERVICE_H
#define BROWSER_SERVICE_H

#include "FileItem.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "lib/libBoxee/bxscheduletaskmanager.h"
#include "utils/Thread.h"

// From IPScanJob.h
class CSMBScanState;

// Should find a better place for this in a refactor at some point
class CUPnPScanState
{
public:
  CUPnPScanState();
  void GetContents( CFileItemList& list ) const;
  void SetContents( CFileItemList& list );
private:
  CCriticalSection m_lock;
  CFileItemList m_upnpShares;
};
  

/**
 * Code for the browser service
 * Browser service provides an entry point for enumerating network resources of varying types
 **/
class CBrowserService : public BOXEE::BoxeeScheduleTask,
                        public CThread  
{
public:  
  enum SHARE_TYPE { NO_SHARE = -1, SMB_SHARE, UPNP_SHARE, BONJOUR_SHARE, LAST_SHARE };

  CBrowserService();
  virtual ~CBrowserService();
  
  bool Init();
  void DeInit();

  bool GetShare( SHARE_TYPE type, CFileItemList& list ) const;
  bool IsHostAvailable(const CStdString &strHost);
  
protected:
  virtual void DoWork();
  virtual void Process();

private:  
  void GenerateUpnpShares();
  void GenerateSmbShares();
  void RefreshShares();
  
  virtual bool ShouldDelete() { return false; }

private:  
  CCriticalSection   m_scanContext;
  BOXEE::BXBGProcess m_scanProcessor;
  
  CUPnPScanState*    m_pUPnPScanState;
  CSMBScanState*     m_pSMBScanState;
};

#endif
