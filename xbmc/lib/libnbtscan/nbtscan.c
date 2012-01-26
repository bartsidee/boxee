#include <sys/types.h>
#ifdef _LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#else
#include "../../win32/c_defs.h"
#include <WinSock2.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "statusq.h"
#include "range.h"
#include "list.h"
#include "errors.h"
#include "nbttime.h"
#include "nbtscan.h"

int quiet=0;

int is_in_list(struct list* lst, unsigned long content);
#ifdef _WIN32
extern "C"
#endif
int inet_aton(const char *cp, struct in_addr *addr);
int send_query( int sock, struct in_addr dest_addr, uint32_t rtt_base, const char* name, int iponly);

#ifdef _WIN32
/* replacement gettimeofday implementation, copy from dvdnav_internal.h */
#include <sys/timeb.h>
static int _private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#define snprintf _snprintf
#define gettimeofday(TV, TZ) _private_gettimeofday((TV), (TZ))
#define usleep(x) Sleep((x)/1000)

static int sleep(unsigned int sec)
{
  Sleep(sec * 1000);
  return 0;
}

#endif


static int set_range( const char* range_str, struct ip_range* range_struct) {
  if(is_ip(range_str, range_struct)) return 1;
  if(is_range1(range_str, range_struct)) return 1;
  if(is_range2(range_str, range_struct)) return 1;
  return 0;
};

#define BUFFSIZE 1024

#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

int scan_network( const char* target_string, share_host_info* host_info ) {
  int timeout=3000;
  int use137=0, bandwidth=0, send_ok=0;
  char* filename =NULL;
  struct ip_range range;
  void *buff;
  int sock;
  socklen_t addr_size;
  struct sockaddr_in src_sockaddr, dest_sockaddr;
  struct  in_addr *prev_in_addr=NULL;
  struct  in_addr *next_in_addr;
  struct timeval select_timeout, last_send_time, current_time, diff_time, send_interval;
  struct timeval transmit_started, now, recv_time;
  struct nb_host_info* hostinfo = NULL;
  fd_set* fdsr;
  fd_set* fdsw;
  struct list* scanned;
  int size;
  uint32_t rtt_base; /* Base time (seconds) for round trip time calculations */
  float rtt; /* most recent measured RTT, seconds */
  float srtt=0; /* smoothed rtt estimator, seconds */
  float rttvar=0.75; /* smoothed mean deviation, seconds */ 
  double delta; /* used in retransmit timeout calculations */
  int rto, retransmits=0, more_to_send=1, i;
  char errmsg[80];
  char str[80];
  FILE* targetlist=NULL;
  int active_hosts = 0;
  int sentQueries = 0;

  set_range( target_string, &range );
  /* Prepare socket and address structures */
  /*****************************************/
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) 
  {
    CLOSE_SOCKET(sock);
    err_die("Failed to create socket", quiet);
  }

  bzero((void*)&src_sockaddr, sizeof(src_sockaddr));
  src_sockaddr.sin_family = AF_INET;
  if(use137) 
  {
    src_sockaddr.sin_port = htons(NB_DGRAM);
    if (bind(sock, (struct sockaddr *)&src_sockaddr, sizeof(src_sockaddr)) == -1) 
    {
      CLOSE_SOCKET(sock);
      err_die("Failed to bind", quiet);
  }
  }
  
  fdsr=(fd_set*)malloc(sizeof(fd_set));
  if(!fdsr)  
  {
    CLOSE_SOCKET(sock);
    err_die("Malloc failed", quiet);
  }
  
  FD_ZERO(fdsr);
  FD_SET(sock, fdsr);
        
  fdsw=(fd_set*)malloc(sizeof(fd_set));
  if(!fdsw)
  {
    CLOSE_SOCKET(sock);
    err_die("Malloc failed", quiet);
  }
  
  FD_ZERO(fdsw);
  FD_SET(sock, fdsw);

  /* timeout is in milliseconds */
  select_timeout.tv_sec = timeout / 1000;
  select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */

  addr_size = sizeof(struct sockaddr_in);

  next_in_addr = (struct in_addr*)malloc(sizeof(struct  in_addr));
  if(!next_in_addr)
  {
    CLOSE_SOCKET(sock);
    err_die("Malloc failed", quiet);
  }

  buff=malloc(BUFFSIZE);
  if(!buff) 
  {
    CLOSE_SOCKET(sock);
    err_die("Malloc failed", quiet);
  }
	
  /* Calculate interval between subsequent sends */

  timerclear(&send_interval);
  if(bandwidth) send_interval.tv_usec = 
		  (NBNAME_REQUEST_SIZE + UDP_HEADER_SIZE + IP_HEADER_SIZE)*8*1000000 /
		  bandwidth;  /* Send interval in microseconds */
  else /* Assuming 10baseT bandwidth */
    send_interval.tv_usec = 1; /* for 10baseT interval should be about 1 ms */
  if (send_interval.tv_usec >= 1000000) {
    send_interval.tv_sec = send_interval.tv_usec / 1000000;
    send_interval.tv_usec = send_interval.tv_usec % 1000000;
  }
	
  gettimeofday(&last_send_time, NULL); /* Get current time */

  rtt_base = last_send_time.tv_sec; 

  /* Send queries, receive answers and print results */
  /***************************************************/
	
  scanned = new_list();

