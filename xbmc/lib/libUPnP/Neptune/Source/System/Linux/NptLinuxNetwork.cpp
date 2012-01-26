/*****************************************************************
|
|      Neptune - Network :: POSIX Implementation
|
|      (c) 2001-2005 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "NptConfig.h"

//#include <netconfig.h>

#include "NptTypes.h"
//#include "NptStreams.h"
//#include "NptThreads.h"
#include "NptNetwork.h"
//#include "NptUtils.h"
//#include "NptConstants.h"

#if defined(_LINUX) && !defined(__APPLE__)
/*----------------------------------------------------------------------
|       NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces( NPT_List< NPT_NetworkInterface* >& interfaces )
{
  interfaces.Clear();

  FILE* fp = fopen( "/proc/net/dev", "r" );

  if( !fp )
     return NPT_FAILURE;

   char* line = NULL;
   size_t linel = 0;
   int n;
   char* p;
   int linenum = 0;
   int net = socket( AF_INET, SOCK_DGRAM, 0 );

   while( getdelim( &line, &linel, '\n', fp ) > 0 )
   {
      // skip first two lines
      if( linenum++ < 2 )
         continue;

      // search where the word begins
      p = line;
      while( isspace( *p ) )
        ++p;

      // read word until :
      n = strcspn( p, ": \t" );
      p[ n ] = 0;

      // make sure the device has ethernet encapsulation
      struct ifreq ifr;

      strcpy( ifr.ifr_name, p );
      if( ioctl( net, SIOCGIFHWADDR, &ifr ) >= 0 &&
            ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER )
      {
        // get detailed info about the interface
        NPT_Flags flags = 0;

        if( ioctl( net, SIOCGIFFLAGS, &ifr ) < 0 )
          continue;

        // process the flags
        if( ( ifr.ifr_flags & IFF_UP ) == 0 )
            // the interface is not up, ignore it
            continue;

        if( ifr.ifr_flags & IFF_BROADCAST )
            flags |= NPT_NETWORK_INTERFACE_FLAG_BROADCAST;

        if( ifr.ifr_flags & IFF_LOOPBACK )
            flags |= NPT_NETWORK_INTERFACE_FLAG_LOOPBACK;

        if( ifr.ifr_flags & IFF_POINTOPOINT )
            flags |= NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT;

        if( ifr.ifr_flags & IFF_PROMISC )
            flags |= NPT_NETWORK_INTERFACE_FLAG_PROMISCUOUS;

        if( ifr.ifr_flags & IFF_MULTICAST )
            flags |= NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

        // get the mac address
        NPT_MacAddress mac;

        if( ioctl( net, SIOCGIFHWADDR, &ifr ) == 0 )
        {
            NPT_MacAddress::Type mac_addr_type;
            unsigned int         mac_addr_length = IFHWADDRLEN;

            if( ifr.ifr_addr.sa_family == ARPHRD_ETHER )
            {
              mac_addr_type = NPT_MacAddress::TYPE_ETHERNET;
              mac_addr_length = IFHWADDRLEN;
            }

            mac.SetAddress( mac_addr_type, ( const unsigned char* )ifr.ifr_addr.sa_data, mac_addr_length );
        }
        // create an interface object
        NPT_NetworkInterface* interface = new NPT_NetworkInterface( ifr.ifr_name, mac, flags );

        // primary address
        NPT_IpAddress primary_address( ntohl( ( ( struct sockaddr_in* )&ifr.ifr_addr )->sin_addr.s_addr ) );

        // broadcast address
        NPT_IpAddress broadcast_address;

        if( flags & NPT_NETWORK_INTERFACE_FLAG_BROADCAST )
          if( ioctl(net, SIOCGIFBRDADDR, &ifr ) == 0 )
                broadcast_address.Set( ntohl( ( ( struct sockaddr_in* )&ifr.ifr_addr )->sin_addr.s_addr ) );

        // point to point address
        NPT_IpAddress destination_address;

        if( flags & NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT )
            if( ioctl( net, SIOCGIFDSTADDR, &ifr ) == 0)
                destination_address.Set( ntohl( ( ( struct sockaddr_in* )&ifr.ifr_addr )->sin_addr.s_addr ) );

        // netmask
        NPT_IpAddress netmask(0xFFFFFFFF);

        if( ioctl( net, SIOCGIFNETMASK, &ifr ) == 0 )
            netmask.Set( ntohl( ( ( struct sockaddr_in* )&ifr.ifr_addr )->sin_addr.s_addr ) );

        // create the interface object
        NPT_NetworkInterfaceAddress iface_address(
            primary_address,
            broadcast_address,
            destination_address,
            netmask);
        interface->AddAddress(iface_address);

        // add the interface to the list
        interfaces.Add( interface );
      }
   }
   free(line);
   fclose(fp);

    return NPT_SUCCESS;
}
#endif