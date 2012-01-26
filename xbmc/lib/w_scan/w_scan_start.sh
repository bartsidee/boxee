#!/bin/sh
#
# Simple MPEG/DVB parser to achieve network/service information without initial tuning data
#
# Copyright (C) 2006, 2007, 2008 Winfried Koehler 
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
# Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#
# The author can be reached at: handygewinnspiel AT gmx DOT de
#
# The project's page is http://wirbel.htpc-forum.de/w_scan/index2.html
#
#######################################################
# w_scan start script
# 20060812 wirbel (handygewinnspiel AET gmx D*T de)
#
# - 20060812: first version
# - 20060822: many improvements
# cu@vdr-portal
#
#######################################################

umask 022
export PATH="$(pwd):/bin:/usr/bin:/sbin:/usr/sbin"
DATE="$(date +%Y)$(date +%m)$(date +%d)" # $DATE == "YYYYMMDD", i.e. 20060710
W_SCAN=$(which w_scan 2>/dev/null)
UDEVSTART=$(which udevstart)
SCRIPT_VERSION=20110502
W_SCAN_VERSION=$($W_SCAN -V 2>&1)


# Width of the Screen
COLUMNS=$(stty size)
COLUMNS=${COLUMNS##* }
if [ "$COLUMNS" = "0" ]; then
	COLUMNS=80;
fi

# Measurements for positioning result messages
COL=$(($COLUMNS - 12))
WCOL=$(($COLUMNS - 30))

# Set Cursur Position Commands, used via echo -e
SET_COL="\\033[${COL}G"
SET_WCOL="\\033[${WCOL}G"
CURS_UP="\\033[A"

# Set color commands, used via echo -e
NORMAL="\\033[40;39m"
SUCCESS="\\033[40;32m"
WARNING="\\033[40;33m"
FAILURE="\\033[40;31m"

echo_ok()
{
	echo -e "$CURS_UP$SET_COL"["$SUCCESS""    OK    ""$NORMAL"]
}

echo_failure()
{
	echo -e "$CURS_UP$SET_COL"["$FAILURE""  FAILED  ""$NORMAL"]
}

echo_warning()
{
	echo -e "$CURS_UP$SET_WCOL$@$SET_COL"["$WARNING""   WARN   ""$NORMAL"]
}

echo_yes()
{
	echo -e "$CURS_UP$SET_COL"["$SUCCESS""    YES   ""$NORMAL"]
}

echo_no()
{
	echo -e "$CURS_UP$SET_COL"["$SUCCESS""    NO    ""$NORMAL"]
}

echo_loaded()
{
	echo -e "$CURS_UP$SET_COL"["$SUCCESS""  LOADED  ""$NORMAL"]
}

echo_not_loaded()
{
	echo -e "$CURS_UP$SET_COL"["$FAILURE""NOT LOADED""$NORMAL"]
}

print_error_msg()
{
	echo -e -n $FAILURE
	echo "****************************************************************"
	echo "* FAILURE. Something doesn't work here,                        *"
	echo "* last command gives me an error.. :-(                         *"
	echo "*                                                              *"
	echo "* Press Enter to continue..                                    *"
	echo "****************************************************************"
	echo -e -n $NORMAL && tput sgr0
	read ENTER
	echo -e -n $NORMAL
}

CheckBinary()
{
	if [ -d $1 ];	then
		echo -e -n $FAILURE
		echo "Checking binary $1.."
		echo "$1 is a directory, not a binary. Cannot continue - EXITING."
		echo -e -n $NORMAL
		tput sgr0 && clear && exit -1
	fi
	if [ ! -f $1 ];	then
		echo -e -n $FAILURE
		echo "Checking binary $1.."
		echo "$1 is not a valid file or not found. Cannot continue - EXITING."
		echo -e -n $NORMAL
		tput sgr0 && clear && exit -1
	fi
	if [ ! -s $1 ];	then
		echo -e -n $FAILURE
		echo "Checking binary $1 (file size).."
		echo "$1 is not a valid file (zero size!!). Cannot continue - EXITING."
		echo -e -n $NORMAL
		tput sgr0 && clear && exit -1
	fi
	if [ ! -x $1 ]; then
		echo -e -n $FAILURE
		echo "Checking permissions of binary $1 .."
		echo "$1 is not executable. Check permissions! Cannot continue - EXITING."
		echo -e -n $NORMAL
		tput sgr0 && clear && exit -1
	fi
}

CheckVersions()
{
	if [ "$W_SCAN_VERSION" != "$SCRIPT_VERSION" ]; then
		echo -e -n $FAILURE
		echo "Using $W_SCAN"
		echo "w_scan is version $W_SCAN_VERSION."
		echo "This start script is version $SCRIPT_VERSION."
		echo "Both of them _must_ have the same version."
		echo "Cannot continue - EXITING."
		echo -e -n $NORMAL
		tput sgr0 && clear && exit -1
	fi
}

eval_retval()
{
	errval=$?
	if [ $errval = 0 ]; then
		print_status success
	else
		print_status failure
	fi
	return 0
}

print_status()
{
        if [ $# = 0 ]; then
                echo "Usage: $0 {success|warning|failure}"
                return 1
        fi
        case "$1" in
                success)
                        echo_ok
                ;;
                warning)
                        case "$2" in
                                running)
                                        echo_warning "Already running"
                                ;;
                                not_running)
                                        echo_warning "Not running"
                                ;;
                                not_available)
                                        echo_warning "Not available"
                                ;;
                        esac
                ;;
                failure)
                        echo_failure
                ;;
        esac
}

