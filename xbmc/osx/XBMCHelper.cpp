

#include <sys/types.h>
#include <sys/sysctl.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mach-o/dyld.h>
#include <Carbon/Carbon.h>
#include "PlatformInclude.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"

#include "GUIDialogYesNo2.h"

using namespace std; 

#include "XBMCHelper.h"

#include "system.h"
#include "GUISettings.h"
#include "Util.h"
#include "XFileUtils.h"
#include "utils/SystemInfo.h"

XBMCHelper g_xbmcHelper;

#define XBMC_HELPER_PROGRAM "boxeeservice"
#define SOFA_CONTROL_PROGRAM "Sofa Control"
#define XBMC_LAUNCH_PLIST "tv.boxee.helper.plist"
#define XBMC_TEMPLATE_PLIST "launch-agent.template"
#define XBMC_LAUNCH_CLIENT_PLIST "tv.boxee.client.plist"

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount);

/////////////////////////////////////////////////////////////////////////////
XBMCHelper::XBMCHelper()
  : m_errorStarting(false)
  , m_mode(APPLE_REMOTE_DISABLED)
  , m_alwaysOn(false)
  , m_sequenceDelay(0)
  , m_bKilled(false)
{
  CStdString homePath;
  CUtil::GetHomePath(homePath);

  // Compute the helper filename.
  m_helperFile = homePath + "/bin/" XBMC_HELPER_PROGRAM ;
  
  // Compute the local (pristine) launch agent filename.
  m_launchAgentLocalFile = homePath + "/config/" XBMC_TEMPLATE_PLIST;
  
  // Compute the install path for the launch agent.
  m_launchAgentInstallFile = getenv("HOME");
  m_launchAgentInstallFile += "/Library/LaunchAgents/" XBMC_LAUNCH_PLIST;  
}

std::string XBMCHelper::GetConfig()
{
  std::string strConfig;
  int remoteMode = g_guiSettings.GetInt("appleremote.mode");
  string port = (const char*)g_guiSettings.GetString("remoteevents.port");
  if (remoteMode == APPLE_REMOTE_UNIVERSAL)
    strConfig = "--universal ";
  
  char strDelay[64];
  sprintf(strDelay, "--timeout %d --port %s", m_sequenceDelay, port.c_str());
  strConfig += strDelay;
  
  return strConfig;
}

