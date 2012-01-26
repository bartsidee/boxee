/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "BoxeeViewDatabase.h"
#include "Util.h"
#include "GUIBoxeeViewState.h"
#include "utils/log.h"

#define BOXEE_VIEW_DATABASE_VERSION 3

//********************************************************************************************************************************
CBoxeeViewDatabase::CBoxeeViewDatabase()
{
  m_preV2version = 0;
  m_version = BOXEE_VIEW_DATABASE_VERSION;
  m_strDatabaseFile = BOXEE_VIEW_DATABASE_NAME;
}

//********************************************************************************************************************************
CBoxeeViewDatabase::~CBoxeeViewDatabase()
{

}

//********************************************************************************************************************************
bool CBoxeeViewDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create view table");
    m_pDS->exec("CREATE TABLE view ( idView integer primary key, window integer, path text, viewMode integer, sortMethod integer, sortOrder integer, sortName text, folderPosition text, changeTime integer)\n");
    CLog::Log(LOGINFO, "create view index");
    m_pDS->exec("CREATE INDEX idxViews ON view(path)");
    CLog::Log(LOGINFO, "create view - window index");
    m_pDS->exec("CREATE INDEX idxViewsWindow ON view(window)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables:%u",
              __FUNCTION__, GetLastError());
    return false;
  }

  return true;
}

bool CBoxeeViewDatabase::UpdateOldVersion(int version)
{
  return true;
}

