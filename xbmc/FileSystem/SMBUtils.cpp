#include "SMBUtils.h"
#include "BoxeeUtils.h"
#include "Directory.h"
#include "URL.h"
#include "log.h"
#include "Application.h"
#include "BrowserService.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"

namespace DIRECTORY
{
CStdString SMBUtils::GetComputerSMBName(const CStdString& computer)
{
  CStdString compName;
  int index = computer.Find('/');

  if(index > 0)
  {
    compName = computer.substr(0,index - 1);
  }
  else
  {
    compName = computer;
  }
  return compName;
}

bool SMBUtils::GetComputers(CFileItemList &items)
{
  const CStdString strPath = "smb://";

  CDirectory::GetDirectory(strPath, items);

  bool bInsert;
  std::set<CStdString> Computers;

  int i = 0;
  while(i < items.Size())
  {
    CStdString compName = GetComputerSMBName(items[i]->GetLabel());
    bInsert = Computers.insert(compName).second;

    if(bInsert)
    {
      items[i]->SetProperty("isNetwork",true);
      items[i]->SetLabel(compName);
      items[i]->m_strPath =  "smb://computers/?name=" + compName;
      i++;
    }
    else
    {
      items.Remove(i);
    }
  }

  return true;
}

bool SMBUtils::GetComputerDevices(const CStdString& strSmbPath,const CStdString& selectedComp, CFileItemList &items, bool allow_prompt/*=false*/)
{
  CBrowserService* pBrowser = g_application.GetBrowserService();

  if(!pBrowser->IsSmbAgressiveScan())
  {

    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      CStdString strId;
      strId.Format("%s-get-computer-devices", selectedComp);
      progress->StartModal(strId);
      progress->Progress();
    }

    CDirectory::GetDirectory("smb://" + selectedComp, items, "", true, allow_prompt);

    progress->Close();
    return true;
  }

  const CStdString strPathTmp = "smb://";
  CDirectory::GetDirectory(strPathTmp, items);

  size_t index;
  int length, i = 0;
  int indexToBeDeleted = -1;
  CStdString label;

  while(i < items.Size())
  {
    items[i]->SetProperty("isNetwork",true);
    CStdString compName = GetComputerSMBName(items[i]->GetLabel());

    if(!selectedComp.Equals(compName))
    {
      items.Remove(i);
    }
    else
    {
      //remove the computer from list displayed
      if(selectedComp.Equals(items[i]->GetLabel()))
      {
        indexToBeDeleted = i;
      }

      //remove computer name
      label = items[i]->GetLabel();
      index = label.find_first_of("//");
      if (index != std::string::npos)
      {
        length = label.length() - (index + 1);
        label = label.substr(index + 2,length);
        items[i]->SetLabel(label);
      }

      i++;
    }
  }

  if(items.Size() > 1 && indexToBeDeleted != -1)
  {
    items.Remove(indexToBeDeleted);
  }

  return true;
}

bool SMBUtils::ProcessPath(const CStdString& strPath, CFileItemList &items, bool allow_prompt/*=false*/)
{
  CURI url(strPath);

  if(url.GetProtocol() != "smb")
  {
    CLog::Log(LOGERROR, "CSMBDirectory::ProcessPath - invalid protocol [%s]", url.GetProtocol().c_str());
    return false;
  }

  CStdString strComp;
  CStdString strDevices;
  std::map<std::string, std::string> mapParams;

  // Parse boxeedb url
  if (!BoxeeUtils::ParseBoxeeDbUrl(strPath, strComp, strDevices, mapParams))
  {
    return false;
  }

  if(strComp.Equals("computers") && mapParams["name"].empty())
  {
    GetComputers(items);
    return true;
  }

  if(strComp.Equals("computers") && !mapParams["name"].empty())
  {
    CStdString selectedComp = mapParams["name"];
    if(selectedComp.IsEmpty())
    {
      return false;
    }
    GetComputerDevices(strPath, selectedComp, items, allow_prompt);
    return true;
  }

  return false;
}

CStdString SMBUtils::TranslatePath(const CStdString& strPath)
{
  CStdString translatedPath = strPath;

  if (CUtil::IsSmb(translatedPath))
  {
    if(translatedPath.Find("smb://computers/?name=") != -1)
    {
      int index = translatedPath.Find("=");
      int length = translatedPath.size() - index;
      if(index > -1)
      {
        CStdString compName = translatedPath.substr(index + 1,length);
        translatedPath = "smb://" + compName;
      }
      else
      {
        translatedPath = "smb://";
      }
    }
    if(translatedPath.Find("smb://computers") != -1)
    {
      translatedPath = "smb://";
    }
    else if(translatedPath.Find("/devices") != -1)
    {
      translatedPath = translatedPath.substr(0,translatedPath.length() - 9);
    }
  }

  return translatedPath;
}

}	// namespace DIRECTORY
