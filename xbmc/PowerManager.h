/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef _POWER_MANAGER_H_
#define _POWER_MANAGER_H_

class IPowerEventsCallback
{
public:
  virtual ~IPowerEventsCallback() { }

  virtual void OnSleep() = 0;
  virtual void OnWake() = 0;

  virtual void OnLowBattery() = 0;
};

class IPowerSyscall
{
public:
  virtual ~IPowerSyscall() {};
  virtual bool Powerdown()    = 0;
  virtual bool Suspend()      = 0;
  virtual bool Hibernate()    = 0;
  virtual bool Reboot()       = 0;

// Might need to be membervariables instead for speed
  virtual bool CanPowerdown() = 0;
  virtual bool CanSuspend()   = 0;
  virtual bool CanHibernate() = 0;
  virtual bool CanReboot()    = 0;
  virtual bool IsSuspended()  = 0;
};

// For systems without PowerSyscalls we have a NullObject
class CNullPowerSyscall : public IPowerSyscall
{
public:
  virtual bool Powerdown()    { return false; }
  virtual bool Suspend()      { return false; }
  virtual bool Hibernate()    { return false; }
  virtual bool Reboot()       { return false; }

  virtual bool CanPowerdown() { return false; }
  virtual bool CanSuspend()   { return false; }
  virtual bool CanHibernate() { return false; }
  virtual bool CanReboot()    { return false; }
  virtual bool IsSuspended()  { return false; }
};

// This class will wrap and handle PowerSyscalls.
// It will handle and decide if syscalls are needed.
class CPowerManager : public IPowerSyscall
{
public:
  CPowerManager();
  virtual ~CPowerManager();

  virtual void Initialize();

  virtual bool Powerdown();
  virtual bool Suspend();
  virtual bool Hibernate();
  virtual bool Reboot();
  virtual void Resume();

  virtual bool CanPowerdown();
  virtual bool CanSuspend();
  virtual bool CanHibernate();
  virtual bool CanReboot();
  virtual bool IsSuspended()  { return m_isSuspended; }
private:
  IPowerSyscall *m_instance;
  bool           m_isSuspended;
  unsigned int   m_suspendTime;
};

extern CPowerManager g_powerManager;
#endif
