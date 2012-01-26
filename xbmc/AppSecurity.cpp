#include "system.h"

#ifdef HAS_EMBEDDED

#include "SpecialProtocol.h"
#include "utils/log.h"
#include "Util.h"
#include "bxxmldocument.h"
#include "File.h"
#include "HalServices.h"
#include "BoxeeUtils.h"
#include "AppSecurity.h"

#ifdef DEVICE_PLATFORM
#include "bxoemconfiguration.h"
#endif

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include "utils/Base64.h"

#define PUBLIC_KEY_LEN        272
#define SIGNATURE_LEN_BASE64  175
#define SIGNATURE_LEN         128

//#define DEBUG

//#define SERVER_URL "http://staging.boxee.tv/api/"
//#define PYTHON_APPS_PUBLIC "/download/boxee/python-apps-public.pem"
#define PYTHON_APPS_PUBLIC "special://xbmc/system/python-apps-public.pem"
#define SERVER_URL "https://app.boxee.tv/api/"

typedef struct AppInfo
{
  CStdString signature;
  CStdString authority;

  CStdString appId;
  CStdString appVersion;
} AppInfo;

typedef struct AuthorityInfo
{
  CStdString publicKey;
  CStdString signature;

} AuthorityInfo;

typedef struct DeviceInfo
{
  CStdString type;
  CStdString serial;
  CStdString mac;

} DeviceInfo;

CAppSecurity::CAppSecurity(const CStdString& appFilename, const CStdString& appSecurityManifest, const CStdString& appId, const CStdString& appVersion)
	: m_appFilename(appFilename), 
          m_appSecurityManifest(appSecurityManifest),
          m_appId(appId), 
          m_appVersion(appVersion)
{
}

CAppSecurity::~CAppSecurity()
{
}

#if 0 
static char twitTestAuthManifest[] = {"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\
<authority id=\"boxee.tv\">\n\
<publickey>\
-----BEGIN PUBLIC KEY-----\n\
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCxVsibE8x6XTJd0pj3xSEnZKhs\n\
BfHSCBXJcN2wz8kqBt3ueRm65GUneCy1l3L4dBHMCj2bvZMNpylGfdMsCAli6LKi\n\
7tBY7IU+SXVzdkcK+F/JG6362sk5L7MMUp6f9Ak+vwC/KtmU+9odA59brobQHe5m\n\
YXV0oiBZAPhgBVqhSwIDAQAB\n\
-----END PUBLIC KEY-----\n\
</publickey>\n\
<signature>\
Awm2eqbL8U5JFka4FCNz8DEDbtK7w3CPcWEc7uj6nuJ75NzOPnqwVJZX1x4GJRe/\n\
KzLsK/Y2YZ40xHizqwsFtGWFI4IcEbyThx16VeOFdor5x8LkWG73BqzECpY6ic+0\n\
BrPGxdtT+g3uyLtO+LJTxbOD6cpuBToUnzPQlwxqWPs=\n\
</signature>\n\
<app id=\"twit-video\" ver=\"1.6\">\n\
<status>open</status>\n\
</app>\n\
</authority>"
};
#endif

#if 0
static void dump_sha1(unsigned char hash[SHA_DIGEST_LENGTH])
{
 char outputBuffer[65];
  for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
  {
   sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
  }
  outputBuffer[64] = 0;
  printf("%s\n", outputBuffer);
}
#endif

static bool ReadDeviceCertificate(const CStdString& deviceCertificate, AppInfo* appInfo)
{
  TiXmlDocument xmlDoc;

  if(!xmlDoc.LoadFile(deviceCertificate))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - Failed to load %s", __func__, deviceCertificate.c_str());
    return false;
  }

  TiXmlElement* root = xmlDoc.RootElement();
  if(!root)
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Failed to get root from BXXMLDocument of %s", __func__, deviceCertificate.c_str());
    return false;
  }

  CStdString type, serial, mac;
  while (root)
  {
    if(strcmp(root->Value(),"device") == 0)
    {
      type = root->Attribute("type");
      serial = root->Attribute("sn");
      mac = root->Attribute("mac");

      root = root->FirstChildElement();
      continue;
    }
    else if(strcmp(root->Value(),"signature") == 0)
    {
      appInfo->signature = root->FirstChild()->Value();
    }

    root = root->NextSiblingElement();
  }  

  bool bVerified = false;
  do
  {
     if(appInfo->signature.IsEmpty())
     {
       CLog::Log(LOGERROR,"CAppSecurity::%s - Device signature is empty", __func__);
       break;
     }

    bVerified = true;

  } while(false);

  return bVerified;
}

