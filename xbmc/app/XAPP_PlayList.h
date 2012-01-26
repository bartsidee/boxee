#ifndef XAPP_PLAYLIST_H_
#define XAPP_PLAYLIST_H_

#include "XAPP_ListItem.h"

namespace PLAYLIST
{
  class CPlayList;
}

namespace XAPP
{

/**
 * Represents a play list.
 */
class PlayList
{
public:
  /**
   * Playlist type - either music or video
   */
  enum PlayListType
  {
    /**
     * Playlist of music items
     */
    PLAYLIST_MUSIC,

    /**
     * Playlist of video items
     */
    PLAYLIST_VIDEO
  };
  
  /**
   * Create playlist of the specifed type (music or video) 
   */
  PlayList(XAPP::PlayList::PlayListType type);
	
  virtual ~PlayList() {}
  
  /**
   * Plays the specified item from the playlist
   */
  void Play(int iItem);
  
  /**
   * Add item to the playlist
   * 
   * @param item item to add
   */
  virtual void Add(ListItem item);
  
  /**
   * Add item to the playlist to be played in the background
   *
   * @param item item to add
   */
  virtual void AddBackground(ListItem item);

  /**
   * Returns the item with the specified index from the playlist
   */
  virtual ListItem GetItem(int index);
  
  /**
   * Clears playlist
   */
  virtual void Clear();
  
  /**
   * Returns the position of the current item in the playlist
   */
  virtual int GetPosition();
  
  /**
   * Returns the size of the playlist
   */
  virtual int Size();

  /**
   * Return true if the playlist is shuffle
   */
  virtual bool IsShuffle();

private:
  PLAYLIST::CPlayList *m_playList;
  PlayListType iPlayList;
};

}

#endif
