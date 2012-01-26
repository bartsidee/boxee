#ifndef __IPSCANJOB_H__
#define __IPSCANJOB_H__

#include "lib/libBoxee/bxbgprocess.h"
#include "lib/libBoxee/bxscheduletaskmanager.h"

#include <map>
#include <string>
#include <set>

class CNetwork;
class CNetworkInterface;
typedef boost::shared_ptr<CNetworkInterface> CNetworkInterfacePtr;

// fdecl from lib/libnbtscan/statusq.h
struct nb_host_info;

/**
 * CSMBScanState - state of the scanned shares broken out by interface
 */
class CSMBScanState
{
public:
  CSMBScanState();
  
  void GetShares( CFileItemList& list ) const;
  void AddShares( CFileItemList& shares );
    
  void MarkHostActive( const CStdString& strInterface, const CStdString& strHostname, const CStdString& strIP );
  bool IsHostAvailable( const CStdString& strHost );
  bool IsFoundHost( const CStdString& strInterface, const CStdString& strHostname );
  
  void MarkDirty();
  void CleanStaleHosts();
private:
  void LogState();

  CCriticalSection m_lock;
    
  // Given the remote IP, map to the set of shares found at that address
  // When scanning, this is postprocessed to remove some of the shares that are no longer available
  std::map<CStdString, CFileItemList> m_smbShares;

  // The most current snapshot of the available hosts on all interfaces (by remote IP)
  std::set<CStdString> m_smbHosts;
  
  // The set of hosts to clean up on the next cleanup pass (by remote IP)
  std::set<CStdString> m_smbHostsToCleanup;
};

/**
 * CSMBScanJob - when IPScanner finds a SMB capable host, we create a smbscanjob instance to enumerate shares on
 * that host
 */
class CSMBScanJob : public BOXEE::BXBGJob
{
public:
  CSMBScanJob( CStdString& local_interface, CStdString& remote_ip, CStdString& remote_hostname, CSMBScanState* scanState );
  virtual void DoWork();
    
private:
  void FormatShareString( CFileItemPtr fileItem, CStdString& strUrl ) const;
  
  CStdString     m_strLocalInterface;
  CStdString     m_strRemoteIP;
  CStdString     m_strRemoteHostname;
  CSMBScanState* m_pScanState;
};

/**
 * CIPScanner - scan the subnet using a variety of techniques for potential NBT supporting hosts
 */
    
class CIPScanner
{
public:
  CIPScanner(BOXEE::BXBGProcess* scanProcessor);
  virtual ~CIPScanner() { }
    
  void Scan( CSMBScanState* smbScanState );
   
private:
  // Functions for execution of the scan
  int ScanAdapterForHosts( CNetworkInterfacePtr pInterface, bool mapping );
  void FoundActiveIP( CStdString& local_interface, CStdString& remote_ip, CStdString& remote_hostname, bool mapped );

  static void ParseNBTSTAT( nb_host_info* hostinfo, CStdString& hostname, CStdString& workgroup );
  static int CreateSocket( struct in_addr local_ip );
  static void Uuidgen( char* uuid );
  static int SendDiscoveryProbe( int sock, struct in_addr local_ip );
  static bool CheckElapsedTime( struct timeval* last_timestamp, int min_elapsed_us );

  // Functions for managing the scan table
  void AddToScanTable( CNetworkInterfacePtr pInterface );
    
  typedef struct scan_row_s
  {
    struct in_addr start;  // host byte order
    struct in_addr end;    // host byte order
  } scan_row_t;

  BOXEE::BXBGProcess* m_pScanProcessor;
  CSMBScanState*     m_pScanState;
  CNetwork*          m_pNetwork;
    
  // Workgroup for this machine
  CStdString         m_strWorkgroup;
    
  // table of address ranges left to scan
  std::map<CNetworkInterfacePtr, std::vector<scan_row_t> > m_scanTable;
};

#endif // __IPSCANJOB_H__