static bool ReadAppSecurityManifest(const CStdString& appSecurityManifestFile, bool bTestApp, AppInfo* appInfo)
{
  if(bTestApp)
  {
     if(ReadDeviceCertificate(appSecurityManifestFile, appInfo))
       return true;
  }

  TiXmlDocument xmlDoc;

  if(!xmlDoc.LoadFile(appSecurityManifestFile))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - Failed to load %s", __func__, appSecurityManifestFile.c_str());
    return false;
  }  

  TiXmlElement* root = xmlDoc.RootElement();
  if(!root)
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Failed to get root from BXXMLDocument of %s", __func__, appSecurityManifestFile.c_str());
    return false;
  }

  CStdString appId, appVersion, authority, signature;
  while (root)
  {
    if(strcmp(root->Value(),"app") == 0)
    {
      appId = root->Attribute("id");
      appVersion = root->Attribute("version");

      root = root->FirstChildElement();
      continue;
    }
    else if(strcmp(root->Value(),"signature") == 0)
    {
      appInfo->signature = root->FirstChild()->Value();
    }
    else if(strcmp(root->Value(),"authority") == 0)
    {
      appInfo->authority = root->FirstChild()->Value();
    }

    root = root->NextSiblingElement();    
  }

  bool bVerified = false;

  do
  {
     if(appId != appInfo->appId)
     {
       CLog::Log(LOGERROR,"CAppSecurity::%s - App id doesn't match [%s]![%s]", __func__, appId.c_str(), appInfo->appVersion.c_str());
       break;
     }

     if(appVersion != appInfo->appVersion)
     {
       CLog::Log(LOGERROR,"CAppSecurity::%s - App version doesn't match [%s]![%s]", __func__, appVersion.c_str(), appInfo->appVersion.c_str());
       break;
     }


     if(appInfo->signature.IsEmpty())
     {
       CLog::Log(LOGERROR,"CAppSecurity::%s - App signature is empty, app id [%s]", __func__, appId.c_str());
       break;
     }

     if(appInfo->authority.IsEmpty())
     {
       CLog::Log(LOGERROR,"CAppSecurity::%s - App authority is empty, app id [%s]", __func__, appId.c_str());
       break;
     }

     bVerified = true;
    
  } while(false);

  return bVerified;
}

