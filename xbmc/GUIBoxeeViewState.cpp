
#include "FileItem.h"
#include "Key.h"
#include "GUIBoxeeViewState.h"
#include "BoxeeViewDatabase.h"
#include "FileItem.h"
#include "utils/log.h"

using namespace std;


CGUIBoxeeViewState::CGUIBoxeeViewState(const CStdString& path, int windowId) : m_path(path), m_windowId(windowId)
{

}

CGUIBoxeeViewState::~CGUIBoxeeViewState()
{

}

const CStdString& CGUIBoxeeViewState::GetPath()
{
  return m_path;
}

//////////////////
// View Section //
//////////////////

void CGUIBoxeeViewState::AddViewTypes(std::vector<CBoxeeView>& viewTypesVec,const CStdString& chosenViewTypeId)
{
  for (size_t i=0; i<viewTypesVec.size(); i++)
  {
    CBoxeeView boxeeView = viewTypesVec[i];
    m_viewTypesVec.push_back(boxeeView);
  }

  m_chosenViewTypeKey = chosenViewTypeId;
  
  CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::AddViewMethods - After addind [%d] view types and set [%s] as the chosen view (vns)",(int)m_viewTypesVec.size(),m_chosenViewTypeKey.c_str());
}

void CGUIBoxeeViewState::GetViewTypes(std::vector<CBoxeeView>& viewVec)
{
  for (size_t i=0; i<m_viewTypesVec.size(); i++)
  {
    CBoxeeView boxeeView = m_viewTypesVec[i];
    viewVec.push_back(boxeeView);
  }
}

bool CGUIBoxeeViewState::GetBoxeeView(CBoxeeView& boxeeView) const
{
  for (size_t i=0; i<m_viewTypesVec.size(); i++)
  {
    CBoxeeView boxeeViewTmp = m_viewTypesVec[i];
    
    if(m_chosenViewTypeKey.Equals(boxeeViewTmp.m_id))
    {
      boxeeView = boxeeViewTmp;
      return true;
    }
  }
  
  CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetBoxeeView - Failed to get BoxeeView for key [m_chosenViewTypeKey=%s]. Going to return FALSE (vns)",m_chosenViewTypeKey.c_str());

  return false;
}

int CGUIBoxeeViewState::GetViewType() const
{
  CBoxeeView boxeeView;
  
  bool success = GetBoxeeView(boxeeView);
  
  if(success)
  {
    return boxeeView.m_type;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetViewType - Failed to get ViewType. Going to return [Line=50] (vns)");
    return 50;    
  }
}

bool CGUIBoxeeViewState::SetViewType(int viewTypeByType)
{
  for (size_t i=0; i<m_viewTypesVec.size(); i++)
  {
    CBoxeeView boxeeView = m_viewTypesVec[i];
    
    if(viewTypeByType == boxeeView.m_type)
    {
      m_chosenViewTypeKey = boxeeView.m_id;
      
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::SetViewType - View type [%s=%d] was set as the chosen view [id=%s] (vns)",(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type,m_chosenViewTypeKey.c_str());

      return true;
    }
  }
  
  CLog::Log(LOGERROR,"CGUIBoxeeViewState::SetViewType - Failed to set ViewType [%d] which is not in the ViewTypesVec (vns)",viewTypeByType);
  return false;  
}

