#ifndef GUIBOXEEVIEWSTATEFACTORY_H_
#define GUIBOXEEVIEWSTATEFACTORY_H_

#include "SortFileItem.h"
#include "GUIBoxeeViewState.h"

class CGUIBoxeeViewStateFactory
{
public:
  CGUIBoxeeViewStateFactory();
  virtual ~CGUIBoxeeViewStateFactory();

  static CGUIBoxeeViewState* GetBoxeeViewState(int windowId, const CFileItemList& items);

protected:
  
private:

  static void AddViewsToBoxeeViewVec(std::vector<CBoxeeView>& boxeeViewsVec,CStdString& defaultViewId,std::vector<int>& viewsToAdd,int defaultViewType);
  
  static bool IsDefaultBoxeeSortIdValid(std::vector<CBoxeeSort>& boxeeSortVec,const CStdString& defaultBoxeeSortId);
  static bool IsFolderPositionInSortValid(const CStdString& folderPositionInSort);
  
  static CGUIBoxeeViewState* CreateBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* CreateViewStateByBrowseWindowType(int windowId, const CFileItemList& items);
  static CGUIBoxeeViewState* InitBoxeeViewState(const CFileItemList& items,std::vector<CBoxeeView>& boxeeViewVec,CStdString& defaultBoxeeViewId,std::vector<CBoxeeSort>& boxeeSortVec,CStdString& defaultBoxeeSortId);
  static CGUIBoxeeViewState* CreateDefaultBoxeeViewState(const CFileItemList& items);
  static bool AnalyzeViewXml(const TiXmlElement* pRootElement,std::vector<CBoxeeView>& boxeeViewVec,CStdString& defaultBoxeeViewId,std::vector<CBoxeeSort>& boxeeSortVec,CStdString& defaultBoxeeSortId);
  static void AnalyzeBoxeeViewOptions(const TiXmlNode* pBoxeeViewTag,std::vector<CBoxeeView>& boxeeViewsVec);
  static void AnalyzeBoxeeSortOptions(const TiXmlNode* pBoxeeSortTag,std::vector<CBoxeeSort>& boxeeSortsVec);

  static CGUIBoxeeViewState* InitializeDefaultBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultRssBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultAppBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultAppAllBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultAppVideoBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultAppMusicBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultAppPictureBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginVideoBlipTvBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginVideoMovieTrailersBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginYoutubeBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginFlickrBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginSeeqpodBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginCnnVideoWithSlashBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultPluginCnnVideoNoSlashBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultHistoryBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBoxeeDbSeriesBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBoxeeDbSeasonBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultShoutWithSlashBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultShoutNoSlashBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultLastfmBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBrowseModeVideoBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBrowseModeMusicBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBrowseModePictureBoxeeViewState(const CFileItemList& items);
  static CGUIBoxeeViewState* InitializeDefaultBrowseModeOtherBoxeeViewState(const CFileItemList& items);  
};

#endif /*GUIBOXEEVIEWSTATEFACTORY_H_*/