function Greeting ()
{
	clear
	echo -e -n $SUCCESS
	echo "=============================================================================="
	echo -e "THIS IS THE ""\\033[44m"" W_SCAN START SCRIPT ""\\033[40m$SUCCESS"" (see http://wirbel.htpc-forum.de/w_scan/index2.html)"
	echo ""
	echo "It will try the following:"
	echo ""
	echo "   step 1) checks wether vdr is running and if so, try to stop it"
	echo "   step 2) check wether your dvb driver is loaded and if not, load it"
	echo "   step 3) do a channel scan and create a vdr channels.conf"
	echo "   step 4) ask you, wether your vdr should be restarted"
	echo ""
	echo "PRESS ANY KEY TO CONTINUE OR CTRL+C TO EXIT"
	echo "=============================================================================="
	echo -e -n $NORMAL && tput sgr0
	read
	echo -e -n $NORMAL
}



function StopVDR ()
{
  echo "Checking wether Video Disk Recorder software is stopped..."
  if [ "$(ps -A | grep vdr)" != "" ]; then
	echo_no
	echo -e -n $FAILURE
	echo "****************************************************************"
	echo "*                                                              *"
	echo "* W_SCAN CANNOT CONTINUE WHILE VDR IS RUNNING.                 *"
	echo "* Should I try to stop vdr for you? [y/n]                      *"
	echo "*                                                              *"
	echo "****************************************************************"
	echo -e -n $NORMAL && tput sgr0
	read RESPONSE
	echo -e -n $NORMAL
	if [ "$RESPONSE" != "y" ]; then
		echo "OKAY. Exiting w_scan start script."
		sleep 2
		tput sgr0 && clear && exit -1
        else
		clear
		stopped=false
		echo "Trying to stop vdr..."
		echo "Checking for ct-vdr style startscript.."
		if [ -e /etc/init.d/vdr ]; then			# c't-vdr
			echo_yes
			$(/etc/init.d/vdr stop)
			stopped=true
		else
			echo_no
		fi
		if [ $stopped == false ]; then
			echo "Checking for linvdr style startscript.."
			if [ -e /etc/init.d/runvdr ]; then		# linvdr
				echo_yes
				$(/etc/init.d/runvdr stop)
				$(/etc/init.d/runvdr loaddriver)
			else
				echo_no
			fi
		fi
		if [ $stopped == false ]; then
			echo "Checking for runvdr in /usr/local/bin.."
			if [ -e /usr/local/bin/runvdr ]; then		# -|-
				echo_yes
				echo "   Checking wether /usr/local/bin/runvdr knows 'stop'..."
				VAR="$(cat /usr/local/bin/runvdr | grep 'stop)')"
				if [ "$VAR" != "" ]; then
					echo_yes
					$(/usr/local/bin/runvdr stop)
				else
					echo_no
				fi
			else
				echo_no
			fi
		fi
		if [ $stopped == false ]; then
			echo "Checking for runvdr in /usr/bin.."
			if [ -e /usr/bin/runvdr ]; then			# -|-
				echo_yes
				echo "   Checking wether this /usr/bin/runvdr knows 'stop'..."
				VAR="$(cat /usr/bin/vdr | grep 'stop)')"
				if [ "$VAR" != "" ]; then
					echo_yes
					$(/usr/bin/runvdr stop)
				else
					echo_no
				fi
			else
				echo_no
			fi
		fi
		if [ $stopped == false ]; then
			if [ "$(ps -A | grep vdr)" != "" ]; then 	# last Chance, but unusual system anyway..
				echo "No stop script found - using killall.."
				KILLALL=$(which killall)
				$($KILLALL -9 vdr 2>&1 > /dev/null)
				$($KILLALL -9 runvdr 2>&1 > /dev/null)
				sleep 6
				if [ "$(ps -A | grep vdr)" != "" ]; then 	# we should re-check here
			    		echo_failure
			    		echo "VDR is running and cannot be stopped - giving up."
			    		tput sgr0 && clear && exit -1
					else
			    		echo_ok
				fi			
			fi
		fi
	fi
  else
  	echo_ok
  fi
}


