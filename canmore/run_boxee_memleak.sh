#!/bin/sh

# collect crash logs
if [ -f /opt/local/bin/collect_logs ]; then
    /opt/local/bin/collect_logs
fi

export LD_LIBRARY_PATH=/opt/local/lib:/opt/local/qt-4.7/lib:/opt/local/qt/lib:/lib:/usr/lib:/usr/local/lib:/opt/boxee/system/players/flashplayer:/opt/boxee/system/players/dvdplayer
export MOZ_PLUGIN_PATH=/opt/boxee/system/players/flashplayer/plugins
export WEBKIT_PLUGIN_PATH=/opt/boxee/system/players/flashplayer/plugins
export QWS_DISPLAY=intelce:bgcolor=black
export QWS_KEYBOARD=intelceir:repeatRate=500                                    
export QT_QWS_FONTDIR=/opt/local/qt/lib/fonts
export QT_PLUGIN_PATH=/opt/local/qt-4.7/plugins                                     
export PYTHON_SANDBOX=1
export NRD_IR_KEYDATA=/opt/local/etc/YFI_CecIr_KeyData.xml
cd /opt/boxee
LD_PRELOAD=system/leakdetector.so ./Boxee