bool CGUIBoxeeViewState::SetBoxeeViewAsChosen(CBoxeeView& boxeeView)
{
  bool success = SetViewType(boxeeView.m_type);
  
  if(success)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//////////////////
// Sort Section //
//////////////////

void CGUIBoxeeViewState::AddSortMethods(std::vector<CBoxeeSort>& sortMethodsVec,const CStdString& chosenSortMedthodId)
{
  for (size_t i=0; i<sortMethodsVec.size(); i++)
  {
    CBoxeeSort boxeeSort = sortMethodsVec[i];
    m_sortMethodsVec.push_back(boxeeSort);
  }

  m_chosenSortMethodKey = chosenSortMedthodId;
  
  //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::AddSortMethods - After addind [%d] sort methods and set [%s] as the chosen sort (vns)",m_sortMethodsVec.size(),m_chosenSortMethodKey.c_str());
}

void CGUIBoxeeViewState::GetSortMethods(std::vector<CBoxeeSort>& sortVec)
{
  for (size_t i=0; i<m_sortMethodsVec.size(); i++)
  {
    CBoxeeSort boxeeSort = m_sortMethodsVec[i];
    sortVec.push_back(boxeeSort);
  }
}

bool CGUIBoxeeViewState::GetBoxeeSort(CBoxeeSort& boxeeSort) const
{
  for (size_t i=0; i<m_sortMethodsVec.size(); i++)
  {
    CBoxeeSort boxeeSortTmp = m_sortMethodsVec[i];
    
    if(m_chosenSortMethodKey.Equals(boxeeSortTmp.m_id))
    {
      boxeeSort = boxeeSortTmp;
      return true;
    }
  }
  
  //CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetBoxeeSort - Failed to find m_chosenSortMethodKey [%s] in m_sortMethodsVec. [m_sortMethodsVecSize=%d] (vns)",m_chosenSortMethodKey.c_str(),m_sortMethodsVec.size());
  return false;
}

bool CGUIBoxeeViewState::SetBoxeeSortAsChosen(CBoxeeSort& boxeeSort)
{
  for (size_t i=0; i<m_sortMethodsVec.size(); i++)
  {
    CBoxeeSort boxeeSortTmp = m_sortMethodsVec[i];
     
    if((boxeeSort.m_sortMethod == boxeeSortTmp.m_sortMethod) &&
       (boxeeSort.m_sortOrder == boxeeSortTmp.m_sortOrder) &&
       ((boxeeSort.m_sortName).Equals(boxeeSortTmp.m_sortName)) &&
       ((boxeeSort.m_folderPosition).Equals(boxeeSortTmp.m_folderPosition)))
    {
      m_chosenSortMethodKey = boxeeSortTmp.m_id;
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::SetBoxeeSortAsChosen - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] was set as chosen. [m_sortMethodsMapSize=%d] (vns)",(boxeeSortTmp.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSortTmp.m_sortMethod)).c_str(),boxeeSortTmp.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSortTmp.m_sortOrder)).c_str(),boxeeSortTmp.m_sortOrder,(boxeeSortTmp.m_sortName).c_str(),(boxeeSortTmp.m_folderPosition).c_str(),m_sortMethodsVec.size());
      return true;
    }
  }
  
  CLog::Log(LOGERROR,"CGUIBoxeeViewState::SetBoxeeSortAsChosen - Failed to set [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] as chosen since it is not in the m_sortMethodsVec. [m_sortMethodsVecSize=%d] (vns)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),(int)m_sortMethodsVec.size());
  return false;
}

bool CGUIBoxeeViewState::SetSortById(const CStdString& sortMethodId)
{
  for (size_t i=0; i<m_sortMethodsVec.size(); i++)
  {
    CBoxeeSort boxeeSort = m_sortMethodsVec[i];
    
    if(sortMethodId.Equals(boxeeSort.m_id))
    {
      m_chosenSortMethodKey = sortMethodId;
      //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::SetSortById - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] was set. [m_sortMethodsMapSize=%d] (vns)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_sortMethodsVec.size());
      return true;
    }
  }
    
  CLog::Log(LOGERROR,"CGUIBoxeeViewState::SetSortById - Failed to find sortMethodId [%s] in m_sortMethodsVec. [m_sortMethodsVecSize=%d] (vns)",sortMethodId.c_str(),(int)m_sortMethodsVec.size());
  return false;
}

SORT_METHOD CGUIBoxeeViewState::GetSortMethod() const
{
  CBoxeeSort boxeeSort;
  
  bool success = GetBoxeeSort(boxeeSort);
  
  if(success)
  {
    return boxeeSort.m_sortMethod;
  }
  else
  {
    //CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetSortMethod - Failed to get BoxeeSort. Going to return [SORT_METHOD_NONE] (vns)");
    return SORT_METHOD_NONE;    
  }
}

SORT_ORDER CGUIBoxeeViewState::GetSortOrder() const
{
  CBoxeeSort boxeeSort;
  
  bool success = GetBoxeeSort(boxeeSort);
  
  if(success)
  {
    return boxeeSort.m_sortOrder;
  }
  else
  {
    //CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetSortOrder - Failed to get BoxeeSort. Going to return [SORT_ORDER_NONE] (vns)");
    return SORT_ORDER_NONE;    
  }
}

CStdString CGUIBoxeeViewState::GetSortName() const
{
  CBoxeeSort boxeeSort;
  
  bool success = GetBoxeeSort(boxeeSort);
  
  if(success)
  {
    return boxeeSort.m_sortName;
  }
  else
  {
    //CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetSortName - Failed to get BoxeeSort. Going to return [default] (vns)");
    return "default";
  }
}

void CGUIBoxeeViewState::SaveBoxeeViewState()
{  
  CBoxeeView boxeeView;
  bool gotBoxeeView = GetBoxeeView(boxeeView);
  
  CBoxeeSort boxeeSort;
  bool gotBoxeeSort = GetBoxeeSort(boxeeSort);
  
  if(gotBoxeeView && gotBoxeeSort)
  {
    CBoxeeViewState boxeeViewState(boxeeView,boxeeSort,time(NULL));
    
    CBoxeeViewDatabase boxeeViewDatabase;
    
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::SaveBoxeeViewState - Going to call save BoxeeViewState of [path=%s]. View - [Type=%s=%d] || Sort - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] (vns)(sbvtdb)",m_path.c_str(),(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str());

    CStdString currentPathStr;
    if (m_windowId == WINDOW_BOXEE_BROWSE_TVSHOWS ||
        m_windowId == WINDOW_BOXEE_BROWSE_MOVIES ||
        m_windowId == WINDOW_BOXEE_BROWSE_ALBUMS ||
        m_windowId == WINDOW_BOXEE_BROWSE_PHOTOS)
    {
      currentPathStr = "";
  }
  else
  {
      currentPathStr = m_path;
    }

    boxeeViewDatabase.SaveBoxeeViewToDB(currentPathStr,m_windowId,boxeeViewState);
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIBoxeeViewState::SaveBoxeeViewState - Failed to get BoxeeView [gotBoxeeView=%d] or/and BoxeeSort [gotBoxeeSort=%d] therefore BoxeeViewState WON'T be save for [path=%s] (vns)(sbvtdb)",gotBoxeeView,gotBoxeeSort,m_path.c_str());
  } 
}

