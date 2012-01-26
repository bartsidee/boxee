
#include "BrowserService.h"
#include "Application.h"
#include "Util.h"
#include "FileSystem/Directory.h"
#include "../lib/libBoxee/boxee.h"
#include "IPScanJob.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "GUISettings.h"

#ifdef _WIN32
#include "lib/libnbtscan/nbtscan.h"
#endif

#include "utils/SystemInfo.h"

#define NON_SMB_SHARES_SCANNING_INTERVAL (5 * 60 * 1000)

using namespace DIRECTORY;
using namespace XBMC;
using namespace BOXEE;


/**********************************************************
 * SMBScanner and UPnPScanner are just bg threads to execute searches on the protocols
 */

class SMBScanner : public CThread
{
public:
  SMBScanner( CSMBScanState* pScanState, BOXEE::BXBGProcess* pScanProcessor );
  void Generate(); 
  virtual void Process();
private:
  CSMBScanState* m_pScanState;
  BOXEE::BXBGProcess* m_pScanProcessor;
};

/**
 * class SMBScanner
 **/
SMBScanner::SMBScanner( CSMBScanState* pScanState, BOXEE::BXBGProcess* pScanProcessor )
  : m_pScanState( pScanState ), m_pScanProcessor( pScanProcessor )
{
}

void SMBScanner::Generate()
{
  this->Create(true);
}

void SMBScanner::Process()
{
  // Set up the ip scanner to queue background work to our scan processor
  CIPScanner ipScanner( m_pScanProcessor );
  ipScanner.Scan( m_pScanState ); // This is long running, but checks the scan processor state to see if a stop is issued 
  
  // TODO fix the lifetime problem here, where bg work is queued to the scan processor, but CIPScanner falls
  // out of scope. Probably shouldn't happen since the subnet mapping will take longer than mapping file shares, but should
  // be guarded against
}

#ifdef _WIN32
int scan_network( const char* target_string, share_host_info* host_info );

