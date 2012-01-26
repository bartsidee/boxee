#ifndef BOXEESOCIALUTILSUIMANAGER_H
#define BOXEESOCIALUTILSUIMANAGER_H

#include <string>
#include "GUIWindow.h"

class CBoxeeSocialUtilsUIManager
{
public:
  CBoxeeSocialUtilsUIManager();
  virtual ~CBoxeeSocialUtilsUIManager();

  bool HandleUISocialUtilConnect(const CStdString& serviceId);
  bool HandleUISocialUtilDisconnect(const CStdString& serviceId);

private:
  bool ConnectUISocialService(const CStdString& serviceId);

};

#endif
