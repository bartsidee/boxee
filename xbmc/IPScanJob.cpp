

#include "StdString.h"
#include "FileItem.h"
#include "FileItem.h"
#include "FileSystem/Directory.h"
#include "../lib/libBoxee/boxee.h"
#include "IPScanJob.h"
#include "utils/SingleLock.h"
#include "Util.h"
#include "utils/log.h"
#include "Application.h"
#include "GUISettings.h"

#include "URL.h"

// networking includes
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

// Mac and Linux have the same include set; Win32 has a variety
// of patches to compensate
#ifndef _WIN32

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define CLOSE_SOCKET(x) close(x)
#define __EXTERNC

#else  // _WIN32

typedef int socklen_t;

#include <WinSock2.h>
#include <ws2ipdef.h> // IP_MULTICAST_IF
#include <sys/timeb.h>

// Remap for gettimeofday based on libnbtscan approach
static int w32_gettimeofday(struct timeval* tv)
{
  struct timeb tm;
  ftime( &tm );
  tv->tv_sec = tm.time;
  tv->tv_usec = tm.millitm * 1000;
  return 0;
}
#define gettimeofday(timeval, timez) w32_gettimeofday(timeval)
#define snprintf _snprintf
#define CLOSE_SOCKET(x) closesocket(x)
#define usleep(x) Sleep((x)/1000)
#include "win32/c_defs.h"
#define __EXTERNC extern "C"

#endif // ! _WIN32

#include "lib/libnbtscan/statusq.h"
#include "lib/libnbtscan/list.h"

using namespace XBMC;
using namespace DIRECTORY;
using namespace BOXEE;

// Plan on a 1500 MTU
#define PKT_BUFFER_SIZE 1500

// Code for the NB response type (no machine name included)
#define NB_RESPONSE_TYPE 0x20

// Code for the group name type
#define NBT_GROUP_FLAG 0x0080

// Ports for WS-Discovery and NBT
#define WS_DISCOVERY_PORT 3702
#define NBT_PORT 137

__EXTERNC int inet_aton(const char *cp, struct in_addr *addr);

/******
 * CSMBScanState tracks the state of scanned SMB shares
 */

CSMBScanState::CSMBScanState()
{
}
/* Pull the list of enumerated shares out */
void CSMBScanState::GetShares( CFileItemList& list ) const
{
  CSingleLock lock( m_lock );
  
  std::map<CStdString, CFileItemList>::const_iterator iter = m_smbShares.begin();
  while (iter != m_smbShares.end())
{
    list.Append(iter->second);

    iter++;
  }
}

/* Called when we find new shares */
void CSMBScanState::AddShares( CFileItemList& items )
{
  std::string ip( items.GetProperty( "HostIP" ) );
  CFileItemList l;

  l.Copy( items );
  l.SetProperty( "NoIPCount", 0 ); // mark ip as valid

  CSingleLock lock( m_lock );

  m_smbShares[ ip ] = l;
}

/* Called when a host is found online */
void CSMBScanState::MarkHostActive( const CStdString& strInterface, const CStdString& strHostname, const CStdString& strIP )
{
  // At a guess, we insert both so IsHostAvailable() can handle hostnames or IPs?
  // Not clear if we should insert the local machine here, or in what cases the local
  // machine has to be in m_smbHosts... since we don't scan it below

  // Disregard the interface, as a live host is a live host for our purposes
  
  CSingleLock lock( m_lock );

  // Add this to the current hosts
  m_smbHosts.insert( strHostname );
  m_smbHosts.insert( strIP );

  // Remove this from the cleanup list
  m_smbHostsToCleanup.erase( strIP );
  m_smbHostsToCleanup.erase( strHostname );
  
  // NoIPCount is cleared when we actually enumerate the shares
}

bool CSMBScanState::IsFoundHost( const CStdString& strInterface, const CStdString& strHostname )
{
  CSingleLock lock( m_lock );
  
  // We've previously found this host if it's in m_smbHosts but not in m_smbHostsToCleanup

  bool is_in_smbhosts = m_smbHosts.find( strHostname ) != m_smbHosts.end();
  bool is_in_cleanup = m_smbHostsToCleanup.find(strHostname) != m_smbHostsToCleanup.end();
  return is_in_smbhosts && !is_in_cleanup;
}

bool CSMBScanState::IsHostAvailable( const CStdString& strHost )
{
  CSingleLock lock( m_lock );
  CStdString up = strHost;
  up.ToUpper();

  return m_smbHosts.find(up) != m_smbHosts.end();
}

bool CSMBScanState::GetHostAddress( const CStdString& strHost, CStdString& strIp, CStdString& strWorkgroup)
{
  CSingleLock lock( m_lock );

  std::map<CStdString, CFileItemList>::const_iterator iter = m_smbShares.begin();
  while (iter != m_smbShares.end())
  {
    const CFileItemList& l = iter->second;
    CStdString h = l.GetProperty("HostName");

    CStdString h1 = strHost;

    if (h1.ToLower() == h.ToLower())
    {
      strIp = iter->first;
      strWorkgroup = l.GetProperty("Workgroup");
      return true;
    }

    iter++;
  }
  return false;
}

