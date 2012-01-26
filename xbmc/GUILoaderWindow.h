/*
 * GUILoaderWindow.h
 *
 *  Created on: Jul 16, 2009
 *      Author: yuvalt
 */

#ifndef GUILOADERWINDOW_H_
#define GUILOADERWINDOW_H_

#include "BackgroundInfoLoader.h"
#include "FileItem.h"
#include "GUIWindow.h"
#include "GUIBaseContainer.h"

class CGUILoaderWindow : public CGUIWindow
{
public:
  CGUILoaderWindow(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUILoaderWindow();
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void RegisterContainers();
  virtual void RegisterContainer(DWORD containerId);  
  virtual void ReloadContainer(DWORD containerId, bool clearCacheFirst = false);
  virtual void OnContainersLoadSuccess(CGUIBaseContainer& container);
  virtual void OnContainersLoadFailed(CGUIBaseContainer& container);
};

#endif /* GUILOADERWINDOW_H_ */
