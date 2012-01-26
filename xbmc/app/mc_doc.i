
// File: index.xml

// File: classXAPP_1_1ActionEvent.xml
%feature("docstring") XAPP::ActionEvent "";


// File: classXAPP_1_1ActionListener.xml
%feature("docstring") XAPP::ActionListener "";

%feature("docstring")  XAPP::ActionListener::ActionPerformed "";

%feature("docstring")  XAPP::ActionListener::~ActionListener "";


// File: classXAPP_1_1App.xml
%feature("docstring") XAPP::App "

Main class for working with an application.

Includes launching of applications, accessing the application
configuration parameters storage, etc. Get the App object by calling
GetApp() function. ";

%feature("docstring")  XAPP::App::ActivateWindow "

Activate a window of the application with parameters.

Parameters:
-----------

windowId:  window id

parameters:  parameters that will be passed to the application ";

%feature("docstring")  XAPP::App::Close "

Closes the application and stops application thread. ";

%feature("docstring")  XAPP::App::GetAppDir "

Returns the full path where the application is installed. ";

%feature("docstring")  XAPP::App::GetAppMediaDir "

Returns the full path where the media files of the application are
stored. ";

%feature("docstring")  XAPP::App::GetAppSkinDir "

Returns the full path where the skin files of the application are
stored. ";

%feature("docstring")  XAPP::App::GetAuthenticationToken "

Returns an authentication token for the application.

For boxee internal use. ";

%feature("docstring")  XAPP::App::GetId "

Returns the id of the currently running application. ";

%feature("docstring")  XAPP::App::GetLaunchedListItem "

Returns the list item that the application was launched with.

this is mainly useful to get an item from the
history/recommendation/rating. ";

%feature("docstring")  XAPP::App::GetLaunchedScriptParameters "

Returns the parameters of the executed script using RunScript.

You can also access those parameters with sys.arv[1]. ";

%feature("docstring")  XAPP::App::GetLaunchedWindowParameters "

Returns the parameters of the activated application window using
ActivateWindow. ";

%feature("docstring")  XAPP::App::GetLocalConfig "

Returns the local configuration storage for an application. ";

%feature("docstring")  XAPP::App::RunScript "

Run a python script of an application.

Parameters:
-----------

scriptName:  script name

parameters:  parameters that will be passed to the application script
";

%feature("docstring")  XAPP::App::SendMessage "

Send a message to an application The mesage will be passed to a global
handler.

Parameters:
-----------

handler:  allows to specify which handler should eventually handle the
message

parameter:  additional string parameter ";


// File: classXAPP_1_1AppException.xml
%feature("docstring") XAPP::AppException "

Exception thrown by the app API. ";

%feature("docstring")  XAPP::AppException::getMessage "

Get the message associated with the exception. ";


// File: classXAPP_1_1Button.xml
%feature("docstring") XAPP::Button "

Represents a button control in the user interface.

Get the Button object by calling GetButton() on the Window. ";

%feature("docstring")  XAPP::Button::GetLabel "

Get button label.

the label of the button ";

%feature("docstring")  XAPP::Button::SetLabel "

Set the label of the button.

Parameters:
-----------

label:  the label of the button ";


// File: classXAPP_1_1Control.xml
%feature("docstring") XAPP::Control "

Represents a control in the user interface.

Get the Control object by calling GetControl() on the Window. ";

%feature("docstring")  XAPP::Control::GetControlId "

Get window id. ";

%feature("docstring")  XAPP::Control::GetWindowId "

Get window id. ";

%feature("docstring")  XAPP::Control::HasFocus "

Returns true if the control holds the focus. ";

%feature("docstring")  XAPP::Control::IsEnabled "

Returns true if the control is enabled. ";

%feature("docstring")  XAPP::Control::IsVisible "

Returns true if the control is visible. ";

%feature("docstring")  XAPP::Control::SetEnabled "

Sets whether the control is enabled.

Controls which are not enabled cannot be focused.

Parameters:
-----------

enabled:  true to make the control enabled. false otherwise. ";

%feature("docstring")  XAPP::Control::SetFocus "

Requests a focus on the control. ";

%feature("docstring")  XAPP::Control::SetVisible "

Changes the visibility of the control.

Parameters:
-----------

visible:  true to make the control visible. false otherwise. ";


// File: classDllSkinNativeApp.xml
%feature("docstring") DllSkinNativeApp "";


// File: classDllSkinNativeAppInterface.xml
%feature("docstring") DllSkinNativeAppInterface "";

%feature("docstring")
DllSkinNativeAppInterface::DllSkinNativeAppInterface "";

%feature("docstring")  DllSkinNativeAppInterface::xapp_initialize "";

%feature("docstring")
DllSkinNativeAppInterface::~DllSkinNativeAppInterface "";


// File: classXAPP_1_1Edit.xml
%feature("docstring") XAPP::Edit "

Represents an edit control in the user interface.

Get the Edit object by calling GetEdit() on the Window. ";

%feature("docstring")  XAPP::Edit::GetText "

Gets the text of the edit control. ";

%feature("docstring")  XAPP::Edit::SetText "

Sets the text of the edit control.

Parameters:
-----------

text:  the text of the edit control ";


// File: classXAPP_1_1Http.xml
%feature("docstring") XAPP::Http "

This class represents the Http object. ";

%feature("docstring")  XAPP::Http::Delete "

perform an HTTP DELETE request (verb - DELETE)

Parameters:
-----------

strUrl:  the url to DELETE ";

