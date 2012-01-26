import sys
import os
import xbmcgui
import xbmc
import string

__scriptname__ = "RTorrent"
__author__ = "Idan"
__url__ = ""
__svn_url__ = ""
__credits__ = "Idan"
__version__ = "1.0"

BASE_RESOURCE_PATH = os.path.join( os.getcwd().replace( ";", "" ), "resources" )
sys.path.append( os.path.join( BASE_RESOURCE_PATH, "lib" ) )
import language
__language__ = language.Language().localized

if ( __name__ == "__main__" ):

	import gui
	window = "main"
	ui = gui.GUI( "script-%s-%s.xml" % ( __scriptname__.replace( " ", "_" ), window, ), os.getcwd(), "Boxee")
	ui.doModal()
	ui.cleanup()
	del ui