bool CBoxeeViewDatabase::SaveBoxeeViewToDB(const CStdString &path, int windowID, const CBoxeeViewState& boxeeViewState)
{
  CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::SaveBoxeeViewToDB - Enter function with [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

  if (CDatabase::Open())
  {
    try
    {
      if (NULL == m_pDB.get())
      {
        CLog::Log(LOGERROR,"CBoxeeViewDatabase::SaveBoxeeViewToDB - Can't get m_pDB [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

        CDatabase::Close();
        
        return false;
      }
    
      if (NULL == m_pDS.get())
      {
        CLog::Log(LOGERROR,"CBoxeeViewDatabase::SaveBoxeeViewToDB - Can't get m_pDS [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

        CDatabase::Close();
        
        return false;
      }

      CBoxeeView boxeeView = boxeeViewState.m_boxeeView;
      CBoxeeSort boxeeSort = boxeeViewState.m_boxeeSort;
    
      CStdString path1(path);
      CUtil::AddSlashAtEnd(path1);
      if (path1.IsEmpty()) path1 = "root://";

      CStdString sql = FormatSQL("select idView from view where window = %i and path like '%s'", windowID, path1.c_str());
      
      m_pDS->query(sql.c_str());
 
      if (!m_pDS->eof())
      { 
        // update the view
        
        int idView = m_pDS->fv("idView").get_asInt();
        m_pDS->close();
        sql = FormatSQL("update view set viewMode=%i,sortMethod=%i,sortOrder=%i,sortName='%s',folderPosition='%s',changeTime=%i where idView=%i", boxeeView.m_type, (int)boxeeSort.m_sortMethod, (int)boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),(int)boxeeViewState.m_changeTime, idView);
        m_pDS->exec(sql.c_str());
      
        CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::SaveBoxeeViewToDB - After saving [Update] BoxeeViewState to DataBase for [path=%s]. View - [Type=%s=%d] || Sort - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] || [changeTime=%d] (vns)(sbvtdb)(vsdb)",path1.c_str(),(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),(int)boxeeViewState.m_changeTime);
      }
      else
      { 
        // add the view

        m_pDS->close();
        sql = FormatSQL("insert into view (idView, path, window, viewMode, sortMethod, sortOrder, sortName, folderPosition, changeTime) values(NULL, '%s', %i, %i, %i, %i, '%s', '%s', %i)", 
          path1.c_str(), windowID, boxeeView.m_type, (int)boxeeSort.m_sortMethod, (int)boxeeSort.m_sortOrder, (boxeeSort.m_sortName).c_str(), (boxeeSort.m_folderPosition).c_str(),(int)boxeeViewState.m_changeTime);
        m_pDS->exec(sql.c_str());
      
        CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::SaveBoxeeViewToDB - After saving [Add] BoxeeViewState to DataBase for [path=%s]. View - [Type=%s=%d] || Sort - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] || [changeTime=%d] (vns)(sbvtdb)(vsdb)",path1.c_str(),(CGUIBoxeeViewState::GetViewTypeAsString(boxeeView.m_type)).c_str(),boxeeView.m_type,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),(int)boxeeViewState.m_changeTime);
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s failed on path '%s'", __FUNCTION__, path.c_str());
    }
    
    CDatabase::Close();

    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::SaveBoxeeViewToDB - Call to Open() failed. Exit function [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

    return false;
  }  
}

bool CBoxeeViewDatabase::GetBoxeeViewState(const CStdString &path, int windowID, CBoxeeViewState& boxeeViewState)
{
  CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::GetBoxeeViewState - Enter function with [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

  if (CDatabase::Open())
  {
    try
    {
      if (NULL == m_pDB.get())
      {
        CLog::Log(LOGERROR,"CBoxeeViewDatabase::GetBoxeeViewState - Can't get m_pDB [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

        CDatabase::Close();

        return false;
      }
      
      if (NULL == m_pDS.get())
      {
        CLog::Log(LOGERROR,"CBoxeeViewDatabase::GetBoxeeViewState - Can't get m_pDS [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

        CDatabase::Close();

        return false;
      }

      CStdString path1(path);
      CUtil::AddSlashAtEnd(path1);
      if (path1.IsEmpty()) path1 = "root://";

      CStdString sql = FormatSQL("select * from view where window = %i and path like '%s'", windowID, path1.c_str());

      m_pDS->query(sql.c_str());

      bool eof = m_pDS->eof();
      
      if (!eof)
      {
        // have some information
        (boxeeViewState.m_boxeeView).m_type = m_pDS->fv("viewMode").get_asInt();
        (boxeeViewState.m_boxeeSort).m_sortMethod = (SORT_METHOD)m_pDS->fv("sortMethod").get_asInt();
        (boxeeViewState.m_boxeeSort).m_sortOrder = (SORT_ORDER)m_pDS->fv("sortOrder").get_asInt();
        (boxeeViewState.m_boxeeSort).m_sortName = m_pDS->fv("sortName").get_asString();
        (boxeeViewState.m_boxeeSort).m_folderPosition = m_pDS->fv("folderPosition").get_asString();
        boxeeViewState.m_changeTime = (time_t)m_pDS->fv("changeTime").get_asInt();
        
        CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::GetBoxeeViewState - After Getting BoxeeViewState from DataBase for [path=%s]. View - [Type=%s=%d] || Sort - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][SortName=%s][FolderPosition=%s] || [changeTime=%d] (vns)(sbvtdb)(vsdb)",path1.c_str(),(CGUIBoxeeViewState::GetViewTypeAsString((boxeeViewState.m_boxeeView).m_type)).c_str(),(boxeeViewState.m_boxeeView).m_type,((boxeeViewState.m_boxeeSort).m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString((boxeeViewState.m_boxeeSort).m_sortMethod)).c_str(),(boxeeViewState.m_boxeeSort).m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString((boxeeViewState.m_boxeeSort).m_sortOrder)).c_str(),(boxeeViewState.m_boxeeSort).m_sortOrder,((boxeeViewState.m_boxeeSort).m_sortName).c_str(),((boxeeViewState.m_boxeeSort).m_folderPosition).c_str(),(int)boxeeViewState.m_changeTime);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::GetBoxeeViewState - [m_pDS->eof()=%d] therefore CAN'T get BoxeeViewState from DataBase. [path=%s][windowID=%d] (sbvtdb)",eof,path.c_str(),windowID);
        CDatabase::Close();
        return false;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s, failed on path '%s'", __FUNCTION__, path.c_str());
      CDatabase::Close();
      return false;
    }
   
    m_pDS->close();
    CDatabase::Close();
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeViewDatabase::GetBoxeeViewState - Call to Open() failed. Exit function [path=%s][windowID=%d] (sbvtdb)",path.c_str(),windowID);

    return false;
  }
}

bool CBoxeeViewDatabase::ClearBoxeeViewStates(int windowID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = FormatSQL("delete from view where window = %i", windowID);
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on window '%i'", __FUNCTION__, windowID);
  }
  return true;
}
