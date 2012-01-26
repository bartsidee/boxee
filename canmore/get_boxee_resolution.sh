#!/bin/sh
export LD_LIBRARY_PATH=/opt/local/lib

# make sure that guisettings.xml exists
if [ ! -f /data/.boxee/UserData/guisettings.xml ]; then
  exit 1
fi

RES=`/opt/local/bin/xmlstarlet sel -t -v //settings/videoscreen/resolution  /data/.boxee/UserData/guisettings.xml`
if [ ! "$?" == "0" ]; then
  exit 1
fi

if [ "$RES" == "" ]; then
  exit 1
fi

RES=`expr $RES - 10`
RES=`/opt/local/bin/xmlstarlet sel -t -v "//settings/resolutions/resolution[$RES]/description"  /data/.boxee/UserData/guisettings.xml`

case $RES in
  PAL) 
    echo 720x576i50; exit 0 
    ;;
  576p) 
    echo 720x576p50; exit 0 
    ;;
  NTSC)
    echo 720x480i59.94; exit 0 
    ;;
  480p)
    echo 720x480p59.94; exit 0 
    ;;
  720p)
    echo 1280x720p59.94; exit 0 
    ;;
  "720p 50Hz")
    echo 1280x720p50; exit 0 
    ;;
  1080p)
    echo 1920x1080p59.94; exit 0 
    ;;
  "1080p 50Hz")
    echo 1920x1080p50; exit 0 
    ;;
  1080i)
    echo 1920x1080i59.94; exit 0 
    ;;
  "1080i 50Hz")
    echo 1920x1080i50; exit 0 
    ;;
esac