bool CGUIBoxeeViewState::InitializeBoxeeViewState(const CFileItemList& items, int windowID)
{ 
  CBoxeeViewState currentPathBoxeeViewState;
  
  CStdString currentPathStr;
  if (windowID == WINDOW_BOXEE_BROWSE_TVSHOWS ||
      windowID == WINDOW_BOXEE_BROWSE_MOVIES ||
      windowID == WINDOW_BOXEE_BROWSE_ALBUMS ||
      windowID == WINDOW_BOXEE_BROWSE_PHOTOS)
  {
    currentPathStr = "";
  }
  else
  {
    currentPathStr = items.m_strPath;
  }

  CBoxeeViewState parentPathBoxeeViewState;
  CStdString parentPathStr = items.GetProperty("parentPath");
  
  CStdString viewAndSortXml = items.GetProperty("ServerViewAndSort");
  bool IsViewAndSortPropertyEmpty = viewAndSortXml.IsEmpty();
  
  //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Enter function with FileItemList [path=%s][ParentPath=%s][IsViewAndSortPropertyEmpty=%d] and [WindowId=%d] (vns)(sbvtdb)",currentPathStr.c_str(),parentPathStr.c_str(),IsViewAndSortPropertyEmpty,windowID);
  
  bool successCurrent = false;
  bool successParent = false;
  
  CBoxeeViewDatabase db;
  
  // Get BoxeeViewState of current path from DataBase
  successCurrent = db.GetBoxeeViewState(currentPathStr,windowID,currentPathBoxeeViewState);

  //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Try to get BoxeeViewState of CurrentPath from DataBase returned [%d]. [CurrentPath=%s] (vns)(sbvtdb)",successCurrent,currentPathStr.c_str());

  if(ShouldGetParentPath(parentPathStr))
  {
    // In case of ParentPath -> get BoxeeViewState of parent path from DataBase
    
    successParent = db.GetBoxeeViewState(parentPathStr,windowID,parentPathBoxeeViewState);
    
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Try to get BoxeeViewState of ParentPath from DataBase returned [%d]. [ParentPath=%s] (vns)(sbvtdb)",successParent,parentPathStr.c_str());
  }
  else
  {
    //CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::InitializeBoxeeViewState - ParentPath is [%s] and we don't want to use it as parent, therefore NOT trying to get its BoxeeViewState from DataBase (vns)(sbvtdb)",parentPathStr.c_str());
  }
  
  bool retVal = false;
  bool needToSave = false;

  if(successCurrent)
  {
    if(successParent)
    {
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to compare between [CurrentPath=%s] and [ParentPath=%s] change times. [CurrentPathChangeTimes=%d][ParentPathChangeTimes=%d] (vns)(sbvtdb)",currentPathStr.c_str(),parentPathStr.c_str(),(int)(currentPathBoxeeViewState.m_changeTime),(int)(parentPathBoxeeViewState.m_changeTime));
      
      if (parentPathBoxeeViewState.m_changeTime > currentPathBoxeeViewState.m_changeTime)
      {
        needToSave = true;

        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - [ParentPath=%s] won the comparison. [needToSave=%d] (vns)(sbvtdb)",parentPathStr.c_str(),needToSave);
        
        // Set VIEW
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeViewAsChosen with parentPathBoxeeViewState (vns)(sbvtdb)");
        retVal = SetBoxeeViewAsChosen(parentPathBoxeeViewState.m_boxeeView);
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeViewAsChosen returned [%d] (vns)(sbvtdb)",retVal);

        // Set SORT
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeSortAsChosen with currentPathBoxeeViewState (vns)(sbvtdb)");
        retVal = SetBoxeeSortAsChosen(currentPathBoxeeViewState.m_boxeeSort);
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeSortAsChosen returned [%d] (vns)(sbvtdb)",retVal);        
      }
      else
      {
        needToSave = false;

        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - [CurrentPath=%s] won the comparison. [needToSave=%d] (vns)(sbvtdb)",currentPathStr.c_str(),needToSave);

        // Set VIEW
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeViewAsChosen with currentPathBoxeeViewState (vns)(sbvtdb)");
        retVal = SetBoxeeViewAsChosen(currentPathBoxeeViewState.m_boxeeView);
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeViewAsChosen returned [%d] (vns)(sbvtdb)",retVal);

        // Set SORT
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeSortAsChosen with currentPathBoxeeViewState (vns)(sbvtdb)");
        retVal = SetBoxeeSortAsChosen(currentPathBoxeeViewState.m_boxeeSort);
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeSortAsChosen returned [%d] (vns)(sbvtdb)",retVal);
      }
    }
    else
    {
      needToSave = false;

      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to set [CurrentPath=%s] BoxeeViewState from DataBase as default. [needToSave=%d] (vns)(sbvtdb)",currentPathStr.c_str(),needToSave);
  
      // Set VIEW
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeViewAsChosen with currentPathBoxeeViewState (vns)(sbvtdb)");
      retVal = SetBoxeeViewAsChosen(currentPathBoxeeViewState.m_boxeeView);
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeViewAsChosen returned [%d] (vns)(sbvtdb)",retVal);

      // Set SORT
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeSortAsChosen with currentPathBoxeeViewState (vns)(sbvtdb)");
      retVal = SetBoxeeSortAsChosen(currentPathBoxeeViewState.m_boxeeSort);
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeSortAsChosen returned [%d] (vns)(sbvtdb)",retVal);      
    }
  }
  else
  {
    //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Failed to get BoxeeViewState from DataBase for [CurrentPath=%s] (vns)(sbvtdb)",currentPathStr.c_str());

    if(IsViewAndSortPropertyEmpty == false)
    {
      needToSave = true;
      
      //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - There is [ServerViewAndSort] property. Going to set created CGUIBoxeeViewState object as default. [needToSave=%d] (vns)(sbvtdb)",needToSave);     
    }
    else
    {
      if(successParent)
      {
        needToSave = true;

        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to set [ParentPath=%s] BoxeeViewState from DataBase as default. [needToSave=%d] (vns)(sbvtdb)",parentPathStr.c_str(),needToSave);

        // Set VIEW
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Going to call SetBoxeeViewAsChosen with parentPathBoxeeViewState (vns)(sbvtdb)");
        retVal = SetBoxeeViewAsChosen(parentPathBoxeeViewState.m_boxeeView);
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Call to SetBoxeeViewAsChosen returned [%d] (vns)(sbvtdb)",retVal);

        // Set SORT
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - The sort will be set with the created CGUIBoxeeViewState object sort (vns)(sbvtdb)");
      }
      else
      {
        needToSave = true;
        
        //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Failed to get BoxeeViewState from DataBase for both [CurrentPath=%s] and [ParentPath=%s]. View and Sort will be set with the created CGUIBoxeeViewState object values [needToSave=%d] (vns)(sbvtdb)",currentPathStr.c_str(),parentPathStr.c_str(),needToSave);
      }
    }
  }
  
  //CLog::Log(LOGDEBUG, "CGUIBoxeeViewState::InitializeBoxeeViewState - Exit function and return [needToSave=%d]. [path=%s][ParentPath=%s][IsViewAndSortPropertyEmpty=%d] and [WindowId=%d] (vns)(sbvtdb)",needToSave,currentPathStr.c_str(),parentPathStr.c_str(),IsViewAndSortPropertyEmpty,windowID);

  return needToSave;
}