class SMBNonAgressiveScanner : public XBMC::IIPScanCallback, 
                        public CThread
{
private:
	/*void GenerateSmbShares();
  void GenerateSmbSharesForInterface( CNetworkInterfacePtr pInterface );
  int  GetActiveHostsInSubnet( const CStdString& strSubnet, share_host_info* pHostInfo ) const;
  void GetAllSmbSharesForIP( const CStdString& strIP );
  int  CalcSubnetSize( const CStdString& strMask ) const;
  void AppendCIDRSuffix( const CStdString& strMask, CStdString& strSubnet, int nSize = 0 ) const;
  void GetFirstIPInSubnet( CNetworkInterfacePtr pInterface, CStdString& strIP, int nSize = 0 ) const;
  void DivideIP( const CStdString& strIP, unsigned short* pIP ) const;*/

	CSMBScanState* m_pScanState;
  BOXEE::BXBGProcess* m_pScanProcessor;
  CCriticalSection   m_scanContext;
  CNetwork*          m_pNetwork;

public:
	SMBNonAgressiveScanner( CSMBScanState* pScanState, BOXEE::BXBGProcess* pScanProcessor ): m_pScanState( pScanState ), m_pScanProcessor( pScanProcessor ) {
	  m_pNetwork = &g_application.getNetwork();
  }

  void Generate()
{  
  CLog::Log(LOGDEBUG, "GenerateSmbShares start...\n");
  m_pScanState->ClearHosts();

  std::vector< CNetworkInterfacePtr>  vecInterfaces = m_pNetwork->GetInterfaceList();

  if( vecInterfaces.empty() )
    return;

  std::vector< CNetworkInterfacePtr >::iterator iter    = vecInterfaces.begin();
  std::vector< CNetworkInterfacePtr >::iterator endIter = vecInterfaces.end();

  while( iter != endIter )
    GenerateSmbSharesForInterface( *iter++ );
  CLog::Log(LOGDEBUG, "GenerateSmbShares end\n");  
}
  virtual void Process()
{
  //if (g_guiSettings.GetBool("upnp.client"))
  //  GenerateUpnpShares();

  // This is a proccess running in the background, we dont want to use it when movie 
  // is playing.
  if (!g_application.IsPlayingVideo()) {
    Generate();
  }
  //GenerateBonjourShares();
}
private:
void GenerateSmbSharesForInterface( CNetworkInterfacePtr pInterface )
{
  if( !pInterface->IsConnected() ) 
    return;

  int nSize = CalcSubnetSize( pInterface->GetCurrentNetmask() );

  if( nSize < 2 )
    return;

  CStdString strSubnet;

  GetFirstIPInSubnet( pInterface, strSubnet, nSize );

  if( strSubnet.Left( 7 ) == "127.0.0" )
    return;

  if( nSize > 256 )
    nSize = 256; // either bug or subnet too big

  AppendCIDRSuffix( pInterface->GetCurrentNetmask(), strSubnet, nSize );

  share_host_info* pHostInfo = new share_host_info[ nSize - 2 ];

  memset( pHostInfo, 0, ( nSize - 2 ) * sizeof( share_host_info ) );

  int nHosts = GetActiveHostsInSubnet( strSubnet, pHostInfo );

  {
    CSingleLock lock( m_scanContext );

    //
    // detect stale ip-s (cleanup shares from ip-s that no longer exist)
    // we keep all the ips that we have in the map in a set and remove the ones that are found in the scan (active)
    // ip-s that are left in the set (were not found in the scan) - are marked and after 2 attempts - are removed.
    //
	//std::set<CStdString> setIPs = m_pScanState->GetIPs();

    for( share_host_info* pInfo = pHostInfo; nHosts--; ++pInfo )
    {
      std::string strIP = inet_ntoa( pInfo->ip );
      //setIPs.erase(strIP);

      CStdString strHost = &pInfo->hostname[ 0 ];
      strHost.Trim();
      m_pScanState->AddHost(strHost/*.c_str()*/);
      m_pScanState->AddHost(strIP);

      if (pInterface->GetCurrentIPAddress() != strIP)
      {
        CIPScanJob* pJob = new CIPScanJob( strSubnet.c_str(), strIP.c_str(), strHost.c_str(), this );
        m_pScanProcessor->QueueJob( pJob );
      }
    }

    //
    // the set now contains ip-s that are in the map but were not found in the scan.
    //
    /*std::set<CStdString>::const_iterator ips =  setIPs.begin();
    while (ips != setIPs.end())
    {
      int nCount = m_pScanState->GetNoIPCount(*ips);
      if (nCount > 0)
      {
        CLog::Log(LOGINFO,"ip <%s> is considered dead. removing froms smb map.", ips->c_str());
        m_pScanState->DeleteShare(*ips);
      }
      else
        m_pScanState->SetNoIPCount(*ips, nCount+1);

      ips++;
    }*/
  }

  delete[] pHostInfo;
}

// This method return the number of active hosts on subnet, < 0 if there was an error
int GetActiveHostsInSubnet( const CStdString& strSubnet, share_host_info* pHostInfo ) const
{
  return scan_network( strSubnet.c_str(), pHostInfo );
}


int CalcSubnetSize( const CStdString& strMask ) const
{
  if( strMask.empty() )
    return 0;

  unsigned short ip[ 4 ] = { 0 };

  DivideIP( strMask, &ip[ 0 ] );

  int nSize = 32;

  for( int i = 0; i < 4; ++i )
  {
    short value =  ip[ i ];

    while( value > 0 )
    {
      if( ( value & 1 ) == 1 )
        --nSize;
      value >>= 1;
    }
  }

  int nSubnetSize = 1;

  while( nSize-- )
    nSubnetSize <<= 1;

  return nSubnetSize;
}


void AppendCIDRSuffix( const CStdString& strMask,  CStdString& strSubnet, int nSize ) const
{
  int nCIDR = 32;

  if( 0 == nSize )
    nSize = CalcSubnetSize( strMask );

  while( nSize > 1 )
  {
    nSize >>= 1;
    --nCIDR;
  }
  CStdString strSuffix;

  strSuffix.Format( "/%d", nCIDR );
  strSubnet += strSuffix;
}


void GetFirstIPInSubnet( CNetworkInterfacePtr pInterface, CStdString& strIP, int nSize ) const
{
  unsigned short ip[ 4 ] = { 0 };

  DivideIP( pInterface->GetCurrentIPAddress(), &ip[ 0 ] );

  if( 0 == nSize )
    nSize = CalcSubnetSize( pInterface->GetCurrentNetmask() );

  int nNewLS = 0;

  while( ( nNewLS += nSize ) < ip[ 3 ] ) ;
  ip[ 3 ] = nNewLS - nSize;
  strIP.Format( "%d.%d.%d.%d", ip[ 0 ], ip[ 1 ], ip[ 2 ], ip[ 3 ] );
}


void DivideIP( const CStdString& strIP, unsigned short* pIP ) const
{
  std::vector< CStdString > tokens;

  CUtil::Tokenize( strIP, tokens, "." );
  for( size_t i = 0; i < tokens.size() && i < 4; ++i )
    pIP[ i ] = atoi( tokens[ i ] );
}

void IPScanned( CFileItemList& items )
{
  std::string ip( items.GetProperty( "HostIP" ) );	// TODO: Debug
  //std::string subnet( items.GetProperty( "Subnet" ) );
  CFileItemList l;

  l.Copy( items );
  l.SetProperty( "NoIPCount", 0 ); // mark ip as valid

  CSingleLock lock( m_scanContext );

  //m_pScanState->GetRawShares()[ /*make_pair(subnet,*/ ip ] = l;

  if (!g_application.m_bStop)
  {
    // add this to the smb scan state
    m_pScanState->AddShares( l );
	}
}
};
#endif