void CSMBScanState::TrackHostAvailability( const CStdString& strPath )
{
  CURI url(strPath);
  CStdString hostname = url.GetHostName();
  const char* hc = hostname.c_str();
  struct in_addr ia;
  
  // If this isn't in dotted notation, then the user is using name resolution
  // With name resolution things are ugly. What if it's DNS only and NBT is disabled? The scan loop relies on using
  // NBTSTAT commands to see if a host is online.
  // This function will split out all of the DNS resolution based cases since those will always fail in the scan loop,
  // and we can't use NBTSTAT. We treat DNS based hosts as always-online from a browser perspective; we have to issue an SMB
  // request to verify. The reason is everything except SMB could be blocked by the remote firewall (including ICMP).
  // For everything else,
  // * If it is dotted notation, we add it to the scan queue and will check it (covers cross subnet)
  // * If it is a NBT hostname, we ignore it here, but will add the NBT name to m_smbHosts when we discover it through the mapper
  // * For all other cases we'll fail here and fail to add in the mapper

  if( IsHostAvailable( hostname ) )
  {
    // If the host is already being found in subnet scans, we do nothing here.
    return ;
}

  if( 0 == inet_aton( hc, &ia ) )
  {
    struct hostent* hent = NULL;
    
    // This just short circuits doing a DNS lookup if we are offline.
    // This also avoids doing lookups with a bad proxy address, which are expensive
    if( g_application.IsOfflineMode() )
    {
      CLog::Log(LOGDEBUG, "SMB: Not looking up hostname <%s> as we are offline.\n", hostname.c_str());
      
      return ;
    }
    else
    {
      hent = gethostbyname( hc );
    }
    
    if( !hent )
    {
      // There is a subtle timing issue here, if this is called before we've done any subnet
      // scanning we hit this case. There is probably a perf impact with this too.
      CLog::Log(LOGINFO, "Host string <%s> wasn't found by gethostbyname - probably a NBT name (err %d)\n", hc, h_errno);
    }
    else
    {
      // Sanity check the lookup, this shouldn't happen
      if( (4 != hent->h_length) || (AF_INET != hent->h_addrtype) )
      {
        CLog::Log(LOGDEBUG, "Host <%s> resolved to a non-ipv4 address\n", hc );
      }
      else
      {
        // set ia to the first entry in the list.
        ia.s_addr = *(unsigned int*)hent->h_addr;
        
        const char* a = inet_ntoa( ia );
        CLog::Log(LOGDEBUG, "Host <%s> resolved to <%s> without netbios - we will treat this host as online by default\n", hc, a);
        m_smbHosts.insert( hostname );
        m_smbHosts.insert( a );
      }
    }
  }
  else
  {
    // ia set by inet_aton
    CSingleLock lock( m_lock );
    m_smbHostsToTrack.insert( ia.s_addr );
  }
}

void CSMBScanState::GetTrackedHosts( std::set<unsigned int>& trackedHosts )
{
  CSingleLock lock( m_lock );
  trackedHosts = m_smbHostsToTrack;
}

void CSMBScanState::MarkDirty()
{
  CSingleLock lock( m_lock );
  
  std::map<CStdString, CFileItemList>::const_iterator iter = m_smbShares.begin();

  while( iter != m_smbShares.end() )
  {
    m_smbHostsToCleanup.insert( iter->first );
    iter++;
  }
}

void CSMBScanState::CleanStaleHosts()
{
  // Cleanup process
  //
  //
  // The scan should update m_smbHostsToCleanup to be the accurate list of items we need to clean via the callback
  // At this point step through and clean up
  //
  CSingleLock lock( m_lock );
  
  std::set< CStdString >::const_iterator staleHost =  m_smbHostsToCleanup.begin();
  while (staleHost != m_smbHostsToCleanup.end())
  {
    int nCount = m_smbShares[ *staleHost ].GetPropertyInt("NoIPCount");
    if (nCount > 1)
    {
      CLog::Log(LOGINFO, "Machine <%s> is considered dead. Removing from list of shares.\n", staleHost->c_str());
      m_smbShares.erase( *staleHost );
    }
    else
    {
      CLog::Log(LOGDEBUG, "Machine <%s> missed in scan; incrementing noipcount.\n", staleHost->c_str());
      m_smbShares[ *staleHost ].SetProperty("NoIPCount", nCount+1);
    }

    staleHost++;
  }
  m_smbHostsToCleanup.clear();
}

void CSMBScanState::LogState()
{
  CSingleLock lock( m_lock );
  
  std::map< CStdString, CFileItemList >::const_iterator shareList = m_smbShares.begin();

  CLog::Log(LOGDEBUG, "Dumping current SMB share list...\n");
  while( shareList != m_smbShares.end() )
  {
    int staleCount = shareList->second.GetPropertyInt("NoIPCount");
    CLog::Log(LOGDEBUG, "Remote IP %s Stale counter %d\n", shareList->first.c_str(), staleCount);
    for(int i = 0, sz = shareList->second.Size(); i < sz; i++ )
    {
      CLog::Log(LOGDEBUG, "  Share: %s\n", shareList->second[i]->m_strPath.c_str());
    }
    shareList++;
  }
}

