#!/bin/sh

#
# Script that runs necessary initializations. Loads driver with the first 
# argumnent (if it is already loaded it is unloaded) and creates device 
# handles.
#
# Note that shis script must be run as root. On host, try 'sudo ./install.sh'
#

mod="animal-i2c"
dev="animal-i2c"

mod_usim="animal-i2c-usim"
dev_usim="animal-i2c-usim"

if ! [ -f $mod.ko ]; then
	echo "No such file: $mod.ko"
	exit 1
fi
if ! [ -f $mod_usim.ko ]; then
	echo "No such file: $mod_usim.ko"
	exit 1
fi

if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root. Try sudo!" 1>&2
	exit 1
fi

# check if module already loaded

loaded=`grep $dev\$ /proc/devices`
if [ -n "$loaded" ]; then
	echo "module $mod already loaded, unloading it first"
	rmmod ./$mod.ko
fi

loaded=`grep $dev_usim\$ /proc/devices`
if [ -n "$loaded" ]; then
	echo "module $mod_usim already loaded, unloading it first"
	rmmod ./$mod_usim.ko
fi

insmod ./$mod_usim.ko

if [ -f /proc/$dev_usim ]; then
    adap_nr=$(awk '{print $1}' /proc/$dev_usim)
    insmod ./$mod.ko sim_adap_nr=$adap_nr simulator=1 $1
else
    insmod ./$mod.ko $1 $2
fi

# (re)load done, create device nodes first removing existing 

for dev in "$mod" "$dev_usim"
do
	rm -f "/dev/$dev"

	major=`grep $dev\$ /proc/devices | sed 's/ '"$dev"'//'`
	if [ -n "$major" ]; then
		mknod "/dev/$dev" c $major 0
		chgrp dialout "/dev/$dev"
		chmod 0664 "/dev/$dev"
	else
		echo "$dev not found, skipping creation of device handle for it."
	fi
done

echo "done."
