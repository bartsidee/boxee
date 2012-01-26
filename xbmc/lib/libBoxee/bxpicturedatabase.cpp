
#include "bxpicturedatabase.h"
#include "bxmetadata.h"
#include "logger.h"
#include "bxutils.h"
#include "boxee.h"
#include <time.h>

using namespace dbiplus;

namespace BOXEE
{

BXPictureDatabase::BXPictureDatabase()
{
	Open();
}

BXPictureDatabase::~BXPictureDatabase()
{
  Close();
}

/*
bool BXPictureDatabase::init()
{
	LOG(LOG_LEVEL_DEBUG, "Going to open Media database file");
	
	bool retVal = Open();
	
	if(retVal == true)
	{
		if (!FileExists()) 
		{
			LOG(LOG_LEVEL_DEBUG, "Media database file does not exist, going to create tables");
			CreateTables();
		}
		else 
		{
			LOG(LOG_LEVEL_DEBUG, "Media database file exists");
			if (!TablesExist()) 
			{
				LOG(LOG_LEVEL_DEBUG, "Media database tables do not exist, going to create tables");
				CreateTables();
			}
		}
		
		return true;
	}
	else
	{
		LOG(LOG_LEVEL_ERROR, "Failed to open media database file");
		
		// TODO - Add error handling
		
		return false;
	}
}
*/

int BXPictureDatabase::AddPictureFolder(const BXFolder* pPictureFolder)
{

  if (!pPictureFolder) return MEDIA_DATABASE_ERROR;

  LOG(LOG_LEVEL_DEBUG, "BXPictureDatabase, Adding picture folder %s", pPictureFolder->GetPath().c_str());

  // Add all audio tracks in the folder
  for (int i = 0; i < (int)pPictureFolder->m_vecFiles.size(); i++) {
    if (pPictureFolder->m_vecFiles[i] == NULL) {
      continue;
    }
    AddPicture(pPictureFolder->m_vecFiles[i]);
  }

  return 0;
}


int BXPictureDatabase::AddPicture(const BXMetadata* pMetadata)
{
  std::string strPath = pMetadata->GetPath();
  LOG(LOG_LEVEL_DEBUG, "Adding picture path = %s", strPath.c_str());
  // We only handle audio files here that have at least title
  if (!pMetadata || pMetadata->GetType() != "picture" || (pMetadata->GetDetail(MEDIA_DETAIL_PICTURE) == NULL)) 
  {
    LOG(LOG_LEVEL_ERROR, "Could not add picture");
    return MEDIA_DATABASE_ERROR;
  }

  int iPictureId = -1;

  Dataset* pDS = NULL;
  try {

    pDS = Query("select * from pictures where strPath='%s'", strPath.c_str());
    if (pDS) {
      if (pDS->num_rows() != 0) {
        iPictureId = pDS->fv("idPicture").get_asInt();
      }
      delete pDS;
    }

  }
  catch(dbiplus::DbErrors& e) {
    LOG(LOG_LEVEL_ERROR, "Exception occured when trying to retrieve pictures. Error: %s ", e.getMsg());
    delete pDS; 
    return MEDIA_DATABASE_ERROR;
  }

  // File with this file exists, return its id
  if (iPictureId != -1) return iPictureId;

  // Otherwise insert new video into the database
  BXPicture* pPicture = (BXPicture*)pMetadata->GetDetail(MEDIA_DETAIL_PICTURE);

  time_t now = time(NULL);

  // Add picture
  iPictureId = Insert("insert into pictures (idPicture, idFile, strPath, iDate, iDateAdded, iHasMetadata, iDateModified) "
      "values( NULL, %i, '%s', %i, %i, %i, %i)", pMetadata->GetMediaFileId(), strPath.c_str(), pPicture->m_iDate, now, pMetadata->m_bResolved ? 1 : 0, pPicture->m_iDateModified);

  if (iPictureId == -1) {
    LOG(LOG_LEVEL_ERROR, "Could not insert picture");
  }
  return iPictureId;

}

void BXPictureDatabase::RemoveDuplicates(std::vector<BXMetadata*> &vecA, std::vector<BXMetadata*> &vecB)
{
  std::vector<BXMetadata*>::iterator a = vecA.begin();
  while (a != vecA.end()) {
    std::vector<BXMetadata*>::iterator b = vecB.begin();
    while (b != vecB.end()) {
      if ((*a)->GetPath() == (*b)->GetPath()) {
        vecB.erase(b);
      }
      else {
        b++;
      }
    }
    a++;
  }
  
}

int BXPictureDatabase::GetPictures(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit)
{
  std::vector<BXMetadata*> vecPictureFiles;
  std::vector<BXMetadata*> vecPictureFolders;

  int iResult = MEDIA_DATABASE_OK;

  // Bring twice as much picture files and twice as much picture folders
  iResult = GetPictureFiles(vecPictureFiles, vecPathFilter, iOrder, iItemLimit * 2);
  if (iResult == MEDIA_DATABASE_ERROR) {
    LOG(LOG_LEVEL_ERROR, "Could not retreive picture files");
    BXUtils::FreeMetaDataVec(vecPictureFiles);
    BXUtils::FreeMetaDataVec(vecPictureFolders);
    return iResult;
  }

  iResult = GetPictureFolders(vecPictureFolders, vecPathFilter, iOrder, iItemLimit * 2);
  if (iResult == MEDIA_DATABASE_ERROR) {
    LOG(LOG_LEVEL_ERROR, "Could not retreive picture files");
    BXUtils::FreeMetaDataVec(vecPictureFiles);
    BXUtils::FreeMetaDataVec(vecPictureFolders);
    return iResult;
  }
  
  RemoveDuplicates(vecPictureFiles, vecPictureFolders);

  // Combine the two lists together into one, ordered by iDateAdded
  BXUtils::MergeByDateModified(vecPictureFiles, vecPictureFolders, vecMediaFiles, iItemLimit);
  BXUtils::FreeMetaDataVec(vecPictureFiles);
  BXUtils::FreeMetaDataVec(vecPictureFolders);
  
  return iResult;
}

int BXPictureDatabase::GetPictureFolders(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit)
{
  LOG(LOG_LEVEL_DEBUG, "Creating list of pictures");

  int iResult = MEDIA_DATABASE_OK;
  Dataset* pDS;
  if (iOrder == MEDIA_ORDER_DATE) {
    pDS = Query("select * from media_folders where strType='pictureFolder' ORDER BY iDateModified DESC");
  }
  else if (iOrder == MEDIA_ORDER_NAME) {
    pDS = Query("select * from media_folders where strType='pictureFolder'");
  }
  else {
    pDS = Query("select * from media_folders where strType='pictureFolder'");
  }

  if (pDS) {
    try {

      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0) 
        {
          // Print file path, for debug purpose
          std::string strPath = pDS->fv("strPath").get_asString();
          LOG(LOG_LEVEL_DEBUG, "GetPictures, Handling picture folder. Path = %s", strPath.c_str());

          // Check if the path is within filter
//          bool bMatch = false;
//          for (int i = 0; i < (int)vecPathFilter.size(); i++) {
//            std::string::size_type startPos = strPath.find(vecPathFilter[i], 0);
//            if (startPos == 0) {
//              bMatch = true;
//              break;
//            }
//          }

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) {
            pDS->next();
            continue; // skip this file and move on to the next one
          }

          // Check if directory with this name already exists in the list
          bool bFound = false;
          for (unsigned int i=0; i < vecMediaFiles.size(); i++) {
            if (vecMediaFiles[i]->GetType() == MEDIA_ITEM_TYPE_DIR) {
              if (vecMediaFiles[i]->GetPath() == strPath) {
                bFound = true;
              }
            }
          }

          if (!bFound) {
            BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_DIR);
            pMetadata->SetPath(strPath);
            pMetadata->m_iDateAdded = pDS->fv("iDateAdded").get_asInt();
            pMetadata->m_iDateModified = pDS->fv("iDateModified").get_asInt();
            vecMediaFiles.push_back(pMetadata);
            iItemLimit--;
          }
          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get picture files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iResult = MEDIA_DATABASE_ERROR;
  }
  return iResult;

}