void XBMCHelper::Restart()
{
  Stop();
  Start();
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Start()
{
  // hack. always run the helper on atv
  bool bIsATV = CSysInfo::IsAppleTV();
  if (!bIsATV && g_guiSettings.GetInt("appleremote.mode") == APPLE_REMOTE_DISABLED)
    return;
  
  if (bIsATV)
  {
    m_mode = APPLE_REMOTE_DISABLED;
    g_guiSettings.SetInt("appleremote.mode", APPLE_REMOTE_DISABLED);
    Uninstall();
    return;
  }
  
  int nPid = GetProcessPid(XBMC_HELPER_PROGRAM); 
  
  // sometimes the process just killed would continue for a short while. we still want to fork the new one in this case.
  if (!m_bKilled && nPid != -1)
  {
    CLog::Log(LOGINFO, "(helper) helper already running. not doing anything (pid: %d)", nPid);
    return;    
  }
  
  std::string strConfig = GetConfig();

  CLog::Log(LOGINFO,"(helper) Starting helper with config: <%s>", strConfig.c_str());

  if (g_guiSettings.GetBool("appleremote.alwayson"))
  {
    Install();  
  }
  else 
  {
    printf("Asking helper to start.\n");
    // use -x to have XBMCHelper read its configure file
    std::string cmd = "\"" + m_helperFile + "\" -x &";
    system(cmd.c_str());    
  }
  
  m_bKilled = false;
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Stop()
{
  CLog::Log(LOGINFO,"(helper) Stopping helper");

  if (m_alwaysOn)
  {
    Uninstall();  
  }
  
  // making sure its dead
  int pid = GetProcessPid(XBMC_HELPER_PROGRAM);
  if (pid != -1)
  {
    CLog::Log(LOGINFO,"(helper) killing process %lu", pid);
    kill(pid, SIGKILL);      
    m_bKilled = true;
  }
}

void XBMCHelper::ConfigureStartup()
{
  std::string strLauncher = std::string(getenv("HOME"))+"/Library/LaunchAgents/" XBMC_LAUNCH_CLIENT_PLIST;
  if (g_guiSettings.GetBool("boxee.runatlogin"))
  {
    char     path[2*MAXPATHLEN];
    uint32_t path_size = 2*MAXPATHLEN;    
    _NSGetExecutablePath(path, &path_size);
    
    string plistData = ReadFile(m_launchAgentLocalFile.c_str());
    ReplaceString(plistData, "${KA}", "false");
    ReplaceString(plistData, "${PATH}", path);
    ReplaceString(plistData, "${ARGS}", "");
    ReplaceString(plistData, "${PROG}", "client");
    m_launchAgentInstallFile = getenv("HOME");
    m_launchAgentInstallFile += "/Library/LaunchAgents/" XBMC_LAUNCH_PLIST;  

    WriteFile(strLauncher.c_str(), plistData);    
  }
  else
  {
    ::DeleteFile(strLauncher.c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Configure()
{
  int oldMode = m_mode;
  int oldDelay = m_sequenceDelay;
  
  // Read the new configuration.
  m_errorStarting = false;
  m_mode = g_guiSettings.GetInt("appleremote.mode");
  m_sequenceDelay = g_guiSettings.GetInt("appleremote.sequencetime");
  
  if (m_mode == APPLE_REMOTE_DISABLED && oldMode != APPLE_REMOTE_DISABLED)
  {
    if (!CGUIDialogYesNo2::ShowAndGetInput(51504, 51505))
    {
      g_guiSettings.SetInt("appleremote.mode", oldMode);
      m_mode = oldMode; 
      return;
    }    
  }
  
  // Don't let it enable if sofa control or remote buddy is around.
  if (IsSofaControlRunning())
  {
    // If we were starting then remember error.
    if (oldMode == APPLE_REMOTE_DISABLED && m_mode != APPLE_REMOTE_DISABLED)
      m_errorStarting = true;
    
    m_mode = APPLE_REMOTE_DISABLED;
    g_guiSettings.SetInt("appleremote.mode", APPLE_REMOTE_DISABLED);
  }

  // New configuration.
  if (oldMode != m_mode || oldDelay != m_sequenceDelay || m_alwaysOn != g_guiSettings.GetBool("appleremote.alwayson"))
  {
    // Build a new config string.
    std::string strConfig;
    switch (m_mode) {
      case APPLE_REMOTE_UNIVERSAL:
        strConfig = "--universal ";
        break;
      case APPLE_REMOTE_MULTIREMOTE:
        strConfig = "--multiremote ";
        break;
      default:
        break;
    }
#ifdef _DEBUG
    strConfig += "--verbose ";
#endif
    char strDelay[64];
    sprintf(strDelay, "--timeout %d ", m_sequenceDelay);
    strConfig += strDelay;

    // Find out where we're running from.
    char real_path[2*MAXPATHLEN];
    char given_path[2*MAXPATHLEN];
    uint32_t path_size = 2*MAXPATHLEN;

    if (_NSGetExecutablePath(given_path, &path_size) == 0)
    {
      if (realpath(given_path, real_path) != NULL)
      {
        strConfig += "--appPath \"";
        strConfig += real_path;
        strConfig += "\" ";

        strConfig += "--appHome \"";
        strConfig += m_homepath;
        strConfig += "\" ";
      }
    }

    Stop();
    m_alwaysOn = g_guiSettings.GetBool("appleremote.alwayson");
    Start();

  }    
}
  
bool XBMCHelper::ReplaceString(std::string &src, const std::string &what, const std::string &with)
{
  int start = src.find(what);
  bool bReplaced = false;
  while (start > 0 && start < src.length())
  {
    bReplaced = true;
    src.replace(start, what.length(), with.c_str(), with.length());
    start = src.find(what, start);
  }
  
  return bReplaced;
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Install()
{
  // Make sure directory exists.
  string strDir = getenv("HOME");
  strDir += "/Library/LaunchAgents";
  CreateDirectory(strDir.c_str(), NULL);
  
  CStdString strConfig = GetConfig();
  strConfig.Replace(" --","</string>\n<string>--");
  CLog::Log(LOGINFO, "(helper) installing helper. config: <%s>", strConfig.c_str());
  
  // Load template.
  string plistData = ReadFile(m_launchAgentLocalFile.c_str());

  // Replace place holders in the file.
  ReplaceString(plistData, "${KA}", "true");
  ReplaceString(plistData, "${PATH}", m_helperFile);
  ReplaceString(strConfig," ","</string><string>");
  ReplaceString(plistData, "${ARGS}", strConfig);
  ReplaceString(plistData, "${PROG}", "helper");

  // if by any chance the file does not exists we create it so that launchctl can unload it
  if (!access(m_launchAgentInstallFile.c_str(), F_OK))
    WriteFile(m_launchAgentInstallFile.c_str(), plistData);  
  
  // first unLoad it.
  string cmd = "/bin/launchctl unload -F ";
  cmd += m_launchAgentInstallFile;
  system(cmd.c_str());

  // Install it.
  WriteFile(m_launchAgentInstallFile.c_str(), plistData);
  
  // Load it.
  cmd = "/bin/launchctl load ";
  cmd += m_launchAgentInstallFile;
  system(cmd.c_str());
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::Uninstall()
{
  CLog::Log(LOGINFO, "(helper) uninstalling launch agent");
  
  // Call the unloader.
  string cmd = "/bin/launchctl unload ";
  cmd += m_launchAgentInstallFile;
  system(cmd.c_str());
  
  //this also stops the helper, so restart it here again, if not disabled
  if(m_mode != APPLE_REMOTE_DISABLED)
    Start();

  // Remove the plist file.
  DeleteFile(m_launchAgentInstallFile.c_str());

  cmd = "/usr/bin/killall -9 boxeeservice ";
  system(cmd.c_str());
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsRunning()
{
  return (GetProcessPid(XBMC_HELPER_PROGRAM)!=-1);
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::CaptureAllInput()
{
  // Take keyboard focus away from FrontRow and native screen saver
  if (g_sysinfo.IsAppleTV())
  {
    ProcessSerialNumber psn = {0, kCurrentProcess};
       
    SetFrontProcess(&psn);
    EnableSecureEventInput();
  }
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::ReleaseAllInput()
{
  // Give keyboard focus back to FrontRow and native screen saver
  if (g_sysinfo.IsAppleTV())
  {
    DisableSecureEventInput();
  }
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsRemoteBuddyInstalled()
{
  return false;
  // Check for existence of kext file.
  return access("/System/Library/Extensions/RBIOKitHelper.kext", R_OK) != -1;
}

/////////////////////////////////////////////////////////////////////////////
bool XBMCHelper::IsSofaControlRunning()
{
  return false;
  // Check for a "Sofa Control" process running.
  return GetProcessPid(SOFA_CONTROL_PROGRAM) != -1;
}

/////////////////////////////////////////////////////////////////////////////
std::string XBMCHelper::ReadFile(const char* fileName)
{
  std::string ret = "";
  ifstream is;
  
  is.open(fileName);
  if( is.good() )
  {
      // Get length of file:
      is.seekg (0, ios::end);
      int length = is.tellg();
      is.seekg (0, ios::beg);

      // Allocate memory:
      char* buffer = new char [length+1];

      // Read data as a block:
      is.read(buffer,length);
      is.close();
      buffer[length] = '\0';

      ret = buffer;
      delete[] buffer;
  }
  else
  {
      std::string ret = "";
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
void XBMCHelper::WriteFile(const char* fileName, const std::string& data)
{
  ofstream out(fileName); 
  if (!out)
  {
    CLog::Log(LOGERROR, "XBMCHelper: Unable to open file '%s'", fileName);
  }
  else
  {
    // Write new configuration.
    out << data << endl;
    out.flush();
    out.close();
  }
}

void XBMCHelper::KillAll(const char* searchString)
{
  kinfo_proc* mylist = (kinfo_proc *)malloc(sizeof(kinfo_proc));
  size_t mycount = 0;
  int ret = -1;
  
  GetBSDProcessList(&mylist, &mycount);
  for (size_t k = 0; k < mycount && ret == -1; k++) 
  {
    kinfo_proc *proc = NULL;
    proc = &mylist[k];

    std::string strCommand = proc->kp_proc.p_comm;
    if (strCommand.find(searchString) != std::string::npos)
      kill(proc->kp_proc.p_pid, SIGKILL);
  }
  
  free (mylist);
}

/////////////////////////////////////////////////////////////////////////////
int XBMCHelper::GetProcessPid(const char* strProgram)
{
  kinfo_proc* mylist = 0;
  size_t mycount = 0;
  int ret = -1;
  
  GetBSDProcessList(&mylist, &mycount);
  for (size_t k = 0; k < mycount && ret == -1; k++) 
  {
    kinfo_proc *proc = NULL;
    proc = &mylist[k];

    if (strcmp(proc->kp_proc.p_comm, strProgram) == 0)
    {
      //if (ignorePid == 0 || ignorePid != proc->kp_proc.p_pid)
      ret = proc->kp_proc.p_pid;
    }
  }
  
  free (mylist);
  
  return ret;
}

typedef struct kinfo_proc kinfo_proc;

// Returns a list of all BSD processes on the system.  This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount.  You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
//
static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
{
  int err;
  kinfo_proc * result;
  bool done;
  static const int name[] =
  { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  // Declaring name as const requires us to cast it when passing it to
  // sysctl because the prototype doesn't include the const modifier.
  size_t length;

  assert(procList != NULL);
  assert(procCount != NULL);

  *procCount = 0;

  // We start by calling sysctl with result == NULL and length == 0.
  // That will succeed, and set length to the appropriate length.
  // We then allocate a buffer of that size and call sysctl again
  // with that buffer.  If that succeeds, we're done.  If that fails
  // with ENOMEM, we have to throw away our buffer and loop.  Note
  // that the loop causes use to call sysctl with NULL again; this
  // is necessary because the ENOMEM failure case sets length to
  // the amount of data returned, not the amount of data that
  // could have been returned.
  //
  result = NULL;
  done = false;
  do
  {
    assert(result == NULL);

    // Call sysctl with a NULL buffer.
    length = 0;
    err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL,
        &length, NULL, 0);
    if (err == -1)
      err = errno;

    // Allocate an appropriately sized buffer based on the results from the previous call.
    if (err == 0)
    {
      result = (kinfo_proc*) malloc(length);
      if (result == NULL)
        err = ENOMEM;
    }

    // Call sysctl again with the new buffer.  If we get an ENOMEM
    // error, toss away our buffer and start again.
    //
    if (err == 0)
    {
      err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, result,
          &length, NULL, 0);

      if (err == -1)
        err = errno;
      else if (err == 0)
        done = true;
      else if (err == ENOMEM)
      {
        assert(result != NULL);
        free(result);
        result = NULL;
        err = 0;
      }
    }
  } while (err == 0 && !done);

  // Clean up and establish post conditions.
  if (err != 0 && result != NULL)
  {
    free(result);
    result = NULL;
  }

  *procList = result;
  if (err == 0)
    *procCount = length / sizeof(kinfo_proc);

  assert( (err == 0) == (*procList != NULL) );
  return err;
}