/**
 * class CCifsScanner
 **/
class CifsScanner : public CThread
{
public:
  CifsScanner( CCifsScanState* scanState );
  void Generate();
  virtual void Process();
private:
  CCifsScanState* m_pScanState;
};


CifsScanner::CifsScanner( CCifsScanState* pScanState )
  : m_pScanState( pScanState )
{
}

// Generate the list of shares; this spins our thread, which calls Process(), which enumerates the directory
void CifsScanner::Generate()
{
  this->Create(true);
}

void CifsScanner::Process()
{
  CFileItemList list;

  CDirectory::GetDirectory( "smb://all", list );
  m_pScanState->SetContents( list );
}

// TODO these should move out
CCifsScanState::CCifsScanState()
{
}

void CCifsScanState::GetContents( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  list = m_cifsShares;
}

void CCifsScanState::SetContents( CFileItemList& list )
{
  CSingleLock lock( m_lock );
  m_cifsShares = list;
}


/**
 * class UpnpScanner
 **/
class UpnpScanner : public CThread
{
public:
  UpnpScanner( CUPnPScanState* scanState );
  void Generate(); 
  virtual void Process();
private:
  CUPnPScanState* m_pScanState;
};


UpnpScanner::UpnpScanner( CUPnPScanState* pScanState )
  : m_pScanState( pScanState )
{
}

// Generate the list of shares; this spins our thread, which calls Process(), which enumerates the directory
void UpnpScanner::Generate()
{
  this->Create(true);
}

void UpnpScanner::Process()
{
  CFileItemList list;

  CDirectory::GetDirectory( "upnp://all", list );
  m_pScanState->SetContents( list );
}

// TODO these should move out
CUPnPScanState::CUPnPScanState()
{
}

void CUPnPScanState::GetContents( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  list = m_upnpShares;
}

void CUPnPScanState::SetContents( CFileItemList& list )
{
  CSingleLock lock( m_lock );
  m_upnpShares = list;
}

/**
 * class NfsScanner
 **/
class NfsScanner : public CThread
{
public:
  NfsScanner( CNfsScanState* pScanState);
  void Generate();
  virtual void Process();
private:
  CNfsScanState* m_pScanState;
};

NfsScanner::NfsScanner( CNfsScanState* pScanState )
  : m_pScanState( pScanState )
{
}

void NfsScanner::Generate()
{
  this->Create(true);
}

void NfsScanner::Process()
{
  CFileItemList list;

  CDirectory::GetDirectory( "nfs://all", list );
  m_pScanState->SetContents( list );
}

CNfsScanState::CNfsScanState()
{
}

void CNfsScanState::GetContents( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  list = m_nfsShares;
}

void CNfsScanState::SetContents( CFileItemList& list )
{
  CSingleLock lock( m_lock );
  m_nfsShares = list;
}

/**
 * class AfpScanner
 **/
class AfpScanner : public CThread
{
public:
  AfpScanner( CAfpScanState* pScanState);
  void Generate();
  virtual void Process();
private:
  CAfpScanState* m_pScanState;
};

AfpScanner::AfpScanner( CAfpScanState* pScanState )
  : m_pScanState( pScanState )
{
}

void AfpScanner::Generate()
{
  this->Create(true);
}

void AfpScanner::Process()
{
  CFileItemList list;

  CDirectory::GetDirectory( "afp://all", list );
  m_pScanState->SetContents( list );
}

CAfpScanState::CAfpScanState()
{
}

void CAfpScanState::GetContents( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  list = m_afpShares;
}

void CAfpScanState::SetContents( CFileItemList& list )
{
  CSingleLock lock( m_lock );
  m_afpShares = list;
}

/**
 * class BmsScanner
 **/