static bool ReadAuthorityInfo(const AppInfo& appInfo, AuthorityInfo* authoritySecurityInfo)
{  
  if(appInfo.authority.IsEmpty())
    return false;
  
  CStdString authorityManifestURL = SERVER_URL;
  
  authorityManifestURL += "getappinfo/?";
  authorityManifestURL += "authorityid=" + appInfo.authority;
  authorityManifestURL += "&";
  authorityManifestURL += "appid=" + appInfo.appId;
  authorityManifestURL += "&";
  authorityManifestURL += "ver=" + appInfo.appVersion;

  BOXEE::BXCurl curl;
  std::string strResp = curl.HttpGetString(authorityManifestURL, false);

  TiXmlDocument xmlDoc;
  BOXEE::BXXMLDocument reader;

  if (strResp.empty())
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Not handling server response to [authorityManifestURL=%s] because it is empty", __func__, authorityManifestURL.c_str());
    return false;
  }

  if(!reader.LoadFromString(strResp))
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Not handling server response to [authorityManifestURL=%s] because failed to load it to BXXMLDocument", __func__, authorityManifestURL.c_str());
    return false;
  }

  TiXmlElement* root = reader.GetRoot();
  if(!root)
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Failed to get root from BXXMLDocument of %s", __func__, authorityManifestURL.c_str());
    return false;
  }
  
  CStdString authorityid, appId, appVersion, status;

  while (root)
  {
    if(strcmp(root->Value(),"authority") == 0)
    {
      authorityid = root->Attribute("id");

      root = root->FirstChildElement();
      continue;
    }
    else if(strcmp(root->Value(),"app") == 0)
    {
       appId = root->Attribute("id");
       appVersion = root->Attribute("ver");

       root = root->FirstChildElement();
       continue;
    }
    else if(strcmp(root->Value(), "publickey") == 0)
    {
      authoritySecurityInfo->publicKey = root->FirstChild()->Value();
      authoritySecurityInfo->publicKey += "\n";
    }
    else if(strcmp(root->Value(), "signature") == 0)
    {
      authoritySecurityInfo->signature = root->FirstChild()->Value();
    }
    else if(strcmp(root->Value(), "status") == 0)
    {
      status = root->FirstChild()->Value();
    }

    root = root->NextSiblingElement();
  }

  bool bVerified = false;

  do 
  {
    // Verify that we've got correct info 
    if(authoritySecurityInfo->publicKey.IsEmpty())
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - Invalid manifest file for autority [%s], public element not found", __func__, appInfo.authority.c_str());
      break;
    }

    if(authoritySecurityInfo->signature.IsEmpty())
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - Invalid manifest file for autority [%s], singature element not found", __func__, appInfo.authority.c_str());
      break;
    }

    if(authorityid != appInfo.authority)
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - Authority id doesn't match [%s]![%s]", __func__, authorityid.c_str(), appInfo.authority.c_str());
      break;
    }

    if(appId != appInfo.appId)
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - App id doesn't match [%s]![%s]", __func__, appId.c_str(), appInfo.appId.c_str());
      break;
    }
   
    if(appVersion != appInfo.appVersion)
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - App version doesn't match [%s]![%s]", __func__, appVersion.c_str(), appInfo.appVersion.c_str());
      break;
    }

    if(status == "blocked")
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - App %s is blocked", __func__, appId.c_str());
      break;
    }
    
    bVerified = true;

  } while(false);

  return bVerified; 
}

static bool GetDeviceInfo(DeviceInfo* deviceInfo)
{
  IHalServices& client = CHalServicesFactory::GetInstance();

  CHalHardwareInfo hwInfo;
  if(!client.GetHardwareInfo(hwInfo))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - failed to fetch hardware info", __func__);
    return false;
  }

  CHalEthernetInfo ethInfo;
  if(!client.GetEthernetInfo(0,ethInfo))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - failed to fetch ethernet info", __func__);
    return false;
  }

  deviceInfo->type = BoxeeUtils::GetPlatformStr();
  deviceInfo->serial = hwInfo.serialNumber;
  deviceInfo->mac = ethInfo.mac_address;

  deviceInfo->mac.Replace(":", "-");

  return true;
}

static int unbase64(unsigned char *input, int length, unsigned char **output)
{
  BIO *b64, *bmem;
  int bytesRead = 0;

  unsigned char *buffer = (unsigned char *)malloc(length);
  memset(buffer, 0, length);

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new_mem_buf((void*)input, length);
  bmem = BIO_push(b64, bmem);

  bytesRead = BIO_read(bmem, buffer, length);

  BIO_free_all(bmem);

  *output = buffer;

  return bytesRead;
}

static bool VerifyBuffer(const unsigned char* buffer, const unsigned int bufferLen, 
		         const unsigned char* signatureEncoded, const unsigned int signatureEncodedLen, const CStdString& appsPublicKeyFile)
{
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA_CTX context;
  FILE *fkey = NULL;
  RSA *rsa = NULL;
  bool bSuccedded = false;
  unsigned char* signature = NULL;
  int signatureLen = 0;

  SHA1_Init(&context);
  SHA1_Update(&context, buffer, bufferLen);
  SHA1_Final(hash, &context);

#ifdef DEBUG
  dump_sha1(hash);
#endif

  do
  {

    signatureLen = unbase64((unsigned char*)signatureEncoded, signatureEncodedLen, &signature);
    if(!signatureLen)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to decode signature [%s]", __func__, signatureEncoded);
      break;
    }

    fkey = fopen(_P(appsPublicKeyFile), "r");
    if (!fkey)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to open publickey file [%s]", __func__, appsPublicKeyFile.c_str());
      break;
    }

    PEM_read_RSA_PUBKEY(fkey, &rsa, NULL, NULL);
    if (!rsa)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to load rsa key from file [%s]", __func__, appsPublicKeyFile.c_str());
      break;
    }

    bSuccedded = RSA_verify(NID_sha1, hash, SHA_DIGEST_LENGTH, signature, signatureLen, rsa);


  } while(false);

  if (rsa)       RSA_free(rsa);
  if (fkey)      fclose(fkey);
  if (signature) free(signature);

  return bSuccedded;  
}