%feature("docstring")  XAPP::Http::Download "

download a file

Parameters:
-----------

strUrl:  the url of the file to download

strLocalPath:  the path to save the file to ";

%feature("docstring")  XAPP::Http::Get "

perform an HTTP GET request

Parameters:
-----------

strUrl:  the url to GET ";

%feature("docstring")  XAPP::Http::GetHttpHeader "

retrieves an http header from the reqponse

Parameters:
-----------

strKey:  header name ";

%feature("docstring")  XAPP::Http::GetHttpResponseCode "

get the http response code from the operation ";

%feature("docstring")  XAPP::Http::Http "

Creates a new http object. ";

%feature("docstring")  XAPP::Http::Post "

perform an HTTP POST request

Parameters:
-----------

strUrl:  the url to POST to

strPostData:  the data (must be textual) to post in the request ";

%feature("docstring")  XAPP::Http::Reset "

reset the object - all data will re-initialize ";

%feature("docstring")  XAPP::Http::SetHttpHeader "

set a request http header

Parameters:
-----------

strKey:  header name

strValue:  header value ";

%feature("docstring")  XAPP::Http::SetUserAgent "

set the user-agent field of the http request

Parameters:
-----------

strUserAgent:  the user-agent value to use ";

%feature("docstring")  XAPP::Http::~Http "";


// File: classXAPP_1_1Image.xml
%feature("docstring") XAPP::Image "

Represents an image control in the user interface.

Get the Image object by calling GetImage() on the Window. ";

%feature("docstring")  XAPP::Image::SetTexture "

Set the texture of the image.

Parameters:
-----------

imagePath:  the full path where the image resides. Should be a local
file only. Supported formats: PNG, GIF, JPG. ";


// File: classXAPP_1_1KeyEvent.xml
%feature("docstring") XAPP::KeyEvent "";


// File: classXAPP_1_1KeyListener.xml
%feature("docstring") XAPP::KeyListener "";

%feature("docstring")  XAPP::KeyListener::KeyPressed "";

%feature("docstring")  XAPP::KeyListener::~KeyListener "";


// File: classXAPP_1_1Label.xml
%feature("docstring") XAPP::Label "

Represents a label control in the user interface.

Get the Label object by calling GetLabel() on the Window. ";

%feature("docstring")  XAPP::Label::GetLabel "

Returns the value of the label. ";

%feature("docstring")  XAPP::Label::SetLabel "

Set the label text.

Parameters:
-----------

label:  the text of the label ";


// File: classXAPP_1_1List.xml
%feature("docstring") XAPP::List "

Represents a list control in the user interface.

Get the List object by calling GetList() on the Window. ";

%feature("docstring")  XAPP::List::GetFocusedItem "

Returns the index of the focused item in the list.

Throws an exception if no item is focused. ";

%feature("docstring")  XAPP::List::GetItem "

Returns a specific item in the list.

Throws an exception if the item does not exist. ";

%feature("docstring")  XAPP::List::GetItems "

Returns all the items in the list. ";

%feature("docstring")  XAPP::List::GetSelected "

Returns all the selected items in the list. ";

%feature("docstring")  XAPP::List::IsSelected "

Returns true if the item with specified index.

Parameters:
-----------

itemIndex:  - index of the item ";

%feature("docstring")  XAPP::List::JumpToLetter "

Moves the focus in the list to the first item with the specified
letter.

Parameters:
-----------

letter:  the letter to jump to. ";

%feature("docstring")  XAPP::List::Refresh "

Refreshes the contents of the list. ";

%feature("docstring")  XAPP::List::ScrollPageDown "

Scrolls the list one page down for vertical lists or right for
horizontal lists. ";

%feature("docstring")  XAPP::List::ScrollPageUp "

Scrolls the list one page up for vertical lists or left for horizontal
lists. ";

%feature("docstring")  XAPP::List::SelectAll "

Selects all the items in the list. ";

%feature("docstring")  XAPP::List::SetContentURL "

Sets the list with items from an RSS feed.

Please see the RSS specification for supported RSS formats.

Parameters:
-----------

url:  url of the RSS. Should have and rss:// scheme. ";

%feature("docstring")  XAPP::List::SetFocusedItem "

Focuses a specific item in the list.

Only one item can be focused in the list.

Parameters:
-----------

item:  index of item in the list that should be focused ";

%feature("docstring")  XAPP::List::SetItems "

Loads the list with items.

Parameters:
-----------

list:  items to be set in the list. ";

%feature("docstring")  XAPP::List::SetSelected "

Selects/unselects an item in the list.

Single/multiple selection is defined in the skin.

Parameters:
-----------

item:  index of item in the list that should be selected/unselected

selected:  true to select the item, false to unselect ";

%feature("docstring")  XAPP::List::UnselectAll "

Unselect all the items in the list. ";

%feature("docstring")  XAPP::List::UpdateItem "

Updates an existing item.

Parameters:
-----------

item:  to be set in the list. ";


// File: classXAPP_1_1ListItem.xml
%feature("docstring") XAPP::ListItem "

This class represents an item that is displayed in a user interface
List or part of a playlist.

It contains a long site of properties that are common for media items.
Additional custom properties can be used as well. Most of the
properties are optional. The required properties are: label, path,
content type and media type. ";

%feature("docstring")  XAPP::ListItem::AddAlternativePath "

Adds an altenative path for playing the media.

When the user clicks on the item, additional Play options will be
listed. This is useful to supporting playback of both SD and HD
content.

