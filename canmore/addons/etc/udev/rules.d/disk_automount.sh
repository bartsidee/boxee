#!/bin/sh -x

# Uncomment the following line to turn on debugging.MOUNTTO
#DEBUG=yes; export DEBUG

source /etc/profile

if [ -x /usr/bin/logger ]; then
    LOGGER=/usr/bin/logger
elif [ -x /bin/logger ]; then
    LOGGER=/bin/logger
fi

mesg () 
{
	$LOGGER -t $(basename $0)"[$$]" "$@"
}

debug_mesg () 
{
#	echo $@
	test "$DEBUG" = "" -o "$DEBUG" = no && return
	mesg "SEQNUM:$SEQNUM $@"
}

do_cleanup ()
{
	# Cleanup any entries in /tmp/mnt which are not in /proc/partitions
	for f in /tmp/mnt/* ; do
		debug_mesg "Checking mount point $f"
		if [ "$f" != "/tmp/mnt/*" ] ; then
			MOUNTED_DEVICE=`grep "$f " /proc/mounts | cut -d " " -f 1`
			FS_TYPE=`grep "$f " /proc/mounts | cut -d " " -f 3`
			if [ -n "$MOUNTED_DEVICE" -a $FS_TYPE != "nfs" ] ; then
				debug_mesg "checking if device $MOUNTED_DEVICE is still attached"
				MOUNTED_DEVICE_SUFFIX=`basename $MOUNTED_DEVICE`
				DEVICE_EXISTS=`grep -c $MOUNTED_DEVICE_SUFFIX /proc/partitions`
				if [ $DEVICE_EXISTS = "0" ] ; then
					debug_mesg "unmounting $MOUNTED_DEVICE since it's not in /proc/partitions"
					umount $MOUNTED_DEVICE
				fi
			fi	
		fi
	done
	for f in /tmp/mnt/* ; do
		debug_mesg "Checking mount point $f"
		if [ "$f" != "/tmp/mnt/*" ] ; then
			MOUNT=`grep "$f " /proc/mounts | cut -d " " -f 2`
			if [ -z "$MOUNT" ] ; then
				debug_mesg "Removing unused mount point: $f"
				/bin/rmdir "$f"
			fi	
		fi
	done
	# Cleanup any entries in /media/* which point to a stale directory
	for f in /media/* ; do
		if [ "$f" != "/media/*" ] ; then
			MOUNT=`readlink "$f"`
			if [ ! -e "$MOUNT" ] ; then
				debug_mesg "Removing unused symbolic link: $f"
				/bin/rm "$f"
			fi	
		fi
	done
}

debug_mesg "arguments ($*) env (`env`)"

if [ "$ACTION" == "add" ] ; then
	# Check if this is a removable partition
	do_cleanup

	REMOVABLE="/sys$DEVPATH/removable"
	if [ `cat $REMOVABLE` -eq 0 ] ; then
		debug_mesg "The flag in the file $REMOVABLE is 0.  It is most likely not a mountable partition.  Exiting."
		exit 1
	else
		debug_mesg "The flag in the file $REMOVABLE is 1.  It is most likely a mountable partition."
	fi

	eval `/opt/local/sbin/blkid -o udev -p ${DEVNAME}`

	if [ -z "$ID_FS_TYPE" ]; then
		debug_mesg "Empty filesystem type. Bailing out."
		exit 0
	fi

	if [ -z "$ID_FS_UUID" ]; then
		debug_mesg "Empty UUID. Bailing out."
		exit 0
	fi
      
	# Look for a mount point
	MOUNTTO="/tmp/mnt/$ID_FS_UUID"
	/bin/mkdir -p "${MOUNTTO}"
	debug_mesg "Mounting device: $DEVNAME with label $ID_FS_UUID to $MOUNTTO($ID_FS_TYPE)"
 	
	if [ -z "$ID_FS_LABEL" ]; then
		ID_FS_LABEL="Unnamed"
	fi

	LABEL=`/opt/local/sbin/blkid -o value -s LABEL -p ${DEVNAME}`
	if [ -z "$LABEL" ]; then
		LABEL="Unnamed"
	fi
    
	debug_mesg "Create softlink for device: $DEVNAME with label $ID_FS_LABEL ($ID_FS_TYPE)"


        LINKTO="/media/$LABEL"
  	FOUND=0
	COUNT=0
	ORIG_LINKTO=$LINKTO
	while [ $FOUND -eq 0 ]; do
		if [ ! -e "${LINKTO}" ]; then
			/bin/ln -sf "${MOUNTTO}" "${LINKTO}"
			echo "Linking to ${LINKTO}"
			FOUND=1
		else
			let COUNT++
			LINKTO="${ORIG_LINKTO} (${COUNT})"
		fi
	done

	MOUNTRC=0

	# Perform the mount
	case "$ID_FS_TYPE" in
		vfat) mount -t vfat $DEVNAME "${MOUNTTO}" -o flush,uid=65534,utf8 || MOUNTRC=$?
		;;
 		ntfs) mount -t auto $DEVNAME "${MOUNTTO}" -o force,flush,uid=65534,locale=en_US.UTF-8 || MOUNTRC=$?  
		;;
		ext2) mount -t auto $DEVNAME "${MOUNTTO}" -o check=none,nosuid,noexec,nodev,errors=continue || MOUNTRC=$?
		;;
		ext3|ext4) mount -t auto $DEVNAME "${MOUNTTO}" -o nosuid,noexec,nodev || MOUNTRC=$?
		;;
		hfs) /opt/local/sbin/fsck.hfs -y $DEVNAME
                     mount -t auto $DEVNAME "${MOUNTTO}" -o nosuid,noexec,nodev || MOUNTRC=$?
		;;
		hfsplus) /opt/local/sbin/fsck.hfsplus -y $DEVNAME
                         mount -t auto $DEVNAME "${MOUNTTO}" -o nosuid,noexec,nodev || MOUNTRC=$?
		;;
	esac

	if [ $MOUNTRC -ne 0 ]; then
		debug_mesg "failed to mount device $DEVNAME, removing excess garbage and bailing out"
		rm "${LINKTO}"
		rmdir "${MOUNTTO}"
		exit 0
	fi

	# Inform HAL
	/opt/local/bin/curl "http://127.0.0.1:5700/storage.OnMount?path=${MOUNTTO}"

elif [ "$ACTION" == "remove" ] ; then

    	if [ -z "$DEVNAME" ]; then
       		echo "Got unmount request without device name. Bailing out."
       		exit 1
    	fi

	MOUNT=`grep "$DEVNAME " /proc/mounts | cut -d " " -f 2`
	UMOUNT_SUCCEEDED=1
	if [ ! -z "$MOUNT" ] ; then
		debug_mesg "Unmounting device: $DEVNAME from $MOUNT"
		/bin/umount -f $MOUNT
		if [ $? != "0" ]; then
			debug_mesg "failed to umount $MOUNT"
			UMOUNT_SUCCEEDED=0
		else
			debug_mesg "umounted $MOUNT"
			UMOUNT_SUCCEEDED=1
		fi
		# Inform HAL
	fi

	do_cleanup
  
	/opt/local/bin/curl "http://127.0.0.1:5700/storage.OnUnmount?path=${MOUNT}\&successful=${UMOUNT_SUCCEEDED}"

elif [ "$ACTION" == "cleanup" ] ; then
	do_cleanup
else
	debug_mesg hotplug $ACTION event not supported
	exit 1
fi
