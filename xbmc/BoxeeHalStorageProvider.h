#pragma once

#include "system.h"

#ifdef HAS_BOXEE_HAL

#include "IStorageProvider.h"
#include "HalServices.h"

class CBoxeeHalStorageProvider: public IStorageProvider, public IHalListener
{
public:
  CBoxeeHalStorageProvider();
  virtual ~CBoxeeHalStorageProvider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { GetDrives(localDrives); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives);

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();

  virtual void HandleNotification(CHalNotification &notification);

private:
  void GetDrives(VECSOURCES &drives);
  void GetDrivesFromHal(VECSOURCES &drives);
  bool m_changeOccured;
  bool m_isDirty;
  VECSOURCES m_drives;
};

#endif

