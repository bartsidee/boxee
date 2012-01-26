
#include "BrowserService.h"
#include "Application.h"
#include "Util.h"
#include "FileSystem/Directory.h"
#include "../lib/libBoxee/boxee.h"
#include "IPScanJob.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "GUISettings.h"

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
  
  m_scanProcessor.Start(1);
  RefreshShares();
}


CBrowserService::~CBrowserService()
{
  DeInit();
  delete m_pUPnPScanState;
  delete m_pSMBScanState;
}


bool CBrowserService::Init()
{
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
  if( g_guiSettings.GetBool("upnp.client") )
  {
    GenerateUpnpShares();
  }
  
  if( g_guiSettings.GetBool("smb.client") )
  {
    // This is a proccess running in the background, we dont want to use it when movie 
    // is playing.
    if( !g_application.IsPlayingVideo() )
    {
      GenerateSmbShares();
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
      m_pSMBScanState->GetShares( list );
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
  SMBScanner *smbScanner = new SMBScanner( m_pSMBScanState, &m_scanProcessor );
  
  smbScanner->Generate();
  smbScanner = NULL; // object deleted on completion of thread execution
}

bool CBrowserService::IsHostAvailable(const CStdString &strHost)
{
  return m_pSMBScanState->IsHostAvailable( strHost );
}

