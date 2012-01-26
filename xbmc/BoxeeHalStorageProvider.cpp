#include "system.h"
#include "Application.h"
#include "guilib/LocalizeStrings.h"

#ifdef HAS_BOXEE_HAL

#include "BoxeeHalStorageProvider.h"
#include "HalServices.h"

CBoxeeHalStorageProvider::CBoxeeHalStorageProvider()
{
  m_isDirty = true;
  CHalServicesFactory::GetInstance().AddListener(this);
  PumpDriveChangeEvents();
  m_changeOccured = false;
}

void CBoxeeHalStorageProvider::GetDrives(VECSOURCES &drives)
{
  std::vector<CStdString> result;

  if (m_isDirty)
  {
    GetDrivesFromHal(m_drives);
    m_isDirty = false;
  }

  drives.insert(drives.end(), m_drives.begin(), m_drives.end());
}

void CBoxeeHalStorageProvider::GetDrivesFromHal(VECSOURCES &drives)
{
  std::vector <CHalStorageDeviceInfo> devices;
  CHalServicesFactory::GetInstance().GetAllStorageDevices(devices);

  drives.clear();
  for (size_t i = 0; i < devices.size(); i++)
  {
    CMediaSource share;
    share.strPath = devices[i].path;
    if (devices[i].label == "")
      share.strName = g_localizeStrings.Get(51363);
    else
      share.strName = devices[i].label;
    share.m_ignore = true;
    if (devices[i].dev_type == "usb")
      share.m_localSourceType = CMediaSource::LOCAL_SOURCE_TYPE_USB;
    else if (devices[i].dev_type == "sd")
      share.m_localSourceType = CMediaSource::LOCAL_SOURCE_TYPE_SD;
    else if (devices[i].dev_type == "internal_hd")
      share.m_localSourceType = CMediaSource::LOCAL_SOURCE_TYPE_INTERNAL_HD;
    drives.push_back(share);
  }
}

std::vector<CStdString> CBoxeeHalStorageProvider::GetDiskUsage()
{
  std::vector<CStdString> result;
  return result;
}

bool CBoxeeHalStorageProvider::PumpDriveChangeEvents()
{
  bool result = m_changeOccured;
  m_changeOccured = false;
  return result;
}

void CBoxeeHalStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  GetDrives(removableDrives);
}

void CBoxeeHalStorageProvider::HandleNotification(CHalNotification &notification)
{
  if (notification.GetType() == HAL_NOTIFY_STORAGE)
  {
    m_changeOccured = true;
    CHalStorageNotification storageNotification = (CHalStorageNotification &)notification;
    uint32_t dwCode;
    if (storageNotification.IsMounted())
      dwCode = 13026;
    else {
      if (storageNotification.IsSuccessful())
        dwCode = 13027;
      else
        dwCode = 13028;
    }
    g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(dwCode), 5000);

    m_isDirty = true;
  }
}

#endif