Parameters:
-----------

label:  the label that will appear in the action menu (for example,
Play HD)

path:  the path of media file. could be either a local file or a URL
of protocols:http://, mms:// or flash://

contentType:  the content type of the media

thumbUrl:  the thumbnail that will be displayed next to the label ";

%feature("docstring")  XAPP::ListItem::AddCast "

Adds a cast to the list of cat. ";

%feature("docstring")  XAPP::ListItem::AddCastAndRole "

Adds a cast with role to the list of cast. ";

%feature("docstring")  XAPP::ListItem::ClearCastAndRole "

Clears the list of cast. ";

%feature("docstring")  XAPP::ListItem::ClearProperty "

Clears custom properties from item.

Parameters:
-----------

key:  the key name for the property ";

%feature("docstring")  XAPP::ListItem::Dump "

Dumps the content of the list item to the boxee log.

This is useful for debugging purposes. ";

%feature("docstring")  XAPP::ListItem::GetAlbum "

Returns the album. ";

%feature("docstring")  XAPP::ListItem::GetArtist "

Returns the artist. ";

%feature("docstring")  XAPP::ListItem::GetAuthor "

Returns the author. ";

%feature("docstring")  XAPP::ListItem::GetCast "

Returns the cast. ";

%feature("docstring")  XAPP::ListItem::GetCastAndRole "

Returns the cast and role. ";

%feature("docstring")  XAPP::ListItem::GetComment "

Returns the comment. ";

%feature("docstring")  XAPP::ListItem::GetContentRating "

Returns the content rating. ";

%feature("docstring")  XAPP::ListItem::GetContentType "

Returns the content tyep. ";

%feature("docstring")  XAPP::ListItem::GetDate "

Returns the release date. ";

%feature("docstring")  XAPP::ListItem::GetDescription "

Returns the description. ";

%feature("docstring")  XAPP::ListItem::GetDirector "

Returns the director. ";

%feature("docstring")  XAPP::ListItem::GetDuration "

Returns the duration in seconds. ";

%feature("docstring")  XAPP::ListItem::GetDurationFormatted "

Returns the duration formatted as HH:MM:SS. ";

%feature("docstring")  XAPP::ListItem::GetEpisode "

Returns the episode number for tv shows. ";

%feature("docstring")  XAPP::ListItem::GetGenre "

Returns the genre. ";

%feature("docstring")  XAPP::ListItem::GetIcon "

Returns the icon. ";

%feature("docstring")  XAPP::ListItem::GetImage "

Gets additional (non thumbnail) image attached to the list item via
SetImage(). ";

%feature("docstring")  XAPP::ListItem::GetKeywords "

Returns the keywords. ";

%feature("docstring")  XAPP::ListItem::GetLabel "

Returns the label. ";

%feature("docstring")  XAPP::ListItem::GetMediaType "

Get the media type. ";

%feature("docstring")  XAPP::ListItem::GetPath "

Returns the path. ";

%feature("docstring")  XAPP::ListItem::GetProperty "

Returns a custom property. ";

%feature("docstring")  XAPP::ListItem::GetProviderSource "

Returns the provider source. ";

%feature("docstring")  XAPP::ListItem::GetReportToServer "

Returns true if playing the item will be reported to boxee. ";

%feature("docstring")  XAPP::ListItem::GetResumable "

Returns true if item can be resumed from last stopped position. ";

%feature("docstring")  XAPP::ListItem::GetSeason "

Returns the season number for tv shows. ";

%feature("docstring")  XAPP::ListItem::GetSize "

Returns the size. ";

%feature("docstring")  XAPP::ListItem::GetSizeFormatted "

Returns the size, formatted with commas. ";

%feature("docstring")  XAPP::ListItem::GetStarRating "

Returns the star rating. ";

%feature("docstring")  XAPP::ListItem::GetStudio "

Returns the studio. ";

%feature("docstring")  XAPP::ListItem::GetTagLine "

Returns the tag line. ";

%feature("docstring")  XAPP::ListItem::GetThumbnail "

Returns the thumbnail. ";

%feature("docstring")  XAPP::ListItem::GetTitle "

Returns the title. ";

%feature("docstring")  XAPP::ListItem::GetTrackNumber "

Returns the track number. ";

%feature("docstring")  XAPP::ListItem::GetTVShowTitle "

Returns the tv show title. ";

%feature("docstring")  XAPP::ListItem::GetViewCount "

Rerturns the view count. ";

%feature("docstring")  XAPP::ListItem::GetViewCountFormatted "

Rerturns the view count formatted with commas,. ";

%feature("docstring")  XAPP::ListItem::GetWatched "

Returns true if item is marked as watched in the boxee client and
false otherwise. ";

%feature("docstring")  XAPP::ListItem::GetWriter "

Returns the writer. ";

%feature("docstring")  XAPP::ListItem::GetYear "

Returns the release year. ";

%feature("docstring")  XAPP::ListItem::HasProperty "

Returns true if the property exist.

Since: 0.9.12 ";

%feature("docstring")  XAPP::ListItem::IsEnabledRate "

Returns a boolean flag whether the rate action is (like/dislike)
enabled/disabled in the boxee client. ";

%feature("docstring")  XAPP::ListItem::IsEnabledRecommend "

Returns a boolean flag whether the recommend action is
enabled/disabled in the boxee client. ";

%feature("docstring")  XAPP::ListItem::ListItem "

Constructor to create a new list item.

Parameters:
-----------

