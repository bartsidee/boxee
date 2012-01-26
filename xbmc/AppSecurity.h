#ifndef _APPSECURITY_H
#define _APPSECURITY_H

#define FLAG_VERIFY_ALL 0x0
#define FLAG_TEST_APP   0x1
#define FLAG_VERIFY_APP_STATUS_ONLY 0x2

class CAppSecurity
{
public: 
  CAppSecurity(const CStdString& appFilename, const CStdString& appSecurityManifest, const CStdString& appId="", const CStdString& appVersion="");
  ~CAppSecurity();

  bool VerifySignature(int flags = FLAG_VERIFY_ALL);
private:
  CStdString m_appFilename;
  CStdString m_appSecurityManifest;
  CStdString m_appId;
  CStdString m_appVersion;
};
#endif
