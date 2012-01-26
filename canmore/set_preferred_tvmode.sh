#!/bin/sh

get_native_resolution () {
  export LD_LIBRARY_PATH=/opt/local/lib

  /opt/boxee/get_hdmi_native > /tmp/hdmi_native_resolutions.xml
  if [ $? != "0" ]; then
    return $?
  fi
  
  WIDTH=`/opt/local/bin/xmlstarlet sel -t -v //modes/mode[$1]/width /tmp/hdmi_native_resolutions.xml`
  if [ -z $WIDTH ]; then
    return 1
  fi
       
  HEIGHT=`/opt/local/bin/xmlstarlet sel -t -v //modes/mode[$1]/height /tmp/hdmi_native_resolutions.xml`
  VFREQ=`/opt/local/bin/xmlstarlet sel -t -v //modes/mode[$1]/vfreq /tmp/hdmi_native_resolutions.xml`
  INTERLACED=`/opt/local/bin/xmlstarlet sel -t -v //modes/mode[$1]/interlaced /tmp/hdmi_native_resolutions.xml`
  
  if [ "$INTERLACED" == "true" ]; then
    HEIGHT=`expr $HEIGHT \* 2`
  fi

  MODE=$HEIGHT
  if [ "$INTERLACED" == "true" ]; then
    MODE="${MODE}i"
  else
    MODE="${MODE}p"
  fi
  
  if [ "$VFREQ" == "50" ]; then
    MODE="${MODE}50"
  fi

  rm /tmp/hdmi_native_resolutions.xml

  #echo $WIDTH
  #echo $HEIGHT
  #echo $INTERLACED
  #echo $MODE

  return 0
}

MODE=`/opt/boxee/get_boxee_resolution.sh`
if [ $? == "0" ]; then
  echo "Setting to boxee resolution: $MODE"
  tvmode $MODE
  if [ $? == "0" ]; then
    exit 0
  fi
  echo "tvmode failed, not giving up"
fi

r=1
while :
do
  get_native_resolution $r
  return_val=$?
  
  if [ $return_val == "0" ]; then
    echo "Setting to HDMI native resolution: $MODE"
    tvmode $MODE
    if [ $? == "0" ]; then
      exit 0
    fi
    echo "tvmode failed, not giving up"
  fi
  
  if [ $return_val == "1" ]; then
    echo "End of native resolutions."
    break;
  fi
  
  r=$(($r+1))
done

echo "Fallthrough to 480p"
tvmode 480p

exit 0