mediaType:  the type of media that the list item represents. ";

%feature("docstring")  XAPP::ListItem::Select "

Marks item state as selected, this flag is usually used by the UI to
highlight marked items in the list. ";

%feature("docstring")  XAPP::ListItem::SetAddToHistory "

Boolean flag that defines whether boxee will add this list item to the
history when it is played.

By default items are not added to history. If the played content is
inappropriate, it should not be added to history. ";

%feature("docstring")  XAPP::ListItem::SetAlbum "

Sets the album name. ";

%feature("docstring")  XAPP::ListItem::SetArtist "

Sets the arist name. ";

%feature("docstring")  XAPP::ListItem::SetAuthor "

Sets the author. ";

%feature("docstring")  XAPP::ListItem::SetCanRepeat "";

%feature("docstring")  XAPP::ListItem::SetCanShuffle "";

%feature("docstring")  XAPP::ListItem::SetComment "

Sets the comment. ";

%feature("docstring")  XAPP::ListItem::SetContentRating "

Sets the content rating (MPAA, TV). ";

%feature("docstring")  XAPP::ListItem::SetContentType "

Sets the content type. ";

%feature("docstring")  XAPP::ListItem::SetDate "

Sets the release date. ";

%feature("docstring")  XAPP::ListItem::SetDescription "

Sets the description. ";

%feature("docstring")  XAPP::ListItem::SetDirector "

Sets the director for video media. ";

%feature("docstring")  XAPP::ListItem::SetDuration "

Sets the duration in seconds. ";

%feature("docstring")  XAPP::ListItem::SetEnableRate "

Boolean flag that enables/disables the rate (like/dislike) action in
the boxee client.

By default the recommend action is enabled. ";

%feature("docstring")  XAPP::ListItem::SetEnableRecommend "

Boolean flag that enables/disables the recommend action in the boxee
client.

By default the recommend action is enabled. ";

%feature("docstring")  XAPP::ListItem::SetEpisode "

Sets the episode number for TV show. ";

%feature("docstring")  XAPP::ListItem::SetExternalItem "

In some (rare) cases you would like the application to play this item
but when reporting to the server or updating the history, you would
like to use another list item.

In that case, create another list item and set it to be the external
item of this one. ";

%feature("docstring")  XAPP::ListItem::SetGenre "

Sets the genre. ";

%feature("docstring")  XAPP::ListItem::SetIcon "

Sets the icon.

Could be either a local file or a URL with the protocolhttp://. The
icon will be scaled to 200x200, so it can not be used for displaying
large images. ";

%feature("docstring")  XAPP::ListItem::SetImage "

Sets additional (non thumbnail) image.

This image is not scaled and loaded in the background. There could be
up to 10 images per item.

Parameters:
-----------

id:  id of the image between 0 and 9

url:  url of the image. protocol should behttp:// ";

%feature("docstring")  XAPP::ListItem::SetKeywords "

Sets the key words. ";

%feature("docstring")  XAPP::ListItem::SetLabel "

Sets the label. ";

%feature("docstring")  XAPP::ListItem::SetLabel2 "

Sets label2. ";

%feature("docstring")  XAPP::ListItem::SetPath "

Sets the path.

It could be either a local file or a URL with the protocols:http://,
mms:// or flash:// (flash is described in the RSS specification). ";

%feature("docstring")  XAPP::ListItem::SetPauseOnSeek "";

%feature("docstring")  XAPP::ListItem::SetProperty "

Sets custom properties to the item.

Parameters:
-----------

key:  the key name for the property

value:  the value of the property ";

%feature("docstring")  XAPP::ListItem::SetProviderSource "

Sets the provider source.

This should be the web site name (e.g. CBS.com). This is used when an
item is played from the feed, so the user will see \"Play on
<provider-name\" ";

%feature("docstring")  XAPP::ListItem::SetReportToServer "

Boolean flag that defines whether boxee will report to the server that
this item was played.

By default every played content is reported to boxee. If the played
content is inappropriate, it should not be reported to boxee. ";

%feature("docstring")  XAPP::ListItem::SetResumable "

Boolean flag which determines whether the item can be resumed from
last stopped position. ";

%feature("docstring")  XAPP::ListItem::SetSeason "

Sets the season number for TV show. ";

%feature("docstring")  XAPP::ListItem::SetSize "

Sets the file size. ";

%feature("docstring")  XAPP::ListItem::SetStarRating "

Sets the star rating for the content.

Should be a value between 0 and 10. ";

%feature("docstring")  XAPP::ListItem::SetStudio "

Sets the studio or network. ";

%feature("docstring")  XAPP::ListItem::SetTagLine "

Sets the tag line. ";

%feature("docstring")  XAPP::ListItem::SetThumbnail "

Sets the thumbnail.

Could be either a local file or a URL with the protocolhttp://. The
thumbnail will be scaled to 200x200, so it can not be used for
displaying large images. ";

%feature("docstring")  XAPP::ListItem::SetTitle "

Sets the title. ";

%feature("docstring")  XAPP::ListItem::SetTrackNumber "

Sets the track number for audio CDs. ";

%feature("docstring")  XAPP::ListItem::SetTVShowTitle "

Sets the TV show title (different than the title which is the episode
title) ";

%feature("docstring")  XAPP::ListItem::SetViewCount "

Sets the number of views. ";

%feature("docstring")  XAPP::ListItem::SetWriter "

Sets the writer. ";

%feature("docstring")  XAPP::ListItem::SetYear "

