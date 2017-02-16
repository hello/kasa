#!/bin/sh

##########################
# History:
#	2013/06/24 - [Tao Wu] create file
#
# Copyright (C) 2004-2013, Ambarella, Inc.
##########################
auth=$1
ssid=$2

INTERFACE=wlan0
CTRL_INTERFACE=/var/run/wpa_supplicant
HOSTAP_CTRL_INTERFACE=/var/run/hostapd
CONFIG=/etc/network/hostapd.conf
DEFAULT_CHANNEL=1
channel=$DEFAULT_CHANNEL

usage()
{
	echo "usage:  $0 wpa SSID PASSWORD CHANNEL"
	echo "        $0 open SSID CHANNEL"
	echo "        $0 wps SSID CHANNEL"
	echo "        [ WPS Connect devices] #hostapd_cli -i<interface> [wps_pbc |wps_pin any <Pin Code> ]"
	echo "        $0 stopap      Stop AP mode and Enter STA mode"
}

auth_wpa()
{
	echo "ctrl_interface=$CTRL_INTERFACE" >>$CONFIG
	echo "ap_scan=2" >>$CONFIG
	echo "network={" >>$CONFIG
	echo "ssid=\"$ssid\"" >>$CONFIG
	echo "mode=2" >>$CONFIG
	echo "frequency=$freq" >>$CONFIG
	echo "key_mgmt=WPA-PSK">>$CONFIG
	echo "proto=RSN" >>$CONFIG
	echo "pairwise=CCMP" >>$CONFIG
	echo "psk=\"$passwd\"" >>$CONFIG
	echo "}" >>$CONFIG
}

auth_open()
{
	echo "ctrl_interface=$CTRL_INTERFACE" >>$CONFIG
	echo "ap_scan=2" >>$CONFIG
	echo "network={" >>$CONFIG
	echo "ssid=\"$ssid\"" >>$CONFIG
	echo "mode=2" >>$CONFIG
	echo "frequency=$freq" >>$CONFIG
	echo "key_mgmt=NONE">>$CONFIG
	echo "}" >>$CONFIG
}

auth_wps()
{
	echo "interface=$INTERFACE" >$CONFIG
	echo "ctrl_interface=$HOSTAP_CTRL_INTERFACE" >>$CONFIG
	echo "ssid=$ssid" >>$CONFIG
	echo "ignore_broadcast_ssid=0" >>$CONFIG
	echo "channel=$channel" >>$CONFIG
	echo "wpa=2" >>$CONFIG
	echo "wpa_key_mgmt=WPA-PSK" >>$CONFIG
	echo "wpa_pairwise=CCMP" >>$CONFIG
	echo "wpa_passphrase=12345670" >>$CONFIG
	echo "wps_state=2" >>$CONFIG
	echo "ap_pin=12345670" >>$CONFIG
	echo "eap_server=1" >>$CONFIG
	echo "ap_setup_locked=0" >>$CONFIG
	echo "config_methods=label display push_button keypad ethernet" >>$CONFIG
	echo "wps_pin_requests=/var/run/hostapd.pin-req" >>$CONFIG
}

start_ap()
{
mkdir -p /var/lib/misc

if [ -x /etc/init.d/S45network-manager ]; then
	/etc/init.d/S45network-manager stop
fi
killall -9 wpa_supplicant hostapd dnsmasq> /dev/null

## Setup interface and set IP,gateway##
ifconfig $INTERFACE up
ifconfig $INTERFACE 192.168.42.1
route add default netmask 255.255.255.0 gw 192.168.42.1
## Start Hostapd ##
wpa_supplicant -Dnl80211 -i$INTERFACE -c$CONFIG &
sleep 1

## Start DHCP Server ##
dnsmasq --no-daemon --no-resolv --no-poll --dhcp-range=192.168.42.2,192.168.42.254,1h &
echo "HostAP Setup Finished."
}

hostapd_start_ap()
{
mkdir -p /var/lib/misc

if [ -x /etc/init.d/S45network-manager ]; then
	/etc/init.d/S45network-manager stop
fi
killall -9 wpa_supplicant hostapd dnsmasq> /dev/null

## Setup interface and set IP,gateway##
ifconfig $INTERFACE up
ifconfig $INTERFACE 192.168.42.1
route add default netmask 255.255.255.0 gw 192.168.42.1
## Start Hostapd ##
hostapd $CONFIG &
sleep 1

## Start DHCP Server ##
dnsmasq --no-daemon --no-resolv --no-poll --dhcp-range=192.168.42.2,192.168.42.254,1h &
echo "HostAP Setup Finished."
}

stop_ap()
{
killall -9 wpa_supplicant hostapd dnsmasq > /dev/null
### Re install module ###
sleep 1
if [ -x /etc/init.d/S45network-manager ]; then
	/etc/init.d/S45network-manager start
fi
echo "HostAP Stop Finished."
}

rm -rf $CONFIG

case $auth in
	wpa)
	passwd=$3
	len=${#passwd}
	if [ $len -lt 8 ]; then
		echo "WPA password length at least 8"
		exit 1
	fi
	if [ $4 != "" ]; then
		channel=$4
	fi
	freq=$(($channel*5 +2407))
	auth_wpa && start_ap
	;;
	open)
	if [ $3 != "" ]; then
		channel=$3
	fi
	freq=$(($channel*5 +2407))
	auth_open && start_ap
	;;
	wps)
	if [ $3 != "" ]; then
		channel=$3
	fi
	freq=$(($channel*5 +2407))
	auth_wps && hostapd_start_ap
	;;
	stopap)
	stop_ap
	;;
	*)
	echo "Please input authentication type."
	usage
	exit 1
	;;
esac