static bool VerifyAuthoritySignature(const AuthorityInfo& authorityInfo, const CStdString& appsPublicKeyFile)
{
  unsigned char* buffer = (unsigned char*)authorityInfo.publicKey.data();
  unsigned int   bufferLen = authorityInfo.publicKey.size();
  unsigned char* signatureEncoded = (unsigned char*)authorityInfo.signature.c_str();
  unsigned int   signatureEncodedLen = authorityInfo.signature.size();

  return VerifyBuffer(buffer, bufferLen, signatureEncoded, signatureEncodedLen, appsPublicKeyFile);
}
 
static bool VerifyDeviceSignature(const AppInfo& securityInfo, const CStdString& appsPublicKeyFile)
{
  DeviceInfo deviceInfo;
  if(!GetDeviceInfo(&deviceInfo))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - failed to build device descriptor", __func__);
    return false;
  }  

  CStdString deviceDescriptor;
  deviceDescriptor = deviceInfo.type;
  deviceDescriptor += "&";
  deviceDescriptor += deviceInfo.serial;
  deviceDescriptor += "&";
  deviceDescriptor += deviceInfo.mac;
  deviceDescriptor += "\n";

  unsigned char* buffer = (unsigned char*)deviceDescriptor.c_str();
  unsigned int bufferLen = deviceDescriptor.size();
  unsigned char* signature = (unsigned char*)securityInfo.signature.c_str();
  unsigned int signatureLen = securityInfo.signature.size();
 
  bool bVerified = VerifyBuffer(buffer, bufferLen, signature, signatureLen, appsPublicKeyFile);
 
  if(!bVerified)
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - FAILED to verify device signature", __func__);
    return false;
  } 

  // now verify the device with server
  CStdString deviceInfoRequest = SERVER_URL;

  deviceInfoRequest += "getdeviceinfo/?";
  deviceInfoRequest += "type=" + deviceInfo.type;
  deviceInfoRequest += "&";
  deviceInfoRequest += "sn=" + deviceInfo.serial;
  deviceInfoRequest += "&";
  deviceInfoRequest += "mac=" + deviceInfo.mac;

  BOXEE::BXCurl curl;
  std::string strResp = curl.HttpGetString(deviceInfoRequest, false);

  TiXmlDocument xmlDoc;
  BOXEE::BXXMLDocument reader;

  if (strResp.empty())
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Not handling server response to [deviceInfoRequest=%s] because it is empty", __func__, deviceInfoRequest.c_str());
    return false;
  }

  if(!reader.LoadFromString(strResp))
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Not handling server response to [authorityManifestURL=%s] because failed to load it to BXXMLDocument", __func__, deviceInfoRequest.c_str());
    return false;
  }

  TiXmlElement* root = reader.GetRoot();
  if(!root)
  {
    CLog::Log(LOGERROR,"CAppSecurity::%s - Failed to get root from BXXMLDocument of %s", __func__, deviceInfoRequest.c_str());
    return false;
  }   

  CStdString type, serial, mac, status;

  while (root)
  {
    if(strcmp(root->Value(),"device") == 0)
    {
      type = root->Attribute("type");
      serial = root->Attribute("sn");
      mac = root->Attribute("mac");

      root = root->FirstChildElement();
      continue;
    }
    else if(strcmp(root->Value(),"status") == 0)
    {
      status = root->FirstChild()->Value();
    }

    root = root->NextSiblingElement();
  }

  bVerified = false;

  do
  {
    if(type != deviceInfo.type)
    {  
      CLog::Log(LOGERROR,"CAppSecurity::%s - Device type doesn't match [%s]![%s]", __func__, type.c_str(), deviceInfo.type.c_str());
      break;
    }
  
    if(serial != deviceInfo.serial)
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - Device serial doesn't match [%s]![%s]", __func__, serial.c_str(), deviceInfo.serial.c_str());
      break;
    }

    if(mac != deviceInfo.mac)
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - Device mac doesn't match [%s]![%s]", __func__, mac.c_str(), deviceInfo.mac.c_str());
      break;
    }
   
    if(status != "open")
    {
      CLog::Log(LOGERROR,"CAppSecurity::%s - Device %s is locked", __func__, deviceInfo.serial.c_str());
      break;
    }
 
    bVerified = true;
  
  } while(false);

  return bVerified;
}

