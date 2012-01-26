/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "FileOperations.h"
#include "Settings.h"
#include "MediaSource.h"
#include "Directory.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "Util.h"
#include "URL.h"
#include "BoxeeMediaSourceList.h"

using namespace XFILE;
using namespace Json;
using namespace JSONRPC;
using namespace DIRECTORY;

JSON_STATUS CFileOperations::GetRootDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString media = parameterObject["media"].asString();
  media = media.ToLower();

  VECSOURCES *sources = g_settings.GetSourcesFromType(media);
  if (sources)
  {
    CFileItemList items;
    for (unsigned int i = 0; i < (unsigned int)sources->size(); i++)
      items.Add(CFileItemPtr(new CFileItem(sources->at(i))));

    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    {
      if (items[i]->IsSmb())
      {
        CURI url(items[i]->m_strPath);
        items[i]->m_strPath = url.GetWithoutUserDetails();
      }
    }

    HandleFileItemList(NULL, true, "shares", items, parameterObject, result);
  }

  result["success"] = true;

  return OK;
}

JSON_STATUS CFileOperations::GetDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString media = parameterObject["media"].asString();
  media = media.ToLower();

  CDirectory directory;
  CFileItemList items;
  CStdString strPath = parameterObject["directory"].asString();

  CStdStringArray regexps;
  CStdString extensions = "";
  if (media.Equals("video"))
  {
    regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
    extensions = g_stSettings.m_videoExtensions;
  }
  else if (media.Equals("music"))
  {
    regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
    extensions = g_stSettings.m_musicExtensions;
  }
  else if (media.Equals("pictures"))
  {
    regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
    extensions = g_stSettings.m_pictureExtensions;
  }

  if (directory.GetDirectory(strPath, items, extensions))
  {
    CFileItemList filteredDirectories, filteredFiles;
    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    {
      if (CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
        continue;

      if (items[i]->IsSmb())
      {
        CURI url(items[i]->m_strPath);
        items[i]->m_strPath = url.GetWithoutUserDetails();
      }

      if (items[i]->m_bIsFolder)
        filteredDirectories.Add(items[i]);
      else
        filteredFiles.Add(items[i]);
    }

    // Check if the "fields" list exists
    // and make sure it contains the "file"
    // field
    Value param = parameterObject;
    if (!param.isMember("fields"))
      param["fields"] = Value(arrayValue);

    bool hasFileField = false;
    for (unsigned int i = 0; i < param["fields"].size(); i++)
    {
      if (param["fields"][i].asString().compare("file") == 0)
      {
        hasFileField = true;
        break;
      }
    }

    if (!hasFileField)
      param["fields"].append("file");

    HandleFileItemList(NULL, true, "files", filteredDirectories, param, result);
    for (unsigned int index = 0; index < result["files"].size(); index++)
    {
      result["files"][index]["filetype"] = "directory";
    }
    int count = result["limits"]["total"].asInt();

    HandleFileItemList(NULL, true, "files", filteredFiles, param, result);
    for (unsigned int index = count; index < result["files"].size(); index++)
    {
      result["files"][index]["filetype"] = "file";
    }
    count += result["limits"]["total"].asInt();

    result["limits"]["end"] = count;
    result["limits"]["total"] = count;

    result["success"] = true;

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CFileOperations::Download(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return transport->Download(parameterObject["path"].asString().c_str(), &result) ? OK : InvalidParams;
}

bool CFileOperations::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  if (parameterObject.isObject() && parameterObject.isMember("directory"))
  {
    CStdString media = "files";
    if (parameterObject.isMember("media"))
    {
      if (parameterObject["media"].isString())
        media = parameterObject["media"].asString();
      else
        return false;
    }

    media = media.ToLower();

    if (media.Equals("video") || media.Equals("music") || media.Equals("pictures") || media.Equals("files") || media.Equals("programs"))
    {
      CDirectory directory;
      CFileItemList items;
      CStdString strPath = parameterObject["directory"].asString();

      CStdStringArray regexps;
      CStdString extensions = "";
      if (media.Equals("video"))
      {
        regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        extensions = g_stSettings.m_videoExtensions;
      }
      else if (media.Equals("music"))
      {
        regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        extensions = g_stSettings.m_musicExtensions;
      }
      else if (media.Equals("pictures"))
      {
        regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;
        extensions = g_stSettings.m_pictureExtensions;
      }

      if (directory.GetDirectory(strPath, items, extensions))
      {
        CFileItemList filteredDirectories;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
            continue;

          if (items[i]->m_bIsFolder)
            filteredDirectories.Add(items[i]);
          else
            list.Add(items[i]);
        }

        if (parameterObject.isMember("recursive") && parameterObject["recursive"].isBool())
        {
          for (int i = 0; i < filteredDirectories.Size(); i++)
          {
            Value val = parameterObject;
            val["directory"] = filteredDirectories[i]->m_strPath;
            FillFileItemList(val, list);
          }
        }

        return true;
      }
    }
  }

  return false;
}

JSON_STATUS CFileOperations::AddSource(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result)
{
  CStdString media = parameterObject["media"].asString();
  CStdString label = parameterObject["label"].asString();
  CStdString path = parameterObject["path"].asString();
  CStdString scanPolicy = parameterObject["scanPolicy"].asString();

  CBoxeeMediaSourceList sourceList;
  if (sourceList.sourceNameExists(label))
  {
    result["success"] = false;
    return FailedToExecute;
  }

  CBoxeeMediaSource source;
  source.name = label;
  source.path = path;
  source.isVideo = false;
  source.isMusic = false;
  source.isPicture = false;

  CStdStringArray mediaArray;
  StringUtils::SplitString(media, ",", mediaArray);

  for (size_t i = 0; i < mediaArray.size(); i++)
  {
    if (mediaArray[i] == "video") source.isVideo = true;
    else if (mediaArray[i] == "music") source.isMusic = true;
    else if (mediaArray[i] == "pictures") source.isPicture = true;
  }

  if (!source.isVideo && !source.isMusic && !source.isPicture)
  {
    result["success"] = false;
    return FailedToExecute;
  }

  if (scanPolicy == "once")
    source.scanType = CMediaSource::SCAN_TYPE_ONCE;
  else if (scanPolicy == "daily")
    source.scanType = CMediaSource::SCAN_TYPE_DAILY;
  else if (scanPolicy == "hourly")
    source.scanType = CMediaSource::SCAN_TYPE_HOURLY;
  else
    source.scanType = CMediaSource::SCAN_TYPE_PRIVATE;

  sourceList.addSource(source);

  result["success"] = true;
  return OK;
}

JSON_STATUS CFileOperations::DeleteSource(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result)
{
  CStdString label = parameterObject["label"].asString();

  CBoxeeMediaSourceList sourceList;
  if (!sourceList.sourceNameExists(label))
  {
    result["success"] = false;
    return FailedToExecute;
  }

  sourceList.deleteSource(label);

  result["success"] = true;

  return OK;
}
