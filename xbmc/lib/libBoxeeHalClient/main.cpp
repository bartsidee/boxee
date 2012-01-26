#include <stdio.h>
#include "BoxeeHalClient.h"


static void print_vector( const StringMapVector& vecMap )
{
  StringMapVector::const_iterator it = vecMap.begin();
  StringMapVector::const_iterator endIt = vecMap.end();
  
  for( ; it != endIt; ++it )
  {
    StringMap::const_iterator i = it->begin();
    StringMap::const_iterator e = it->end();

    printf( "\n" );
    for( ; i != e; ++i )
      printf( "'%s':'%s'\n", i->first.c_str(), i->second.c_str() );
    printf( "\n" );
  }  
}


static void print_map( const StringMap& map )
{
  StringMap::const_iterator i = map.begin();
  StringMap::const_iterator e = map.end();
  
  for( ; i != e; ++i )
    printf( "'%s':'%s'\n", i->first.c_str(), i->second.c_str() );
  printf( "\n" );
}


static void print_devices( const std::map< std::string, StringMapVector >& devices )
{
  std::map< std::string, StringMapVector >::const_iterator iter = devices.begin();
  std::map< std::string, StringMapVector >::const_iterator endIter = devices.end();
  
  for( ; iter != endIter; ++iter )
  {
     const StringMapVector& vecMap = iter->second;
     
     printf( "class: %s\n{\n", iter->first.c_str() );
     StringMapVector::const_iterator it = vecMap.begin();
     StringMapVector::const_iterator endIt = vecMap.end();
     
     for( ; it != endIt; ++it )
     {
        StringMap::const_iterator i = it->begin();
        StringMap::const_iterator e = it->end();

        for( ; i != e; ++i )
          printf( "\t'%s':'%s'\n", i->first.c_str(), i->second.c_str() );
        printf( "\n" );
     }
     printf( "}\n" );
  }  
}


class Listener : public IHalListener
{
  public:
    virtual void HandleNotification(CHalNotification &notification)
    {
//      StringMap::const_iterator iter = map.begin();
//      StringMap::const_iterator endIter = map.end();

      //printf( "\nNew Notification: !!!!\n" );
//      for( ; iter != endIter; ++iter )
//        printf( "%s: %s\t", iter->first.c_str(), iter->second.c_str() );
//      printf( "\nNotification End.\n" );
//
//      return true;
    }
};


int main()
{
  std::map< std::string, StringMapVector > devices;
  IHalServices& client = CHalServicesFactory::GetInstance();

  Listener l ;
  StringMap map;
  StringMapVector vecMap;
    client.ListObjects( devices );
    print_devices( devices );
    devices.clear();

//    return 1;

    client.SetListener( &l );
  printf( "add notification listener.\n");
//  client.SetStorageShare( true, "test", "/opt/local", "", "");
  client.SetClockDateTime(2010U, 7U, 7U, 14U, 0U, 0U);
//  sleep(10); return 1;

//  return 0;
  while(1);

  printf("client.GetAllInputDevices:\n");

  std::vector<CHalInputDevice> inputDevices;
  if( !client.GetAllInputDevices( inputDevices ) )
    printf("  fail\n");
  else
  {
    if( inputDevices.empty() )
      printf( " no input devices.\n" );
    for (size_t i = 0; i < inputDevices.size(); i++)
      printf("  instance: %d label: %s path: %s\n", inputDevices[i].instance, inputDevices[i].label.c_str(), inputDevices[i].path.c_str());
  }

  // ethernet 
  CHalEthernetConfig ethernetConfig;
//  bool rc1;
  bool rc1 = client.GetEthernetConfig( 0, ethernetConfig);
  printf("ethernet config- %d %d %x %x %x %x \n", rc1, ethernetConfig.addr_type,
      ethernetConfig.ip_address.s_addr, ethernetConfig.netmask.s_addr, ethernetConfig.gateway.s_addr, ethernetConfig.dns.s_addr);

  //print_map( map );
  //map.clear();
  printf("///////////////////////////////////////////////\n");
  //sleep(5);
  //client.SetEthernetConfig( unsigned int nInstance, const std::string& strAddressType, 
  //			    const std::string& strIpAddress = "", const std::string& strNetmask = "",
  //			    const std::string& strGateway = "", const std::string& strDns = "" ) const;
  //client.GetEthernetInfo( 0, map );
  CHalEthernetInfo ethernetInfo;
  rc1 = client.GetEthernetInfo( 0, ethernetInfo);
  printf("ethernet info - %d %d %d %d %x %x %x %x \n", rc1, ethernetInfo.running, ethernetInfo.link_up, ethernetInfo.addr_type,
      ethernetInfo.ip_address.s_addr, ethernetInfo.netmask.s_addr, ethernetInfo.gateway.s_addr, ethernetInfo.dns.s_addr);
//  printf("///////////////////////////////////////////////\n");
//  sleep(5);
  // wireless
//  client.GetWirelessConfig( 0, map );
//  print_map( map );
//  map.clear();
  //client.SetWirelessConfig( unsigned int nInstance, const std::string& strAddressType, 
  //			    const std::string& strSsid = "", const std::string& strSecurity = "",
  //			    const std::string& strPassword = "", const std::string& strIpAddress = "",
  //			    const std::string& strNetmask = "", const std::string& strGateway = "",
  //			    const std::string& strDns = "" ) const;
//  client.GetWirelessInfo( 0, map );
//  print_map( map );
//  map.clear();
  // clock 
  //client.SetClockConfig( const std::string& strTimezone, bool bEnableNtp,
  //			 const std::string& strNtpServer, bool bEnableDst ) const;
  std::string strTimezone;
  bool bNtpEnabled;
  std::string strNtpServer;
  bool bDSTEnabled;
  client.GetClockConfig( strTimezone, bNtpEnabled, strNtpServer, bDSTEnabled );
//  sleep (10); return 1;
  std::vector <CHalStorageDeviceInfo> storageDevices;
  client.GetAllStorageDevices( storageDevices);
  for (int i = 0; i < storageDevices.size(); i++)
    printf("storageDevices %d : %s %s %s\n", i, storageDevices[i].fs_type.c_str(), storageDevices[i].path.c_str(), storageDevices[i].label.c_str());

//  client.GetClockConfig( strTimezone, bNtpEnabled, strNtpServer, bDSTEnabled );
  CHalShareInfo storageShare;
  std::string strLabel;
  //client.SetStorageShare( "true", strLabel, storageShare );

//  print_map( map );
//  map.clear();
//  client.SetClockDateTime( 2010, 5, 31, 19, 30, 0 );
  // input
//  client.GetAllInputDevices( vecMap );
//  client.GetInputInfo( 0, map );
//  return 0;
  // storage 
//  client.GetAllStorageDevices( vecMap );
//  print_vector( vecMap );
//  vecMap.clear();
//  client.GetStorageInfo( 0, map );
//  print_map( map );
//  map.clear();
//  client.GetStorageShare( "public", map );
//  print_map( map );
//  map.clear();
//
//    //void EjectStorage( unsigned int nInstance ) const;
//    // host
//  client.SetHostName( "shabobo" );

  return 0; 
}
