#ifndef BROWSER_SERVICE_H
#define BROWSER_SERVICE_H

#include "FileItem.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "lib/libBoxee/bxscheduletaskmanager.h"
#include "utils/Thread.h"

// From IPScanJob.h
class CSMBScanState;

class CCifsScanState
{
public:
  CCifsScanState();
  void GetContents( CFileItemList& list ) const;
  void SetContents( CFileItemList& list );
private:
  CCriticalSection m_lock;
  CFileItemList m_cifsShares;
};

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

class CNfsScanState
{
public:
  CNfsScanState();
  void GetContents( CFileItemList& list ) const;
  void SetContents( CFileItemList& list );
private:
  CCriticalSection m_lock;
  CFileItemList m_nfsShares;
};

class CAfpScanState
{
public:
  CAfpScanState();
  void GetContents( CFileItemList& list ) const;
  void SetContents( CFileItemList& list );
private:
  CCriticalSection m_lock;
  CFileItemList m_afpShares;
};

class CBmsScanState
{
public:
  CBmsScanState();
  void GetContents( CFileItemList& list ) const;
  void SetContents( CFileItemList& list );
private:
  CCriticalSection m_lock;
  CFileItemList m_bmsShares;
};


/**
 * Code for the browser service
 * Browser service provides an entry point for enumerating network resources of varying types
 **/
class CBrowserService : public BOXEE::BoxeeScheduleTask,
                        public CThread  
{
public:  
  enum SHARE_TYPE { NO_SHARE = -1, SMB_SHARE, UPNP_SHARE, BONJOUR_SHARE, NFS_SHARE, AFP_SHARE, BMS_SHARE, LAST_SHARE };

  CBrowserService();
  virtual ~CBrowserService();
  
  bool Init();
  void DeInit();

  bool GetShare( SHARE_TYPE type, CFileItemList& list ) const;
  bool IsHostAvailable(const CStdString &strHost);
  bool GetHostAddress( const CStdString& strHost, CStdString& strIp, CStdString& strWorkgroup);
  
  void TrackHostAvailability( const CStdString& strPath );

  void Refresh();
  
  bool IsSmbAgressiveScan() {return m_bSmbAgressiveScan;}

protected:
  virtual void DoWork();
  virtual void Process();

private:
  void GenerateUpnpShares();
  void GenerateSmbShares();
  void GenerateNfsShares();
  void GenerateAfpShares();
  void GenerateBmsShares();

  void RefreshShares();
  
  virtual bool ShouldDelete() { return false; }

private:  
  CCriticalSection   m_scanContext;
  BOXEE::BXBGProcess m_scanProcessor;
  
  CUPnPScanState*    m_pUPnPScanState;
  CCifsScanState*    m_pCifsScanState;
  CSMBScanState*     m_pSMBScanState;
  CNfsScanState*     m_pNfsScanState;
  CAfpScanState*     m_pAfpScanState;
  CBmsScanState*     m_pBmsScanState;

  bool m_bSmbAgressiveScan;
};

#endif
