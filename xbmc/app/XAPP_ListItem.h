#ifndef XAPP_LISTITEM_H_
#define XAPP_LISTITEM_H_

#include "AppException.h"
#include <string>
#include <vector>
#include "boost/shared_ptr.hpp"

class CFileItem;

/*!
  \brief A shared pointer to CFileItem
  \sa CFileItem
  */
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

namespace XAPP
{

/**
 * This class represents an item that is displayed in a user interface List or part of a playlist. It
 * contains a long site of properties that are common for media items. Additional custom properties
 * can be used as well. Most of the properties are optional. The required properties are: label, path, 
 * content type and media type.
 */
class ListItem
{
public:
  /**
   * Media type enumeration. Used to hint boxee on the type of media in the list item.
   */
  enum MediaType
  { 
    /**
     * Unknown media type
     */
    MEDIA_UNKNOWN,
    /**
     * Music
     */
    MEDIA_AUDIO_MUSIC,
    /**
     * Speech
     */
    MEDIA_AUDIO_SPEECH,
    /**
     * Radio
     */
    MEDIA_AUDIO_RADIO,
    /**
     * Other audio (not music nor speech)
     */
    MEDIA_AUDIO_OTHER, 
    /**
     * Music video
     */
    MEDIA_VIDEO_MUSIC_VIDEO,
    /**
     * Feature film
     */
    MEDIA_VIDEO_FEATURE_FILM,
    /**
     * Trailer
     */
    MEDIA_VIDEO_TRAILER,
    /**
     * TV episode
     */
    MEDIA_VIDEO_EPISODE,
    /**
     * Video clip
     */
    MEDIA_VIDEO_CLIP,
    /**
     * Other video (not video clip, tv episode, trailer nor feature film)
     */
    MEDIA_VIDEO_OTHER,
    /**
     * Picture
     */
    MEDIA_PICTURE,
    /**
     * General file 
     */
    MEDIA_FILE,
  };
  
  /**
   * Constructor to create a new list item.
   * 
   * @param mediaType the type of media that the list item represents.
   */
  ListItem(XAPP::ListItem::MediaType mediaType = MEDIA_UNKNOWN);

#ifndef DOXYGEN_SHOULD_SKIP_THIS  
  ListItem(CFileItemPtr item);
#endif
	
	/**
	 * Sets the label.
	 */
  void SetLabel(const std::string& label);
  
  /**
   * Sets label2.
   */
  void SetLabel2(const std::string& label2);

  /**
   * Sets the path. It could be either a local file or a URL with the protocols: http://, mms:// or flash:// (flash is
   * described in the RSS specification).
   */
  void SetPath(const std::string& path);
  
  /**
   * Sets the content type.
   */
  void SetContentType(const std::string& contentType);
  
  void SetPauseOnSeek(bool pauseOnSeek);

  /**
   * Sets the title.
   */
  void SetTitle(const std::string& title);
  
  /**
   * Sets the thumbnail. Could be either a local file or a URL with the protocol http://.
   * The thumbnail will be scaled to 200x200, so it can not be used for displaying large images.
   */
  void SetThumbnail(const std::string& thumbnail);
  
  /**
   * Sets the icon. Could be either a local file or a URL with the protocol http://.
   * The icon will be scaled to 200x200, so it can not be used for displaying large images.
   */  
  void SetIcon(const std::string& thumbnail);
  
  /**
   * Sets the track number for audio CDs.
   */
  void SetTrackNumber(const int trackNumber);
  
  /**
   * Sets the arist name.
   */
  void SetArtist(const std::string& artist);
  
  /**
   * Sets the album name.
   */
  void SetAlbum(const std::string& album);
  
  /**
   * Sets the release year.
   */
  void SetYear(int year);
  
  /**
   * Sets the release date
   */
  void SetDate(int year, int month, int day);
  
  /**
   * Sets the genre.
   */
  void SetGenre(const std::string& genre);
  
  /**
   * Sets the director for video media.
   */
  void SetDirector(const std::string& director);
  
  /**
   * Sets the file size.
   */
  void SetSize(long long  size);
  
  /**
   * Sets the duration in seconds.
   */
  void SetDuration(int seconds);
  
  /**
   * Sets the star rating for the content. Should be a value between 0 and 10.
   */
  void SetStarRating(float rating);
  
  /**
   * Sets the number of views.
   */
  void SetViewCount(int viewCount);
  
  /**
   * Sets the content rating (MPAA, TV).
   */
  void SetContentRating(const std::string& rating);
  