int BXPictureDatabase::GetPictureFiles(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iOrder, int iItemLimit)
{
  LOG(LOG_LEVEL_DEBUG, "Creating list of pictures");

  int iResult = MEDIA_DATABASE_OK;
  Dataset* pDS;
  if (iOrder == MEDIA_ORDER_DATE) {
    pDS = Query("select * from pictures ORDER BY iDateAdded DESC");
  }
  else if (iOrder == MEDIA_ORDER_NAME) {
    pDS = Query("select * from pictures ORDER BY strTitle");
  }
  else {
    pDS = Query("select * from pictures");
  }


  if (pDS) {
    try {

      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0) 
        {
          // Print file path, for debug purpose
          std::string strPath = pDS->fv("strPath").get_asString();
          LOG(LOG_LEVEL_DEBUG, "GetPictures, Handling picture file. Path = %s", strPath.c_str());

          // Check if the path is within filter
//          bool bMatch = false;
//          for (int i = 0; i < (int)vecPathFilter.size(); i++) {
//            std::string::size_type startPos = strPath.find(vecPathFilter[i], 0);
//            if (startPos == 0) {
//              bMatch = true;
//              break;
//            }
//          }

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) {
            pDS->next();
            continue; // skip this file and move on to the next one
          }
          
            // Always get picture folders
          
            // No metadata, get folder 
            std::string strFolderPath = BXUtils::GetParentPath(strPath);

            // Check if directory with this name already exists in the list
            bool bFound = false;
            for (unsigned int i=0; i < vecMediaFiles.size(); i++) {
              if (vecMediaFiles[i]->GetType() == MEDIA_ITEM_TYPE_DIR) {
                if (vecMediaFiles[i]->GetPath() == strFolderPath) {
                  bFound = true;
                }
              }
            }

            if (!bFound) {
              BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_DIR);
              pMetadata->SetPath(strFolderPath);
              pMetadata->m_iDateAdded = pDS->fv("iDateAdded").get_asInt();
              pMetadata->m_iDateModified = pDS->fv("iDateModified").get_asInt();
              vecMediaFiles.push_back(pMetadata);
              iItemLimit--;
            }

          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get picture files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iResult = MEDIA_DATABASE_ERROR;
  }
  return iResult;

}

}
