#!/bin/sh

#
# Script that runs necessary initializations. Loads driver with the first 
# argumnent (if it is already loaded it is unloaded) and creates device 
# handles.
#
# Note that shis script must be run as root. On host, try 'sudo ./install.sh'
#

mod="bifrost"
dev="bifrost"

if ! [ -f $mod.ko ]; then
	echo "No such file: $mod.ko"
	exit 1
fi

if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root. Try sudo!" 1>&2
	exit 1
fi

# Load generic system modules required by frambeuffer
# NOTE this requires frambeuffer support is enabled in OS
#modprobe syscopyarea
#modprobe sysfillrect 
#modprobe sysimgblt 

# check if module already loaded

loaded=`grep $mod /proc/modules`
if [ -n "$loaded" ]; then
	echo "module $mod already loaded, unloading it first"
	rmmod ./$mod.ko
fi

insmod ./$mod.ko $1

# (re)load done, create device nodes first removing existing 

rm -f "/dev/$dev"

major=`grep $dev\$ /proc/devices | sed 's/ '"$dev"'//'`
if [ -n "$major" ]; then
	mknod "/dev/$dev" c $major 0
	chgrp dialout "/dev/$dev"
	chmod 0664 "/dev/$dev"
else
	echo "$dev not found, skipping creation of device handle for it."
fi

echo "done."

