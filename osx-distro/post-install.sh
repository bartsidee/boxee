#!/bin/bash

# $0 is this script's path
# $1 would be the package's path
# $2 would be the install path
# $3 target volume

#make sure helper is down although we did that already in pre-script
/bin/launchctl unload $HOME/Library/LaunchAgents/tv.boxee.helper.plist > /dev/null 2>&1

#boxee will create it on launch
/bin/rm -f $HOME/Library/LaunchAgents/tv.boxee.helper.plist

# remove the keymap file so that new changes will apply
/bin/mv -f ~/Library/Application\ Support/BOXEE/UserData/Keymap.xml ~/Library/Application\ Support/BOXEE/UserData/Keymap.xml.$$ > /dev/null 2>&1
/usr/sbin/chown -f $USER $HOME/Library/Application\ Support/BOXEE/UserData/Keymap.xml.$$

# apply required changes to every profiles sources.xml
if [ -d "$HOME/Library/Application Support/BOXEE" ]
then 
  $2/Boxee.app/Contents/MacOS/Boxee -usf
  /usr/sbin/chown -f -R $USER "$HOME/Library/Application Support/BOXEE"
  /usr/bin/find "$HOME/Library/Application Support/BOXEE" -name ViewModes.db -exec /bin/rm -f {} \;
fi

#make sure owner of the app is the user and not root
/usr/sbin/chown -f -R $USER /Applications/Boxee.app

#fix permissions on Dcp stuff
/usr/sbin/chown -f root:admin /Applications/Boxee.app/Contents/MacOS/DcpMon*
/bin/chmod -f 06755 /Applications/Boxee.app/Contents/MacOS/DcpMon*

$2/Boxee.app/Contents/Resources/Boxee/bin/install_SCR

/bin/rm -rf /tmp/boxee
/bin/rm -f skin/boxee/Fonts/H*.ttf
/bin/rm -f skin/boxee/Fonts/My*.otf
/bin/rm -f skin/boxee/Fonts/boxee.ttf
/bin/rm -f system/players/dvdplayer/libdvdcss-x86-osx.so
/bin/rm -f bin/rtorrent
/bin/rm -f bin/screen
/bin/rm /Applications/Boxee.app/Contents/Resources/Boxee/boxee_catalog.db

exit 0