////////////////////////
CSMBScanJob::CSMBScanJob( CStdString& local_interface,
                          CStdString& remote_ip,
                          CStdString& remote_hostname,
                          CStdString& work_group,
                          CSMBScanState* scanState ) :
                    BXBGJob("CSMBScanJob"), 
                    m_strLocalInterface( local_interface ),
                    m_strRemoteIP( remote_ip ),
                    m_strRemoteHostname( remote_hostname ),
                    m_strWorkgroup( work_group ),
                    m_pScanState( scanState )
{
  m_strRemoteHostname.Trim();
}
void CSMBScanJob::DoWork()
{
  if (g_application.m_bStop)
    return;

  CFileItemList list;
  CStdString    smbUrl = "smb://" + m_strRemoteIP;

  CLog::Log( LOGDEBUG, "Looking for shares on host %s ip %s", m_strRemoteHostname.c_str(), m_strRemoteIP.c_str() );

  CDirectory::GetDirectory( smbUrl, list );

  CLog::Log( LOGDEBUG, "Found %d shares on host %s", list.Size(), m_strRemoteHostname.c_str());

  for( int i = 0, nSize = list.Size(); i < nSize && !g_application.m_bStop; ++i )
  {
    CFileItemPtr pFileItem = list[ i ];    
    
    FormatShareString( pFileItem, smbUrl );
    pFileItem->SetLabel( smbUrl );
    
    CURI u(pFileItem->m_strPath);

    pFileItem->m_strPath = "smb://" + m_strRemoteHostname + "/" + u.GetShareName();
  }
  
  CFileItemPtr pHostItem ( new CFileItem );

  pHostItem->m_strPath = "smb://" + m_strRemoteHostname;
  pHostItem->SetLabel( m_strRemoteHostname);
  pHostItem->m_bIsFolder = true;
  list.Add(pHostItem);
  list.SetProperty( "HostIP", m_strRemoteIP );
  list.SetProperty( "Interface", m_strLocalInterface );
  list.SetProperty( "HostName", m_strRemoteHostname );
  list.SetProperty( "Workgroup", m_strWorkgroup);

  if (!g_application.m_bStop)
  {
    // add this to the smb scan state
    m_pScanState->AddShares( list );
}
}


void CSMBScanJob::FormatShareString( CFileItemPtr fileItem, CStdString& strUrl ) const
{
  // from smb://<hostname>/<sharename>/ we will make <sharename> on <hostname>.
  CStdString strPath = fileItem->m_strPath;
  CUtil::RemoveSlashAtEnd(strPath);
  CStdString strShare = CUtil::GetFileName(strPath);
  strUrl.Format( "%s / %s ", m_strRemoteHostname.c_str(), strShare.c_str() );
}

////////////////////////
CIPScanner::CIPScanner(BOXEE::BXBGProcess* scanProcessor) : m_pScanProcessor( scanProcessor )
{
  m_pNetwork = &( g_application.getNetwork() );
}

// CIPScanner::Scan
// This function issues the broadcast scans across all adapters on the current thread
// After that, it creates bg jobs to map the network out 
void CIPScanner::Scan( CSMBScanState* smbScanState )
{
  m_pScanState = smbScanState;
  
  // Get the stored workgroup for the machine or use the default on windows
  // Should probably query for the machine workgroup instead
#ifdef _WIN32
  m_strWorkgroup = "WORKGROUP";
#else
  m_strWorkgroup = g_guiSettings.GetString("smb.workgroup");
#endif

  // Pull the current interface list
  std::vector< CNetworkInterfacePtr>  vecInterfaces = m_pNetwork->GetInterfaceList();

  // If there are no adapters, go through and mark everything we already have for cleanup. Doing this through three scans will
  // cause a cleanup, so we should be ok through simple adapter offline/online
  
  m_pScanState->MarkDirty();

  if( !vecInterfaces.empty() )
  {
    // Otherwise loop over the current interfaces and generate the list of smb shares for each
    // TODO we need to pull the WINS server address if available. There are a few possible places.
    // One is we can get it through the DHCP response on windows. In this case it's per interface
    // Another is we can pull it from the SMB configuration. Not sure if this works for the samba lib
    //  we use
    // A third, pretty sure there is a user setting for this in boxee
    // If we pull this, we should be able to add an entry to the scan table to send directed traffic to it,
    // but i need to test this with an MS wins server to be sure..
    //const char* wins_server = NULL;   

    CLog::Log(LOGDEBUG, "Scanning interfaces for shares\n");
    std::set<unsigned int> hostsToTrack;

    m_pScanState->GetTrackedHosts( hostsToTrack );
    
    m_scanTable.clear();

    std::vector< CNetworkInterfacePtr >::iterator iter    = vecInterfaces.begin();
    for( ; iter != vecInterfaces.end() ; iter++ )
    {
      CStdString macAddr = (*iter)->GetMacAddress();
  
      CLog::Log(LOGDEBUG, "Interface %s, %sconnected\n", macAddr.c_str(), (*iter)->IsConnected() ? "" : "not ");
      
      if( (*iter)->IsConnected() )
      {
        // Flag the interface for mapping
        AddToScanTable( *iter, hostsToTrack );
        
        // Do the broadcast based scan, no mapping
        int hostsFound = ScanAdapterForHosts( *iter, false );
    
        CLog::Log(LOGDEBUG, "Interface %s scan complete, found %d host%s\n", macAddr.c_str(), hostsFound, (hostsFound > 1 ? "s" : ""));
      }
    }
    
    // Then process everything in the network map
    for( iter = vecInterfaces.begin(); iter != vecInterfaces.end(); iter++ )
    {
      if( (*iter)->IsConnected() )
      {
        CStdString ma = (*iter)->GetMacAddress();
        const char* macaddr = ma.c_str();
        
        CLog::Log(LOGDEBUG, "Mapping out hosts on interface %s\n", macaddr);

        (void)ScanAdapterForHosts( (*iter), true );
        
        CLog::Log(LOGDEBUG, "Done mapping interface %s\n", macaddr);
      }
    }
  }
  
  // At this point we have mapped everything, so clean up any hosts still marked stale
  m_pScanState->CleanStaleHosts();

  // All hosts are mapped. Background jobs are running to finish the share mapping.
  CLog::Log(LOGDEBUG, "Network scan finished\n");  
}