Sets the release year. ";


// File: classXAPP_1_1ListItems.xml
%feature("docstring") XAPP::ListItems "

This class represents a list of ListItem.

Should be used as a python array. Use append(), remove() etc. ";


// File: classXAPP_1_1LocalConfig.xml
%feature("docstring") XAPP::LocalConfig "

Represents the storage of local configuration for an application.

Local means that it is stored in the client (in the future there might
be server side configuration). Get the LocalConfig object by calling
GetLocalConfig() on the App object. Configuration is stored in a
key/value manner. A key may have multiple values. ";

%feature("docstring")  XAPP::LocalConfig::GetValue "

Retrieves a value from the configuration file.

If the key has multiple values, you can specify the index of the of
key between curly brackets. For example, if the key \"friends\" has
multiple values, you can retreive the 5th element by requesting key:
friend{4}

Parameters:
-----------

key:  the key identifier of the value to retrieve ";

%feature("docstring")  XAPP::LocalConfig::Implode "

Imploes all the values of a key to a single string, glued together.

Parameters:
-----------

glue:  the glue string between values

key:  the key identifier of the value to retrieve ";

%feature("docstring")  XAPP::LocalConfig::PushBackValue "

Adds value to a key.

Add the value to the back of the list of values.

Parameters:
-----------

key:  the key identifier of the value to store

value:  the actual value to store

limit:  maximum number of values allowed for this key. if the number
is passed, values will be chopped from the front. ";

%feature("docstring")  XAPP::LocalConfig::PushFrontValue "

Adds value to a key.

Add the value to the front of the list of values.

Parameters:
-----------

key:  the key identifier of the value to store

value:  the actual value to store

limit:  maximum number of values allowed for this key. if the number
is passed, values will be chopped from the back. ";

%feature("docstring")  XAPP::LocalConfig::Reset "

Resets and deletes a value from the configuration file.

Parameters:
-----------

key:  the key identifier of the value to reset ";

%feature("docstring")  XAPP::LocalConfig::ResetAll "

Resets and deletes all values from the configuration file. ";

%feature("docstring")  XAPP::LocalConfig::SetValue "

Sets and stores a value in the configuraion file.

Parameters:
-----------

key:  the key identifier of the value to store

value:  the actual value to store ";


// File: classXAPP_1_1MC.xml
%feature("docstring") XAPP::MC "

Main class for interfacing with BOXEE. ";

%feature("docstring")  XAPP::MC::ActivateWindow "

Activate a specific window by its id.

Parameters:
-----------

id:  the id of the window ";

%feature("docstring")  XAPP::MC::CloseWindow "

Closes the currently active window and go to the previously open
window. ";

%feature("docstring")  XAPP::MC::GetActiveWindow "

Get a reference to the currently active window. ";

%feature("docstring")  XAPP::MC::GetApp "

Returns a reference to the App object, that should be used for
application specific operations. ";

%feature("docstring")  XAPP::MC::GetCookieJar "

Returns the cookie jar used by boxee. ";

%feature("docstring")  XAPP::MC::GetCurrentPositionInSec "

Returns current position of the played video in seconds.

Parameters:
-----------

strPath:  - path of the video ";

%feature("docstring")  XAPP::MC::GetDeviceId "

Returns device id. ";

%feature("docstring")  XAPP::MC::GetDirectory "

Returns contents of specified path.

Parameters:
-----------

strPath:  - path ";

%feature("docstring")  XAPP::MC::GetFocusedItem "

Returns focused item from the list.

Parameters:
-----------

windowId:  - window id

listId:  - id of the list ";

%feature("docstring")  XAPP::MC::GetGeoLocation "

Returns geo location of the current user. ";

%feature("docstring")  XAPP::MC::GetHardwareModel "

Returns box model name. ";

%feature("docstring")  XAPP::MC::GetHardwareRevision "

Returns box revision number. ";

%feature("docstring")  XAPP::MC::GetHardwareSerialNumber "

Returns box revision number. ";

%feature("docstring")  XAPP::MC::GetHardwareVendor "

Returns box vendor name. ";

%feature("docstring")  XAPP::MC::GetInfoString "

Returns information about the user interface.

See separate documentation regarding information that can be
retrieved.

Parameters:
-----------

info:  a string representing the information to be retrieved. ";

%feature("docstring")  XAPP::MC::GetLocalizedString "

Returns a localized string based on an id of the string.

Parameters:
-----------

id:  id of the string. could be either a system wide id or an
application specific id. ";

%feature("docstring")  XAPP::MC::GetPlatform "

Returns platform id. ";

%feature("docstring")  XAPP::MC::GetPlayer "

Returns a reference to a media player that can be used for playing
media. ";

%feature("docstring")  XAPP::MC::GetSystemLanguage "

Returns the system language. ";

%feature("docstring")  XAPP::MC::GetTempDir "

Returns the full path of a directory where temporary files can be
placed. ";

%feature("docstring")  XAPP::MC::GetTemperatureScale "

Return the current temperature scale.

Either 'C' or 'F'. ";

%feature("docstring")  XAPP::MC::GetTimezoneCity "

Return the current timezone city setting location. ";

%feature("docstring")  XAPP::MC::GetTimezoneCountry "

Return the current timezone country setting location. ";

%feature("docstring")  XAPP::MC::GetUniqueId "

Returns unique box id. ";

%feature("docstring")  XAPP::MC::GetWeatherLocation "

Return the current weather setting location.

For example: \"USNY0996 - New York, NY\" ";