function StartVDR ()
{
    echo -e -n $SUCCESS
    echo "****************************************************************"
    echo "*                                                              *"
    echo "* All is done now.                                             *"
    echo "* Should I try to restart vdr for you? [y/n]                   *"
    echo "*                                                              *"
    echo "****************************************************************"
    echo -e -n $NORMAL && tput sgr0
    read RESPONSE
    echo -e -n $NORMAL
	if [ "$RESPONSE" != "y" ]; then
		echo -n -e "\\033[1;34m"
		echo ""
		echo "Okay, restarting vdr skipped."
		echo "GOODBYE! Exiting now."
		echo ""
		echo -e -n $NORMAL
		tput sgr0 && clear && exit 0
        else
		started=false
		if [ -e /etc/init.d/vdr ]; then			# c't-vdr
			/etc/init.d/vdr start
			started=true
		fi
		if [ $started == false ]; then
			if [ -e /etc/init.d/runvdr ]; then		# linvdr
				/etc/init.d/runvdr start
				started=true	
			fi
		fi
		if [ $started == false ]; then
			if [ -e /usr/local/bin/runvdr ]; then		# -|-
				echo  "   Checking wether /usr/local/bin/runvdr knows 'start'.."
				VAR="$(cat /usr/local/bin/runvdr | grep 'start)')"
				if [ "$VAR" != "" ]; then
					echo_yes
					/usr/local/bin/runvdr start
					started=true
				else
					echo_no
				fi
			fi
		fi
		if [ $started == true ]; then
			if [ -e /usr/bin/runvdr ]; then			# -|-
				echo "   Checking wether this /usr/bin/runvdr knows 'start'..."
				VAR="$(cat /usr/bin/runvdr | grep 'start)')"
				if [ "$VAR" != "" ]; then
					echo_yes
					/usr/bin/runvdr start
					started=true
				else
					echo_no
				fi

			fi
		fi
		echo -n -e "\\033[1;34m"
		echo ""
		echo "On most systems vdr should run now - if not, reboot."
		echo "GOODBYE! Exiting now."
		echo ""
		echo -e -n $NORMAL
	fi
}


