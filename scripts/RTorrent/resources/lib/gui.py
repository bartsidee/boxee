import sys
import os
import shutil
import xbmc
import xbmcgui
import threading
from utilities import *
import urllib
import xmlrpc2scgi

try: current_dlg_id = xbmcgui.getCurrentWindowDialogId()
except: current_dlg_id = 0
current_win_id = xbmcgui.getCurrentWindowId()

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__

RTORRENT_LOCALHOST = "scgi://localhost:5000"
VIEWMODE = ( ( "main", "All" ), ( "incomplete", "Active" ), ( "stopped", "Paused" ), ( "complete", "Completed" ) )
LIST_MODE_ITEM_HEIGHT = 35
ID_BUTTON_VIEWMODE = 111
ID_LIST_VIEWMODE = 112
ID_IMAGE_VIEWMODE_TOP = 113
ID_IMAGE_VIEWMODE_BOTTOM = 114
ID_LABEL_VIEWMODE = 115
ID_BUTTON_STOP_START_ALL = 116
ID_IMAGE_STOP_START_ALL = 117
ID_IMAGE_STOP_START_ALL_FOCUS = 118
ID_BUTTON_SETTINGS = 119
ID_LABEL_DOWNRATE = 102
ID_LABEL_UPRATE = 104
ID_LABEL_STATUS = 101
ID_BUTTON_RESUME_PAUSE_PLAY = 131
ID_BUTTON_REMOVE_TORRENT = 132
ID_LIST_DOWNLOADS = 150
TIMER_UPDATE = 1.0