// Add address ranges from the interface to the scan table
// Do this in a way that makes it most likely that we find shares quickly
void CIPScanner::AddToScanTable( CNetworkInterfacePtr pInterface, std::set<unsigned int>& specificHosts )
{
  // Here is the ideal logic - right now we do the limited version of just scanning from .1 to .254
  // Most home networks are structured with a single subnet with a router at x.x.x.1
  // The rest of the nodes on the network are likely DHCP leases starting at x.x.x.2, x.x.x.50, or x.x.x.100
  // The edge cases are static addresses or routers that use some other starting point or are manually configured
  // When we build the scan table we should bias towards the most likely places for hosts (the first 15 addresses in each chunk)
  // and put everything else on the backburner. For now though we just scan the full range all at once
  // This like many other things will need some amount of user feedback or reported issues to warrant the complexity
  CStdString bcast = pInterface->GetCurrentBroadcastAddress();
  CStdString nmask = pInterface->GetCurrentNetmask();
  
  struct in_addr bcaddr, nmaddr;
  
  if( !inet_aton( bcast.c_str(), &bcaddr ) || !inet_aton( nmask.c_str(), &nmaddr ) )
  {
    CLog::Log( LOGINFO, "Failed to process interface <%s>\n", pInterface->GetMacAddress().c_str() );
  }
  else
  {
    // Enumerate through the specific hosts added and put an entry in the table for each one
    std::set<unsigned int>::iterator iter = specificHosts.begin();
    while( iter != specificHosts.end() )
    {
      scan_row_t host;
      
      host.start.s_addr = *iter;
      host.end.s_addr = *iter;
      
      m_scanTable[pInterface].push_back(host);
    
      iter++;
    } 
    
    // Now pull in the full range, scoping down to a /24 if needed.
    scan_row_t full_range;

    if( !(nmaddr.s_addr & 0xff100000) )
    {
      CLog::Log( LOGINFO, "Network interface %s is larger than 256 addresses; not scanning the full address range\n", pInterface->GetMacAddress().c_str() );
      inet_aton(pInterface->GetCurrentIPAddress().c_str(), &full_range.start);
      full_range.start.s_addr = ntohl(full_range.start.s_addr & htonl(0xffffff00));
      full_range.end.s_addr = full_range.start.s_addr | 0x000000ff;
    }
    else
    {
      full_range.start.s_addr = ntohl( (bcaddr.s_addr & nmaddr.s_addr) );
      full_range.end.s_addr = ntohl( (bcaddr.s_addr) );
    }
    
    m_scanTable[pInterface].push_back(full_range);
  }
}

// Called if we find a live IP; queues a smb scan job to enumerate shares
void CIPScanner::FoundActiveIP( CStdString& local_interface, CStdString& remote_ip, CStdString& remote_hostname, CStdString& work_group, bool mapped )
{
  remote_hostname.Trim();

  // If mapped == true then this host was found using the subnet mapper. So we need to pay some special attention, since it may have
  // been previously found through the standard methods.
  if( !mapped || !m_pScanState->IsFoundHost( local_interface, remote_ip ) )
  {
    CLog::Log( LOGDEBUG, "Found machine %s address %s interface %s%s\n", remote_hostname.c_str(), remote_ip.c_str(), local_interface.c_str(),(mapped?" via mapping":"") );
    m_pScanState->MarkHostActive( local_interface, remote_hostname, remote_ip );
  
    CSMBScanJob* pJob = new CSMBScanJob( local_interface, remote_ip, remote_hostname, work_group, m_pScanState );
    m_pScanProcessor->QueueJob( pJob );
  }
}



