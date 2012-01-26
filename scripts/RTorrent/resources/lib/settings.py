import sys
import os
import xbmc
import xbmcgui

from utilities import *

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__

class GUI( xbmcgui.WindowXMLDialog ):
	""" Settings module: used for changing settings """
	def __init__( self, *args, **kwargs ):
		pass

	def onInit( self ):
		self.download_path = Settings().get_download_path()
		self._set_labels()
		self._set_functions()
		self._set_controls_values()

	def set_server( self, rtorrent ):
		self.rtorrent = rtorrent
	
	def _set_labels( self ):
		print "_set_labels"
		try:
			xbmcgui.lock()
			for x in range( 1, 8 ):
				print _( 200 + x )
				self.getControl( 200 + x ).setLabel( _( 200 + x ) )
			xbmcgui.unlock()
		except Exception, e:
			error = _( 634 ) % ( "_set_labels:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
			xbmcgui.unlock()
			
	def _set_functions( self ):
		self.functions = {}
		for x in range( 1, 8 ):
			self.functions[ 200 + x ] = eval( "self._change_setting%d" % x )

###### End of Special defs #####################################################

	def _set_controls_values( self ):
		""" sets the value labels """
		try:
			xbmcgui.lock()
			self.getControl( 221 ).setLabel( "%sKb/s" % ( self.rtorrent.get_download_rate() / 1024 ) )
			self.getControl( 222 ).setLabel( "%sKb/s" % ( self.rtorrent.get_upload_rate() / 1024 ) )
			self.getControl( 223 ).setLabel( str( self.rtorrent.get_max_uploads() ) )
			self.getControl( 224 ).setLabel( str( self.rtorrent.get_port_range() ) )
			self.getControl( 225 ).setLabel( str( self.rtorrent.get_max_peers() ) )
			self.getControl( 226 ).setLabel( str( self.rtorrent.get_min_peers() ) )
			self.getControl( 227 ).setLabel( self.download_path )
			xbmcgui.unlock()
		except Exception, e:
			error = _( 634 ) % ( "_set_controls_values:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
			xbmcgui.unlock()

	def _change_setting1( self ):
		""" changes settings #1 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_download_rate() / 1024 ), _( 200 + 1 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_download_rate() / 1024 ), _( 200 + 1 ) )
			self.rtorrent.set_download_rate( int( num ) * 1024 )		
		except Exception, e:
			error = _( 634 ) % ( "_change_setting1:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

	def _change_setting2( self ):
		""" changes settings #2 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_upload_rate() / 1024 ), _( 200 + 2 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_upload_rate() / 1024 ), _( 200 + 2 ) )
			self.rtorrent.set_upload_rate( int( num ) * 1024 )
		except Exception, e:
			error = _( 634 ) % ( "_change_setting2:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

	def _change_setting3( self ):
		""" changes settings #3 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_max_uploads() ), _( 200 + 3 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_max_uploads() ), _( 200 + 3 ) )
			self.rtorrent.set_max_uploads( int( num ) )
		except Exception, e:
			error = _( 634 ) % ( "_change_setting3:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

	def _change_setting4( self ):
		""" changes settings #4 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_max_uploads() ), _( 200 + 4 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_port_range() ), _( 200 + 4 ) )
			self.rtorrent.set_port_range( num )
		except Exception, e:
			error = _( 634 ) % ( "_change_setting4:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

	def _change_setting5( self ):
		""" changes settings #5 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_max_uploads() ), _( 200 + 5 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_max_peers() ), _( 200 + 5 ) )
			self.rtorrent.set_max_peers( int( num ) )
		except Exception, e:
			error = _( 634 ) % ( "_change_setting5:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

	def _change_setting6( self ):
		""" changes settings #6 """
		try:
			#num = get_keyboard( str( self.rtorrent.get_max_uploads() ), _( 200 + 6 ), 0 )
			num = get_keyboard( str( self.rtorrent.get_min_peers() ), _( 200 + 6 ) )
			self.rtorrent.set_min_peers( int( num ) )
		except Exception, e:
			error = _( 634 ) % ( "_change_setting6:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()
		
	def _change_setting7( self ):
		""" changes settings #7 """
		try:
			path = get_browse_dialog( self.download_path, _( 200 + 7 ), 0 )#, treat_as_folder=False )
			if not os.path.exists( os.path.expanduser( path ) ):
				LOG( LOG_ERROR, "_change_setting7: path %s does not exist!" % ( path, ) )
				xbmcgui.Dialog().ok( _( 649 ), _( 634 ) % _( 1011 ), path )
			else:
				self.download_path = path
		except Exception, e:
			error = _( 634 ) % ( "_change_setting7:" + str ( e ) ) 
			LOG( LOG_ERROR, error )
		self._set_controls_values()

##### End of unique defs ######################################################
	
	def _save_settings( self ):
		""" saves settings """
		ok = Settings().save_settings( self.rtorrent, self.download_path )
		if ( not ok ):
			ok = xbmcgui.Dialog().ok( __scriptname__, _( 230 ) )
		self._close_dialog()

	def _update_script( self ):
		""" checks for updates to the script """
		import update
		updt = update.Update()
		del updt
		
	def _close_dialog( self, changed=False, restart=False, refresh=False ):
		""" closes this dialog window """
		self.changed = changed
		self.restart = restart
		self.refresh = refresh
		self.close()

	def onClick( self, controlId ):
		#xbmc.sleep(5)
		self.functions[ controlId ]()

	def onFocus( self, controlId ):
		self.controlId = controlId

	def onAction( self, action ):
		if ( action.getButtonCode() in CANCEL_DIALOG ):
			#self._close_dialog()
			self._save_settings()