class GUI( xbmcgui.WindowXMLDialog ):
	def __init__( self, *args, **kwargs ):
		pass
	
	def onInit( self ):
		LOG( LOG_INFO, "onInit" )
		self.setup_all()
		try:
			self.rtorrent = xmlrpc2scgi.RTorrentXMLRPCClient( RTORRENT_LOCALHOST )
			if self.rtorrent.get_bind():
				#methods = self.rtorrent.system.listMethods()
				#for item in methods:
				#	print item
				LOG( LOG_INFO, _( 630 ) )
				self.update()
				if( len( self.downloads_list ) == 0 ):
					self.setFocus( self.getControl( ID_BUTTON_VIEWMODE ) )
		except Exception, e:
			error = _( 662 )
			LOG( LOG_ERROR, error )
			self.getControl( ID_LABEL_STATUS ).setLabel( error )
			self.setFocus( self.getControl( ID_BUTTON_VIEWMODE ) )
	
	def update( self ):
		self.timer_activated = False
		self.update_rates()
		self.get_downloads_list()
		self.downloads_update()
		self.stop_start_all_button_update()
		self.timer = threading.Timer( TIMER_UPDATE, self.update, () )
		self.timer.start()
		self.timer_activated = True
	
	def setup_all( self ):
		self.setup_variables()
	
	def setup_variables( self ):
		self.downloads_list = []
		self.timer_activated = False
		self.stop_all = True
		self.viewmode_setup()
		self.context_menu = True
		self.last_focusedId = ID_LIST_DOWNLOADS
		self.context_toggole()
		self.selected_item = -1
		self.exit = False
		
	def update_rates( self ):
		try:
			#these functions stopped working using this other method instead
			#self.getControl( ID_LABEL_DOWNRATE ).setLabel( _( 632 ) %  ( self.rtorrent.get_down_rate()))# / 1024 ) )
			#self.getControl( ID_LABEL_UPRATE ).setLabel( _( 632 ) %	 ( self.rtorrent.get_up_rate()))# / 1024 ) )
			downloads_list = self.rtorrent.d.multicall( VIEWMODE[self.viewmode_value][0], "d.get_down_rate=", "d.get_up_rate=" )
			down_rate = 0
			up_rate = 0
			for item in downloads_list:
				down_rate = down_rate + item[0]
				up_rate = up_rate + item[1]
			self.getControl( ID_LABEL_DOWNRATE ).setLabel( _( 632 ) %  ( down_rate / 1024 ) )
			self.getControl( ID_LABEL_UPRATE ).setLabel( _( 632 ) %	 ( up_rate / 1024 ) )
		except Exception, e:
			error = _( 634 ) % ( "update_rates:" + str ( e ) )
			LOG( LOG_ERROR, error )
			
	def get_downloads_list( self ):
		try:
			self.downloads_list = []
			self.downloads_list = self.rtorrent.d.multicall( VIEWMODE[self.viewmode_value][0], "d.get_hash=", "d.get_name=", "d.get_bytes_done=", "d.get_size_bytes=", "d.get_down_rate=", "d.is_active=", "d.get_complete=", "d.get_hashing=", "d.is_hash_checking=", "d.get_connection_current=", "d.get_message=" )
		except Exception, e:
			error = _( 634 ) % ( "get_downloads_list:" + str ( e ) )
			LOG( LOG_ERROR, error )

	def downloads_update( self ):
		if ( len( self.downloads_list ) == self.getControl( ID_LIST_DOWNLOADS ).size() ):
			list_reset = False
		else:
			list_reset = True
			self.getControl( ID_LIST_DOWNLOADS ).reset()
		try:
			xbmcgui.lock()
			i = 0
			self.stop_all = False
			for item in self.downloads_list:
				name = item[1] #d.get_name()
				bytes_done = item[2] #d.get_bytes_done()
				bytes_total = item[3] #d.get_size_bytes()
				bytes_rate = item[4] #d.get_down_rate()
				progress = _( 638 ) % ( float ( bytes_done / ( 1000*1024 ) ), float ( bytes_rate / 1024 ), )
				if item[8]: #d.is_hash_checking()
					icon = "downloads_icon_downloading.png"
					progress = _( 639 ) % ( float ( bytes_done / ( 1000*1024 ) ), float ( bytes_total / ( 1000*1024 ) ), )
					completed = _( 642 )
					active = True
					error = _( 658 )
				if item[6]: #d.get_complete()
					icon = "downloads_icon_completed.png"
					progress = _( 640 ) % ( float ( bytes_done / ( 1000*1024 ) ), )
					completed = _( 643 )
					active = False
					error = _( 659 )
				elif item[5]: #d.is_active()
					icon = "downloads_icon_downloading.png"
					progress = _( 641 ) % ( float ( bytes_done / ( 1000*1024 ) ), float ( bytes_total / ( 1000*1024 ) ), float ( bytes_rate / 1024 ), )
					completed = _( 644 ) % ( str( ( bytes_done * 100 ) / bytes_total ) + "%" )
					active = True
					self.stop_all = True
					error = _( 660 )
				else:
					icon = "downloads_icon_paused.png"
					progress = _( 639 ) % ( float ( bytes_done / ( 1000*1024 ) ), float ( bytes_total / ( 1000*1024 ) ), )
					completed = _( 656 ) % ( str( ( bytes_done * 100 ) / bytes_total ) + "%" )
					active = False
					error = _( 661 )
				#error = item[10]

				if list_reset:
					listitem = xbmcgui.ListItem( name, completed, iconImage="", thumbnailImage="" )
					listitem.setProperty( "progressbar", str( int( ( bytes_done * 10 ) / bytes_total ) * 10 ) )
					listitem.setProperty( "progress", progress )
					listitem.setProperty( "error", error )
					self.getControl( ID_LIST_DOWNLOADS ).addItem( listitem )
				else:
					listitem = self.getControl( ID_LIST_DOWNLOADS ).getListItem( i )
					listitem.setLabel( name )
					listitem.setLabel2( completed )
					listitem.setIconImage( "" )
					listitem.setProperty( "progressbar", str( int( ( bytes_done * 10 ) / bytes_total ) * 10 ) )
					listitem.setProperty( "progress", progress )
					listitem.setProperty( "error", error )
				i = i + 1
			xbmcgui.unlock()
		except Exception, e:
			error = _( 634 ) % ( "downloads_update:" + str ( e ) )
			LOG( LOG_ERROR, error )
			xbmcgui.unlock()
		try:
			self.getControl( ID_LABEL_STATUS ).setLabel( _( 645 ) % len( self.downloads_list ) )
		except Exception, e:
			error = _( 634 ) % ( "downloads_update:" + str ( e ) )
			LOG( LOG_ERROR, error )


	def viewmode_setup( self ):
		try:
			self.viewmode_value = 0
			self.viewmode_dropdown = False
			xbmcgui.lock()
			self.getControl( ID_LIST_VIEWMODE ).reset()
			for item in VIEWMODE:
				self.getControl( ID_LIST_VIEWMODE ).addItem( item[1] )
			self.getControl( ID_LIST_VIEWMODE ).setVisible( False )
			x, y = self.getControl( ID_LIST_VIEWMODE ).getPosition()
			self.getControl( ID_IMAGE_VIEWMODE_BOTTOM ).setPosition( x, ( y + len( VIEWMODE ) * LIST_MODE_ITEM_HEIGHT ) )
			self.getControl( ID_LABEL_VIEWMODE ).setLabel( VIEWMODE[0][1] )
			xbmcgui.unlock()
		except Exception, e:
			xbmcgui.unlock()
			error = _( 634 ) % ( "viewmode_setup:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def viewmode_toggole_dropdown( self ):
		try:
			xbmcgui.lock()
			if self.viewmode_dropdown:
				self.getControl( ID_LIST_VIEWMODE ).setVisible( False )
				self.setFocus( self.getControl( ID_BUTTON_VIEWMODE ) )
				self.viewmode_dropdown = False
			else:
				self.getControl( ID_LIST_VIEWMODE ).setVisible( True )
				self.setFocus( self.getControl( ID_LIST_VIEWMODE ) )
				self.viewmode_dropdown = True
			xbmcgui.unlock()
		except Exception, e:
			xbmcgui.unlock()
			error = _( 634 ) % ( "viewmode_toggole_dropdown:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def viewmode_update( self, pos ):
		try:
			if self.viewmode_value == pos:
				self.viewmode_toggole_dropdown()
			elif ( pos < len( VIEWMODE ) ):
				self.viewmode_value = pos
				self.getControl( ID_LABEL_VIEWMODE ).setLabel( VIEWMODE[pos][1] )
				self.viewmode_toggole_dropdown()
				if self.timer_activated:
					self.timer.cancel()
				self.update()
		except Exception, e:
			error = _( 634 ) % ( "viewmode_update:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def stop_all_downloads( self ):
		downloads = []
		try:
			downloads = self.rtorrent.download_list( VIEWMODE[1][0] )
			if ( len( downloads ) == 0 ):
				return
			self.getControl( ID_LABEL_STATUS ).setLabel( _( 633 ) )
			for item in downloads:
				self.rtorrent.d.stop( item )
		except Exception, e:
			error = _( 634 ) % ( "stop_all_downloads:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def start_all_downloads( self ):
		downloads = []
		try:
			downloads = self.rtorrent.download_list( VIEWMODE[2][0] )
			if ( len( downloads ) == 0 ):
				return
			self.getControl( ID_LABEL_STATUS ).setLabel( _( 635 ) )
			for item in downloads:
				self.rtorrent.d.start( item )
		except Exception, e:
			error = _( 634 ) % ( "start_all_downloads:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def stop_start_all_button_update( self ):
		try:
			if( len( self.downloads_list ) > 0 ):
				self.getControl( ID_BUTTON_STOP_START_ALL ).setEnabled( True )
			else:
				self.getControl( ID_BUTTON_STOP_START_ALL ).setEnabled( False )
			if( self.stop_all ):
				self.getControl( ID_BUTTON_STOP_START_ALL ).setLabel( _( 636 ) )
				self.getControl( ID_IMAGE_STOP_START_ALL ).setImage( "button_pause.png" )
				self.getControl( ID_IMAGE_STOP_START_ALL_FOCUS ).setImage( "button_pause_on.png" )
			else:
				self.getControl( ID_BUTTON_STOP_START_ALL ).setLabel( _( 637 ) )
				self.getControl( ID_IMAGE_STOP_START_ALL ).setImage( "button_play.png" )
				self.getControl( ID_IMAGE_STOP_START_ALL_FOCUS ).setImage( "button_play_on.png" )
		except Exception, e:
			error = _( 634 ) % ( "stop_start_all_button_update:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def context_toggole( self ):
		try:
			if self.context_menu:
				self.getControl( 130 ).setVisible( False )
				self.context_menu = False
				self.setFocus( self.getControl( self.last_focusedId ) )
			else:
				self.context_menu = True
				self.getControl( 130 ).setVisible( True )
				self.setFocus( self.getControl( ID_BUTTON_RESUME_PAUSE_PLAY ) )
				self.last_focusedId = self.controlId
		except Exception, e:
			error = _( 634 ) % ( "context_toggole:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def start_stop_play_download( self, pos ):
		try:
			downloads = []
			downloads = self.rtorrent.download_list( VIEWMODE[self.viewmode_value][0] )
			item = downloads[pos]
			name = self.rtorrent.d.get_name( item )
			path = self.rtorrent.d.get_base_path( item )
			active = self.rtorrent.d.is_active( item )
			complete = self.rtorrent.d.get_complete( item )
			if active and not complete:
				self.getControl( ID_LABEL_STATUS ).setLabel( _( 646 ) % ( name, ) )
				self.rtorrent.d.stop( item )
			elif not complete:
				self.getControl( ID_LABEL_STATUS ).setLabel( _( 647 ) % ( name, ) )
				self.rtorrent.d.start( item )
			else:
				self.getControl( ID_LABEL_STATUS ).setLabel( _( 648 ) % ( name, ) )
				if os.path.isdir( path ):
					xbmc.openBrowse( path )
				elif os.path.isfile( path ):
					xbmc.executebuiltin( "XBMC.PlayMedia(%s)" % ( path, ) )
				self.exit_script()
		except Exception, e:
			error = _( 634 ) % ( "stop_download:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def remove_download_torrent( self, pos ):
		try:
			downloads = []
			downloads = self.rtorrent.download_list( VIEWMODE[self.viewmode_value][0] )
			item = downloads[pos]
			name = self.rtorrent.d.get_name( item )
			path = self.rtorrent.d.get_base_path( item )
			torrent = self.rtorrent.d.get_tied_to_file( item )
			active = self.rtorrent.d.is_active( item )
			ok = xbmcgui.Dialog().yesno( _( 649 ), name, _( 650 ), "", _( 260 ), _( 259 ) )
			if not ok:
				return
			self.getControl( ID_LABEL_STATUS ).setLabel( _( 651 ) % ( name, ) )
			if active:
				LOG( LOG_INFO, "Stopping item %s" % ( name, ) )
				self.rtorrent.d.stop( item )
				xbmc.sleep( 100 )
			LOG( LOG_INFO, "Erasing item %s" % ( name, ) )
			path = "special://home/torrent/.tmp"
			path = xbmc.translatePath( path )
			if os.path.exists( path ):
				LOG( LOG_INFO, "Deleting item %s" % ( path, ) )
				if os.path.isfile( path ):
					os.remove( path )
				elif os.path.isdir( path ):
					#os.rmdir( path )
					shutil.rmtree( path )
			self.rtorrent.d.erase( item )
			xbmc.sleep( 100 )
			self.get_downloads_list()
			self.downloads_update()
			if os.path.exists( torrent ):
				LOG( LOG_INFO, "%s was not erased!" % ( torrent, ) )
		except Exception, e:
			error = _( 634 ) % ( "remove_download_torrent:" + str( e ) )
			LOG( LOG_ERROR, error )
	
	def set_download_item( self, pos ):
		try:
			downloads = []
			self.selected_item = pos
			downloads = self.rtorrent.download_list( VIEWMODE[self.viewmode_value][0] )
			item = downloads[pos]
			#self.getControl( ID_DOWNLOADS_GROUP + ( 10 * pos ) + 6 ).setVisible( True )
			name = self.rtorrent.d.get_name( item )
			filename = self.rtorrent.d.get_base_filename( item )
			filepath = self.rtorrent.d.get_base_path( item )
			active = self.rtorrent.d.is_active( item )
			complete = self.rtorrent.d.get_complete( item )
			self.context_toggole()
			if active and not complete:
				self.getControl( ID_BUTTON_RESUME_PAUSE_PLAY ).setLabel( _( 653 ) )
			elif not complete:
				self.getControl( ID_BUTTON_RESUME_PAUSE_PLAY ).setLabel( _( 654 ) )
			else:
				if os.path.isdir( filepath ):
					self.getControl( ID_BUTTON_RESUME_PAUSE_PLAY ).setLabel( _( 657 ) )
				elif os.path.isfile( filepath ):
					self.getControl( ID_BUTTON_RESUME_PAUSE_PLAY ).setLabel( _( 655 ) )
		except Exception, e:
			error = _( 634 ) % ( "set_download_item:" + str ( e ) )
			LOG( LOG_ERROR, error )
	
	def change_settings( self ):
		import settings
		settings = settings.GUI( "script-%s-settings.xml" % ( __scriptname__.replace( " ", "_" ), ), os.getcwd(), "Boxee" )
		settings.set_server( self.rtorrent )
		settings.doModal()
		ok = False
		del settings
		
	def cleanup( self ):
		if self.timer_activated:
			self.timer.cancel()

	def exit_script( self, restart=False ):
		if self.timer_activated:
			self.timer.cancel()
		self.close()

	def onClick( self, controlId ):
		if ( controlId == ID_BUTTON_STOP_START_ALL ):
			if self.stop_all:
				self.stop_all_downloads()
			else:
				self.start_all_downloads()
		elif ( controlId == ID_BUTTON_SETTINGS ):
			self.change_settings()
			#xbmc.openBrowse('/Users/idan/Movies')
			#xbmc.showNotification('title','message','')
		elif ( controlId == ID_BUTTON_VIEWMODE ):
			if not self.viewmode_dropdown:
				self.viewmode_toggole_dropdown()
		elif ( controlId == ID_LIST_VIEWMODE ):
			self.viewmode_update( self.getControl( ID_LIST_VIEWMODE ).getSelectedPosition() )
		elif ( controlId == ID_LIST_DOWNLOADS ):
			self.set_download_item( self.getControl( ID_LIST_DOWNLOADS ).getSelectedPosition() )
		elif ( controlId == ID_BUTTON_RESUME_PAUSE_PLAY ):
			if ( self.selected_item > -1 ):
				self.start_stop_play_download( self.selected_item )
			self.context_toggole()
			self.setFocus( self.getControl( ID_LIST_DOWNLOADS ) )
		elif ( controlId == ID_BUTTON_REMOVE_TORRENT ):
			if ( self.selected_item > -1 ):
				self.remove_download_torrent( self.selected_item )
			self.context_toggole()
			self.setFocus( self.getControl( ID_LIST_DOWNLOADS ) )
		
	def onFocus( self, controlId ):
		self.controlId = controlId
	
	def onAction( self, action ):
		if ( action.getButtonCode() in CANCEL_DIALOG ):
			if self.context_menu:
				self.context_toggole()
			elif self.viewmode_dropdown:
				self.viewmode_toggole_dropdown()
			else:
				self.exit_script()
			
