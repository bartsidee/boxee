#ifndef __XBMCHELPER_H__
#define __XBMCHELPER_H__

#include <string>

class XBMCHelper
{
 public:
  
  XBMCHelper();
   
  void Start();
  void Stop();
  void Restart();
  
  void Configure();
  void ConfigureStartup();
  
  void Install();
  void Uninstall();

  bool IsRunning();
  static bool IsAppleTV();
  static void CaptureAllInput();
  static void ReleaseAllInput();

  bool IsRemoteBuddyInstalled();
  bool IsSofaControlRunning();
 
  bool IsAlwaysOn() const { return m_alwaysOn; }
  int  GetMode() const { return m_mode; }
  
  bool ErrorStarting() { return m_errorStarting; }
  
  static void KillAll(const char* searchString);
  
 private:
  bool ReplaceString(std::string &src, const std::string &what, const std::string &with); 
  std::string GetConfig();
  
  int GetProcessPid(const char* processName);
  
  std::string ReadFile(const char* fileName);
  void WriteFile(const char* fileName, const std::string& data);
   
  bool m_alwaysOn;
  int  m_mode;
  int  m_sequenceDelay;
  bool m_errorStarting;
  bool m_bKilled;
  
  std::string m_launchAgentLocalFile;
  std::string m_launchAgentInstallFile;
  std::string m_homepath;
  std::string m_helperFile;
};

extern XBMCHelper g_xbmcHelper;

#endif