int CGUIBoxeeViewState::GetViewTypeAsEnum(const CStdString& viewType)
{
  CStdString viewTypeLower = viewType;
  viewTypeLower.ToLower();
  
  if(viewTypeLower == "line")
  {
    return 50;
  }
  else if((viewTypeLower == "thumb with preview") || (viewTypeLower == "thumb-with-preview"))
  {
    return 51;
  }
  else if(viewTypeLower == "thumb")
  {
    return 52;
  }
  else if((viewTypeLower == "detailed list") || (viewTypeLower == "detailed-list"))
  {
    return 53;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIBoxeeViewState::GetViewTypeAsEnum - Failed to convert [viewTypeLower=%s] to ENUM. Going to return LINE view (vns)",viewTypeLower.c_str());
    return 50;
  } 
}

const CStdString CGUIBoxeeViewState::GetViewTypeAsString(int viewType)
{
  switch(viewType)
  {
  case 50:
    return "line";
    break;
  case 51:
    return "thumb with preview";
    break;
  case 52:
    return "thumb";
    break;
  case 53:
    return "Detailed list";
    break;
  default:
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetViewTypeAsString - Failed to convert [viewType=%d] to view name. Going to return [View not found] (vns)",viewType);
    return "View not found";
    break;
  }  
}

SORT_METHOD CGUIBoxeeViewState::GetSortMethodAsEnum(const CStdString& sortMethod)
{  
  CStdString sortMethodLower = sortMethod;
  sortMethodLower.ToLower();
  
  if(sortMethodLower == "label")
  {
    return SORT_METHOD_LABEL;
  }
  if(sortMethodLower == "label_ignore_the")
  {
    return SORT_METHOD_LABEL_IGNORE_THE;
  }
  else if(sortMethodLower == "date")
  {
    return SORT_METHOD_DATE;
  }
  else if(sortMethodLower == "episode")
  {
    return SORT_METHOD_EPISODE;
  }
  else if(sortMethodLower == "none")
  {
    return SORT_METHOD_NONE;
  }
  else if(sortMethodLower == "default")
  {
    return SORT_METHOD_DEFAULT;
  }
  else if(sortMethodLower == "app-popularity")
  {
    return SORT_METHOD_APP_POPULARITY;
  }
  else if(sortMethodLower == "app-releasedate")
  {
    return SORT_METHOD_APP_RELEASE_DATE;
  }  
  else if(sortMethodLower == "year")
  {
    return SORT_METHOD_YEAR;
  }
  /*
  else if(sortMethodLower == "date added")
  {
    return SORT_METHOD_DATE_ADDED;
  }
  */
  else if (sortMethodLower == "app-usage")
  {
    return SORT_METHOD_APP_USAGE;
  }
  else if (sortMethodLower == "app-last-used")
  {
    return SORT_METHOD_APP_LAST_USED_DATE;
  }
  else if(sortMethodLower == "quality")
  {
    return SORT_METHOD_VIDEO_QUALITY;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortMethodAsEnum - Failed to convert [sortMethodLower=%s] to ENUM. Going to return [SORT_METHOD_NONE] (vns)",sortMethodLower.c_str());    
    return SORT_METHOD_NONE;    
  }  
}

