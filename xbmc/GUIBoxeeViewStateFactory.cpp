
#include "tinyXML/tinyxml.h"
#include "FileItem.h"
#include "GUIBoxeeViewStateFactory.h"
#include "URL.h"
#include "Util.h"
#include "GUISettings.h"
#include "utils/log.h"

#define LINE_VIEW                50
#define THUMB_WITH_PREVIEW_VIEW  51
#define THUMB_VIEW               52
#define DETAILED_VIEW            53

// Defintions of new set of views in Carla UI
#define THUMB_VIEW_LIST       50
#define LINE_VIEW_LIST        51
#define QUEUE_VIEW            52
#define MUSIC_THUMB_VIEW      53
#define MUSIC_LINE_VIEW       54
#define APP_THUMB_VIEW        55
#define APP_LINE_VIEW         56
#define SHORTCUTS_THUMB_VIEW  57
#define PHOTOS_THUMB_VIEW     53
#define PHOTOS_LINE_VIEW_LIST 54

using namespace std;

CGUIBoxeeViewStateFactory::CGUIBoxeeViewStateFactory()
{

}

CGUIBoxeeViewStateFactory::~CGUIBoxeeViewStateFactory()
{

}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::GetBoxeeViewState(int windowId, const CFileItemList& items)
{ 
  CGUIBoxeeViewState* boxeeViewState = NULL;

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::GetBoxeeViewState - Try to create GUIBoxeeViewState object by window id [%d].  (vns)",windowId);

  boxeeViewState = CreateViewStateByBrowseWindowType(windowId, items);

  if(!boxeeViewState)
  {
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::GetBoxeeViewState - Could not create GUIBoxeeViewState object by window id. Going to call CGUIBoxeeViewState::CreateBoxeeViewState(). [path=%s] (vns)",(items.m_strPath).c_str());
    boxeeViewState = CreateBoxeeViewState(items);
  }

  if(boxeeViewState)
  {
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::GetBoxeeViewState - Manage to create GUIBoxeeViewState object. Going to call CGUIBoxeeViewState::InitializeBoxeeViewState(). [path=%s] (vns)",(items.m_strPath).c_str());
    bool needToSaveToDB = boxeeViewState->InitializeBoxeeViewState(items,windowId);

    if(needToSaveToDB)
    {
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::GetBoxeeViewState - Call to CGUIBoxeeViewState::InitializeBoxeeViewState() return TRUE. Going to call SaveBoxeeViewState(). [path=%s] (vns)",(items.m_strPath).c_str());
      boxeeViewState->SaveBoxeeViewState();
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::GetBoxeeViewState - Failed to create CGUIBoxeeViewState for FileItemList with [path=%s]. Going to return NULL (vns)",(items.m_strPath).c_str());
  }

  return boxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::CreateViewStateByBrowseWindowType(int windowId, const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateViewStateByBrowseWindowType - Entered function. [windowId = %d] [path=%s] (vns)",windowId, (items.m_strPath).c_str());
  CGUIBoxeeViewState* boxeeViewState = new CGUIBoxeeViewState(items.m_strPath, windowId);
  std::vector<int> viewsToAdd;

  int defaultView;

  if (windowId == WINDOW_BOXEE_BROWSE_TVSHOWS)
  {
    viewsToAdd.push_back(THUMB_VIEW_LIST);
    viewsToAdd.push_back(LINE_VIEW_LIST);
    defaultView = THUMB_VIEW_LIST;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_TVEPISODES)
  {
    viewsToAdd.push_back(THUMB_VIEW_LIST);
    viewsToAdd.push_back(LINE_VIEW_LIST);
    defaultView = THUMB_VIEW_LIST;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_MOVIES)
  {
    viewsToAdd.push_back(THUMB_VIEW_LIST);
    viewsToAdd.push_back(LINE_VIEW_LIST);
    defaultView = THUMB_VIEW_LIST;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_ALBUMS)
  {
    viewsToAdd.push_back(MUSIC_THUMB_VIEW);
    viewsToAdd.push_back(MUSIC_LINE_VIEW);
    defaultView = MUSIC_THUMB_VIEW;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_SHORTCUTS)
  {
    viewsToAdd.push_back(SHORTCUTS_THUMB_VIEW);
    defaultView = SHORTCUTS_THUMB_VIEW;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_PHOTOS)
  {
    viewsToAdd.push_back(PHOTOS_LINE_VIEW_LIST);
    viewsToAdd.push_back(PHOTOS_THUMB_VIEW);
    defaultView = PHOTOS_THUMB_VIEW;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_APPS)
  {
    viewsToAdd.push_back(APP_THUMB_VIEW);
    viewsToAdd.push_back(APP_LINE_VIEW);
    defaultView = APP_THUMB_VIEW;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_QUEUE)
  {
    viewsToAdd.push_back(QUEUE_VIEW);
    defaultView = QUEUE_VIEW;
  }
  else if (windowId == WINDOW_BOXEE_BROWSE_DISCOVER)
  {
    viewsToAdd.push_back(THUMB_VIEW_LIST);
    viewsToAdd.push_back(LINE_VIEW_LIST);
    defaultView = THUMB_VIEW_LIST;
  }
  else
  {
    delete boxeeViewState;
    return NULL;
  }

  CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateViewStateByBrowseWindowType - Adding views. [viewsToAdd = %d] (vns)",(int)viewsToAdd.size());

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;
  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultView);

  boxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);

  std::vector<CBoxeeSort> boxeeSortsVec;
  CBoxeeSort boxeeSort("1", SORT_METHOD_NONE, SORT_ORDER_NONE, "", "start");
  boxeeSortsVec.push_back(boxeeSort);

  boxeeViewState->AddSortMethods(boxeeSortsVec,"1");

  return boxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::CreateBoxeeViewState(const CFileItemList& items)
{  
  CGUIBoxeeViewState* BoxeeViewState = NULL;

  // Get the ViewAndSort XML that was received from the server
  CStdString viewAndSortXml = items.GetProperty("ServerViewAndSort");

  if(viewAndSortXml.IsEmpty())
  {
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - For ItemList [path=%s], the value of the property ServerViewAndSort is [%s]. Going to call CreateDefaultBoxeeViewState() for this path (vns)",(items.m_strPath).c_str(),viewAndSortXml.c_str());

    BoxeeViewState = CreateDefaultBoxeeViewState(items);
  }
  else
  {
    // Going to parse the viewXml and create a suitable CGUIBoxeeViewState

    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - For ItemList [path=%s], the value of the property ServerViewAndSort is [%s]. Going to create the suitable CGUIBoxeeViewState for this path (vns)",(items.m_strPath).c_str(),viewAndSortXml.c_str());

    TiXmlDocument xmlDoc;
    xmlDoc.Parse(viewAndSortXml.c_str());

    const TiXmlElement* pRootElement = NULL;
    pRootElement = xmlDoc.RootElement();

    CStdString strValue = pRootElement->Value();
    if (strValue != "boxee:display")
    {
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - The root tag is [%s] which is NOT <boxee:display>. Going to call CreateDefaultBoxeeViewState() for path [%s] (vns)",strValue.c_str(),(items.m_strPath).c_str());
      BoxeeViewState = CreateDefaultBoxeeViewState(items);
    }

    std::vector<CBoxeeView> boxeeViewVec;
    CStdString defaultBoxeeViewId;
    std::vector<CBoxeeSort> boxeeSortVec;
    CStdString defaultBoxeeSortId;

    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - The root tag is [%s]. Going to call AnalyzeViewXml() for path [%s] (vns)",strValue.c_str(),(items.m_strPath).c_str());

    bool retVal = AnalyzeViewXml(pRootElement,boxeeViewVec,defaultBoxeeViewId,boxeeSortVec,defaultBoxeeSortId);

    if(retVal == true)
    {
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - Call to AnalyzeViewXml() for path [%s] returned TRUE. Going to call InitBoxeeViewState() [boxeeViewVecSize=%lu][defaultBoxeeViewId=%s][boxeeSortVecSize=%lu][defaultBoxeeSortId=%s] (vns)",(items.m_strPath).c_str(),boxeeViewVec.size(),defaultBoxeeViewId.c_str(),boxeeSortVec.size(),defaultBoxeeSortId.c_str());

      BoxeeViewState = InitBoxeeViewState(items,boxeeViewVec,defaultBoxeeViewId,boxeeSortVec,defaultBoxeeSortId);
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::CreateBoxeeViewState - Call to AnalyzeViewXml() returned FALSE [boxeeViewVecSize=%lu][defaultBoxeeViewId=%s][boxeeSortVecSize=%lu][defaultBoxeeSortId=%s]. Going to call CreateDefaultBoxeeViewState() for path [%s] (vns)",boxeeViewVec.size(),defaultBoxeeViewId.c_str(),boxeeSortVec.size(),defaultBoxeeSortId.c_str(),(items.m_strPath).c_str());
      BoxeeViewState = CreateDefaultBoxeeViewState(items);      
    }
  }

  return BoxeeViewState;
}

bool CGUIBoxeeViewStateFactory::AnalyzeViewXml(const TiXmlElement* pRootElement,std::vector<CBoxeeView>& boxeeViewVec,CStdString& defaultBoxeeViewId,std::vector<CBoxeeSort>& boxeeSortVec,CStdString& defaultBoxeeSortId)
{
  // Run over the file and search for <boxee:display> and <delete> tags
  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found [%s] tag (vns)",(pTag->ValueStr()).c_str());

    if(pTag->ValueStr() == "boxee:view")
    {
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found [%s] tag. Going to call AnalyzeBoxeeViewOptions() (vns)",(pTag->ValueStr()).c_str());

      AnalyzeBoxeeViewOptions(pTag,boxeeViewVec);

      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Call to AnalyzeBoxeeViewOptions() for tag [%s] returned [boxeeViewVecSize=%lu] (vns)",(pTag->ValueStr()).c_str(),boxeeViewVec.size());

      if(boxeeViewVec.size() > 0)
      {
        const TiXmlElement* boxeeViewElement = pTag->ToElement();
        if(boxeeViewElement)
        {
          defaultBoxeeViewId = boxeeViewElement->Attribute("default");

          //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found attribute [default] in tag [%s] and defaultBoxeeViewId was set to [%s] (vns)",(pTag->ValueStr()).c_str(),defaultBoxeeViewId.c_str());

          if(defaultBoxeeViewId.IsEmpty())
          {
            defaultBoxeeViewId = (boxeeViewVec[0]).m_id;
            CLog::Log(LOGWARNING,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Failed to get default id of view from [%s] tag while there are [%lu] view options. Set [%s] as the default id (vns)",(pTag->ValueStr()).c_str(),boxeeViewVec.size(),defaultBoxeeViewId.c_str());
          }
          else
          {
            // Check if defaultBoxeeViewId exist in the vector
            bool defaultBoxeeViewIdExist = false;
            for (size_t i = 0; i < boxeeViewVec.size(); i++)
            {
              CBoxeeView boxeeView = boxeeViewVec[i];

              if(defaultBoxeeViewId.Equals(boxeeView.m_id))
              {
                defaultBoxeeViewIdExist = true;
                //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - The default id [%s] of view from [%s] tag exist in the vector [Size=%lu] (vns)",defaultBoxeeViewId.c_str(),(pTag->ValueStr()).c_str(),boxeeViewVec.size());                
                break;
              }
            }

            if(defaultBoxeeViewIdExist == false)
            {
              defaultBoxeeViewId = (boxeeViewVec[0]).m_id;
              CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - The default id [%s] of view from [%s] tag doesn't exist in the vector [Size=%lu]. Set [%s] as the default id (vns)",defaultBoxeeViewId.c_str(),(pTag->ValueStr()).c_str(),boxeeViewVec.size(),defaultBoxeeViewId.c_str());                
            }
          }
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Failed to cast Tag [%s] from TiXmlNode to TiXmlElement (vns)",(pTag->ValueStr()).c_str());
          return false;
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - The size of the boxeeViewVec is [%lu] for [%s] tag (vns)",boxeeViewVec.size(),(pTag->ValueStr()).c_str());
        return false;        
      }
    }
    else if(pTag->ValueStr() == "boxee:sort")
    {
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found [%s] tag. Going to call AnalyzeBoxeeSortOptions() (vns)",(pTag->ValueStr()).c_str());

      AnalyzeBoxeeSortOptions(pTag,boxeeSortVec);

      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Call to AnalyzeBoxeeSortOptions() for tag [%s] returned [boxeeSortVecSize=%lu] (vns)",(pTag->ValueStr()).c_str(),boxeeSortVec.size());

      if(boxeeSortVec.size() > 0)
      {
        const TiXmlElement* boxeeSortElement = pTag->ToElement();
        if(boxeeSortElement)
        {
          defaultBoxeeSortId = boxeeSortElement->Attribute("default");

          //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found attribute [default=%s] in tag [%s] (vns)",(pTag->ValueStr()).c_str(),defaultBoxeeSortId.c_str());

          if(defaultBoxeeSortId.IsEmpty() || (IsDefaultBoxeeSortIdValid(boxeeSortVec,defaultBoxeeSortId) == false))
          {
            defaultBoxeeSortId = (boxeeSortVec[0]).m_id;
            CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Failed to get default id of sort from [%s] tag while there are [%lu] sort methods. Set [%s] as the default id (vns)",(pTag->ValueStr()).c_str(),boxeeSortVec.size(),defaultBoxeeSortId.c_str());
          }

          CStdString folderPositionInSort = boxeeSortElement->Attribute("folder-position");

          //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Found attribute [folder-position=%s] in tag [%s] (vns)",(pTag->ValueStr()).c_str(),folderPositionInSort.c_str());

          if(folderPositionInSort.IsEmpty() || (IsFolderPositionInSortValid(folderPositionInSort) == false))
          {
            folderPositionInSort = "start";
            CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Failed to get [folder-position] of sort from [%s] tag while there are [%lu] sort options. Set [start] as default (vns)",(pTag->ValueStr()).c_str(),boxeeSortVec.size());
          }

          // Add folderPositionInSort to each CBoxeeSort object
          for (size_t i = 0; i < boxeeSortVec.size(); i++)
          {
            CBoxeeSort* boxeeSort = &(boxeeSortVec[i]);
            boxeeSort->m_folderPosition = folderPositionInSort;
          }
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Failed to cast Tag [%s] from TiXmlNode to TiXmlElement (vns)",(pTag->ValueStr()).c_str());
          return false;
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - The size of the boxeeSortVec is [%lu] for [%s] tag (vns)",boxeeSortVec.size(),(pTag->ValueStr()).c_str());
        return false;        
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - Tag [%s] was not handled (vns)",(pTag->ValueStr()).c_str());
    }    
  }

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeViewXml - After Analyzing [%s] going to return with [boxeeViewVecSize=%lu][defaultBoxeeViewId=%s][boxeeSortVecSize=%lu][defaultBoxeeSortId=%s] (vns)",pRootElement->Value(),boxeeViewVec.size(),defaultBoxeeViewId.c_str(),boxeeSortVec.size(),defaultBoxeeSortId.c_str());
  return true;
}

void CGUIBoxeeViewStateFactory::AnalyzeBoxeeViewOptions(const TiXmlNode* pBoxeeViewTag,std::vector<CBoxeeView>& boxeeViewsVec)
{
  // Run over the <boxee:view> tag and handle <boxee:view-option> elements

  const TiXmlElement* boxeeViewOption = pBoxeeViewTag->FirstChildElement("boxee:view-option");
  while (boxeeViewOption)
  {
    CBoxeeView boxeeView;
    boxeeView.m_id = boxeeViewOption->Attribute("id");
    boxeeView.m_type = CGUIBoxeeViewState::GetViewTypeAsEnum(boxeeViewOption->Attribute("view-type"));
    boxeeViewsVec.push_back(boxeeView);

    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeViewOptions - After adding CBoxeeView object [type=%s=%d] to ViewsMap (vns)",(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type);

    boxeeViewOption = boxeeViewOption->NextSiblingElement("boxee:view-option");
  }

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeViewOptions - After Analyzing [%s] element, ViewsMap contains [%lu] CBoxeeView objects (vns)",pBoxeeViewTag->Value(),boxeeViewsVec.size());
}

void CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions(const TiXmlNode* pBoxeeSortTag,std::vector<CBoxeeSort>& boxeeSortsVec)
{
  int counter = 1;

  CStdString sortId;
  CStdString sortMethod;
  CStdString sortOrder;
  CStdString sortName;

  // Run over the <boxee:view> tag and handle <boxee:view-option> elements

  const TiXmlElement* boxeeSortOption = pBoxeeSortTag->FirstChildElement("boxee:sort-option");
  while (boxeeSortOption)
  {
    CBoxeeSort boxeeSort;

    sortId = boxeeSortOption->Attribute("id");
    if(sortId.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - Failed to read Attribute [id]. Continue to next <boxee:sort-option> tag [%d] (vns)",counter);
      continue;
    }
    boxeeSort.m_id = sortId;

    sortMethod = boxeeSortOption->Attribute("sort-by");
    if(sortMethod.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - Failed to read Attribute [sort-by]. Going to set it to [none] [%d] (vns)",counter);
      sortMethod = "none";
    }
    boxeeSort.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum(sortMethod);

    sortOrder = boxeeSortOption->Attribute("sort-order");
    if(sortOrder.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - Failed to read Attribute [sort-order]. Going to set it to [none] [%d] (vns)",counter);
      sortOrder = "none";      
    }
    boxeeSort.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum(sortOrder);

    sortName = boxeeSortOption->Attribute("sort-type-name");
    if(sortName.IsEmpty())
    {
      CLog::Log(LOGWARNING,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - Failed to read Attribute [sort-type-name]. Going to set it to [default] [%d] (vns)",counter);
      sortName = "Default";            
    }
    boxeeSort.m_sortName = sortName;

    boxeeSortsVec.push_back(boxeeSort);

    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - After adding CBoxeeSort object [SortId=%s][sortMethod=%s=%d][sortOrder=%s=%d][sortName=%s=%d] to SortsVec (vns)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),CGUIBoxeeViewState::GetSortNameAsInt(boxeeSort.m_sortName));

    boxeeSortOption = boxeeSortOption->NextSiblingElement("boxee:sort-option");

    counter++;
  }

  CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AnalyzeBoxeeSortOptions - After Analyzing [%s] element, boxeeSortsVec contains [%lu] CBoxeeSort objects (vns)",pBoxeeSortTag->Value(),boxeeSortsVec.size());
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitBoxeeViewState(const CFileItemList& items,std::vector<CBoxeeView>& boxeeViewVec,CStdString& defaultBoxeeViewId,std::vector<CBoxeeSort>& boxeeSortVec,CStdString& defaultBoxeeSortId)
{
  CGUIBoxeeViewState* boxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  boxeeViewState->AddViewTypes(boxeeViewVec,defaultBoxeeViewId);
  boxeeViewState->AddSortMethods(boxeeSortVec,defaultBoxeeSortId);

  return boxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState(const CFileItemList& items)
{
  const CURI& url=items.GetAsUrl();
  CStdString strUrl;
  url.GetURL(strUrl);

  //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Enter functon with FileItemList [path=%s]. [url=%s][protocol=%s][host=%s][share=%s][domain=%s][filename=%s][property browsemode=%d] (vns)(bvs)",(items.m_strPath).c_str(),strUrl.c_str(),url.GetProtocol().c_str(),url.GetHostName().c_str(),url.GetShareName().c_str(),url.GetDomain().c_str(),url.GetFileName().c_str(),items.GetPropertyInt("browsemode"));

  CGUIBoxeeViewState* DefaultBoxeeViewState = NULL;

  // IMPORTANT: In Carla version, default view state will always be returned for browse window
  if(DefaultBoxeeViewState == NULL)
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Failed to allocate a BoxeeViewState for protocol is [path=%s], therefore going to call InitializeDefaultBoxeeViewState() (vns)(bvs)",(items.m_strPath).c_str());
    DefaultBoxeeViewState = InitializeDefaultBoxeeViewState(items);
  }

  if(url.GetProtocol() == "rss")
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Protocol is [%s], therefore going to call InitializeDefaultRssBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),(items.m_strPath).c_str());

    DefaultBoxeeViewState = InitializeDefaultRssBoxeeViewState(items);
  }
  else if(url.GetProtocol() == "apps")
  {
    if(url.GetHostName() == "all" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultAppAllBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultAppAllBoxeeViewState(items); // CGUIViewStateBoxee1
    }

    else if(url.GetHostName() == "video" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultAppVideoBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultAppVideoBoxeeViewState(items); // CGUIViewStateBoxee1
    }

    else if(url.GetHostName() == "music" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultAppMusicBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultAppMusicBoxeeViewState(items); // CGUIViewStateBoxee1
    }

    else if(url.GetHostName() == "pictures" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultAppPictureBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultAppPictureBoxeeViewState(items); // CGUIViewStateBoxee1      
    }
  }
  else if(url.GetProtocol() == "history")
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s], therefore going to call InitializeDefaultHistoryBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),(items.m_strPath).c_str());
    DefaultBoxeeViewState = InitializeDefaultHistoryBoxeeViewState(items); // CGUIViewStateBoxee9
  }
  else if(url.GetProtocol() == "boxeedb")
  {
    if(url.GetHostName() == "series" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultBoxeeDbSeriesBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBoxeeDbSeriesBoxeeViewState(items); // CGUIViewStateBoxee5
    }
    else if(url.GetHostName() == "season" && CUtil::HasSlashAtEnd(strUrl))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][host=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultBoxeeDbSeasonBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),url.GetHostName().c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBoxeeDbSeasonBoxeeViewState(items); // CGUIViewStateBoxee7
    }
    else
    {
      if(items.GetProperty("browsemode") == BROWSE_MODE_VIDEO)
      {
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_VIDEO=%d], therefore going to call InitializeDefaultBrowseModeVideoBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
        DefaultBoxeeViewState = InitializeDefaultBrowseModeVideoBoxeeViewState(items); // CGUIViewStateBoxeeBrowseVideo
      }

      else if(items.GetProperty("browsemode") == BROWSE_MODE_MUSIC)
      {
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_MUSIC=%d], therefore going to call InitializeDefaultBrowseModeMusicBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
        DefaultBoxeeViewState = InitializeDefaultBrowseModeMusicBoxeeViewState(items); // CGUIViewStateBoxee2
      }

      else if(items.GetProperty("browsemode") == BROWSE_MODE_PICTURES)
      {
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_PICTURES=%d], therefore going to call InitializeDefaultBrowseModePictureBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
        DefaultBoxeeViewState = InitializeDefaultBrowseModePictureBoxeeViewState(items); // CGUIViewStateBoxeeBrowsePictures
      }

      else if(items.GetProperty("browsemode") == BROWSE_MODE_OTHER)
      {
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_OTHER=%d], therefore going to call InitializeDefaultBrowseModeOtherBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
        DefaultBoxeeViewState = InitializeDefaultBrowseModeOtherBoxeeViewState(items); // CGUIViewStateBoxeeBrowseOther
      }
    }
  }
  else if(url.GetProtocol() == "shout")
  {
    if(CUtil::HasSlashAtEnd(url.GetFileName()))
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultShoutWithSlashBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultShoutWithSlashBoxeeViewState(items); // CGUIViewStateMusicShoutcast      
    }    
    else
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s][HasSlashAtEnd=%d], therefore going to call InitializeDefaultShoutNoSlashBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),CUtil::HasSlashAtEnd(strUrl),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultShoutNoSlashBoxeeViewState(items); // CGUIViewStateMusicShoutcastGenre      
    }
  }
  else if(url.GetProtocol() == "lastfm")
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - [protocol=%s], therefore going to call InitializeDefaultLastfmBoxeeViewState() for [path=%s] (vns)(bvs)",(url.GetProtocol()).c_str(),(items.m_strPath).c_str());
    DefaultBoxeeViewState = InitializeDefaultLastfmBoxeeViewState(items); // CGUIViewStateBoxee5          
  }
  else
  {
    if(items.GetProperty("browsemode") == BROWSE_MODE_VIDEO)
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_VIDEO=%d], therefore going to call InitializeDefaultBrowseModeVideoBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBrowseModeVideoBoxeeViewState(items); // CGUIViewStateBoxeeBrowseVideo
    }

    else if(items.GetProperty("browsemode") == BROWSE_MODE_MUSIC)
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_MUSIC=%d], therefore going to call InitializeDefaultBrowseModeMusicBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBrowseModeMusicBoxeeViewState(items); // CGUIViewStateBoxee2
    }

    else if(items.GetProperty("browsemode") == BROWSE_MODE_PICTURES)
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_PICTURES=%d], therefore going to call InitializeDefaultBrowseModePictureBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBrowseModePictureBoxeeViewState(items); // CGUIViewStateBoxeeBrowsePictures
    }

    else if(items.GetProperty("browsemode") == BROWSE_MODE_OTHER)
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Property [browsemode=BROWSE_MODE_OTHER=%d], therefore going to call InitializeDefaultBrowseModeOtherBoxeeViewState() for [path=%s] (vns)(bvs)",(items.GetPropertyInt("browsemode")),(items.m_strPath).c_str());
      DefaultBoxeeViewState = InitializeDefaultBrowseModeOtherBoxeeViewState(items); // CGUIViewStateBoxeeBrowseOther
    }
  }

  if(DefaultBoxeeViewState == NULL)
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewStateFactory::CreateDefaultBoxeeViewState - Failed to allocate a BoxeeViewState for protocol is [path=%s], therefore going to call InitializeDefaultBoxeeViewState() (vns)(bvs)",(items.m_strPath).c_str());
    DefaultBoxeeViewState = InitializeDefaultBoxeeViewState(items);
  }

  return DefaultBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath, WINDOW_BOXEE_BROWSE);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  //viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  //viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSort;
  boxeeSort.m_id = "1";
  boxeeSort.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSort.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSort.m_sortName = "Name";
  boxeeSort.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSort);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultRssBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultRssBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSort;
  boxeeSort.m_id = "1";
  boxeeSort.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("none");
  boxeeSort.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSort.m_sortName = "Default";
  boxeeSort.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSort);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultRssBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultRssBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultRssBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultRssBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultAppAllBoxeeViewState(const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppAllBoxeeViewState - Going to call InitializeDefaultAppBoxeeViewState() in order to create CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = InitializeDefaultAppBoxeeViewState(items);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppAllBoxeeViewState - Going to return CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultAppVideoBoxeeViewState(const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppVideoBoxeeViewState - Going to call InitializeDefaultAppBoxeeViewState() in order to create CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = InitializeDefaultAppBoxeeViewState(items);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppVideoBoxeeViewState - Going to return CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultAppMusicBoxeeViewState(const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppMusicBoxeeViewState - Going to call InitializeDefaultAppBoxeeViewState() in order to create CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  CGUIBoxeeViewState* defaultAppMusicBoxeeViewState = InitializeDefaultAppBoxeeViewState(items);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppMusicBoxeeViewState - Going to return CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  return defaultAppMusicBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultAppPictureBoxeeViewState(const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppPictureBoxeeViewState - Going to call InitializeDefaultAppBoxeeViewState() in order to create CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  CGUIBoxeeViewState* defaultAppPictureBoxeeViewState = InitializeDefaultAppBoxeeViewState(items);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppPictureBoxeeViewState - Going to return CGUIBoxeeViewState object for FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  return defaultAppPictureBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultAppBoxeeViewState(const CFileItemList& items)
{
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppBoxeeViewState - Enter function with FileItemList [path=%s] (vns)",(items.m_strPath).c_str());

  CGUIBoxeeViewState* defaultAppBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSort;
  boxeeSort.m_id = "1";
  boxeeSort.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSort.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSort.m_sortName = "Name";
  boxeeSort.m_folderPosition = "start";

  CStdString defaultSortId = "1";

  boxeeSortsVec.push_back(boxeeSort);

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultAppBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultAppBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultAppBoxeeViewState - Created a CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultAppBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginVideoBlipTvBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  CBoxeeSort boxeeSort;
  boxeeSort.m_id = "1";
  boxeeSort.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSort.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSort.m_sortName = "Name";
  boxeeSort.m_folderPosition = "start";

  CStdString defaultSortId = "1";

  std::vector<CBoxeeSort> boxeeSortsVec;
  boxeeSortsVec.push_back(boxeeSort);

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultAppVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultAppVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginVideoBlipTvBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginVideoMovieTrailersBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_WITH_PREVIEW_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabelWithShare;
  boxeeSortLabelWithShare.m_id = "1";
  boxeeSortLabelWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");  
  boxeeSortLabelWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabelWithShare.m_sortName = "Name";
  boxeeSortLabelWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabelWithShare);

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "2";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");  
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultAppVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultAppVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginVideoMovieTrailersBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginYoutubeBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultAppVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultAppVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginYoutubeBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginFlickrBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultAppVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultAppVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultAppVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginFlickrBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultAppVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginSeeqpodBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultPluginSeeqpodBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultPluginSeeqpodBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultPluginSeeqpodBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginSeeqpodBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultPluginSeeqpodBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginCnnVideoWithSlashBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultPluginCnnVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CBoxeeSort boxeeSortDate;
  boxeeSortDate.m_id = "2";
  boxeeSortDate.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");
  boxeeSortDate.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDate.m_sortName = "Date";
  boxeeSortDate.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDate);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultPluginCnnVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultPluginCnnVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginCnnVideoWithSlashBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultPluginCnnVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultPluginCnnVideoNoSlashBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultPluginCnnVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CBoxeeSort boxeeSortDate;
  boxeeSortDate.m_id = "2";
  boxeeSortDate.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");
  boxeeSortDate.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDate.m_sortName = "Date";
  boxeeSortDate.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDate);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultPluginCnnVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultPluginCnnVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultPluginCnnVideoNoSlashBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultPluginCnnVideoBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultHistoryBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultHistoryBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("none");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("none");
  boxeeSortLabel.m_sortName = "Default";
  boxeeSortLabel.m_folderPosition = "integral";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultHistoryBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultHistoryBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultHistoryBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultHistoryBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeDbSeriesBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultDbSeriesBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultDbSeriesBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultDbSeriesBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeDbSeriesBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultDbSeriesBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeDbSeasonBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultDbSeasonBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortEpisode;
  boxeeSortEpisode.m_id = "1";
  boxeeSortEpisode.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("episode");
  boxeeSortEpisode.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortEpisode.m_sortName = "Episode";
  boxeeSortEpisode.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortEpisode);

  CBoxeeSort boxeeSortLabelWithShare;
  boxeeSortLabelWithShare.m_id = "2";
  boxeeSortLabelWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabelWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabelWithShare.m_sortName = "Name";
  boxeeSortLabelWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabelWithShare);

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "3";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultDbSeasonBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultDbSeasonBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBoxeeDbSeasonBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultDbSeasonBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultShoutWithSlashBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultShoutWithSlashBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultShoutWithSlashBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultShoutWithSlashBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultShoutWithSlashBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultShoutWithSlashBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultShoutNoSlashBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultShoutNoSlashBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultShoutNoSlashBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultShoutNoSlashBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultShoutNoSlashBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultShoutNoSlashBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultLastfmBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultLastfmBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "end";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultLastfmBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultLastfmBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultLastfmBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultLastfmBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeVideoBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultBrowseModeVideoBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  const CURI& url=items.GetAsUrl();

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CStdString defaultSortId;

  CBoxeeSort boxeeSortLabel;
  boxeeSortLabel.m_id = "1";
  boxeeSortLabel.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label_ignore_the");  
  boxeeSortLabel.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabel.m_sortName = "Name";
  boxeeSortLabel.m_folderPosition = "integral";
  boxeeSortsVec.push_back(boxeeSortLabel);

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "2";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");  
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  if (url.GetHostName() == "season")
  {
    CBoxeeSort boxeeSortEpisode;
    boxeeSortEpisode.m_id = "3";
    boxeeSortEpisode.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("episode");  
    boxeeSortEpisode.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
    boxeeSortEpisode.m_sortName = "Episode";
    boxeeSortEpisode.m_folderPosition = "start";
    boxeeSortsVec.push_back(boxeeSortEpisode);

    defaultSortId = "3";
  }
  else if(url.GetHostName() == "rss")
  {
    defaultSortId = "2";
  }
  else
  {
    defaultSortId = "1";    
  }

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultBrowseModeVideoBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultBrowseModeVideoBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeVideoBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultBrowseModeVideoBoxeeViewState;

}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeMusicBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultBrowseModeMusicBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = THUMB_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabelWithShare;
  boxeeSortLabelWithShare.m_id = "1";
  boxeeSortLabelWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label_ignore_the");  
  boxeeSortLabelWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabelWithShare.m_sortName = "Name";
  boxeeSortLabelWithShare.m_folderPosition = "integral";
  boxeeSortsVec.push_back(boxeeSortLabelWithShare);

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "2";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");  
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  CBoxeeSort boxeeSortAlbumYear;
  boxeeSortAlbumYear.m_id = "3";
  boxeeSortAlbumYear.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("year");  
  boxeeSortAlbumYear.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortAlbumYear.m_sortName = "Year";
  boxeeSortAlbumYear.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortAlbumYear);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultBrowseModeMusicBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultBrowseModeMusicBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeMusicBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultBrowseModeMusicBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModePictureBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultBrowseModePictureBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CBoxeeSort boxeeSortLabelWithShare;
  boxeeSortLabelWithShare.m_id = "1";
  boxeeSortLabelWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");  
  boxeeSortLabelWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabelWithShare.m_sortName = "Name";
  boxeeSortLabelWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabelWithShare);

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "2";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");  
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  CStdString defaultSortId = "1";

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultBrowseModePictureBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultBrowseModePictureBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModePictureBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultBrowseModePictureBoxeeViewState;
}

