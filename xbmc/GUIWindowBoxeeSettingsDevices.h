#ifndef GUI_WINDOW_BOXEE_SETTINGS_DEVICES
#define GUI_WINDOW_BOXEE_SETTINGS_DEVICES

#include "GUIDialog.h"
#include "FileItem.h"

class CGUIWindowBoxeeSettingsDevices : public CGUIDialog
{
public:
  CGUIWindowBoxeeSettingsDevices();
  virtual ~CGUIWindowBoxeeSettingsDevices();
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);

protected:

  void LoadPairedDevicesList();
  bool HandleClickOnPairedDeviceList();

  CFileItemList m_pairedDeviceList;
};


#endif // GUI_WINDOW_BOXEE_SETTINGS_DEVICES