// Scan adapter for hosts - scan the given subnet/interface for hosts
// Calls FoundActiveIP whenever a host is found
// long running function (seconds) that returns after the final technique is done
//
// This uses a variety of techniques to get the fullest possible list of nodes on the subnet:
//
//  mapping == false:
// 1. NBT broadcast - we search for '*' in a broadcast query
//                    this locates everyone running samba (eg linux, osx, and anything non windows)
// 2. NBT workgroup1 - we search for the user specified workgroup using a broadcast query
//                     this locates everything in the user specified workgroup (superset of 1)
// 3. NBT workgroup2 - we search for any workgroups found from previous queries using a broadcast query
//                     this locates everything in all discovered workgroups (superset of 2)
// 4. WS-Discovery   - we search for 'p:Computer' types on the local subnet
//                     this locates all W7 and Vista windows machines, independent of workgroup
//
//  mapping == true:
// 5. nbt mapping    - we ping every individual IP on the /24 subnet with a unicast NBT packet
//                     this will assure us of finding everything in the IP range, but will take longer
//                     as we pace the packets out to avoid flooding the router
//
// The design for this function is as follows
//
// We create a single search socket. This will let us simplify the FW exception process for users later if needed
// (by allowing a user specified port that they separately allow through their FW)
//
// In our reader loop we cycle through each method, caching the IPs we find through each, and issuing separate
// queries over NBT unicast to get the hostname
// Each time we obtain a hostname we issue a FoundActiveIP with the new info
// 