%feature("docstring")  XAPP::MC::GetWindow "

Get a reference to the window by its id.

Parameters:
-----------

id:  the id of the window ";

%feature("docstring")  XAPP::MC::HideDialogWait "

Hides the wait dialog. ";

%feature("docstring")  XAPP::MC::IsConnectedToInternet "

Returns true if has internet connection. ";

%feature("docstring")  XAPP::MC::IsEmbedded "

Returns true if running on embedded platform and false otherwise. ";

%feature("docstring")  XAPP::MC::LogDebug "

Log debug message into the Boxee log file.

Parameters:
-----------

msg:  message to be written to the log file ";

%feature("docstring")  XAPP::MC::LogError "

Log error message into the Boxee log file.

Parameters:
-----------

msg:  message to be written to the log file ";

%feature("docstring")  XAPP::MC::LogInfo "

Log information message into the Boxee log file.

Parameters:
-----------

msg:  message to be written to the log file ";

%feature("docstring")  XAPP::MC::SetItems "

Set the list of item to specified list.

Parameters:
-----------

windowId:  - window id

controlId:  - control id

items:  - item list

selectedItem:  - selected item ";

%feature("docstring")  XAPP::MC::SetTemperatureScale "

Set the temperature scale.

Input should be either 'C' or 'F'. ";

%feature("docstring")  XAPP::MC::SetWeatherLocation "

Return the current weather setting location.

For example: \"USNY0996 - New York, NY\" ";

%feature("docstring")  XAPP::MC::SetWeatherLocation2 "

Set the current weather setting location.

Parameters:
-----------

cityName:  - the city name

countryCode:  - the city country code Returns true on success and
false on failure ";

%feature("docstring")  XAPP::MC::ShowDialogConfirm "

Displays a confirmation dialog, such as Ok/Canel or Yes/No.

Returns true if the confirm button was clicked or false if the cancel
button was pressed or if the dialog was closed.

Parameters:
-----------

heading:  heading for the dialog

body:  contents of the dialog. use [CR] for line breaks.

cancelButton:  text to appear in the cancel button. Default is
\"Cancel\". Optional.

confirmButton:  text to appear in the confirm button. Default is
\"Ok\". Optional. ";

%feature("docstring")  XAPP::MC::ShowDialogKeyboard "

Displays a keyboard dialog for text input.

Returns true if a value was entered or false if the dialog was
cancelled.

Parameters:
-----------

heading:  heading of the dialog

defaultValue:  value to be pre-populated in the dialog when displayed.
if the dialog was closed with \"Ok\" it also contains the value that
was types.

hiddenInput:  false - the typed value is displayed, true - the typed
value is hidden and * are shown instead. ";

%feature("docstring")  XAPP::MC::ShowDialogNotification "

Displays a notification at the upper right corner of the screen for 5
seconds.

Parameters:
-----------

msg:  mesage to be notified to the user.

thumbnail:  file name that contains the image to be displayed.
Optional. ";

%feature("docstring")  XAPP::MC::ShowDialogOk "

Displays an \"Ok\" dialog for displaying information to the user.

Parameters:
-----------

heading:  heading of the dialog

body:  contents of the dialog. use [CR] for line breaks. ";

%feature("docstring")  XAPP::MC::ShowDialogSelect "

Displays a selection dialog between several string options.

Parameters:
-----------

heading:  heading of the dialog

choices:  choices to select from ";

%feature("docstring")  XAPP::MC::ShowDialogWait "

Displays a wait dialog.

This should be displayed during long operations. ";


// File: classXAPP_1_1NativeApp.xml
%feature("docstring") XAPP::NativeApp "";

%feature("docstring")  XAPP::NativeApp::Call "";

%feature("docstring")  XAPP::NativeApp::NativeApp "";

%feature("docstring")  XAPP::NativeApp::Process "";

%feature("docstring")  XAPP::NativeApp::Start "";

%feature("docstring")  XAPP::NativeApp::~NativeApp "";


// File: classXAPP_1_1Parameters.xml
%feature("docstring") XAPP::Parameters "

This class represents parameters that are passed to application
launch.

Should be used as a dictionary. ";

%feature("docstring")  XAPP::Parameters::toQueryString "

Converts the parameters in a URL query to a string:
key=value&key2=value. ";


// File: classXAPP_1_1Player.xml
%feature("docstring") XAPP::Player "

This class represents the Player object used for playing all kinds of
media items. ";

%feature("docstring")  XAPP::Player::FeedRaw "";

%feature("docstring")  XAPP::Player::FlushRaw "";

%feature("docstring")  XAPP::Player::GetLastPlayerAction "

Get last action performed by the player. ";

%feature("docstring")  XAPP::Player::GetLastPlayerEvent "

Returns the last event that occured in the player. ";

%feature("docstring")  XAPP::Player::GetPlayingItem "

Returns the currently playing item. ";

%feature("docstring")  XAPP::Player::GetPlaylistTimecode "

Returns the current time (EXT-X-PROGRAM-DATE-TIME) in seconds of the
currently playing HTTP live stream. ";

%feature("docstring")  XAPP::Player::GetRepeatState "

Return true if the current playlist is repeat. ";

%feature("docstring")  XAPP::Player::GetSeekRequestTime "

Get lst player seek time.

Parameters:
-----------

action:  player action ";

%feature("docstring")  XAPP::Player::GetTime "

Returns the current time in seconds of the currently playing media.

