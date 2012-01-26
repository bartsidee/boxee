#include "Win32StorageProvider.h"
#include "WIN32Util.h"
#include "LocalizeStrings.h"

void CWin32StorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CWIN32Util::GetDrivesByType(localDrives, LOCAL_DRIVES);
}

void CWin32StorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  CWIN32Util::GetDrivesByType(removableDrives, REMOVABLE_DRIVES);
}

std::vector<CStdString> CWin32StorageProvider::GetDiskUsage()
{
  return CWIN32Util::GetDiskUsage();
}