  /**
   * Sets the description. 
   */
  void SetDescription(const std::string& plotOutline, bool isHTML = false);
  
  /**
   * Sets the episode number for TV show.
   */
  void SetEpisode(int episode);
  
  /**
   * Sets the season number for TV show.
   */
  void SetSeason(int season);
  
  /**
   * Sets the TV show title (different than the title which is the episode title) 
   */
  void SetTVShowTitle(const std::string& title);
  
  /**
   * Sets the comment.
   */
  void SetComment(const std::string& comment);
  
  /**
   * Sets the studio or network.
   */
  void SetStudio(const std::string& studio);
  
  /**
   * Sets the author.
   */
  void SetAuthor(const std::string& author);
  
  /**
   * Adds a cast with role to the list of cast.
   */
  void AddCastAndRole(const std::string& name, const std::string& role);
  
  /**
   * Adds a cast to the list of cat.
   */
  void AddCast(const std::string& name);
  
  /**
   * Clears the list of cast.
   */
  void ClearCastAndRole();
  
  /**
   * Sets the writer.
   */
  void SetWriter(const std::string& write);
  
  /**
   * Sets the tag line.
   */
  void SetTagLine(const std::string& tagLine);
  
  /**
   * Sets the provider source. This should be the web site name (e.g. CBS.com). 
   * This is used when an item is played from the feed, so the user will see "Play on <provider-name"
   */
  void SetProviderSource(const std::string& provider);
  
  /**
   * Sets the key words
   */
  void SetKeywords(const std::string& keywords);
  
  /**
   * Sets additional (non thumbnail) image. This image is not scaled and loaded in the background.
   * There could be up to 10 images per item. 
   * 
   * @param id id of the image between 0 and 9
   * @param url url of the image. protocol should be http://
   */
  void SetImage(int id, const std::string& url);
  
  /**
   * Adds an altenative path for playing the media. When the user clicks on the item, additional Play
   * options will be listed. This is useful to supporting playback of both SD and HD content.
   * 
   * @param label the label that will appear in the action menu (for example, Play HD)
   * @param path the path of media file. could be either a local file or a URL of protocols: http://, mms:// or flash://
   * @param contentType the content type of the media
   * @param thumbUrl the thumbnail that will be displayed next to the label
   */
  void AddAlternativePath(const std::string& label, const std::string& path, const std::string& contentType, const std::string& thumbUrl);
  
  /**
   * Sets custom properties to the item
   * 
   * @param key the key name for the property
   * @param value the value of the property
   */
  void SetProperty(const std::string& key, const std::string& value);
  
  /**
   * Clears custom properties from item
   *
   * @param key the key name for the property
   */

  void ClearProperty(const std::string &strKey);
  /**
   * Boolean flag that defines whether boxee will report to the server that this item was played. By default every 
   * played content is reported to boxee. If the played content is inappropriate, it should not be reported to boxee.
   */
  void SetReportToServer(bool reportToServer);
  
  
  /**
   * Boolean flag which determines whether the item can be resumed from last stopped position
   */      
  void SetResumable(bool resumable);
  

  /**
   * Boolean flag that defines whether boxee will add this list item to the history when it is played. By
   * default items are not added to history. If the played content is inappropriate, it should not be added to history.
   */
  void SetAddToHistory(bool addToHistory);

  /**
   * Boolean flag that enables/disables the recommend action in the boxee client. By default
   * the recommend action is enabled.
   */
  void SetEnableRecommend(bool enabled);

  /**
   * Boolean flag that enables/disables the rate (like/dislike) action in the boxee client. By default
   * the recommend action is enabled.
   */
  void SetEnableRate(bool enabled);

  /**
   * In some (rare) cases you would like the application to play this item but when reporting to the server
   * or updating the history, you would like to use another list item. In that case, create another list item
   * and set it to be the external item of this one.
   */
  void SetExternalItem(ListItem externalItem);

  void SetCanShuffle(bool canShuffle);
  void SetCanRepeat(bool canRepeat);

  /**
   * Marks item state as selected, this flag is usually used by the UI to highlight marked items in the list
   */
  void Select(bool on);
  
  /**
   * Get the media type.
   */
  XAPP::ListItem::MediaType GetMediaType() const;
  
  /**
   * Returns the label.
   */
  std::string GetLabel() const;
  
  /**
   * Returns the content tyep.
   */
  std::string GetContentType() const;
  
  /**
   * Returns the path.
   */
  std::string GetPath() const;
  
