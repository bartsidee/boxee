#include "PosixMountProvider.h"
#include "RegExp.h"
#include "StdString.h"
#include "Util.h"
#include "LocalizeStrings.h"
#include "SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "FileItem.h"

CPosixMountProvider::CPosixMountProvider()
{
  m_removableLength = 0;
  PumpDriveChangeEvents();
}

void CPosixMountProvider::GetDrives(VECSOURCES &drives)
{
  std::vector<std::pair<CStdString, CStdString>  > result;

  CRegExp reMount;
#ifdef __APPLE__
  reMount.RegComp("(.+) on (.+) \\(([^,]+)");
#else
  reMount.RegComp("(.+) on (.+) type ([^ ]+)");
#endif
  char line[1024];

  FILE* pipe = popen("mount", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      if (reMount.RegFind(line) != -1)
      {
        bool accepted = false;
        char* dev   = reMount.GetReplaceString("\\1");
        char* mount = reMount.GetReplaceString("\\2");
        char* fs    = reMount.GetReplaceString("\\3");
          
        // Here we choose which filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0
            || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0
            || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0
            || strcmp(fs, "ntfs-3g") == 0 || strcmp(fs, "iso9660") == 0
            || strcmp(fs, "fusefs") == 0 || strcmp(fs, "hfs") == 0
            || strcmp(fs, "msdos") == 0 || strcmp(fs, "fusefs_txantfs") == 0)

          accepted = true;

        // Ignore root
        if (strcmp(mount, "/") == 0)
          accepted = false;          
        
        if(accepted)
          result.push_back(std::pair<CStdString, CStdString>(mount, dev));

        free(fs);
        free(mount);
      }
    }
    pclose(pipe);
  }

#ifdef HAS_LOCAL_MEDIA
  CStdString home = _P("special://userhome");
  CUtil::RemoveSlashAtEnd(home);
  result.push_back(std::pair<CStdString, CStdString>(home, ""));
#endif
  
  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = result[i].first;
    share.strName = CUtil::GetFileName(result[i].first);
    share.m_ignore = true;
    share.strDev = result[i].second;
    drives.push_back(share);
  }
}

std::vector<CStdString> CPosixMountProvider::GetDiskUsage()
{
  std::vector<CStdString> result;
  char line[1024];

#ifdef __APPLE__
  FILE* pipe = popen("df -hT ufs,cd9660,hfs,udf", "r");
#else
  FILE* pipe = popen("df -hx tmpfs", "r");
#endif

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      result.push_back(line);
    }
    pclose(pipe);
  }

  return result;
}

bool CPosixMountProvider::PumpDriveChangeEvents()
{
  VECSOURCES drives;
  GetRemovableDrives(drives);
  bool changed = drives.size() != m_removableLength;
  m_removableLength = drives.size();
  return changed;
}

void CPosixMountProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  GetDrives(removableDrives);

#ifdef __APPLE__
  // All drives are HDs, unless proven otherwise
  for (size_t i = 0; i < removableDrives.size(); i++)
  {
    removableDrives[i].m_localSourceType = CMediaSource::LOCAL_SOURCE_TYPE_INTERNAL_HD;
  }

  char line[1024];
  FILE* pipe = popen("system_profiler SPUSBDataType | grep 'BSD Name:'  | sed 's/ *BSD Name: //g'", "r");

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      // Remove \n
      if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

      CStdString lineStr = line;

      for (size_t i = 0; i < removableDrives.size(); i++)
      {
        if (lineStr.length() && removableDrives[i].strDev.Find(lineStr) != -1)
        {
          removableDrives[i].m_localSourceType = CMediaSource::LOCAL_SOURCE_TYPE_USB;
        }
      }
    }

    pclose(pipe);
  }
#endif
}

