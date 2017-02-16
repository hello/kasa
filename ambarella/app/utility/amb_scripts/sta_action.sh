#!/bin/sh
##########################
# History:
#	2015/08/19 - [Tao Wu] create file
#
# Copyright (C) 2004-2015 Ambarella, Inc.
##########################

IFNAME=$1
CMD=$2

WIFI_STATUS=/dev/console

if [ "$CMD" = "CONNECTED" ]; then
    # configure network, signal DHCP client, etc.
	echo $(date) ":: CONNECTED"  > $WIFI_STATUS
	udhcpc -i${IFNAME} > $WIFI_STATUS
fi

if [ "$CMD" = "DISCONNECTED" ]; then
    # remove network configuration, if needed
	echo $(date) ":: DISCONNECTED"  > $WIFI_STATUS
fi