CGUIBoxeeViewState* CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeOtherBoxeeViewState(const CFileItemList& items)
{
  CGUIBoxeeViewState* defaultBrowseModeOtherBoxeeViewState = new CGUIBoxeeViewState(items.m_strPath);

  const CURI& url=items.GetAsUrl();

  //////////
  // View //
  //////////

  std::vector<int> viewsToAdd;
  viewsToAdd.push_back(LINE_VIEW);
  viewsToAdd.push_back(THUMB_WITH_PREVIEW_VIEW);
  viewsToAdd.push_back(THUMB_VIEW);
  viewsToAdd.push_back(DETAILED_VIEW);

  int defaultViewType = LINE_VIEW;  

  std::vector<CBoxeeView> boxeeViewsVec;
  CStdString defaultViewId;

  AddViewsToBoxeeViewVec(boxeeViewsVec,defaultViewId,viewsToAdd,defaultViewType);

  //////////
  // Sort //
  //////////

  std::vector<CBoxeeSort> boxeeSortsVec;

  CStdString defaultSortId;

  CBoxeeSort boxeeSortDateWithShare;
  boxeeSortDateWithShare.m_id = "1";
  boxeeSortDateWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date");  
  boxeeSortDateWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
  boxeeSortDateWithShare.m_sortName = "Date";
  boxeeSortDateWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortDateWithShare);

  CBoxeeSort boxeeSortLabelWithShare;
  boxeeSortLabelWithShare.m_id = "2";
  boxeeSortLabelWithShare.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("label");  
  boxeeSortLabelWithShare.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("ascending");
  boxeeSortLabelWithShare.m_sortName = "Name";
  boxeeSortLabelWithShare.m_folderPosition = "start";
  boxeeSortsVec.push_back(boxeeSortLabelWithShare);

  if (url.GetHostName() == "activity" || url.GetHostName() == "recommend" || url.GetHostName() == "all")
  {
    defaultSortId = "1";
  }
  else if (url.GetHostName() == "recent")
  {
    CBoxeeSort boxeeSortDateAdded;
    boxeeSortDateAdded.m_id = "3";
    boxeeSortDateAdded.m_sortMethod = CGUIBoxeeViewState::GetSortMethodAsEnum("date added");  
    boxeeSortDateAdded.m_sortOrder = CGUIBoxeeViewState::GetSortOrderAsEnum("descending");
    boxeeSortDateAdded.m_sortName = "Date Added";
    boxeeSortDateAdded.m_folderPosition = "start";
    boxeeSortsVec.push_back(boxeeSortDateAdded);

    defaultSortId = "3";
  }
  else
  {
    defaultSortId = "2";
  }

  /////////////////////////
  // Apply view and sort //
  /////////////////////////

  defaultBrowseModeOtherBoxeeViewState->AddViewTypes(boxeeViewsVec,defaultViewId);
  defaultBrowseModeOtherBoxeeViewState->AddSortMethods(boxeeSortsVec,defaultSortId);

  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::InitializeDefaultBrowseModeOtherBoxeeViewState - Going to return CGUIBoxeeViewState object for items [path=%s] with [defaultViewId=%s][defaultSortId=%s] (vns)",(items.m_strPath).c_str(),defaultViewId.c_str(),defaultSortId.c_str());

  return defaultBrowseModeOtherBoxeeViewState;
}

void CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec(std::vector<CBoxeeView>& boxeeViewsVec,CStdString& defaultViewId,std::vector<int>& viewsToAdd,int defaultViewType)
{
  CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - Enter function with [viewsToAddSize=%lu][ViewDefault=%d] (vns)(av)",viewsToAdd.size(),defaultViewType);

  for (size_t i = 0; i < viewsToAdd.size(); i++)
  {
    CBoxeeView boxeeView;

    // Set view id as a string (starting from 1)
    boxeeView.m_id.Format("%lu", i+1);

    int viewType = viewsToAdd[i];

    CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - Got from viewsToVec [view=%d]. [%lu/%lu] (vns)(av)",viewType,i+1,viewsToAdd.size());

    boxeeView.m_type = viewType;

    //    switch(viewType)
    //    {
    //    case LINE_VIEW:
    //      boxeeView.m_type = CGUIBoxeeViewState::GetViewTypeAsEnum("line");
    //      break;
    //    case THUMB_WITH_PREVIEW_VIEW:
    //      boxeeView.m_type = CGUIBoxeeViewState::GetViewTypeAsEnum("thumb with preview");
    //      break;
    //    case THUMB_VIEW:
    //      boxeeView.m_type = CGUIBoxeeViewState::GetViewTypeAsEnum("thumb");
    //      break;
    //    case DETAILED_VIEW:
    //      boxeeView.m_type = CGUIBoxeeViewState::GetViewTypeAsEnum("detailed list");
    //      break;
    //    default:
    //      CLog::Log(LOGERROR,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - Failed to add [ViewType=%d] to boxeeViewsVec (vns)(av)",viewType);
    //      continue;
    //    }

    if(viewType == defaultViewType)
    {
      defaultViewId = boxeeView.m_id;

      CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - [viewType=%d]==[defaultViewType=%d]=%s -> defaultViewId was set to [%s] (vns)(av)",viewType,defaultViewType,(CGUIBoxeeViewState::GetViewTypeAsString(viewType)).c_str(),defaultViewId.c_str());
    }

    boxeeViewsVec.push_back(boxeeView);

    CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - Going to add to boxeeViewsVec CBoxeeView [viewId=%s][ViewType=%s=%d]. [%lu/%lu] (vns)(av)",(boxeeView.m_id).c_str(),(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type,i+1,viewsToAdd.size());
  }

  CLog::Log(LOGDEBUG,"CGUIBoxeeViewStateFactory::AddViewsToBoxeeViewVec - Going to return from function [boxeeViewsVecSize=%lu] (vns)(av)",boxeeViewsVec.size());
}

bool CGUIBoxeeViewStateFactory::IsDefaultBoxeeSortIdValid(std::vector<CBoxeeSort>& boxeeSortVec,const CStdString& defaultBoxeeSortId)
{
  for (size_t i = 0; i < boxeeSortVec.size(); i++)
  {
    CBoxeeSort boxeeSort = boxeeSortVec[i];

    if(defaultBoxeeSortId.Equals(boxeeSort.m_id))
    {
      return true;
    }
  }

  return false;
}

bool CGUIBoxeeViewStateFactory::IsFolderPositionInSortValid(const CStdString& folderPositionInSort)
{
  if(folderPositionInSort.Equals("start"))
  {
    return true;
  }
  else if(folderPositionInSort.Equals("end"))
  {
    return true;
  }
  else if(folderPositionInSort.Equals("integral"))
  {
    return true;
  }
  else
  {
    return false;
  }
}