const CStdString CGUIBoxeeViewState::GetSortMethodAsString(SORT_METHOD sortMethod)
{
  switch(sortMethod)
  {
  case SORT_METHOD_LABEL:
    return "label";
    break;
  case SORT_METHOD_LABEL_IGNORE_THE:
    return "label_ignore_the";
    break;
  case SORT_METHOD_DATE:
    return "date";
    break;
  case SORT_METHOD_EPISODE:
    return "episode";
    break;
  case SORT_METHOD_DEFAULT:
    return "default";
    break;
  case SORT_METHOD_APP_POPULARITY:
    return "app-popularity";
    break;
  case SORT_METHOD_VIDEO_QUALITY:
    return "qualitiy";
    break;
  case SORT_METHOD_APP_RELEASE_DATE:
    return "app-releasedate";
    break;
  case SORT_METHOD_YEAR:
      return "year";
      break;
  case SORT_METHOD_NONE:
    return "none";
    break;
    /*
  case SORT_METHOD_DATE_ADDED:
    return "date added";
    break;
    */
  case SORT_METHOD_APP_USAGE:
    return "app-usage";
    break;
  case SORT_METHOD_APP_LAST_USED_DATE:
    return "app-last-used";
    break;
  default:
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortMethodAsString - Failed to convert [sortMethod=%d] to String. Going to return [none] (vns)",sortMethod);
    return "none";
    break;
  }  
}

