/*
 * GUILoaderWindow.h
 *
 *  Created on: Jul 16, 2009
 *      Author: yuvalt
 */

#ifndef GUILOADERDIALOG_H_
#define GUILOADERDIALOG_H_

#include "BackgroundInfoLoader.h"
#include "FileItem.h"
#include "GUIDialog.h"
#include "GUIBaseContainer.h"

class CGUILoaderDialog : public CGUIDialog
{
public:
  CGUILoaderDialog(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUILoaderDialog();
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void RegisterContainers();
  virtual void RegisterContainer(DWORD containerId);  
  virtual void OnContainersLoadSuccess(CGUIBaseContainer& container);
  virtual void OnContainersLoadFailed(CGUIBaseContainer& container);
};

#endif /* GUILOADERDIALOG_H_ */
