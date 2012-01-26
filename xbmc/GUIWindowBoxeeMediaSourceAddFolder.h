#ifndef GUI_WINDOW_BOXEE_MEDIA_ADD_FOLDER
#define GUI_WINDOW_BOXEE_MEDIA_ADD_FOLDER

#pragma once

#include "GUIDialog.h"
#include "utils/Thread.h"

class CGUIWindowBoxeeMediaSourceAddFolder : public CGUIDialog
{
  
  class BrowseDirectoryJob : public IRunnable
  {
  public:
    BrowseDirectoryJob(const CStdString& folderPath, CFileItemList *folders) :
      m_folders(folders)
    {
      m_folderPath = folderPath;
    }

    virtual void Run();
    CStdString m_folderPath;
    CFileItemList *m_folders;
  };
public:
  CGUIWindowBoxeeMediaSourceAddFolder(void);
  virtual ~CGUIWindowBoxeeMediaSourceAddFolder(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

private:
  bool InitBrowseDirectory();
  void BrowseDirectory();
  void ProccessItemSelectedInControlFoldersList();
  void ProccessHitOnAddButton();
  
  CStdString GetSelectedEncodedPath();
  CStdString GetSelectedFolderThumbPath();

  CFileItemList m_devices;
  bool m_devicesFound;
  CStdString m_rootFolder;
  CStdString m_currentFolder;
  CFileItemList m_folders;
  int m_selectedSource;
  std::vector<int> m_LastPosition;
  int m_currentPosition;
  CStdString m_currentFolderThumbPath;
};

#endif
