"""
Scraper for http://www.lyricwiki.org

Nuka1195
"""

import sys
import os
import urllib

__title__ = "LyricWiki.org"
__allow_exceptions__ = True

class LyricsFetcher:
    """ required: Fetcher Class for www.lyricwiki.org """
    def __init__( self ):
        self.base_url = "http://lyricwiki.org/api.php?fmt=text"
        self._set_exceptions()
        
    def get_lyrics( self, artist, song ):
        """ *required: Returns song lyrics or a list of choices from artist & song """
	url = self.base_url + "&artist=%s&song=%s"
        artist = self._format_param( artist )
        song = self._format_param( song, False )
	print url % ( artist, song, ) 
	lyrics = self._fetch_lyrics( url % ( artist, song, ) )
	# if no lyrics found try just artist for a list of songs
        if ( not lyrics ):
		song_list = self._get_song_list( artist )
		return song_list
        else: return self._clean_text( lyrics )
    
    def get_lyrics_from_list( self, item ):
        """ *required: Returns song lyrics from user selection - item[1]"""
	url = self.base_url + "&artist=%s&song=%s"
	artist = self._format_param( item.split( ":" )[0] )
        song = self._format_param( item.split( ":" )[1], False )
	lyrics = self._fetch_lyrics( url % ( artist, song, ) )
	return self._clean_text( lyrics )
        
    def _set_exceptions( self, exception=None ):
        """ Sets exceptions for formatting artist """
        try:
            if ( __name__ == "__main__" ):
                ex_path = os.path.join( os.getcwd().replace( ";", "" ), "exceptions.txt" )
            else:
                ex_path = os.path.join( "special://home/script_data", sys.modules[ "__main__" ].__scriptname__, "scrapers", os.path.split( os.path.dirname( sys.modules[ "lyricsScraper" ].__file__ ) )[ 1 ], "exceptions.txt" )
            ex_file = open( ex_path, "r" )
            self.exceptions = eval( ex_file.read() )
            ex_file.close()
        except:
            self.exceptions = {}
        if ( exception is not None ):
            self.exceptions[ exception[ 0 ] ] = exception[ 1 ]
            self._save_exception_file( ex_path, self.exceptions )

    def _save_exception_file( self, ex_path, exceptions ):
        """ Saves the exception file as a repr(dict) """
        try:
            if ( not os.path.isdir( os.path.split( ex_path )[ 0 ] ) ):
                os.makedirs( os.path.split( ex_path )[ 0 ] )
            ex_file = open( ex_path, "w" )
            ex_file.write( repr( exceptions ) )
            ex_file.close()
        except: pass
        
    def _fetch_lyrics( self, url ):
        """ Fetch lyrics if available """
        try:
            # Open url or local file (if debug)
            if ( not debug ):
                usock = urllib.urlopen( url )
            else:
                usock = open( os.path.join( os.getcwd().replace( ";", "" ), "lyrics_source.txt" ), "r" )
            htmlSource = usock.read()
            usock.close()
            # Save htmlSource to a file for testing scraper (if debugWrite)
            if ( debugWrite ):
                file_object = open( os.path.join( os.getcwd().replace( ";", "" ), "lyrics_source.txt" ), "w" )
                file_object.write( htmlSource )
                file_object.close()
            return htmlSource
        except:
            return None
        
    def _get_song_list( self, artist ):
        """ If no lyrics found, fetch a list of choices """
        try:
            url = self.base_url + "&artist=%s"
            # Open url or local file (if debug)
            if ( not debug ):
                usock = urllib.urlopen( url % ( artist, ) )
            else:
                usock = open( os.path.join( os.getcwd().replace( ";", "" ), "songs_source.txt" ), "r" )
            htmlSource = usock.read()
            usock.close()
            # Save htmlSource to a file for testing scraper (if debugWrite)
            if ( debugWrite ):
                file_object = open( os.path.join( os.getcwd().replace( ";", "" ), "songs_source.txt" ), "w" )
                file_object.write( htmlSource )
                file_object.close()
            # Parse htmlSource for song links
            song_list = htmlSource.split( "\n" )
	    # Create sorted return list
            song_list = self._remove_dupes( song_list )
            return song_list
        except:
            return None
            
    def _remove_dupes( self, song_list ):
        """ Returns a sorted list with duplicates removed """
        # this is apparently the fastest method
        dupes = {}
        for x in song_list:
            dupes[ x ] = x
        new_song_list = dupes.values()
        new_song_list.sort()
        return new_song_list

    def _format_param( self, param, exception=True ):
        """ Converts param to the form expected by www.lyricwiki.org """
        caps = True
        result = ""
        # enumerate thru string to properly capitalize words (why doesn't title() do this properly?)
        for letter in param.upper().strip():
            if ( letter == " " ):
                letter = "_"
                caps = True
            elif ( letter in "(_-." ):
                caps = True
            elif ( unicode( letter.isalpha() ) and caps ):
                caps = False
            else:
                letter = letter.lower()
                caps = False
            result += letter
        result = result.replace( "/", "_" )
        # properly quote string for url
        result = urllib.quote( result )
        # replace any exceptions
        if ( exception and result in self.exceptions ):
            result = self.exceptions[ result ]
        return result
    
    def _clean_text( self, text ):
        """ Convert line terminators and html entities """
        try:
            text = text.replace( "\t", "" )
            text = text.replace( "<br> ", "\n" )
            text = text.replace( "<br>", "\n" )
            text = text.replace( "<br /> ", "\n" )
            text = text.replace( "<br />", "\n" )
            text = text.replace( "<div>", "\n" )
            text = text.replace( "> ", "\n" )
            text = text.replace( ">", "\n" )
            text = text.replace( "&amp;", "&" )
            text = text.replace( "&gt;", ">" )
            text = text.replace( "&lt;", "<" )
            text = text.replace( "&quot;", '"' )
        except: 
            pass
        return text

# used for testing only
debug = False
debugWrite = False

if ( __name__ == "__main__" ):
    # used to test get_lyrics() 
    artist = [ "The Charlie Daniels Band", "ABBA", "Jem","Stealers Wheel","Paul McCartney & Wings","ABBA","AC/DC", "Tom Jones", "Kim Mitchell", "Ted Nugent", "Blue Öyster Cult", "The 5th Dimension", "Big & Rich", "Don Felder" ]
    song = [ "(What This World Needs Is) A Few More Rednecks", "S.O.S","24","Stuck in the middle with you","Band on the run", "Dancing Queen", "T.N.T.", "She's A Lady", "Go for Soda", "Free-for-all", "(Don't Fear) The Reaper", "Age of Aquarius", "Save a Horse (Ride a Cowboy)", "Heavy Metal (Takin' a Ride)" ]
    for cnt in range( 1 ):
        lyrics = LyricsFetcher().get_lyrics( artist[ cnt ], song[ cnt ] )
    
    # used to test get_lyrics_from_list() 
    #url = ("Big & Rich:Save a Horse (Ride a Cowboy)", "/Big_%26_Rich:Save_a_Horse_%28Ride_a_Cowboy%29")
    #url = (u"Stuck In The Middle With You", "/Stealers_Wheel:Stuck_In_The_Middle_With_You")
    #lyrics = LyricsFetcher().get_lyrics_from_list( url )
    
    # print the results
    if ( isinstance( lyrics, list ) ):
        for song in lyrics:
            print song
    else:
        print lyrics.encode( "utf-8", "ignore" )
