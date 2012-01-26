import sys
import os
import xbmc
import xbmcgui
import xbmcplugin
import threading
import osdb
from osdb import OSDBServer
from utilities import *
import socket
import urllib
import unzip

try: current_dlg_id = xbmcgui.getCurrentWindowDialogId()
except: current_dlg_id = 0
current_win_id = xbmcgui.getCurrentWindowId()

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__

STATUS_LABEL = 100
LOADING_IMAGE = 110
SUBTITLES_LIST = 120

class GUI( xbmcgui.WindowXMLDialog ):
    socket.setdefaulttimeout(10.0) #seconds
	
    def __init__( self, *args, **kwargs ):
        pass

    def set_filepath( self, path ):
        LOG( LOG_INFO, "set_filepath [%s]" , ( path ) )
        self.file_original_path = path
        self.file_path = path[path.find(os.sep):len(path)]

    def set_filehash( self, hash ):
        LOG( LOG_INFO, "set_filehash [%s]" , ( hash ) )
        self.file_hash = hash

    def set_filesize( self, size ):
        LOG( LOG_INFO, "set_filesize [%s]" , ( size ) )
        self.file_size = size

    def set_searchstring( self, search ):
        LOG( LOG_INFO, "set_searchstring [%s]" , ( search ) )
        self.search_string = search

    def onInit( self ):
        LOG( LOG_INFO, "onInit" )
        self.setup_all()
        if self.file_path or self.search_string:
            #self.timer = threading.Timer( 0.5, self.connect(), () )
            #self.timer.start()        
            self.connThread = threading.Thread( target=self.connect, args=() )
            self.connThread.start()
        
    def setup_all( self ):
        self.setup_variables()
        
    def setup_variables( self ):
        self.controlId = -1
        self.allow_exception = False
        self.osdb_server = OSDBServer()
        self.osdb_server.Create()
        if xbmc.Player().isPlayingVideo():
            self.set_filepath( xbmc.Player().getPlayingFile() )

    def connect( self ):
        self.getControl( LOADING_IMAGE ).setVisible( True )
        self.getControl( STATUS_LABEL ).setLabel( _( 646 ) )
        ok,msg = self.osdb_server.connect( "", "" )
        if not ok:
            self.getControl( STATUS_LABEL ).setLabel( _( 653 ) )
            self.getControl( LOADING_IMAGE ).setVisible( False )
            return
        else:
            self.getControl( STATUS_LABEL ).setLabel( _( 635 ) )

        self.osdb_server.getlanguages()
        self.search_subtitles()
        self.getControl( LOADING_IMAGE ).setVisible( False )
        self.getControl( STATUS_LABEL ).setVisible( False )
        
    def search_subtitles( self ):
        try:
            if ( len( self.file_path ) > 0 ):
                LOG( LOG_INFO, _( 642 ) % ( os.path.basename( self.file_path ), ) )
                self.getControl( STATUS_LABEL ).setLabel( _( 642 ) % ( "...", ) )
                #ok,msg = self.osdb_server.searchsubtitlesbyhash( self.file_path )#, "en" )
                self.set_filehash( xbmc.getFileHash( self.file_original_path ) )
                self.set_filesize( xbmc.getFileSize( self.file_original_path ) )    
                ok,msg = self.osdb_server.searchsubtitles( self.file_path, self.file_hash, self.file_size )#, "en" )
                LOG( LOG_INFO, msg )        
            if ( len( self.search_string ) > 0 ):
                LOG( LOG_INFO, _( 642 ) % ( os.path.basename( self.search_string ), ) )
                self.getControl( STATUS_LABEL ).setLabel( _( 642 ) % ( "......", ) )
                ok2,msg2 = self.osdb_server.searchsubtitlesbyname( self.search_string )#, "en" )
                LOG( LOG_INFO, msg2 )
            self.osdb_server.mergesubtitles()
            if not ok and not ok2:
                self.getControl( STATUS_LABEL ).setLabel( _( 634 ) % ( msg, ) )
            elif self.osdb_server.subtitles_list:
                for item in self.osdb_server.subtitles_list:
                    listitem = xbmcgui.ListItem( label=item["language_name"], label2=item["filename"], iconImage=item["rating"], thumbnailImage=item["language_flag"] )
                    #self.getControl( SUBTITLES_LIST ).addItem( xbmcgui.ListItem( item["filename"], item["language_name"], iconImage=item["rating"], thumbnailImage = item["language_flag"] ) )
                    if item["sync"]:
                        listitem.setProperty( "sync", "true" )
                    else:
                        listitem.setProperty( "sync", "false" )
                    self.getControl( SUBTITLES_LIST ).addItem( listitem )