static bool VerifyAppSignature(const CStdString& appFile, const AppInfo& securityInfo, const AuthorityInfo& authorityInfo)
{
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA_CTX context;
  RSA *rsa = NULL;
  BIO *bio = NULL;
  bool bSuccedded = false;
  unsigned char buffer[4096];
  unsigned char* signature = (unsigned char*)securityInfo.signature.c_str();
  int signatureLen = securityInfo.signature.size();
  XFILE::CFile file;
  unsigned int bytesRead = 0;

  if(!file.Open(appFile))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s - failed to open file [%s]", __func__, appFile.c_str());
    return false;
  }

  SHA1_Init(&context);

  while((bytesRead = file.Read(buffer, sizeof buffer)))
  {
    SHA1_Update(&context, buffer, bytesRead);
  }

  SHA1_Final(hash, &context); 
  file.Close();

#ifdef DEBUG
  dump_sha1(hash);
#endif

  do
  {

    signatureLen = unbase64((unsigned char*)securityInfo.signature.c_str(), securityInfo.signature.size(), &signature); 
    if(!signatureLen)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to decode signature [%s]", __func__, securityInfo.signature.c_str());
      break;
    }

    bio = BIO_new_mem_buf((void*)authorityInfo.publicKey.data(), authorityInfo.publicKey.size());
    if(!bio)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to create BIO from public key[%s]", __func__, authorityInfo.publicKey.c_str());
      break;
    }

    PEM_read_bio_RSA_PUBKEY(bio, &rsa, NULL, NULL);
    if(!rsa)
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to create RSA public key from BIO", __func__);
      break;
    }
    
    bSuccedded = RSA_verify(NID_sha1, hash, SHA_DIGEST_LENGTH, signature, signatureLen, rsa);

   
  } while(false);


  if (rsa)       RSA_free(rsa);
  if (bio)       BIO_free(bio);
  if (signature) free(signature);

  return bSuccedded;
}

bool CAppSecurity::VerifySignature(int flags)
{
  if( (!(flags & FLAG_VERIFY_APP_STATUS_ONLY) && !XFILE::CFile::Exists(m_appFilename)) || 
      !XFILE::CFile::Exists(m_appSecurityManifest))
  {
    CLog::Log(LOGERROR, "CAppSecurity::%s Invalid manifest or app file", __func__);
    return false;
  }

  bool bVerified = false;
  bool bTestApp = flags & FLAG_TEST_APP;
  do
  {

    AppInfo appInfo;

    appInfo.appId = m_appId;
    appInfo.appVersion = m_appVersion;

    if(!ReadAppSecurityManifest(m_appSecurityManifest, bTestApp, &appInfo))
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to read app security manifest", __func__);
      break;
    } 
  
    if(bTestApp)
    {
      bVerified = VerifyDeviceSignature(appInfo, PYTHON_APPS_PUBLIC);
      break;
    }

    AuthorityInfo authorityInfo;
    if(!ReadAuthorityInfo(appInfo, &authorityInfo))
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to read authority manifest", __func__);
      break;
    } 

    if(flags & FLAG_VERIFY_APP_STATUS_ONLY)
    {
      bVerified = true;
      break;
    }

    if(!VerifyAuthoritySignature(authorityInfo, PYTHON_APPS_PUBLIC))
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to verify authority public key", __func__);
      break;
    }

    if(!VerifyAppSignature(m_appFilename, appInfo, authorityInfo))
    {
      CLog::Log(LOGERROR, "CAppSecurity::%s - failed to verify signature app [%s]", __func__, m_appFilename.c_str());
      break;
    }

    bVerified = true;

  } while(false);

  if(bVerified)
  {
    CLog::Log(LOGINFO, "CAppSecurity::%s - Succesfully verified signature for %s app [%s]", __func__, bTestApp ? "test" : "", m_appId ? m_appId.c_str() : m_appFilename.c_str());
  }

  return bVerified;       
}

#endif