class BmsScanner : public CThread
{
public:
  BmsScanner( CBmsScanState* pScanState);
  void Generate();
  virtual void Process();
private:
  CBmsScanState* m_pScanState;
};

BmsScanner::BmsScanner( CBmsScanState* pScanState )
  : m_pScanState( pScanState )
{
}

void BmsScanner::Generate()
{
  this->Create(true);
}

void BmsScanner::Process()
{
  CFileItemList list;

  CDirectory::GetDirectory( "bms://all", list );
  m_pScanState->SetContents( list );
}

CBmsScanState::CBmsScanState()
{
}

void CBmsScanState::GetContents( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  list = m_bmsShares;
}

void CBmsScanState::SetContents( CFileItemList& list )
{
  CSingleLock lock( m_lock );
  m_bmsShares = list;
}

/**********************************************************
 * Browser service code - acts as indirection to the underlying protocols we have scanned
 */

/**
 * class CBrowserService
 **/
CBrowserService::CBrowserService()
  : BoxeeScheduleTask( "SharesScanning", NON_SMB_SHARES_SCANNING_INTERVAL, true )
{
  m_pUPnPScanState = new CUPnPScanState();
  m_pSMBScanState = new CSMBScanState();
  m_pCifsScanState = new CCifsScanState();
  m_pNfsScanState = new CNfsScanState();
  m_pAfpScanState = new CAfpScanState();
  m_pBmsScanState = new CBmsScanState();

  m_bSmbAgressiveScan = false;
}


CBrowserService::~CBrowserService()
{
  DeInit();
  delete m_pUPnPScanState;
  delete m_pSMBScanState;
  delete m_pCifsScanState;
  delete m_pNfsScanState;
  delete m_pAfpScanState;
  delete m_pBmsScanState;
}


bool CBrowserService::Init()
{
	if(g_sysinfo.GetPlatform() == PLATFORM_WINDOWS)
		 m_scanProcessor.Start(5);
	else
	  m_scanProcessor.Start(1);
  RefreshShares();
  
  BOXEE::Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask( this );
  return true;
}

void CBrowserService::DoWork()
{
  RefreshShares();
}


void CBrowserService::RefreshShares()
{
  StopThread();

  // TODO need to double check on what this locks against... all the internal state is self locking
  // and none of the scanners are locked when running, so... why sync
  CSingleLock lock( m_scanContext );

  Create();
}


void CBrowserService::Process()
{

  // Eli: For testing purpose we're temporarily removing UPNP scanning in the background
  // We will only scan for UPNP for it's selected from the network file browser
  // The reason being many UPNP crashed for users not actually using this.
  /*
  if( g_guiSettings.GetBool("upnp.client") )
  {
    GenerateUpnpShares();
  }
  */

  if( g_guiSettings.GetBool("smb.client") )
  {
    // This is a proccess running in the background, we dont want to use it when movie 
    // is playing.
    if( !g_application.IsPlayingVideo() && !g_application.IsPlayingAudio() )
    {
      GenerateSmbShares();
    }
  }


  //if( g_guiSettings.GetBool("nfs.client") )
  {
    if( !g_application.IsPlayingVideo() && !g_application.IsPlayingAudio() )
    {
      GenerateNfsShares();
    }
  }

  //if( g_guiSettings.GetBool("afp.client") )
  {
    if( !g_application.IsPlayingVideo() && !g_application.IsPlayingAudio() )
    {
      GenerateAfpShares();
    }
  }

  //if( g_guiSettings.GetBool("afp.client") )
  {
    if( !g_application.IsPlayingVideo() && !g_application.IsPlayingAudio() )
    {
      GenerateBmsShares();
    }
  }
}

void CBrowserService::DeInit()
{
  // This stops ourselves and queued IP scan jobs properly, but doesn't abort the network mapping (potentially long running)
  BOXEE::Boxee::GetInstance().GetBoxeeScheduleTaskManager().RemoveScheduleTask( this );
  printf("stopping CIPScanJob service...\n");    
  m_scanProcessor.Stop(8);
  StopThread();
}

