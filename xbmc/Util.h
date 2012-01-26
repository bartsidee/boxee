#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <climits>
#include <cmath>
#include <vector>
#include <map>
#include <limits>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "MediaSource.h"
#include "utils/CriticalSection.h"
#include "StringUtils.h"
#include "GUIBaseContainer.h"
#include "utils/Thread.h"

// A list of filesystem types for LegalPath/FileName
#define LEGAL_NONE            0
#define LEGAL_WIN32_COMPAT    1
#define LEGAL_FATX            2

namespace XFILE
{
  class IFileCallback;
}

class CFileItem;
class CFileItemList;
class IRunnable;

struct sortstringbyname
{
  bool operator()(const CStdString& strItem1, const CStdString& strItem2)
  {
    CStdString strLine1 = strItem1;
    CStdString strLine2 = strItem2;
    strLine1 = strLine1.ToLower();
    strLine2 = strLine2.ToLower();
    return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
  }
};

struct XBOXDETECTION
{
  std::vector<CStdString> client_ip;
  std::vector<CStdString> client_info;
  std::vector<unsigned int> client_lookup_count;
  std::vector<bool> client_informed;
};

typedef enum
{
  JOB_FAILED=0,
  JOB_SUCCEEDED,
  JOB_ABORTED
}Job_Result;

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static const CStdString GetExtension(const CStdString& strFileName);
  static void RemoveExtension(CStdString& strFileName);
  static bool GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber);
  static void CleanString(CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bIsFolder = false);
  static const CStdString GetFileName(const CStdString& strFileNameAndPath);
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static CStdString GetTitleFromBoxeePath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static void GetCommonPath(CStdString& strPath, const CStdString& strPath2);
  static bool IsDOSPath(const CStdString &path);
  static bool IsHD(const CStdString& strFileName);
  static bool IsPlugin(const CStdString& strFileName);
  static bool GetRecursiveParentPath(const CStdString& strChildPath, std::vector<CStdString>& output, unsigned int countHowManyLevels = 0);
  static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
  static void RunShortcut(const char* szPath);
  static void GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath);
  static void GetHomePath(CStdString& strPath);
  static void ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile);
  static void GetExtension(const CStdString& strFile, CStdString& strExtension);
  static bool HasSlashAtEnd(const CStdString& strFile);
  static bool IsRemote(const CStdString& strFile);
  static bool IsOnDVD(const CStdString& strFile);
  static bool IsOnLAN(const CStdString& strFile);
  static bool IsDVD(const CStdString& strFile);
  static bool IsVirtualPath(const CStdString& strFile);
  static bool IsMultiPath(const CStdString& strPath);
  static bool IsStack(const CStdString& strFile);
  static bool IsRAR(const CStdString& strFile);
  static bool IsInRAR(const CStdString& strFile);
  static bool IsZIP(const CStdString& strFile);
  static bool IsInZIP(const CStdString& strFile);
  static bool IsInArchive(const CStdString& strFile);
  static bool IsSpecial(const CStdString& strFile);
  static bool IsCDDA(const CStdString& strFile);
  static bool IsMemCard(const CStdString& strFile);
  static bool IsTuxBox(const CStdString& strFile);
  static bool IsMythTV(const CStdString& strFile);
  static bool IsDVB(const CStdString& strFile);
  static bool IsLastFM(const CStdString& strFile);
  static bool IsShoutCast(const CStdString& strFile);
  static bool IsHDHomeRun(const CStdString& strFile);
  static bool IsVTP(const CStdString& strFile);
  static bool IsHTSP(const CStdString& strFile);
  static bool IsLiveTV(const CStdString& strFile);
  static bool IsBoxeeDb(const CStdString& strFile);
  static bool ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static int GetDVDIfoTitle(const CStdString& strPathFile);
  static void UrlDecode(CStdString& strURLData);
  static void URLEncode(CStdString& strURLData);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static bool IsISO9660(const CStdString& strFile);
  static bool IsSmb(const CStdString& strFile);
  static bool IsDAAP(const CStdString& strFile);
  static bool IsUPnP(const CStdString& strFile);
  static bool IsNfs(const CStdString& strFile);
  static bool IsAfp(const CStdString& strFile);
  static bool IsBms(const CStdString& strFile);
  static bool IsRSS(const CStdString& strFile);
  static bool IsFlash(const CStdString& strFile);
  static bool IsMMS(const CStdString& strFile);
  static bool IsScript(const CStdString& strFile);
  static bool IsApp(const CStdString& strFile);
  static bool IsHTTP(const CStdString& strFile);  
  static void ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl );
  static bool ConstructStringFromTemplate(const CStdString& strTemplate, const std::map<CStdString , CStdString>& mapTemplateKeyToValue, CStdString& output, const CStdString& delimiter = "%");
  static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
  static void RemoveTempFiles();

  static void CacheSubtitles(const CStdString& strMovie, const CStdString& strContent, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback = NULL);
  static bool CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt="");
  static void ClearSubtitles();
  static int64_t ToInt64(uint32_t high, uint32_t low);
  static void AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult);
  static CStdString AddFileToFolder(const CStdString &strFolder, const CStdString &strFile)
  {
    CStdString result;
    AddFileToFolder(strFolder, strFile, result);
    return result;
  }
  static void AddSlashAtEnd(CStdString& strFolder);
  static void RemoveSlashAtEnd(CStdString& strFolder);
  static void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  static void CreateArchivePath(CStdString& strUrlPath, const CStdString& strType, const CStdString& strArchivePath,
    const CStdString& strFilePathInArchive, const CStdString& strPwd="");
  static bool ThumbExists(const CStdString& strFileName, bool bAddCache = false);
  static bool ThumbCached(const CStdString& strFileName);
  static void ThumbCacheAdd(const CStdString& strFileName, bool bFileExists);
  static void ThumbCacheClear();
  static void PlayDVD();
  static CStdString GetNextFilename(const CStdString &fn_template, int max);
  static CStdString GetNextPathname(const CStdString &path_template, int max);
  static void TakeScreenshot();
  static void TakeScreenshot(const char* fn, bool flash);
  static void SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate);
  static void SetBrightnessContrastGammaPercent(float brightness, float contrast, float gamma, bool immediate);
  static void Tokenize(const CStdString& path, std::vector<CStdString>& tokens, const std::string& delimiters);
  static void FlashScreen(bool bImmediate, bool bOn);
  static void RestoreBrightnessContrastGamma();
  static void InitGamma();
  static void ClearCache();
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
  static bool CreateDirectoryEx(const CStdString& strPath);