SORT_ORDER CGUIBoxeeViewState::GetSortOrderAsEnum(const CStdString& sortOrder)
{
  CStdString sortOrderLower = sortOrder;
  sortOrderLower.ToLower();

  if(sortOrderLower == "ascending")
  {
    return SORT_ORDER_ASC;
  }
  else if(sortOrderLower == "descending")
  {
    return SORT_ORDER_DESC;
  }
  else if(sortOrderLower == "none")
  {
    return SORT_ORDER_NONE;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortOrderAsEnum - Failed to convert [sortOrderLower=%s] to ENUM. Going to return [SORT_ORDER_NONE] (vns)",sortOrderLower.c_str());    
    return SORT_ORDER_NONE;    
  }
}

const CStdString CGUIBoxeeViewState::GetSortOrderAsString(SORT_ORDER sortOrder)
{
  switch(sortOrder)
  {
  case SORT_ORDER_ASC:
    return "ascending";
    break;
  case SORT_ORDER_DESC:
    return "descending";
    break;
  default:
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortOrderAsString - Failed to convert [sortOrder=%d] to ENUM. Going to return [none] (vns)",sortOrder);    
    return "none";
    break;
  }
}

int CGUIBoxeeViewState::GetSortNameAsInt(const CStdString& sortTypeName)
{
  int sortNameAsInt;
  
  CStdString sortTypeNameLower = sortTypeName;
  sortTypeNameLower.ToLower();
  
  if(sortTypeNameLower == "name")
  {
    sortNameAsInt = 551;
  }
  else if(sortTypeNameLower == "date")
  {
    sortNameAsInt = 552;
  }
  /*
  else if(sortTypeNameLower == "Date Added")
  {
    sortNameAsInt = 570;
  }
  */
  else if(sortTypeNameLower == "default")
  {
    sortNameAsInt = 571;
  }
  else if(sortTypeNameLower == "episode")
  {
    sortNameAsInt = 20359;
  }
  else if(sortTypeNameLower == "year")
  {
    sortNameAsInt = 562;
  }
  else
  {
    sortNameAsInt = 571;
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortNameAsInt - Failed to convert [sortTypeNameLower=%s] to ENUM. Going to return [sortNameAsInt=%d] (vns)",sortTypeNameLower.c_str(),sortNameAsInt);
  }
  
  return sortNameAsInt;
}

const CStdString CGUIBoxeeViewState::GetSortNameAsString(int sortName)
{
  switch(sortName)
  {
  case 551:
    return "name";
  case 552:
    return "date";
    /*
  case 570:
    return "Date Added";
    */
  case 571:
    return "default";
  case 20359:
    return "episode";
  default:
    CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::GetSortNameAsString - Failed to convert [sortName=%d] to ENUM. Going to return [No name found] (vns)",sortName);    
    return "No name found";
  }
}

bool CGUIBoxeeViewState::ShouldGetParentPath(const CStdString& ParentPath)
{
  CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::ShouldGetParentPath - Enter function with [ParentPath=%s] (vns)",ParentPath.c_str());
  
  bool shouldGetParentPath = false;
  
  if(ParentPath == "")
  {
    shouldGetParentPath = false;
  }
  else if(ParentPath == "apps://video/")
  {
    shouldGetParentPath = false;
  }
  else if(ParentPath == "apps://music/")
  {
    shouldGetParentPath = false;
  }
  else if(ParentPath == "apps://pictures/")
  {
    shouldGetParentPath = false;
  }
  else
  {
    shouldGetParentPath = true;
  }
  
  CLog::Log(LOGDEBUG,"CGUIBoxeeViewState::ShouldGetParentPath - Exit function and return [%d] for [ParentPath=%s] (vns)",shouldGetParentPath,ParentPath.c_str());

  return shouldGetParentPath;
}