bool CBrowserService::GetShare( SHARE_TYPE type, CFileItemList& list ) const
{
  if( type <= NO_SHARE || type >= LAST_SHARE )
    return false;

  switch (type)
  {
    case SMB_SHARE:
    {
      if(m_bSmbAgressiveScan || (g_sysinfo.GetPlatform() == PLATFORM_WINDOWS))
        m_pSMBScanState->GetShares( list );
      else
        m_pCifsScanState->GetContents( list );
      break;
    }
    case UPNP_SHARE:
    {
      m_pUPnPScanState->GetContents( list );
      break;
    }
    case BONJOUR_SHARE:
      // Not actually supported
      break;
    case NFS_SHARE:
    {
       m_pNfsScanState->GetContents( list );
       break;
    }
    case AFP_SHARE:
    {
       m_pAfpScanState->GetContents( list );
       break;
    }
    case BMS_SHARE:
    {
       m_pBmsScanState->GetContents( list );
       break;
    }
    default:
      CLog::Log(LOGERROR,"%s invalid type requested (%d)", __FUNCTION__, type);
      return false;
  }

  return true;
}

void CBrowserService::GenerateUpnpShares()
{
  UpnpScanner *upnpScanner = new UpnpScanner( m_pUPnPScanState );

  upnpScanner->Generate();
  upnpScanner = NULL;  // object deleted on completion of thread execution
}


void CBrowserService::GenerateSmbShares()
{  
  m_bSmbAgressiveScan = g_guiSettings.GetBool("smb.agressivescan2");

  if(m_bSmbAgressiveScan)
  {
    SMBScanner *smbScanner = new SMBScanner( m_pSMBScanState, &m_scanProcessor );

    smbScanner->Generate();
    smbScanner = NULL; // object deleted on completion of thread execution
  }
  else
  {
#ifdef _WIN32
	SMBNonAgressiveScanner *smbScanner = new SMBNonAgressiveScanner( m_pSMBScanState, &m_scanProcessor );
    smbScanner->Generate();
    smbScanner = NULL; // object deleted on completion of thread execution
#else
    CifsScanner *cifsScanner = new CifsScanner( m_pCifsScanState);

    cifsScanner->Generate();
    cifsScanner = NULL; // object deleted on completion of thread execution
#endif
  }
}

void CBrowserService::GenerateNfsShares()
{
  NfsScanner *nfsScanner = new NfsScanner( m_pNfsScanState);

  nfsScanner->Generate();
  nfsScanner = NULL; // object deleted on completion of thread execution
}

void CBrowserService::GenerateAfpShares()
{
  AfpScanner *nfsScanner = new AfpScanner( m_pAfpScanState);

  nfsScanner->Generate();
  nfsScanner = NULL; // object deleted on completion of thread execution
}

void CBrowserService::GenerateBmsShares()
{
  BmsScanner *bmsScanner = new BmsScanner( m_pBmsScanState);

  bmsScanner->Generate();
  bmsScanner = NULL; // object deleted on completion of thread execution
}

// Ping the scan state to see if the host is online
bool CBrowserService::IsHostAvailable(const CStdString &strHost)
{
	if((g_sysinfo.GetPlatform() == PLATFORM_WINDOWS) && (!m_bSmbAgressiveScan))
		return true;
  return m_pSMBScanState->IsHostAvailable( strHost );
}

// Strip the path down to a host and add it to the scanner
// For all protocols except SMB this is a no-op
void CBrowserService::TrackHostAvailability( const CStdString& strPath )
{
  if( CUtil::IsSmb( strPath ) )
    m_pSMBScanState->TrackHostAvailability( strPath );
}

bool CBrowserService::GetHostAddress( const CStdString& strHost, CStdString& strIp, CStdString& strWorkgroup)
{
  bool bFound = m_pSMBScanState->GetHostAddress( strHost, strIp, strWorkgroup );

  if(bFound)
    return true;

  CFileItemList list;
  m_pCifsScanState->GetContents(list);

  for(int i=0; i<list.Size() && !bFound; i++)
  {
    CStdString s1 = list[i]->GetProperty("HostName");
    CStdString s2 = strHost;

    if(s1.ToLower() == s2.ToLower())
    {
      strWorkgroup = list[i]->GetProperty("Workgroup");
      strIp = list[i]->GetProperty("HostIP");

      if(strIp.IsEmpty())
      {
        bFound = CUtil::GetHostByName(strHost, strIp);
      }
      if(!bFound)
      {
        CLog::Log(LOGERROR,"%s failed to resolve (%s)", __FUNCTION__, strHost.c_str());
        break;
      }
    }
  }

  return bFound;
}

// Trigger a re-scan of the network, called when we get an address or interface change
void CBrowserService::Refresh()
{
  // There are four main scenarios:
  // * Interface was offline but came online
  // * Interface was online and went offline
  // * Online/offline state change on a second interface
  // * IP address change on the primary interface
  // To the extent possible, we *only* want to do this for the first two. IP address
  // changes are expensive and best left to the passive scan
  DeInit();
  Init();
}
