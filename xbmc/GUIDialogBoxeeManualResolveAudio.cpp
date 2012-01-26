
#include "GUIDialogBoxeeManualResolveAudio.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/bxmetadataengine.h"
#include "lib/libBoxee/boxee.h"
#include "GUIEditControl.h"
#include "MetadataResolver.h"
#include "SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "GUIDialogBoxeeManualResolveAlbum.h"
#include "MusicInfoTag.h"

using namespace BOXEE;

#define TITLE_EDIT 5010
#define BTN_SEARCH 5020
#define BTN_DROP   5025
#define RESULT_LIST 5030

CGetAlbumResultListBG::CGetAlbumResultListBG(const CStdString& strTitle, CFileItemList* fileList)
{
  m_strTitle = strTitle;
  m_pFileList = fileList;
}

void CGetAlbumResultListBG::Run()
{
  BXXMLDocument   doc;
  bool bRetVal = false;

  //establish the connection and query the server
  bRetVal = CMetadataResolverMusic::GetResultsFromServer(m_strTitle,"",5,doc);

  if (bRetVal) //read the data from the xml
    bRetVal = CMetadataResolverMusic::LoadAlbumsInfo(doc,m_vectorAlbums);

  //cast to CFileItemPtr and add it to the list
  for (unsigned int i = 0 ; i < m_vectorAlbums.size() ; ++i)
  {
    BXAlbum* pAlbum = (BXAlbum*)m_vectorAlbums[i].GetDetail(MEDIA_DETAIL_ALBUM);

    CFileItemPtr resultItem (new CFileItem(pAlbum->m_strTitle));
    
    DIRECTORY::CBoxeeDatabaseDirectory::CreateAlbumItem(&m_vectorAlbums[i], resultItem.get());

    //cache the album thumb for results
    CStdString strCachedThumb = CUtil::GetCachedAlbumThumb(pAlbum->m_strTitle,pAlbum->m_strArtist);

    // TODO: Move image loading to a separate job, in case for some reason the image is not avaialble
    if (CPicture::CreateThumbnail(resultItem->GetThumbnailImage(), strCachedThumb, false))
    {
      CUtil::ThumbCacheAdd(strCachedThumb,true);
      resultItem->SetThumbnailImage(strCachedThumb);
    }

    m_pFileList->Add(resultItem);
  }
}

CGUIDialogBoxeeManualResolveAudio::CGUIDialogBoxeeManualResolveAudio() :
      CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_AUDIO, "boxee_manual_resolve_audio.xml"), m_resolvedMetadata(MEDIA_ITEM_TYPE_AUDIO)
      {
        m_bConfirmed = false;
      }

CGUIDialogBoxeeManualResolveAudio::~CGUIDialogBoxeeManualResolveAudio() {
}

bool CGUIDialogBoxeeManualResolveAudio::Show(CFileItemPtr pItem)
{
  if (!pItem) return false;

  CGUIDialogBoxeeManualResolveAudio *pDialog = (CGUIDialogBoxeeManualResolveAudio*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_RESOLVE_AUDIO);
  if (pDialog)
  {
    // Copy the item into the dialog
    pDialog->Reset();
    pDialog->m_unresolvedItem = pItem;
    pDialog->DoModal();

    return pDialog->m_bConfirmed;
  }

  return false;
}

void CGUIDialogBoxeeManualResolveAudio::Reset()
{
  m_resolvedMetadata.Reset();
  m_resultListItems.Clear();
}