  /**
   * Returns the title.
   */
  std::string GetTitle() const;
  
  /**
   * Returns the thumbnail.
   */
  std::string GetThumbnail() const;

  /**
   * Returns the icon.
   */
  std::string GetIcon() const;
  
  /**
   * Returns the track number.
   */
  int GetTrackNumber() const;
  
  /**
   * Returns the artist.
   */
  std::string GetArtist() const;
  
  /**
   * Returns the album.
   */
  std::string GetAlbum() const;
  
  /**
   * Returns the release year.
   */
  int GetYear() const;
  
  /**
   * Returns the release date.
   */
  std::string GetDate();
  
  /**
   * Returns the genre.
   */
  std::string GetGenre() const;
  
  /**
   * Returns the director.
   */
  std::string GetDirector() const;
  
  /**
   * Returns the size.
   */
  int GetSize() const;
  
  /**
   * Returns the size, formatted with commas.
   */
  std::string GetSizeFormatted() const;
  
  /**
   * Returns the duration in seconds.
   */
  int GetDuration() const;
  
  /**
   * Returns the duration formatted as HH:MM:SS.
   */
  std::string GetDurationFormatted() const;
  
  /**
   * Returns the star rating.
   */
  float GetStarRating() const;
  
  /**
   * Rerturns the view count.
   */
  int GetViewCount() const;

  /**
   * Rerturns the view count formatted with commas,
   */
  std::string GetViewCountFormatted() const;
  
  /**
   * Returns the content rating.
   */
  std::string GetContentRating() const;
  
  /**
   * Returns the description.
   */
  std::string GetDescription() const;
  
  /**
   * Returns the episode number for tv shows.
   */
  int GetEpisode() const;

  /**
   * Returns the season number for tv shows.
   */  
  int GetSeason() const;
  
  /**
   * Returns the tv show title.
   */
  std::string GetTVShowTitle() const;
  
  /**
   * Returns the comment.
   */
  std::string GetComment() const;
  
  /**
   * Returns the studio.
   */
  std::string GetStudio() const;
  
  /**
   * Returns the author.
   */
  std::string GetAuthor() const;
  
  /**
   * Returns the cast and role. 
   */
  std::string GetCastAndRole() const;
  
  /**
   * Returns the cast.
   */
  std::string GetCast() const;
  
  /**
   * Returns the writer.
   */
  std::string GetWriter() const;
  
  /**
   * Returns the tag line.
   */
  std::string GetTagLine() const;
  
  /**
   * Returns the provider source.
   */
  std::string GetProviderSource() const;
  
  /**
   * Returns the keywords.
   */
  std::string GetKeywords() const;
  
  /**
   * Returns a custom property.
   */
  std::string GetProperty(const std::string& key) const;

  /**
   * Returns true if the property exist.
   * 
   * Since: 0.9.12
   */
  bool HasProperty(const std::string& key) const;
  
  /**
   * Returns true if playing the item will be reported to boxee.
   */
  bool GetReportToServer();
  
  /**
   * Returns true if item can be resumed from last stopped position
   */
  bool GetResumable();
  
  /**
   * Returns true if item is marked as watched in the boxee client and false otherwise
   */
  bool GetWatched();
    

  /**
   * Returns a boolean flag whether the recommend action is enabled/disabled in the boxee client.
   */
  bool IsEnabledRecommend();

  /**
   * Returns a boolean flag whether the rate action is (like/dislike) enabled/disabled in the boxee client.
   */
  bool IsEnabledRate();
  
  /**
   * Gets additional (non thumbnail) image attached to the list item via SetImage().
   */
  std::string GetImage(int id) const;
  
  /**
   * Dumps the content of the list item to the boxee log. This is useful for debugging purposes.
   */
  void Dump();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  void SetMusicOSDButton(int id, const std::string thumbFocus, const std::string thumbNoFocus);

  void DeleteMusicOSDButton(int id);

  CFileItemPtr GetFileItem();
  
  void SetArbitratyProperty(const std::string& key, void* value);

  void* GetArbitratyProperty(const std::string& key);

protected:
  /**
   * Set the media type. This hints boxee which media will be played.
   */
  void SetMediaType(XAPP::ListItem::MediaType mediaType);
  
  CFileItemPtr m_fileItem;
#endif
};

/**
 * This class represents a list of ListItem. Should be used as a python array. Use append(), remove() etc.
 */
class ListItems : public std::vector<ListItem>
{
};

}

#endif /* LISTITEM_H_ */