int CIPScanner::ScanAdapterForHosts( CNetworkInterfacePtr pInterface, bool mapping)
{
  // calculate the delay in us between packets (200ms)
  int transmit_delay_us = 200 * 1000;
  
  void* buff;
  int sock;
  
  int timeout = 3000;
  int more_to_send = 1;
  int broadcast_sent = 0;
  
  struct sockaddr_in dest_sockaddr;
  struct in_addr bcast_addr, local_addr;  /* in network byte order */
  
  struct timeval select_timeout, transmit_time;
  
  fd_set fdsr, fdsw;

  struct list* wg_members;
  struct list* scanned;
  int active_hosts = 0;

  std::vector< CStdString > workgroups;
  size_t scanned_workgroups = 0;

  scan_row_t mapping_row;
  bzero(&mapping_row, sizeof(scan_row_t));
  
  if( mapping )
  {
    std::vector<scan_row_t>::const_iterator it = m_scanTable[pInterface].begin();
    if( it != m_scanTable[pInterface].end() )
    {
      mapping_row = *( it );
    }
    else
    {
      // nothing to scan
      return -1;
    }
  }
  
  scanned = new_list();
  wg_members = new_list();  

  /*
   * List of workgroups to search/searched
   */
  
  if( !mapping && 0 != m_strWorkgroup[0] )
  {
    workgroups.push_back( m_strWorkgroup );
  }

  /*
   * Create the socket and our fdsets for the select loop
   */
  inet_aton( pInterface->GetCurrentIPAddress().c_str(), &local_addr );
  inet_aton( pInterface->GetCurrentBroadcastAddress().c_str(), &bcast_addr );
  
  sock = CreateSocket( local_addr );
  if( -1 == sock )
  {
    return -1;
  }
  
  FD_ZERO( &fdsr );
  FD_SET( sock, &fdsr );
          
  FD_ZERO( &fdsw );
  FD_SET( sock, &fdsw );

  /*
   * Set up our read timeout (timeout in ms) and our read buffer
   */
  select_timeout.tv_sec = timeout / 1000;
  select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */

  buff = malloc(PKT_BUFFER_SIZE);
  if(!buff) 
  {
    CLOSE_SOCKET(sock);
    return -1;
  }

  /* Send queries, receive answers and print results */
  /***************************************************/
       
  /****
   * The select loop works like this
   *
   * We wait for the ability to either read or write on the socket. We read first so we can clear space
   * for future responses, and parse whatever we get, then issue whatever writes we have remaining
   *
   * If we are mapping the subnet, then we don't employ most of the methods; we stick with direct traffic based on the specified range
   * If we are running normally, we track workgroups and try the other techniques
   */

  // Lie and say we've scannned our IP; we do this to avoid reporting back that we found ourselves
  insert( scanned, local_addr.s_addr );
  
  /* seed this before the first send */
  gettimeofday( &transmit_time, NULL);

  while( !g_application.m_bStop && m_pScanProcessor->IsRunning() && ( select( sock+1, &fdsr, &fdsw, NULL, &select_timeout ) ) > 0 )
  {
    /********
     * Read block
     */
    if( FD_ISSET(sock, &fdsr) )
    {
      struct nb_host_info* hostinfo = NULL;
      int is_nbtstat = 0;
      
      socklen_t addr_size = sizeof(struct sockaddr_in);
      int size = recvfrom( sock, (char*)buff, PKT_BUFFER_SIZE, 0, (struct sockaddr*)&dest_sockaddr, &addr_size );
      if( size < 0 )
      {
        continue;
      }

      /* 
       * if it's nbtstat we process the name and issue FoundActiveIP
       * all other cases we store the IP
       */
      if( htons(NBT_PORT) == dest_sockaddr.sin_port )
      {
        hostinfo = parse_response( (char*)buff, size );
        if( !hostinfo )
        {
          continue;
        }
        
        if( NB_RESPONSE_TYPE != hostinfo->header->question_type )
        {
          is_nbtstat = 1;
        }
      }
      
      
      if( is_nbtstat )
      {
        /* We have a potential name to store; do the dupe check here */
        if( insert( scanned, ntohl( dest_sockaddr.sin_addr.s_addr ) ) )
        {
          CStdString hostname, remote_ip, wg;
          
          if( ParseNBTSTAT( hostinfo, hostname, wg ) )
          {
            remote_ip = inet_ntoa(dest_sockaddr.sin_addr);
            ++active_hosts;

            // Save off the workgroup name so we can scan this workgroup later
            if(!wg.IsEmpty() && !mapping && wg[0] )
            {
              std::vector< CStdString >::iterator w = workgroups.begin();
              while( w != workgroups.end() )
              {
                if( *w == wg )
                  break;
                w++;
              }
              if( w == workgroups.end() )
              {
                workgroups.push_back(wg);
                more_to_send++;
              }
            }
          
#ifdef HAS_INTEL_SMD
            // On embedded we ignore shares from the local machine
            // since we only have one interface this check is sufficient
            // ignore wg folder
            if( remote_ip == pInterface->GetCurrentIPAddress() || (hostname == wg))
            {
              CLog::Log(LOGDEBUG, "Ignoring file shares from local host and workgroup folder [%s]", hostname.c_str());
            }
            else
#endif
            {
              // We found an IP and have the hostname; time to queue the scanner to find shares
              CStdString macAddr =  pInterface->GetMacAddress();
              FoundActiveIP( macAddr, remote_ip, hostname, wg, mapping );
            }
          }
          else
          {
            CLog::Log(LOGDEBUG, "Found IP %s via netbios but no fileserver available\n", inet_ntoa(dest_sockaddr.sin_addr) );
          }
        }
      }
      else
      {
        /* store the remote IP for later in network byte order; the loop will process the wg_members */
        /* weed out dupes here in case someone responds multiple times */
        /* never hit if mapping == true as the mapper only uses nbtstat */
        if( !is_in_list( scanned, ntohl( dest_sockaddr.sin_addr.s_addr ) ) )
        {
          insert( wg_members, dest_sockaddr.sin_addr.s_addr );
          more_to_send++;
        }
      }

      // Cleanup the nb/nbstat packet
      if( hostinfo )
      {
        free_hostinfo( hostinfo );
        hostinfo = NULL;
      }
    }
    
    /*************
     * Send block
     */
    
    if( more_to_send && FD_ISSET( sock, &fdsw ) )
    {
      /*
       * Throttle our overall traffic
       */
      if( CheckElapsedTime( &transmit_time, transmit_delay_us ) )
      {
        if( mapping )
        {
          // Step through the mapping table. We update the map to reflect where we are currently at...
          std::set<scan_row_t>::size_type sz = m_scanTable[pInterface].size();
          if( sz > 0 )
          {
            /* Scan the next address in our range
             */
            if( !is_in_list( scanned, mapping_row.start.s_addr ) ) 
            {
              struct in_addr a;
              a.s_addr = htonl(mapping_row.start.s_addr);
              send_query( sock, a, transmit_time.tv_usec, NULL, 0 );
            }
            if( mapping_row.start.s_addr == mapping_row.end.s_addr )
            {
              m_scanTable[pInterface].erase( m_scanTable[pInterface].begin() );
              if( m_scanTable[pInterface].begin() != m_scanTable[pInterface].end() )
              {
                mapping_row = *( m_scanTable[pInterface].begin() );
              }
            }
            else
            {
              mapping_row.start.s_addr++; // step through the range
            }
          }
          else
          {
            more_to_send = 0;
            /* timeout is in milliseconds */
            select_timeout.tv_sec = timeout / 1000;
            select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
          }
        }
        else
        {
          /* If we haven't sent the broadcast, then do that
           * Otherwise, if we have workgroups to scan, do those
           * Otherwise, if we have nodes to nbstat, then do those
           */
      
          if( !broadcast_sent )
          {
            /* Send our NBTSTAT for * and our WS-Discovery probe at the same time
             */
            send_query( sock, bcast_addr, transmit_time.tv_usec, NULL, 0);
        
            SendDiscoveryProbe( sock, local_addr );
        
            broadcast_sent = 1;
          }
          else if( scanned_workgroups < workgroups.size() )
          {
            /* Scan one workgroup at a time for new IPs
             * we will send the next broadcast the next time we iterate through the loop
             */
            const char* wg_name = workgroups[scanned_workgroups++].c_str();
        
            send_query( sock, bcast_addr, transmit_time.tv_usec, wg_name, 1 );
          }
          else if( !is_list_empty( wg_members ) )
          {
            /* Scan the specific node, whose IP we obtained through some other method
             * Similar to the wg scan, we'll do one of these per pass
             */
            struct in_addr node;
        
            head( wg_members, (unsigned long*)&( node.s_addr ), 1 );
            send_query( sock, node, transmit_time.tv_usec, NULL,  0 );
          }
          else
          {
            more_to_send = 0;
            /* timeout is in milliseconds */
            select_timeout.tv_sec = timeout / 1000;
            select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
          }
        }
        /* At this point we've sent something, excepting the case where the address
         * is already in the scanned list (edge case) so update our tx time
         */
        gettimeofday( &transmit_time, NULL);
      }
      else
      {
        /* This avoids the case where we are doing sends in a tight loop because
         * no read/processing is coming back; worst case we cycle through 2 times
         * per send.
         */
        usleep( transmit_delay_us >> 1 );
      }
    }
    
    // Reset the selectors; only set the write one if we will actually be writing
    FD_ZERO( &fdsw );
    FD_ZERO( &fdsr );
    FD_SET( sock, &fdsr );
    if( more_to_send )
    {
      FD_SET( sock, &fdsw );
    }
  }

  delete_list( scanned );
  delete_list( wg_members );
  workgroups.clear();
  
  CLOSE_SOCKET( sock );
  free( buff );
  return active_hosts;
}

