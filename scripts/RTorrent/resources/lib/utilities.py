import sys
import os
import xbmc
import xbmcgui

DEBUG_MODE = 5

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__

# comapatble versions
SETTINGS_VERSIONS = ( "1.1", )
# base paths
BASE_DATA_PATH = "special://home/script_data/" + __scriptname__ 
BASE_SETTINGS_PATH = "special://profile/script_data/" + __scriptname__
BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
# special action codes
SELECT_ITEM = ( 11, 256, 61453, )
EXIT_SCRIPT = ( 247, 275, 61467, 216, 257, 61448, )
CANCEL_DIALOG = EXIT_SCRIPT + ( 216, 257, 61448, )
GET_EXCEPTION = ( 216, 260, 61448, )
SELECT_BUTTON = ( 229, 259, 261, 61453, )
MOVEMENT_UP = ( 166, 270, 61478, )
MOVEMENT_DOWN = ( 167, 271, 61480, )
# Log status codes
LOG_INFO, LOG_ERROR, LOG_NOTICE, LOG_DEBUG = range( 1, 5 )

DOWNLOAD_RATE = "download_rate"
UPLOAD_RATE = "upload_rate"
MAX_UPLOADS = "max_uploads"
PORT_RANGE = "port_range"
MAX_PEERS = "max_peers"
MIN_PEERS = "min_peers"
ON_FINISHED = "on_finished"

DOWNLOAD_RATE_STR = "download_rate = %s\n"
UPLOAD_RATE_STR = "upload_rate = %s\n"
MAX_UPLOADS_STR = "max_uploads = %s\n"
PORT_RANGE_STR = "port_range = %s\n"
MAX_PEERS_STR = "max_peers = %s\n"
MIN_PEERS_STR = "min_peers = %s\n"
ON_FINISHED_STR = "on_finished = move_complete,\"execute=/bin/mv,-n,$d.get_base_path=,%s ;d.set_directory=%s\"\n"
	
def _create_base_paths():
	""" creates the base folders """
	if ( not os.path.isdir( BASE_DATA_PATH ) ):
		os.makedirs( BASE_DATA_PATH )
	if ( not os.path.isdir( BASE_SETTINGS_PATH ) ):
		os.makedirs( BASE_SETTINGS_PATH )
_create_base_paths()
	
def get_keyboard( default="", heading="", hidden=False ):
	""" shows a keyboard and returns a value """
	keyboard = xbmc.Keyboard( default, heading, hidden )
	keyboard.doModal()
	if ( keyboard.isConfirmed() ):
		return keyboard.getText()
	return default

def get_numeric_dialog( default="", heading="", type=0 ):
	""" shows a numeric dialog and returns a value
		- 0 : ShowAndGetNumber		(default format: #)
		- 1 : ShowAndGetDate		(default format: DD/MM/YYYY)
		- 2 : ShowAndGetTime		(default format: HH:MM)
		- 3 : ShowAndGetIPAddress	(default format: #.#.#.#)
	"""
	dialog = xbmcgui.Dialog()
	value = dialog.numeric( type, heading, default )
	return value

def get_browse_dialog( default="", heading="", type=1, shares="files", mask="", use_thumbs=False, treat_as_folder=False ):
	""" shows a browse dialog and returns a value
		- 0 : ShowAndGetDirectory
		- 1 : ShowAndGetFile
		- 2 : ShowAndGetImage
		- 3 : ShowAndGetWriteableDirectory
	"""
	dialog = xbmcgui.Dialog()
	value = dialog.browse( type, heading, shares, mask, use_thumbs, treat_as_folder, default )
	return value

def LOG( status, format, *args ):
	if ( DEBUG_MODE >= status ):
		xbmc.output( "%s: %s\n" % ( ( "INFO", "ERROR", "NOTICE", "DEBUG", )[ status - 1 ], format % args, ) )

def show_settings():
	""" shows a settings window """
	import settings
	s = settings.GUI( "script-%s-settings.xml" % ( __scriptname__.replace( " ", "_" ), ), BASE_RESOURCE_PATH, "Default" )
	s.doModal()
	del s

class Settings:
	""" Settings class """
	
	def save_settings( self, rtorrent, path ):
		""" save settings to a settings.txt file in BASE_SETTINGS_PATH """
		try:
			if os.path.exists( xbmc.translatePath("special://home/rtorrent.rc") ):
				#open file, transfer content into list self.array
				torrentrc = open( xbmc.translatePath("special://home/rtorrent.rc"), 'r' )
				torrentrc_lines = torrentrc.readlines()
				torrentrc.close()
				
				for i in range( 0, len( torrentrc_lines ) ):
					if ( torrentrc_lines[i].find( DOWNLOAD_RATE ) > -1 ):
						torrentrc_lines[i] = DOWNLOAD_RATE_STR % ( rtorrent.get_download_rate() / 1024 )
					if ( torrentrc_lines[i].find( UPLOAD_RATE ) > -1 ):
						torrentrc_lines[i] = UPLOAD_RATE_STR % ( rtorrent.get_upload_rate() / 1024 )
					if ( torrentrc_lines[i].find( MAX_UPLOADS ) > -1 ):
						torrentrc_lines[i] = MAX_UPLOADS_STR % ( rtorrent.get_max_uploads() )
					if ( torrentrc_lines[i].find( PORT_RANGE ) > -1 ):
						torrentrc_lines[i] = PORT_RANGE_STR % ( rtorrent.get_port_range() )
					if ( torrentrc_lines[i].find( MAX_PEERS ) > -1 ):
						torrentrc_lines[i] = MAX_PEERS_STR % ( rtorrent.get_max_peers() )
					if ( torrentrc_lines[i].find( MIN_PEERS ) > -1 ):
						torrentrc_lines[i] = MIN_PEERS_STR % ( rtorrent.get_min_peers() )
					if ( torrentrc_lines[i].find( ON_FINISHED ) > -1 ):
						torrentrc_lines[i] = ON_FINISHED_STR % ( path, path )
				
				torrentrc = open( xbmc.translatePath("special://home/rtorrent.rc") , 'w' )
				torrentrc.writelines( torrentrc_lines )
				torrentrc.close()
				return True
			else:
				LOG( LOG_ERROR, "RTorrent: %s settings file not found!", xbmc.translatePath("special://home/rtorrent.rc"), )
				return False
		except:
			LOG( LOG_ERROR, "RTorrent: %s (ver: %s) Settings::save_settings [%s]", __scriptname__, __version__, sys.exc_info()[ 1 ], )
			return False

	def get_download_path( self ):
		try:
			if os.path.exists( xbmc.translatePath("special://home/rtorrent.rc") ):
				#open file, transfer content into list self.array
				torrentrc = open( xbmc.translatePath("special://home/rtorrent.rc"), 'r' )
				torrentrc_lines = torrentrc.readlines()
				torrentrc.close()
				
				path = ""
				for i in range( 0, len( torrentrc_lines ) ):
					if ( torrentrc_lines[i].find( ON_FINISHED ) > -1 ):
						path = torrentrc_lines[i]
						print path
						path = path[path.find( "d.set_directory=" ) + len( "d.set_directory=" ):path.rfind( "\"" )]
						print path
				return path
			else:
				LOG( LOG_ERROR, "RTorrent: %s settings file not found!",  xbmc.translatePath("special://home/rtorrent.rc"), )
				return ""
		except:
			LOG( LOG_ERROR, "RTorrent: %s (ver: %s) Settings::get_download_path [%s]", __scriptname__, __version__, sys.exc_info()[ 1 ], )
			return ""
			