Fractional portions of a second are possible. This returns a double to
be consistent with GetTotalTime() and SeekTime(). Throws an exception
if no item is currently playing. ";

%feature("docstring")  XAPP::Player::GetTotalTime "

Returns the total time in seconds of the current media.

Fractional portions of a second are possible - but not necessarily
supported by the player class. This returns a double to be consistent
with GetTime() and SeekTime(). Throws an exception if no item is
currently playing. ";

%feature("docstring")  XAPP::Player::GetVolume "

Returns current volume of the application. ";

%feature("docstring")  XAPP::Player::IsCaching "

Returns true if the player is currently at the caching phase. ";

%feature("docstring")  XAPP::Player::IsForwarding "

Returns true if the player is currently perfroming fast forward
operation. ";

%feature("docstring")  XAPP::Player::IsPaused "

Returns true if playback is paused. ";

%feature("docstring")  XAPP::Player::IsPlaying "

Returns true if Boxee is currently playing media, false otherwise. ";

%feature("docstring")  XAPP::Player::IsPlayingAudio "

Returns true if Boxee is currently playing audio, false otherwise. ";

%feature("docstring")  XAPP::Player::IsPlayingVideo "

Returns true if Boxee is currently playing video, false otherwise. ";

%feature("docstring")  XAPP::Player::IsRewinding "

Returns true if the player is currently perfroming rewind operation.
";

%feature("docstring")  XAPP::Player::IsShuffle "

Return true if the current playlist is shuffle. ";

%feature("docstring")  XAPP::Player::LockPlayerAction "

Disable specific player action Use XAPP_PLAYER_ACTION_NONE to unlock
all action.

Parameters:
-----------

action:  action to lock ";

%feature("docstring")  XAPP::Player::Pause "

Pause playback. ";

%feature("docstring")  XAPP::Player::Play "

Plays the specified list item.

Parameters:
-----------

item:  item to play ";

%feature("docstring")  XAPP::Player::Player "

Creates a new player object.

Parameters:
-----------

bRegisterCallbacks:  indicates whether player action and event
callbacks are tracked, false by default. If you enable callbacks you
must have a single instance of a player in your application. ";

%feature("docstring")  XAPP::Player::PlayInBackground "

Plays the specified list item without switching to full screen video
window.

Parameters:
-----------

item:  item to play ";

%feature("docstring")  XAPP::Player::PlayNext "

Skip to next item in the playlist. ";

%feature("docstring")  XAPP::Player::PlayPrevious "

Skip to the previous item in the playlist. ";

%feature("docstring")  XAPP::Player::PlayRaw "

Plays the specified list item.

Parameters:
-----------

item:  item to play ";

%feature("docstring")  XAPP::Player::PlaySelected "

Play item with specified index from the current playlist.

Parameters:
-----------

iItem:  index of the item to play from the current playlist

TODO: Can not use the enum because of name conflict with existing
PLAYLIST_MUSIC and PLAYLIST_VIDEO ";

%feature("docstring")  XAPP::Player::PlaySlideshow "

Plays list of pictures in slideshow.

Parameters:
-----------

pictures:  - list of pictures to present

bRandom:  - always shuffle list

bNotRandom:  - do not shuffle list (if true) if false use
\"slideshow.shuffle\" setting

strPictureToStart:  - picture to start with

startPaused:  - whether the slide show will be started in paused state
";

%feature("docstring")  XAPP::Player::PlayWithActionMenu "

Open Media Action Dialog on specified item.

Parameters:
-----------

item:  item to play ";

%feature("docstring")  XAPP::Player::QueueNextItem "";

%feature("docstring")  XAPP::Player::QueueNextRaw "";

%feature("docstring")  XAPP::Player::RawClose "";

%feature("docstring")  XAPP::Player::RawIsEmpty "";

%feature("docstring")  XAPP::Player::RawSetEOF "";

%feature("docstring")  XAPP::Player::ResumeAudio "";

%feature("docstring")  XAPP::Player::SeekTime "

Sets the current position of the currently playing media to the
specified time in seconds.

Fractional portions of a second are valid. The passed time is the time
offset from the beginning of the file as opposed to a delta from the
current position. This method accepts a double to be consistent with
GetTime() and GetTotalTime().

Parameters:
-----------

seekTime:  time in seconds ";

%feature("docstring")  XAPP::Player::SetLastPlayerAction "

Set lst player action.

Parameters:
-----------

action:  player action ";

%feature("docstring")  XAPP::Player::SetLastPlayerEvent "

Set player event.

Parameters:
-----------

event:  player event ";

%feature("docstring")  XAPP::Player::SetOsdExtAmount "";

%feature("docstring")  XAPP::Player::SetSeekRequestTime "

Set lst player seek time.

Parameters:
-----------

action:  player action ";

%feature("docstring")  XAPP::Player::SetVolume "

Set Boxee volume.

Parameters:
-----------

percent:  volume value (in percent) ";

%feature("docstring")  XAPP::Player::Stop "

Stops playback. ";

%feature("docstring")  XAPP::Player::ToggleMute "

Mute Boxee. ";

%feature("docstring")  XAPP::Player::UpdateItem "";

%feature("docstring")  XAPP::Player::~Player "";


// File: classXAPP_1_1PlayList.xml
%feature("docstring") XAPP::PlayList "

Represents a play list. ";

%feature("docstring")  XAPP::PlayList::Add "

Add item to the playlist.

Parameters:
-----------

item:  item to add ";

%feature("docstring")  XAPP::PlayList::AddBackground "