#ifdef _WIN32
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_WIN32_COMPAT);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_NONE);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_NONE);
#endif
  
  static bool IsUsingTTFSubtitles();
  static void SplitExecFunction(const CStdString &execString, CStdString &function, std::vector<CStdString> &parameters);
  static int GetMatchingSource(const CStdString& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static CStdString TranslateSpecialSource(const CStdString &strSpecial);
  static void DeleteDirectoryCache(const CStdString strType = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static CStdString MusicPlaylistsLocation();
  static CStdString VideoPlaylistsLocation();
  static CStdString SubstitutePath(const CStdString& strFileName);

  static bool SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute);
  static int GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew);
  static bool IsFTP(const CStdString& strFile);
  static bool GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax );
  static bool SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword);
  static bool AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort);
  static bool AutoDetection();
  static void AutoDetectionGetSource(VECSOURCES &share);
  static void GetSkinThemes(std::vector<CStdString>& vecTheme);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories=false);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& items);
  static void WipeDir(const CStdString& strPath);
  static void CopyDirRecursive(const CStdString& strSrcPath, const CStdString& strDstPath);
  static void ForceForwardSlashes(CStdString& strPath);

  static double AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1);
  static bool MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength);
  static bool HideExternalHDPath(CStdString inputPath, CStdString& outputPath);
  static bool SupportsFileOperations(const CStdString& strPath);

  static CStdString GetCachedMusicThumb(const CStdString &path);
  static CStdString GetCachedAlbumThumb(const CStdString &album, const CStdString &artist);
  static CStdString GetDefaultFolderThumb(const CStdString &folderThumb);
  static void ClearFileItemCache();

  static Job_Result RunInBG(IRunnable* pJob, bool deleteJob = true);
  static bool IsFrontMost();
  
  static void InitRandomSeed();
  
  static bool IsPasswordHidden(CStdString& path);
  static bool RemovePasswordFromPath(CStdString &path,bool completelyRemove = false);

  static bool IsDVDFolder(const CStdString& strPath, const CFileItemList* pPathItems = NULL);
  static bool IsBlurayFolder(const CStdString& strPath, const CFileItemList* pPathItems = NULL);
  static bool GetIsoDiskType(const CStdString& strIsoPath, CStdString& strDiskType);