#            elif msg:
#                self.getControl( STATUS_LABEL ).setLabel( msg )

            self.setFocus( self.getControl( SUBTITLES_LIST ) )
            self.getControl( SUBTITLES_LIST ).selectItem( 0 )

        except Exception, e:
            error = _( 634 ) % ( "search_subtitles:" + str ( e ) ) 
            LOG( LOG_ERROR, error )
            return False, error
        

    def show_control( self, controlId ):
        self.getControl( STATUS_LABEL ).setVisible( controlId == STATUS_LABEL )
        self.getControl( SUBTITLES_LIST ).setVisible( controlId == SUBTITLES_LIST )
        page_control = ( controlId == STATUS_LABEL )
        try: self.setFocus( self.getControl( controlId + page_control ) )
        except: self.setFocus( self.getControl( controlId ) )


    def file_download(self, url, dest):
        dp = xbmcgui.DialogProgress()
        dp.create( __scriptname__, _( 633 ), os.path.basename( dest ) )
        try:
            urllib.urlretrieve( url, dest, lambda nb, bs, fs, url=url: self._pbhook( nb, bs, fs, url, dp ) )
            return True, "Downloaded"
        except Exception, e:
            error = _( 634 ) % ( str ( e ) ) 
            LOG( LOG_ERROR, error )
            return False, error

    def _pbhook(self, numblocks, blocksize, filesize, url=None, dp=None):
        try:
            percent = min( ( numblocks*blocksize*100 ) / filesize, 100 )
            LOG( LOG_INFO, "download precent %s" % ( precent, ) )
            dp.update(percent)
        except:
            percent = 100
            dp.update( percent )
        if dp.iscanceled(): 
            LOG( LOG_INFO, "Subtitle download cancelled" )  
            dp.close()
            

    def download_subtitles(self, pos):
        LOG( LOG_INFO, "download_subtitles" )
        if self.osdb_server.subtitles_list:
            subtitle_set = False

            filename = self.osdb_server.subtitles_list[pos]["filename"]
            subtitle_format = self.osdb_server.subtitles_list[pos]["format"]
            url = self.osdb_server.subtitles_list[pos]["link"]

            zip_filename = filename[0:filename.rfind(".")] + ".zip"
            zip_filename = xbmc.translatePath( os.path.join( "special://home/subtitles", zip_filename ) )
            sub_filename = os.path.basename( self.file_path )
            sub_filename = sub_filename[0:sub_filename.rfind(".")] + "." + self.osdb_server.subtitles_list[pos]["language_id"] + "." + self.osdb_server.subtitles_list[pos]["format"]
            
            local_path = xbmc.translatePath("special://home/subtitles")
    
            ok = xbmcgui.Dialog().yesno( __scriptname__, _( 242 ), ( _( 243 ) % ( filename, ) ), "", _( 260 ), _( 259 ) )
            if not ok:
                self.getControl( STATUS_LABEL ).setLabel( _( 645 ) )
                return
            
            self.getControl( STATUS_LABEL ).setLabel( _( 649 ) )
            LOG( LOG_INFO, _( 632 ) % ( zip_filename, url, ) )

            ok, msg = self.file_download( url, zip_filename )
            if not ok:
                self.getControl( STATUS_LABEL ).setLabel( msg )
                return
            else:
                ok = self.extract_subtitles( filename, sub_filename, subtitle_format, zip_filename, local_path )

    def extract_subtitles(self, filename, sub_filename, subtitle_format, zip_filename, local_path ):
        LOG( LOG_INFO, "extract_subtitles" )
        subtitle_set = False 
        try:
            un = unzip.unzip()
            #if os.path.exists( zip_filename ):
                #return

            files = un.get_file_list( zip_filename )

            if ( len( files ) == 2 ):
                if not ( filename in files ):
                    for item in files:
                        if ( item.find( subtitle_format ) > 0 ):
                            filename = item
                            break
                self.getControl( STATUS_LABEL ).setLabel( _( 650 ) )
                LOG( LOG_INFO, _( 631 ) % ( zip_filename, local_path ) )
                un.extract_file( zip_filename, filename, sub_filename, local_path )
                LOG( LOG_INFO, _( 644 ) % ( local_path ) )
                self.getControl( STATUS_LABEL ).setLabel( _( 651 ) )
                if xbmc.Player().isPlayingVideo():
                    xbmc.Player().setSubtitles( os.path.join( local_path, sub_filename ) )
                    subtitle_set = True
            else:
                self.getControl( STATUS_LABEL ).setLabel( _( 650 ) )
                LOG( LOG_INFO, _( 631 ) % ( zip_filename, local_path ) )
                un.extract( zip_filename, local_path )
                LOG( LOG_INFO, _( 644 ) % ( local_path ) )
                self.getControl( STATUS_LABEL ).setLabel( _( 651 ) )
                if xbmc.Player().isPlayingVideo():
                    for item in files:
                        if ( item.find( subtitle_format ) > 0 ):
                            xbmc.Player().setSubtitles( os.path.join( local_path, item ) )
                            subtitle_set = True
        except Exception, e:
            error = _( 634 ) % ( str ( e ) )
            LOG( LOG_ERROR, error )
            
        self.getControl( STATUS_LABEL ).setLabel( _( 652 ) )
        if subtitle_set:
            os.remove( zip_filename )
            xbmc.showNotification( 652, '', '' )
            self.exit_script()


    def exit_script( self, restart=False ):
        self.connThread.join()
        self.close()

    def onClick( self, controlId ):
        if ( self.controlId == SUBTITLES_LIST ):
            self.download_subtitles( self.getControl( SUBTITLES_LIST ).getSelectedPosition() )

    def onFocus( self, controlId ):
        self.controlId = controlId

    def onAction( self, action ):
        try:
                if ( action.getButtonCode() in CANCEL_DIALOG ):
                    self.exit_script()
        except:
                self.exit_script()