void CGUIDialogBoxeeManualResolveAudio::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bConfirmed = false;

  m_resultListItems.Clear();

  m_unresolvedItem->Dump();

  // Send the item to the special container to allow skin acceess
  CFileItemPtr itemPtr(new CFileItem(*m_unresolvedItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolve::OnInitWindow, initialized manual resolve video, path = %s (manual)", m_unresolvedItem->m_strPath.c_str());

  // Set extracted name as a starting point for manual resolving
  ((CGUIEditControl*)GetControl(TITLE_EDIT))->SetLabel2(m_unresolvedItem->GetLabel());

}

void CGUIDialogBoxeeManualResolveAudio::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveAudio::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == BTN_SEARCH)
    {
      GetList();
      return true;
    }
    else if (iControl == RESULT_LIST)
    {
      // Get selected item from the list and open matching details dialog
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), RESULT_LIST);
      OnMessage(msg);

      int iSelectedItem = msg.GetParam1();

      if (iSelectedItem < 0 || iSelectedItem > m_resultListItems.Size() -1)
        return true;

      m_selectedItem = m_resultListItems.Get(iSelectedItem);

      if (m_selectedItem->HasProperty("listerror"))
      {
        return true;
      }
      
      m_bConfirmed = CGUIDialogBoxeeManualResolveAlbum::Show(m_selectedItem);

      if (m_bConfirmed)
      {
        AddAlbum(m_selectedItem);
        Close();
        return true;
        //TODO: change items properties
      }
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeManualResolveAudio::AddAlbum(CFileItemPtr _selectedItem)
{
  BOXEE::BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
  std::vector<BOXEE::BXMetadata*> vectorAlbumTracks;
  BOXEE::BXAlbum album;
  CStdString folderPath = m_unresolvedItem->GetProperty("AlbumFolderPath");
  int iAlbumId;

  if (MDE.GetAlbumByPath(folderPath,&album) != MEDIA_DATABASE_ERROR)
  {
    //read all track in the current album
    if (MDE.GetSongsFromAlbum(album.m_iId,vectorAlbumTracks) == MEDIA_DATABASE_ERROR)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAudio::AddAlbum, Could not get album track list for album id: %d",album.m_iId);
      return false;
    }

    //delete the old album
    if (MDE.RemoveAlbumById(album.m_iId) == MEDIA_DATABASE_ERROR)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAudio::AddAlbum, could not remove album id: %d , does it exist?",album.m_iId);
      return false;
    }
    
    //delete the old tracks related to that album
    if (MDE.RemoveAudioByPath(folderPath) == MEDIA_DATABASE_ERROR)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAudio::AddAlbum, could not find audio tracks with this path %s",folderPath.c_str());
      return false;
    }

    album.m_strArtwork = _selectedItem->GetThumbnailImage();
    album.m_strTitle = _selectedItem->GetMusicInfoTag()->GetAlbum();
    album.m_strGenre = _selectedItem->GetMusicInfoTag()->GetGenre();
    album.m_strDescription = _selectedItem->GetMusicInfoTag()->GetComment();
   
    // if its a totally different artist
    if (album.m_strArtist != _selectedItem->GetMusicInfoTag()->GetArtist())
    {
      BOXEE::BXArtist artist;
      int iArtistId;
      artist.m_strName = _selectedItem->GetMusicInfoTag()->GetArtist();
      //add the new artist to our db
      iArtistId = MDE.AddArtist(&artist);

      if (iArtistId == MEDIA_DATABASE_ERROR)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAudio::AddAlbum, could not add the artist..");
        artist.Dump();
        return false;
      }
      
      //set the value of this album to the corresponding artist
      album.m_strArtist = _selectedItem->GetMusicInfoTag()->GetArtist();
      album.m_iArtistId = iArtistId;

      std::vector<BOXEE::BXMetadata*>::iterator it = vectorAlbumTracks.begin();
      for (; it != vectorAlbumTracks.end(); it++)
      {
        BXAudio* pAudio = (BXAudio*) (*it)->GetDetail(MEDIA_DETAIL_AUDIO);
        pAudio->m_iArtistId = iArtistId;
      }
    }
      
    //add the new album to the database 
    iAlbumId = MDE.AddAlbum(&album);
    if (iAlbumId == MEDIA_DATABASE_ERROR)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAudio::AddAlbum, could not add the album..");
      album.Dump();
      return false;
    }

    m_unresolvedItem->SetProperty("BoxeeDBAlbumId",iAlbumId);
    //set the files to be related to the new album id
    for (std::vector<BOXEE::BXMetadata*>::iterator it = vectorAlbumTracks.begin() ;
      it != vectorAlbumTracks.end(); it++)
    {
      BXAudio* pAudio = (BXAudio*) (*it)->GetDetail(MEDIA_DETAIL_AUDIO);
      pAudio->m_iAlbumId = iAlbumId;
      MDE.AddAudio(pAudio);
    }

    
    //update the its resolved already.
    MDE.UpdateAudioFileStatus(folderPath,STATUS_RESOLVED);
  }

  return true;
}

bool CGUIDialogBoxeeManualResolveAudio::GetList()
{
  // Get the name with which we should resolve
  CStdString title = ((CGUIEditControl*)GetControl(TITLE_EDIT))->GetLabel2();

  m_resultListItems.Clear();

  CLog::Log(LOGDEBUG, "CGUIDialogBoxeeManualResolveAudio::GetList, get all results for title = %s (manual)", title.c_str());
  CGetAlbumResultListBG* pJob = new CGetAlbumResultListBG(title , &m_resultListItems);
  bool bResult = (CUtil::RunInBG(pJob) == JOB_SUCCEEDED);
  if (!bResult)
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeManualResolveAudio::GetList, could not get list for title = %s (manual)", title.c_str());
    CFileItemPtr itemPtr(new CFileItem(g_localizeStrings.Get(52116)));
    itemPtr->SetProperty("listerror", true);

    m_resultListItems.Add(itemPtr);
  }

  CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), RESULT_LIST);
  OnMessage(message);

  // Populate the list with results
  CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), RESULT_LIST, 0, 0, &m_resultListItems);
  OnMessage(message2);

  return bResult;
}