#ifdef _LINUX
  // this will run the command using sudo in a new process.
  // the user that runs xbmc should be allowed to issue the given sudo command.
  // in order to allow a user to run sudo without supplying the password you'll need to edit sudoers
  // # sudo visudo
  // and add a line at the end defining the user and allowed commands
  static bool SudoCommand(const CStdString &strCommand);

  //
  // Forks to execute a shell command.
  //
  static bool Command(const CStdStringArray& arrArgs, bool waitExit = false);

  //
  // Forks to execute an unparsed shell command line.
  //
  static bool RunCommandLine(const CStdString& cmdLine, bool waitExit = false);

  static bool IsMountpoint(const CStdString& strPath);
  static int GetFsMagic(const CStdString& strPath);
  static bool ValidateIpAddress(const CStdString& ipAddress);
#endif

  static bool GetHostByName(const CStdString& strHostName, CStdString& strHostIp);
  static bool CreatePictureFolderThumb(CFileItem* pictureFolderItem);
  
  static int  VersionCompare(const CStdString version1, const CStdString version2);

  // helper utility to compute an md5 hash on a file
  static CStdString MD5File(const CStdString& _strFilePath);
  
  static CStdString GetPlatform();
  static bool IsEmbedded();
  
  static CStdString GetSpecialPathPrefix(const CStdString& specialPath);
  
  static bool IsCountryAllowed(const CStdString& _countries, bool allow);
  static bool IsAdultAllowed();
  static void FilterUnallowedItems(std::vector<CGUIListItemPtr>& items);
  
  static bool MatchesPlatform(const CStdString& platform);
  static void CreateTempDirectory(const CStdString &origPath);

  static CStdString LongestCommonPrefix(const CStdString& str1, const CStdString& str2);
  static Uint32 GetDirSize(const CStdString& path);

  static bool IsValidIp(const CStdString& ip);
  static bool CheckFileSignature(const CStdString &file, const CStdString &validSig);
  static bool GetHDDDirectory(const CStdString& path, CFileItemList& items);

  static bool FindFile(const CStdString& filename, const CStdString& directory, std::vector<CStdString>& vecResult);
#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
  static CStdString DumpStack(bool printStack = false);
  static void MemLeakDetectorStart();
  static void MemLeakDetectorStop();
  static void MemLeakDetectorReport(const CStdString& reportFile);
#endif
private:

  static HANDLE m_hCurrentCpuUsage;

};

class CPlayableFolderType
{
public:
  enum PlayableFolderEnums
  {
    PF_NO=0,
    PF_BLURAY=1,
    PF_DVD=2,
    NUM_OF_PF=3
  };
};

class IsPlayableFolderJob : public IRunnable
{
public:
  IsPlayableFolderJob(CStdString path);
  virtual ~IsPlayableFolderJob();
  virtual void Run();

  CPlayableFolderType::PlayableFolderEnums m_result;
  CStdString m_strPath;
};

#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

#ifdef _WIN32
long long atoll(const char *instr);
#define localtime_r(a,b) localtime_s(b,a)
#endif