Add item to the playlist to be played in the background.

Parameters:
-----------

item:  item to add ";

%feature("docstring")  XAPP::PlayList::Clear "

Clears playlist. ";

%feature("docstring")  XAPP::PlayList::GetItem "

Returns the item with the specified index from the playlist. ";

%feature("docstring")  XAPP::PlayList::GetPosition "

Returns the position of the current item in the playlist. ";

%feature("docstring")  XAPP::PlayList::IsShuffle "

Return true if the playlist is shuffle. ";

%feature("docstring")  XAPP::PlayList::Play "

Plays the specified item from the playlist. ";

%feature("docstring")  XAPP::PlayList::PlayList "

Create playlist of the specifed type (music or video) ";

%feature("docstring")  XAPP::PlayList::Size "

Returns the size of the playlist. ";

%feature("docstring")  XAPP::PlayList::~PlayList "";


// File: classXAPP_1_1Textbox.xml
%feature("docstring") XAPP::Textbox "";

%feature("docstring")  XAPP::Textbox::SetText "";


// File: classXAPP_1_1ToggleButton.xml
%feature("docstring") XAPP::ToggleButton "

Represents a toggle button control in the user interface.

Get the ToggleButton object by calling GetToggleButton() on the
Window. ";

%feature("docstring")  XAPP::ToggleButton::IsSelected "

Returns true if the toggle button is selected, false otherwise. ";

%feature("docstring")  XAPP::ToggleButton::SetLabel "

Set the label of the button.

Parameters:
-----------

label:  the label of the button ";

%feature("docstring")  XAPP::ToggleButton::SetSelected "

Mark the toggle button as selected/unselected.

Parameters:
-----------

selected:  true to select the toggle button, false for unselected ";


// File: classXAPP_1_1Window.xml
%feature("docstring") XAPP::Window "

Represents a toggle button control in the user interface.

Get the Window object by calling GetActiveWindow() or GetWindow()
functions. ";

%feature("docstring")  XAPP::Window::ClearStateStack "

Clears all the saved states.

Parameters:
-----------

restoreState:  restore the state of the bottom most state in the
stack? ";

%feature("docstring")  XAPP::Window::GetButton "

Returns a button control in the window. ";

%feature("docstring")  XAPP::Window::GetControl "

Returns a control in the window.

Parameters:
-----------

id:  the id of the control ";

%feature("docstring")  XAPP::Window::GetEdit "

Returns an edit control in the window. ";

%feature("docstring")  XAPP::Window::GetImage "

Returns an image control in the window. ";

%feature("docstring")  XAPP::Window::GetLabel "

Returns a label control in the window. ";

%feature("docstring")  XAPP::Window::GetList "

Returns a list control in the window. ";

%feature("docstring")  XAPP::Window::GetTextbox "

Returns a textbox control in the window. ";

%feature("docstring")  XAPP::Window::GetToggleButton "

Returns a toggle button control in the window. ";

%feature("docstring")  XAPP::Window::GetWindowId "

Get window id. ";

%feature("docstring")  XAPP::Window::PopState "

Pops a state from the state stack (see PushState()) and sets the user
interface accordingly.

Parameters:
-----------

restoreState:  true to restore the top most state in the stack ";

%feature("docstring")  XAPP::Window::PopToState "

Pops states from the state stack (see PushState()) and leaves
\"count\" states in the stack.

Parameters:
-----------

remainInStack:  number of items to keep remained in the stack ";

%feature("docstring")  XAPP::Window::PushState "

Saves the state of the window and pushes it to the top of the window
state stack.

By default, if user hits ESC or Back, instead of closing the window,
it will pop the state and return it. The state includes contents of
lists and the selected items in lists. This is useful if you want to
support \"drill down\" navigation in a window. ";

%feature("docstring")  XAPP::Window::SetProperty "";


// File: classXAPP_1_1WindowEvent.xml
%feature("docstring") XAPP::WindowEvent "";


// File: classXAPP_1_1WindowListener.xml
%feature("docstring") XAPP::WindowListener "";

%feature("docstring")  XAPP::WindowListener::WindowClosed "";

%feature("docstring")  XAPP::WindowListener::WindowClosing "";

%feature("docstring")  XAPP::WindowListener::WindowOpened "";

%feature("docstring")  XAPP::WindowListener::WindowOpening "";

%feature("docstring")  XAPP::WindowListener::WindowRender "";

%feature("docstring")  XAPP::WindowListener::~WindowListener "";


// File: namespacePLAYLIST.xml


// File: namespaceXAPP.xml


// File: AppException_8h.xml


// File: DllSkinNativeApp_8h.xml


// File: doxygen2boxeedoc_8php.xml
%feature("docstring")  printMethods "";


// File: XAPP__App_8h.xml


// File: XAPP__Button_8h.xml


// File: XAPP__Control_8h.xml


// File: XAPP__Edit_8h.xml


// File: XAPP__Http_8h.xml


// File: XAPP__Image_8h.xml


// File: XAPP__Label_8h.xml


// File: XAPP__List_8h.xml


// File: XAPP__ListItem_8h.xml


// File: XAPP__LocalConfig_8h.xml


// File: XAPP__MC_8h.xml


// File: XAPP__Native_8h.xml


// File: XAPP__Player_8h.xml


// File: XAPP__PlayList_8h.xml


// File: XAPP__Textbox_8h.xml


// File: XAPP__ToggleButton_8h.xml


// File: XAPP__Window_8h.xml

