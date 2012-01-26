#pragma once
#include "IStorageProvider.h"
#include "HALProvider.h"
#include "DeviceKitDisksProvider.h"
#include "PosixMountProvider.h"
#include "BoxeeHalStorageProvider.h"

  
class CLinuxStorageProvider : public IStorageProvider
{
public:
  CLinuxStorageProvider()
  {
#ifdef HAS_HAL
    m_instance = new CHALProvider();
#elif defined(HAS_DBUS)
    if (CDeviceKitDisksProvider::HasDeviceKitDisks())
      m_instance = new CDeviceKitDisksProvider();
    else
      m_instance = new CPosixMountProvider();
#elif defined(HAS_BOXEE_HAL)
    m_instance = new CBoxeeHalStorageProvider();
#elif !defined(HAS_EMBEDDED)
    m_instance = new CPosixMountProvider();
#else
    m_instance = NULL;
#endif
  }

  virtual ~CLinuxStorageProvider()
  {
    if (m_instance)
      delete m_instance;
  }

  virtual void GetLocalDrives(VECSOURCES &localDrives)
  {
#ifndef HAS_EMBEDDED
    // Home directory
    CMediaSource share;
    share.strPath = getenv("HOME");
    share.strName = g_localizeStrings.Get(21440);
    share.m_ignore = true;
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    localDrives.push_back(share);
#endif

    if (m_instance)
      m_instance->GetLocalDrives(localDrives);
  }

  virtual void GetRemovableDrives(VECSOURCES &removableDrives)
  {
    if (m_instance)
      m_instance->GetRemovableDrives(removableDrives);
  }

  virtual std::vector<CStdString> GetDiskUsage()
  {
    if (m_instance)
      return m_instance->GetDiskUsage();
    else
    {
      std::vector<CStdString> emptyResult;
      return emptyResult;
    }
  }

  virtual bool PumpDriveChangeEvents()
  {
    if (m_instance)
      return m_instance->PumpDriveChangeEvents();
    else
      return false;
  }

private:
  IStorageProvider *m_instance;
};
