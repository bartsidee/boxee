#ifndef ITEMLOADER_H_
#define ITEMLOADER_H_

#ifdef _WIN32
#include "win32/PlatformDefs.h"
#else
#include "linux/PlatformDefs.h"
#endif
#include "StdString.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "FileItem.h"
#include "BackgroundInfoLoader.h"
#include "utils/CriticalSection.h"


#include <map>
#include <set>
#include <string>

#define PLACEHOLDER_IMAGE "defaultloading.gif"

class IItemLoaderCallback {
public:
  // TODO: Fix this interface to receive window and control ids
	virtual void ItemsLoaded(CFileItemList& items) = 0;
	virtual void LoadFailed(int windowId, int controlId, bool bErrorOccured = true) = 0;
	virtual ~IItemLoaderCallback() {}
};

class CBackgroundLoaderFactory {
public:
  static bool RunBackgroundLoader(CFileItemPtr pItem, bool bCanBlock);
};


/**
 * The purpose of this class is to load items in the background using GetDirectory
 * method and then return them
 *
 */

class CGetDirectoryJob : public BOXEE::BXBGJob
{
public:
	CGetDirectoryJob(const CStdString& strPath, int iWindowId, int iControlId, IItemLoaderCallback* pCallback);
	CGetDirectoryJob(const CStdString& strPath, int iWindowId, int iControlId, IItemLoaderCallback* pCallback, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem);
//	CGetDirectoryJob(CFileItemPtr pItem, int iWindowId, int iControlId, IItemLoaderCallback* pCallback);
//	CGetDirectoryJob(CFileItemPtr pItem, int iWindowId, int iControlId, IItemLoaderCallback* pCallback, SORT_METHOD iSortMethod, SORT_ORDER iSortOrder, int iSelectedItem);
	virtual ~CGetDirectoryJob();
	virtual void DoWork();
	virtual bool Equals(const BXBGJob& other) const;
private:
	CStdString m_strPath;
	CFileItemPtr m_pItem;
	int m_iWindowId;
	int m_iControlId;
	SORT_METHOD m_iSortMethod; 
	SORT_ORDER m_iSortOrder;
  int m_iSelectedItem;
	IItemLoaderCallback* m_pCallback;
};

class CLoadFileItemJob : public BOXEE::BXBGJob
{
public:
	CLoadFileItemJob(CFileItemPtr pItem, IBackgroundLoaderObserver* pCallback);
	virtual ~CLoadFileItemJob();
	virtual void DoWork();
private:
	CFileItemPtr m_pItem;
	IBackgroundLoaderObserver* m_pCallback;
};

class CLoadMetadataJob : public BOXEE::BXBGJob
{
public:
  CLoadMetadataJob(CFileItemPtr pItem, IBackgroundLoaderObserver* pCallback);
  virtual ~CLoadMetadataJob();
  virtual void DoWork();
private:
  CFileItemPtr m_pItem;
  IBackgroundLoaderObserver* m_pCallback;
};

class CLoadBackgroundImageJob : public BOXEE::BXBGJob
{
public:
  CLoadBackgroundImageJob(int windowId, const CStdString& strImagePath);
  virtual ~CLoadBackgroundImageJob();
  virtual void DoWork();
private:
  int m_windowId;
  CStdString m_strImagePath;
};

/**
 * The purpose of this class is to retrieve and load items used
 * in the UI containers
 */

class CItemLoader : public IItemLoaderCallback, IBackgroundLoaderObserver
{
public:
	CItemLoader();
	virtual ~CItemLoader();

	bool Init();
	int AddControl(int dwWindowId, int dwControlId, const CStdString& strPath, SORT_METHOD iSortMethod = SORT_METHOD_NONE, SORT_ORDER iSortOrder = SORT_ORDER_NONE, int iSelectedItem = -1);
	void AddControl(int dwWindowId, int dwControlId, CFileItemList items, SORT_METHOD iSortMethod = SORT_METHOD_NONE, SORT_ORDER iSortOrder = SORT_ORDER_NONE, int iSelectedItem = -1);
  
	// HHH: Remove this when not needed
	//void AddControl(int dwWindowId, int dwControlId, CFileItem* pItem, SORT_METHOD iSortMethod = SORT_METHOD_NONE, SORT_ORDER iSortOrder = SORT_ORDER_NONE, int iSelectedItem = -1);
  //void LoadImage(int dwWindowId, int dwControlId, const CStdString& strPath);
  
	void LoadBackgroundImage(int dwWindowId, const CStdString& strImagePath);
  
	void LoadFileMetadata(int dwWindowId, int dwControlId, CFileItem* pItem);
	void LoadItem(CFileItem* pItem, bool bAsyncOnly = false);

	// Implementation of IItemLoaderCallback
	virtual void ItemsLoaded(CFileItemList& items);
	virtual void LoadFailed(int windowId, int controlId, bool bErrorOccured = true);

	// Implementation of IBackgroundLoaderObserver
	virtual void OnItemLoaded(CFileItem* pItem);

  void SignalStop();
  void Stop();

  int GetQueueSize();
 
  void Pause(bool bVideoProcessorOnly = false);
  void Resume(bool bVideoProcessorOnly = false); 

  void AddItemLoadedObserver(const CStdString& strItemId , int windowId);
  void RemoveItemLoadedObserver(const CStdString& strItemId , int windowId);

private:
  
  std::map< CStdString , std::set<int> > m_itemLoadedObservers;
	// Maps each control to its path by control id and parent id

	BOXEE::BXBGProcess m_processor;
	BOXEE::BXBGProcess m_videoProcessor;
	BOXEE::BXBGProcess m_directoryProcessor;
	bool m_bStopped;
	//CCriticalSection    m_cs;

};

#endif /*ITEMLOADER_H_*/
