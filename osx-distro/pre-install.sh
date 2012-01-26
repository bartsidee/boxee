#!/bin/sh

#attempt to do the same actions using sudo. it results in deferent launchctl behavior
if [ `id -u` = 0 ]
then
  sudo -S /bin/launchctl unload $HOME/Library/LaunchAgents/tv.boxee.helper.plist < /dev/null
  sudo -S -u $USER /bin/launchctl unload $HOME/Library/LaunchAgents/tv.boxee.helper.plist < /dev/null
  sudo -S /bin/rm -f $HOME/Library/LaunchAgents/tv.boxee.helper.plist < /dev/null

else
  /bin/launchctl unload $HOME/Library/LaunchAgents/tv.boxee.helper.plist > /dev/null 2>&1
  /bin/rm -f $HOME/Library/LaunchAgents/tv.boxee.helper.plist
fi

/bin/rm -rf "/Applications/Boxee.app/Contents/Resources/Boxee/skin"

exit 0