bool CIPScanner::ParseNBTSTAT(nb_host_info* hostinfo, CStdString& hostname, CStdString& workgroup)
{
  int done = 0;
  
  if(!hostinfo)
    return false;

  if(!hostinfo->header)
	return false;

  /* There are no order guarantees. Loop through all names finding the workgroup
   * and machine names
   */
  for( int idx = 0; done != 3 && idx < hostinfo->header->number_of_names; idx++ )
  {
    bool group = (NBT_GROUP_FLAG == ( hostinfo->names[idx].rr_flags & NBT_GROUP_FLAG ));
    // Verify that the group flag is set and that the string type == domain name
    if( group && NBS_DOMAINNAME == hostinfo->names[idx].ascii_name[15] )
    {
      workgroup = hostinfo->names[idx].ascii_name;
      workgroup = workgroup.Trim();

      done |= 1;
    }
    else if( !group && NBS_FILESERVER == hostinfo->names[idx].ascii_name[15] )
    {
      CStdString hn( hostinfo->names->ascii_name, 15);
      hostname = hn.Trim();

      done |= 2;
    }
  }

  // Return true if we at least found a hostname
  return 2 == (done & 2);
}

// create our socket
int CIPScanner::CreateSocket(struct in_addr local_ip)
{
  int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
  
  if( sock >= 0 )
  {
    // Note that on macos and linux this is a const void*, but on windows it's const char*...
    int val = 1;
    if( 0 != setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (const char*)&val, sizeof(val) ) )
    {
      CLOSE_SOCKET( sock );
      sock = -1;
    }
  }

  if( sock >= 0 )
  {
    struct sockaddr_in src_sockaddr;
    bzero( (void*)&src_sockaddr, sizeof(src_sockaddr) );

    src_sockaddr.sin_family = AF_INET;
    src_sockaddr.sin_addr = local_ip;
  
    if( 0 != bind( sock, (struct sockaddr*)&src_sockaddr, sizeof(src_sockaddr) ) )
    {
      CLOSE_SOCKET( sock );
      sock = -1;
    }
  }
  
  if( sock < 0 )
  {
    CLog::Log(LOGERROR, "Failed to create a socket for SMB share discovery\n");
  }
  return sock;
}



/* this is not to the rfc, but it is good enough for our needs */
void CIPScanner::Uuidgen(char* uuid)
{
  char set[] = "0123456789abcdef";
  int i, idx = 0;
  for(i = 0; i < 8; i++)
  {
    short val = rand() % 0xffff;
    uuid[idx++] = set[ (val>>12) & 0xf];
    uuid[idx++] = set[ (val>> 8) & 0xf];
    uuid[idx++] = set[ (val>> 4) & 0xf];
    uuid[idx++] = set[ (val>> 0) & 0xf];
    if( idx == 8 || idx == 13 || idx == 18 || idx == 23 )
    uuid[idx++] = '-';
  }
}