function LoadDriver ()
{
	# debug messages to stdout [true/false]
	DEBUG_LOADDRIVER=false

	# <SPACE> separated lists 

	# I don't actually know which generic dvb Modules are *really* needed here
	# may be i should add some more here? 

	# which generic dvb modules should be always loaded?
	MODULES_TO_LOAD="videodev v4l1_compat \
			v4l2_common video_buf dvb_core ves1820 \
			dvb_ttpci \
			budget_ci budget_av budget_ci budget "

	# module ves1820 doesnt state that it needs dvb-ttpci (hg from 08/2006), therefore
	# my algorithm will fail and depmod will not add the dependency. :-(
	# may be more modules?

	# which modules should *not* be loaded?
	# i.e. ivtv as it will be found by the automatic algorithm below..
	MODULES_BLACKLISTED="ivtv"


	#------------------------------------------ usb device search -----------------
	if	[ -e /sys/bus/usb/devices ] &&
		[ -e /lib/modules/$(uname -r)/modules.usbmap ]; then
	for i in /sys/bus/usb/devices/*; do
	  if [ ! -e $i/idVendor ]; then	# no Vendor ID given, ignore this item
	    continue
	  fi
	  read bDeviceSubClass < $i/bDeviceSubClass
	  read bDeviceClass < $i/bDeviceClass
	  class=$bDeviceClass$bDeviceSubClass
	  # accept only USB Ethernet Networking, USB Video, USB Miscellaneous Device Cable Based
	  # and USB Vendor Specific Class/Subclass
	  if 	[ "$class" != "0206" ] &&
		[ "$bDeviceClass" != "0e" ] &&
		[ "$class" != "ef03" ] &&
		[ "$class" != "ffff" ]; then
	    continue
	  fi
	  class_name="unknown"
	  vendor_name="unknown"
	  device_name="unknown"
	  subvendor_name="unknown"
	  driver=""
	  read idVendor < $i/idVendor
	  read idProduct < $i/idProduct
	  # avoid errors if ressource not available
	  if	[ ! -e /usr/share/w_scan/usb.ids ] ||
		[ ! -e /usr/share/w_scan/usb.classes ]; then
	    DEBUG_LOADDRIVER=false
	  fi
	  if [ $DEBUG_LOADDRIVER == true ]; then
		while read line; do
		  if [ "${line:0:4}" == "$class" ]; then
		    class_name=${line:6:40}
		    break
		  fi
		done < /usr/share/w_scan/usb.classes
		while read line; do
		  if [ "${line:0:4}" == "$idVendor" ]; then
		    vendor_name=${line:6:80}
		    while read line; do
		      if [ "${line:0:6}" == "||$idProduct" ]; then
		        device_name=${line:8:80}
		      fi
		      if [ "${line:0:2}" != "||" ]; then # next pci id
		        break
		      fi 
		    done		
		    break
		  fi
		done < /usr/share/w_scan/usb.ids
	  fi
	  while read line; do
	    data=$line
	    for id in "usb module" match_flags idVendor idProduct bcdDevice_lo bcdDevice_hi \
	               bDeviceClass bDeviceSubClass bDeviceProtocol bInterfaceClass \
	               bInterfaceSubClass bInterfaceProtocol driver_info; do
	      if [ "$id" == "usb module" ]; then
	        d=${data:0:21}
	      fi 
	      if [ "$id" == "idVendor" ]; then
	        if [ "${data:35:4}" != "$idVendor" ]; then
	          break
	        fi
	      fi
	      if [ "$id" == "idProduct" ]; then
	        if [ "${data:44:4}" != "$idProduct" ] && [ "${data:45:4}" != "ffff" ]; then
	          break
	        fi
	        driver=$d
	      fi
	    if [ "$driver" != "" ]; then
	      break
	    fi   
	    done        
	  done < /lib/modules/$(uname -r)/modules.usbmap
	  MODULES_TO_LOAD="$MODULES_TO_LOAD$driver "
	  if [ $DEBUG_LOADDRIVER == true ]; then
	    echo "----------------------------------------------------------------------------"
	    echo $i
	    echo "vendor:		$idVendor	($vendor_name)"
	    echo "product:	$idProduct	($device_name)"
	    echo "class:		$class	($class_name)"
	    echo "module:		$driver"
	  fi
	done
	else
		# fallback solution :-(
		# which usb modules should be loaded if auto detection fails?
		echo "WARNING: using stupid fallback solution for usb devices."

		MODULES_TO_LOAD="$MODULES_TO_LOADcinergyT2 ttusb_dec \
				dvb-ttusb-budget dvb-usb-vp7045 dvb-usb-umt-010 \
				dvb-usb-nova-t-usb2 dvb-usb-dtt200u dvb-usb-digitv \
				dvb-usb-dibusb-mc dvb-usb-cxusb dvb-usb-a800 "
	fi
	#-------------------------------------- end usb device search -----------------
	#------------------------------------------ pci device search -----------------
	if	[ -e /sys/bus/pci/devices ] &&
		[ -e /lib/modules/$(uname -r)/modules.pcimap ]; then
	for i in /sys/bus/pci/devices/*; do
	  read class < $i/class
	  # accept only PCI Multimedia video controller, PCI Multimedia controller (not specified) and
	  # PCI Network controller (not specified)
	  if 	[ "${class:2:4}" != "0480" ] &&
		[ "${class:2:4}" != "0400" ] &&
		[ "${class:2:4}" != "0280" ]; then
	    continue
	  fi
	  class_name="unknown"
	  vendor_name="unknown"
	  device_name="unknown"
	  subvendor_name="unknown"
	  driver=""
	  read vendor < $i/vendor
	  read device < $i/device
	  read subvendor < $i/subsystem_vendor
	  read subdevice < $i/subsystem_device
	  # avoid errors if ressource not available
	  if	[ ! -e /usr/share/w_scan/pci.ids ] ||
		[ ! -e /usr/share/w_scan/pci.classes ]; then
	    DEBUG_LOADDRIVER=false
	  fi
	  if [ $DEBUG_LOADDRIVER == true ]; then
		while read line; do
		  if [ "${line:0:4}" == "${class:2:4}" ]; then
		  class_name=${line:5:40}
		  break
		  fi
		done < /usr/share/w_scan/pci.classes
		while read line; do
		  if [ "${line:0:4}" == "${vendor:2:4}" ]; then
		  vendor_name=${line:5:80}
		  while read line; do
		    if [ "${line:0:6}" == "||${device:2:4}" ]; then
		      device_name=${line:7:80}
		    fi
		    if [ "${line:0:2}" != "||" ]; then # next pci id
		      break
		    fi 
		  done		
		  break
		  fi
		done < /usr/share/w_scan/pci.ids
		while read line; do
		  if [ "${line:0:4}" == "${subvendor:2:4}" ]; then
		    subvendor_name=${line:5:80}
		    break
		  fi
		done < /usr/share/w_scan/pci.ids
	  fi
	  while read line; do
	    data=$line
	    for id in "pci module" vendor device subvendor subdevice class class_mask driver_data; do
	      if [ "$id" == "pci module" ]; then
	        d=${data:0:21}
	      fi 
	      if [ "$id" == "vendor" ]; then
	        if [ "${data:27:4}" != "${vendor:2:4}" ]; then
	          break
	        fi
	      fi
	      if [ "$id" == "device" ]; then
	        if [ "${data:38:4}" != "${device:2:4}" ] && [ "${data:38:4}" != "ffff" ]; then
	          break
	        fi
	      fi
	      if [ "$id" == "subvendor" ]; then
	        if [ "${data:49:4}" != "${subvendor:2:4}" ] && [ "${data:49:4}" != "ffff" ]; then
	          break
	        fi
	      fi
	      if [ "$id" == "subdevice" ]; then
	        if [ "${data:60:4}" != "${subdevice:2:4}" ] && [ "${data:60:4}" != "ffff" ]; then
	          break
	        fi
	        driver=$d
	      fi
	    if [ "$driver" != "" ]; then
	      break
	    fi   
	    done        
	  done < /lib/modules/$(uname -r)/modules.pcimap
	  MODULES_TO_LOAD="$MODULES_TO_LOAD$driver "
	  if [ $DEBUG_LOADDRIVER == true ]; then
	    echo "----------------------------------------------------------------------------"
	    echo $i
	    echo "vendor:		$vendor	($vendor_name)"
	    echo "device:		$device	($device_name)"
	    echo "subvendor: 	$subvendor	($subvendor_name)"
	    echo "subdevice:	$subdevice"
	    echo "class:		${class:0:6}	($class_name)"
	    echo "module:		$driver"
	  fi
	done
	else
		# fallback solution :-(
		# which pci modules should be loaded if auto detection fails?

		echo "WARNING: using stupid fallback solution for pci devices."
		MODULES_TO_LOAD="$MODULES_TO_LOADb2c2-flexcop-pci \
				tda1004x ves1x93 ves1820 cx24110 \
				mt352 saa7146 saa7146_vv "
	fi
	#-------------------------------------- end pci device search -----------------
	#-------------------------------------- loading kernel modules ----------------
	for m in $MODULES_TO_LOAD; do
	  blacklisted=false
	  for n in $MODULES_BLACKLISTED; do
	    if [ "$n" == "$m" ]; then
	      blacklisted=true
	      break
	    fi
	  done
	  if [ $blacklisted == false ]; then
		PATTERN=$(echo $m | sed 's/-/_/g')
		MODULES=$(lsmod | grep $PATTERN)
		echo "Checking for kernel module $m.."
		module_loaded=false
		for ml in $MODULES; do
		  if [ "$PATTERN" == "$ml" ]; then
		    module_loaded=true
		    break
		  fi
		done  
		if [ $module_loaded == false ]; then
			echo_not_loaded
			echo "	Loading module $m..."
			modprobe $m
			eval_retval
			if [ -f $UDEVSTART ]; then
				$UDEVSTART
			fi
		else
		    echo_loaded
		fi
	  fi
	done
	sleep 3 # give udev time to create all device nodes
	#---------------------------------- end loading kernel modules ----------------
}

function DoScan ()
{ 
	if [ -e channels.conf-$DATE ]; then
		rm -f channels.conf-$DATE
	fi
	touch channels.conf-$DATE
	WHAT_TO_SCAN=$($W_SCAN -f? 2>&1)
	echo "Checking for DVB-T..."
	if [ ${WHAT_TO_SCAN:0:2} != 0 ]; then
		echo_yes
	else
		echo_no
	fi
	echo "Checking for DVB-C..."
	if [ ${WHAT_TO_SCAN:3:2} != 0 ]; then
		echo_yes
	else
		echo_no
	fi
	echo -e -n $SUCCESS
	echo "****************************************************************"
	echo "*                                                              *"
	echo "* Channel scan will take some minutes - have a cup of coffee.. *"
	echo "* PRESS ANY KEY TO CONTINUE OR CTRL+C TO EXIT.                 *"
	echo "*                                                              *"
	echo "****************************************************************"
	echo -e -n $NORMAL && tput sgr0
	read
	echo -e -n $NORMAL
	echo -e -n "\\033[1;34m"
	if [ ${WHAT_TO_SCAN:0:2} != 0 ]; then
		$W_SCAN >> channels.conf-$DATE
	fi
	if [ ${WHAT_TO_SCAN:3:2} != 0 ]; then 
		$W_SCAN -fc >> channels.conf-$DATE
	fi 
	echo -e -n $NORMAL
}

function CopyConf ()
{
	if [ -s channels.conf-$DATE ]; then
		clear
		echo -e -n $SUCCESS
		echo "****************************************************************"
		echo "*                                                              *"
		echo "* w_scan is done.                                              *"
		echo -e "* The output file is called "$FAILURE"channels.conf-$DATE"$SUCCESS".            *"
		echo "* Should I copy this to default location for you? [y/n]        *"
		echo "*                                                              *"
		echo "****************************************************************"
		echo -e -n $NORMAL && tput sgr0
		read RESPONSE
		echo -e -n $NORMAL
		if [ "$RESPONSE" != "y" ]; then
			echo "Copying channels.conf skipped - okay."
			echo "The channels.conf generated can be found at"
			echo -n -e "\\033[1;34m"
			echo "$(pwd)/channels.conf-$DATE"
			echo ""
			echo -e -n $NORMAL
		else
			if [ -d /etc/vdr ]; then
				mv /etc/vdr/channels.conf /etc/vdr/channels.conf-backup$DATE
				cp channels.conf-$DATE /etc/vdr/channels.conf
			fi
			if [ -d /var/lib/vdr ]; then
				mv /var/lib/vdr/channels.conf /var/lib/vdr/channels.conf-backup$DATE
				cp channels.conf-$DATE /var/lib/vdr/channels.conf
			fi			
		fi
        else
		clear
		echo -e -n $FAILURE
		echo "****************************************************************"
		echo "*                                                              *"
		echo "* Something went wrong. The channels.conf generated is *EMPTY*!*"
		echo "* To analyze your problem I recommend the command              *"
		echo "*    './w_scan 2>&1 | tee w_scan.log-dvbt &&                   *"
		echo "*     ./w_scan -fc 2>&1 | tee w_scan.log-dvbc'                 *"
		echo "*                                                              *"
		echo "* w_scan will report the problem into the logfiles             *"
		echo "* w_scan.log-dvbt and w_scan.log-dvbc.                         *"
		echo "*                                                              *"
		echo "* PRESS ANY KEY TO CONTINUE                                    *"
		echo "****************************************************************"
		echo -e -n $NORMAL
		read
	fi

}

# the program itself.
# init global vars here (if not avoidable..)
# and start functions
# --wirbel

echo -e -n $NORMAL
CheckBinary "$W_SCAN"
CheckVersions
Greeting
StopVDR
LoadDriver
DoScan
CopyConf
StartVDR


