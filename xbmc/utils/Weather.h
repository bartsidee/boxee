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

#include "InfoLoader.h"
#include "StdString.h"

class CWeather : public CInfoLoader
{
public:
  static CWeather& GetInstance();
  virtual ~CWeather(void) { }

  void Reset();

  int GetTemperature() { return m_temperature; }
  CStdString& GetConditionsIcon() { return m_conditionsIcon; }
  bool IsFetched();

  // Inherited from CInfoLoader
  virtual void DoWork();

private:
  CWeather(void);

  bool LoadWeather();

  bool       m_isFetched;
  CStdString m_conditionsIcon;
  int        m_temperature;
};
