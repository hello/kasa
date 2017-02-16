#!/bin/sh
##########################
# History:
#	2015/04/28 - [Tao Wu] modify file
#
# Copyright (C) 2004-2015, Ambarella, Inc.
##########################

DIR_CONFIG=/tmp/config
WPA_CONFIG=$DIR_CONFIG/wpa_supplicant.conf
COOEE_CONFIG=$DIR_CONFIG/cooee.conf
DEVICE=wlan0

SSID=""
PASSWORD=""

if [ -f $WPA_CONFIG ]; then
	wifi_setup.sh sta nl80211
else
	mkdir -p $DIR_CONFIG
	ifconfig  $DEVICE up
	while [ ! -f $COOEE_CONFIG ]
	do
		echo "Cooee..."
		cooee -f $COOEE_CONFIG
	done
	echo "Cooee Done"

	SSID=`cat ${COOEE_CONFIG} | grep ssid | cut -c 6-`
	PASSWORD=`cat ${COOEE_CONFIG} | grep password | cut -c 10-`
	echo "Cooee: SSID=${SSID}, PASSWORD=${PASSWORD}"
	wifi_setup.sh sta nl80211 ${SSID} ${PASSWORD}
fi
########################################