//  if(!(quiet || verbose || dump || sf || lmhosts || etc_hosts)) print_header();
  for( i = 0; i <= retransmits; ++i ) {
    gettimeofday(&transmit_started, NULL);
    while ( (select(sock+1, fdsr, fdsw, NULL, &select_timeout)) > 0) {
      if(FD_ISSET(sock, fdsr)) {
        if ( (size = recvfrom(sock, (char *)buff, BUFFSIZE, 0,
                              (struct sockaddr*)&dest_sockaddr, &addr_size)) <= 0 ) {
          snprintf(errmsg, 80, "%s\tRecvfrom failed", inet_ntoa(dest_sockaddr.sin_addr));
          err_print(errmsg, quiet);
          continue;
        };
        gettimeofday(&recv_time, NULL);
        hostinfo = parse_response( (char *)buff, size );
        if( !hostinfo || !hostinfo->names || !hostinfo->names->ascii_name) {
          err_print("parse_response returned NULL", quiet);
          if (hostinfo)
            free_hostinfo(hostinfo);
          continue;
        };
				/* If this packet isn't a duplicate */
        if(insert(scanned, ntohl(dest_sockaddr.sin_addr.s_addr))) {
          rtt = recv_time.tv_sec + 
          recv_time.tv_usec/1000000 - rtt_base - 
          hostinfo->header->transaction_id/1000;
          /* Using algorithm described in Stevens' 
           Unix Network Programming */
          delta = rtt - srtt;
          srtt += delta / 8;
          if(delta < 0.0) delta = - delta;
          rttvar += (delta - rttvar) / 4 ;
          host_info->ip = dest_sockaddr.sin_addr;
          memset(host_info->hostname, 0, 16);
          strncpy( host_info->hostname, hostinfo->names->ascii_name, 15 );
          ++active_hosts;
          ++host_info;
        };
        
        free_hostinfo(hostinfo);
      };
      
      FD_ZERO(fdsr);
      FD_SET(sock, fdsr);		
      
      /* check if send_interval time passed since last send */
      gettimeofday(&current_time, NULL);
      timersub(&current_time, &last_send_time, &diff_time);
      send_ok = timercmp(&diff_time, &send_interval, >=);
			
      // send batches of 10 udp packets
      if (sentQueries && (sentQueries % 10 == 0))
      {
        usleep(100000); 
      }
      
      if(more_to_send && FD_ISSET(sock, fdsw) && send_ok) {
        if(targetlist) {
          if(fgets(str, 80, targetlist)) {
            if(!inet_aton(str, next_in_addr)) {
              /* if(!inet_pton(AF_INET, str, next_in_addr)) { */
              fprintf(stderr,"%s - bad IP address\n", str);
            } else {
              if(!is_in_list(scanned, ntohl(next_in_addr->s_addr))) 
              {
                send_query(sock, *next_in_addr, rtt_base);
                sentQueries++;
            }
            }
          } else {
            if(feof(targetlist)) {
              more_to_send=0; 
              FD_ZERO(fdsw);
              /* timeout is in milliseconds */
              select_timeout.tv_sec = timeout / 1000;
              select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
              continue;
            } else {
              snprintf(errmsg, 80, "Read failed from file %s", filename);
              CLOSE_SOCKET(sock);
			  delete_list(scanned);
			  free(buff);
			  free(next_in_addr);
			  free(fdsw);
			  free(fdsr);
              err_die(errmsg, quiet);
            }
          }
        } else if(next_address(&range, prev_in_addr, next_in_addr) ) {
          if(!is_in_list(scanned, ntohl(next_in_addr->s_addr))) 
          {
            send_query(sock, *next_in_addr, rtt_base);
            sentQueries++;
          }
          prev_in_addr=next_in_addr;
          /* Update last send time */
          gettimeofday(&last_send_time, NULL); 
        } else { /* No more queries to send */
          more_to_send=0; 
          FD_ZERO(fdsw);
          /* timeout is in milliseconds */
          select_timeout.tv_sec = timeout / 1000;
          select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
          continue;
        };
      };	
      if(more_to_send) {
        FD_ZERO(fdsw);
        FD_SET(sock, fdsw);
      };
    };
    
    if (i>=retransmits) break; /* If we are not going to retransmit
     we can finish right now without waiting */
    
    rto = (srtt + 4 * rttvar) * (i+1);
    
    if ( rto < 2.0 ) rto = 2.0;
    if ( rto > 60.0 ) rto = 60.0;
    gettimeofday(&now, NULL);
		
    if(now.tv_sec < (transmit_started.tv_sec+rto)) 
      sleep((transmit_started.tv_sec+rto)-now.tv_sec);
    prev_in_addr = NULL ;
    more_to_send=1;
    FD_ZERO(fdsw);
    FD_SET(sock, fdsw);
    FD_ZERO(fdsr);
    FD_SET(sock, fdsr);
  };

  delete_list(scanned);
  
  CLOSE_SOCKET(sock);
  free(buff);
  free(next_in_addr);
  free(fdsw);
  free(fdsr);
  
  return active_hosts;
};