// Send a probe message per http://specs.xmlsoap.org/ws/2005/04/discovery/
// We search for the 'pub:Computer' type which is advertised by Vista and Win7 machines with sharing enabled
// This will match regardless of workgroup settings, so it should be a superset of the machines we get with
// the nbt scanning
int CIPScanner::SendDiscoveryProbe(int sock, struct in_addr local_ip)
{
  // The content in a probe is very static, just the message ID (a uuid) needs to change with each probe
  const char* format =
    "<s:Envelope xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "\
    "xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "\
    "xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "\
    "xmlns:p=\"http://schemas.microsoft.com/windows/pub/2005/07\">"\
    "<s:Header>"\
    "<a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action>"\
    "<a:MessageID>uuid:%s</a:MessageID>"\
    "<a:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>"\
    "</s:Header>"\
    "<s:Body><d:Probe>"\
    "<d:Types>p:Computer</d:Types>" \
    "</d:Probe></s:Body>"\
    "</s:Envelope>";
    
    
  char packet[640];  
  char uuid[37];
  int len,status;
  struct sockaddr_in dest_sockaddr;
  
  // Build our uuid and print it into our packet
  uuid[36] = 0;
  Uuidgen(uuid);
  sprintf(packet, format, uuid);
  
  len = strlen(packet);
  bzero((void*)&dest_sockaddr, sizeof(dest_sockaddr));
  dest_sockaddr.sin_family = AF_INET;
  dest_sockaddr.sin_port = htons(3702);
  dest_sockaddr.sin_addr.s_addr = htonl( 0xeffffffa ); /* 239.255.255.250 */
 
  /* bind multicast to the proper interface here. osx/linux will pick the 'most likely' adapter otherwise
   * (which is usually the one with the default route). windows will pick a random adapter (sometimes the
   * loopback one - seriously)
   */
  status = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&local_ip, sizeof(local_ip) );
  if( 0 == status )
  {
    status = sendto(sock, packet, len, 0, (struct sockaddr*)&dest_sockaddr, sizeof(dest_sockaddr));
    if( len == status )
      status = 0;
  }
      
  if( 0 == status )
  {
    usleep(75*1000); /* wait for 75ms; a little on the short side but still in spec */
    status = sendto(sock, packet, len, 0, (struct sockaddr*)&dest_sockaddr, sizeof(dest_sockaddr));
    if( len == status )
      status = 0;
  }

  if( 0 != status )
  {
    CLog::Log( LOGDEBUG, "Failed to issue ws-discovery, status %d", status );
  }
  
  return status;
}

bool CIPScanner::CheckElapsedTime( struct timeval* last_timestamp, int min_elapsed_us )
{
  /* this won't handle min_elapsed_us >= 1 second */
  struct timeval elapsed;
  gettimeofday( &elapsed, NULL );
        
  /* Calculate time elapsed */
  elapsed.tv_sec -= last_timestamp->tv_sec;
  elapsed.tv_usec -= last_timestamp->tv_usec;
  if( elapsed.tv_usec < 0 )
  {
    elapsed.tv_sec--;
    elapsed.tv_usec += (1000000);
  }
  return elapsed.tv_sec || ( elapsed.tv_usec > min_elapsed_us );
}

#ifdef _WIN32
CIPScanJob::CIPScanJob( const char* subnet, const char* ip_address, const char* hostname, IIPScanCallback* pCallback ) : BXBGJob("CIPScanJob"),
  m_pCallback( pCallback ), m_strIP( ip_address ), m_strSubnet(subnet),
    m_strHostname( hostname )
{
  m_strHostname.Trim();
}
  
void CIPScanJob::DoWork()
{
  CStdString    smbUrl = "smb://" + m_strIP;
  CFileItemList list;

  CLog::Log( LOGDEBUG, "Looking for shares in ip %s", m_strIP.c_str() );
  CDirectory::GetDirectory( smbUrl, list );

  int nSize = list.Size();
  
  for( int i = 0; i < nSize && !g_application.m_bStop; ++i )
  {
    CFileItemPtr pFileItem = list[ i ];
    
    FormatShareString( pFileItem, smbUrl );
    pFileItem->SetLabel( smbUrl );
    
    CURI u( pFileItem->m_strPath );

    pFileItem->m_strPath = "smb://" + m_strHostname + "/"+u.GetShareName();
  }
  
  CFileItemPtr pHostItem ( new CFileItem );

  pHostItem->m_strPath = "smb://" + m_strHostname;
  pHostItem->SetLabel( m_strHostname );
  pHostItem->m_bIsFolder = true;
  list.Add(pHostItem);
  list.SetProperty( "HostIP", m_strIP );
  //list.SetProperty( "Subnet", m_strSubnet );

  //list.SetProperty( "Interface", m_strLocalInterface );
  //list.SetProperty( "HostName", m_strRemoteHostname );
  //list.SetProperty( "Workgroup", m_strWorkgroup);

  if (!g_application.m_bStop)
    m_pCallback->IPScanned( list );
}


void CIPScanJob::FormatShareString( CFileItemPtr fileItem, CStdString& strUrl ) const
{
  // from smb://<hostname>/<sharename>/ we will make <sharename> on <hostname>.
  CStdString strPath = fileItem->m_strPath;
  CUtil::RemoveSlashAtEnd(strPath);
  CStdString strShare = CUtil::GetFileName(strPath);
  strUrl.Format( "%s / %s", m_strHostname.c_str(), strShare.c_str() );
}

#endif

std::set<CStdString> CSMBScanState::GetIPs()
{
	std::set<CStdString> setIPs;
	std::map<CStdString, CFileItemList>::const_iterator iter = m_smbShares.begin();
    while (iter != m_smbShares.end())
    {
      //Add current smb share ip to the delete candidates list only if in the current interface subnet
        setIPs.insert(iter->first);
      iter++;
    }
	return setIPs;
}