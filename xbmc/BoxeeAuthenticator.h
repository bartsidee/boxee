#ifndef BoxxeAuthenticator_H_
#define BoxxeAuthenticator_H_

extern "C"
{
  typedef void (*BXAuthInitType)(const char* platform, const char* version, const char *pathToCookieJar, const char* pathToAppsDir, const char* serverUrlPrefix);
  typedef char* (*BXAuthAppType)(const char* appId);
  typedef bool (*BXAuthMainType)();
  typedef void (*BXAuthFreeType)(void*);
  typedef const char* (*BXAuthVersionType)();
}

class BoxeeAuthenticator
{
public:
  BoxeeAuthenticator();
  virtual ~BoxeeAuthenticator();
  CStdString AuthenticateApp(const CStdString& appId);
  bool Init();
  bool AuthenticateMain();

private:
  bool LoadDll();
  void UnloadDll();
  bool IsDllLoaded();
  CStdString GetDllFilename();

  BXAuthInitType _AuthInit;
  BXAuthAppType  _AuthApp;
  BXAuthMainType _AuthMain;
  BXAuthFreeType _AuthFree;
  BXAuthVersionType  _AuthVersion;
  
#if defined(_LINUX)
  void* m_DllHandle;
#else
  HMODULE m_DllHandle;
#endif
};

#endif /* BoxeeAuthenticator_H_ */
