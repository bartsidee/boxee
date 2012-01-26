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

#include "GUIDialogBoxBase.h"
#include "IProgressCallback.h"
#include "FileSystem/File.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback, public XFILE::IFileCallback
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  void StartModal(const std::string &strId="");
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  void Progress();
  void ProgressKeys();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  int GetPercentage() const { return m_percentage; };
  void ShowProgressBar(bool bOnOff);
  void SetHeading(const std::string& strLine);
  void SetHeading(int iString);             // for convenience to lookup in strings.xml

  void Close(bool bForce = false);

  // Implements IProgressCallback
  virtual void SetProgressMax(int iMax);
  virtual void SetProgressAdvance(int nSteps=1);
  virtual bool Abort();

  void SetCanCancel(bool bCanCancel);
  inline bool CanCancel() const { return m_bCanCancel; }

  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) ; // to allow file download with progress

  void SetOperationID(const std::string &strId);
  const std::string &GetOperationID();
  
  static void EndProgressOperation(const std::string &strId);
  
protected:
  bool m_bCanCancel;
  bool m_bCanceled;
  std::string m_strHeading;
  std::string m_strId;
  
  int  m_iCurrent;
  int  m_iMax;
  int m_percentage;
};
