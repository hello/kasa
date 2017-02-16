#!/bin/sh
##########################
# History:
#	2014/04/01 - [Tao Wu] modify file
#
# Copyright (C) 2004-2014, Ambarella, Inc.
##########################
mode=$1
driver=$2
ssid=$3
passwd=$4
encryption=$5
channel=$6

DEVICE=wlan0
wpa_supplicant_version="unknown"
DIR_CONFIG=/tmp/config
frequency=2412
############# Broadcom WiFi module bcmdhd CONFIG ###############
BCMDHD_CONFIG=$DIR_CONFIG/dhd.conf

############# WPA CONFIG ###############
CTRL_INTERFACE=/var/run/wpa_supplicant
CONFIG=$DIR_CONFIG/wpa_supplicant.conf
WPA_LOG=/var/log/wpa_supplicant.log
NO_SCAN=1
#the max times to kill app
KILL_NUM_MAX=10
#the max timies to connect AP
CONNECT_NUM_MAX=100
STA_ACTION=/usr/local/bin/sta_action.sh

############# HOSTAP CONFIG ###############
HOSTAP_CTRL_INTERFACE=/var/run/hostapd
HOST_CONFIG=$DIR_CONFIG/hostapd.conf
DEFAULT_CHANNEL=1
HOST_MAX_STA=5

#############  DHCP Server ###############
IP_CONFIG=$DIR_CONFIG/ip.conf
LOCAL_IP=192.168.42.1
LOCAL_NETMASK=255.255.255.0
DHCP_IP_START=192.168.42.2
DHCP_IP_END=192.168.42.20

############# Wifi Direct configuration  ###############
P2P_CONFIG=$DIR_CONFIG/p2p.conf
# Find devices with correct name prefix and automatically connect at startup
export P2P_AUTO_CONNECT=yes
# Auto-connect with devices if the name prefix matches
export P2P_CONNECT_PREFIX=amba
# Do not enable this optional field unless you are certain, please provide a unique name amoung multiple devices to prevent confusion
#P2P_DEVICE_NAME=amba-1
P2P_ACTION=/usr/local/bin/p2p_action.sh

############### Module ID ################
MODULE_ID=AR6003
if [ -e /sys/module/bcmdhd ]; then
	MODULE_ID=BCMDHD
elif [ -e /sys/module/8189es ]; then
	MODULE_ID=RTL8189ES
fi

############### Function ################

# usage
usages()
{
	echo "usage: $0 [sta|ap|p2p] [wext|nl80211]  <SSID> <Password> [open|wpa] <Channel>"
	echo "Example:"
	echo "Connect To AP: 		$0 sta nl80211 <SSID>"
	echo "Connect To security AP: $0 sta nl80211 <SSID> <Password>"
	echo "Setup AP[Open]: 	$0 ap nl80211 <SSID> 0 open <Channel>"
	echo "Setup AP[WPA]: 		$0 ap nl80211 <SSID> <Password> wpa <Channel>"
	echo "Setup P2P: 		$0 p2p nl80211 <SSID>"
	echo "Stop all APP(STA, AP, P2P): 	$0 stop"
	echo "Notice: If you setup AP mode, WPS is enable by default. #hostapd_cli -i<interface> [wps_pbc |wps_pin any <Pin Code> ]"
}

# check usage param
check_param()
{
	if [ ${mode} != "sta" ] && [ ${mode} != "ap" ] && [ ${mode} != "p2p" ] && [ ${mode} != "stop" ]; then
		echo "Please Select Mode [sta|ap|p2p] or stop"
		exit 1;
	fi

	if [ ${driver} != "wext" ] && [ ${driver} != "nl80211" ]; then
		echo "Please Select Driver [wext|nl80211]"
		exit 1;
	fi

	if [ ${mode} == "ap" ]; then
		if [ ${encryption} != "open" ] && [ ${encryption} != "wpa" ]; then
			echo "Please Select Encryption [open|wpa]"
			exit 1;
		fi
		if [ ${#channel} -gt 0 ]; then
			if [ ${channel} -gt 14 ] || [ ${channel}  -lt 1 ]; then
				echo "Your Channel is wrong(1 ~ 13), using Channl ${DEFAULT_CHANNEL} by default."
				channel=$DEFAULT_CHANNEL
			fi
		else
			echo "Using Channl ${DEFAULT_CHANNEL} by default."
			channel=$DEFAULT_CHANNEL
		fi
	fi
}

# check wpa_supplicant version
check_wpa_supplicant()
{
	wpa_version=`wpa_supplicant -v | head -n 1`
	wpa_supplicant_version=${wpa_version##*wpa_supplicant}
	wpa_supplicant_version=${wpa_supplicant_version:2}
	echo "wpa_supplicant version=${wpa_supplicant_version}"
}

# kill process
kill_apps()
{
	for app in "$@"
	do
		kill_num=0
		#echo "Kill ${app}"
		while [ "$(pgrep ${app})" != "" ]
		do
			if [ $kill_num -eq $KILL_NUM_MAX ]; then
				echo "Please try execute \"killall ${app}\" by yourself"
				exit 1
			else
				killall -9 ${app}
			fi
			kill_num=$((kill_num+1));
		done
	done
}

# for bcmdhd roam
generate_bcmdhd_conf()
{
	echo "roam_cache_ssid=${ssid}" > ${BCMDHD_CONFIG}
	echo "roam_cache_channel=${channel}" >> ${BCMDHD_CONFIG}
}

generate_driver_conf()
{
	if [ "${MODULE_ID}" == "BCMDHD" ]; then
		generate_bcmdhd_conf
	fi
}

# wpa_supplicant scan
WPA_SCAN()
{
	wpa_supplicant -i${DEVICE} -D${driver} -C${CTRL_INTERFACE} -B
	wpa_cli -p${CTRL_INTERFACE} -i${DEVICE} scan
	sleep 3
	scan_result=`wpa_cli -p${CTRL_INTERFACE} -i${DEVICE} scan_result`

	kill_apps wpa_supplicant
	rm -r ${CTRL_INTERFACE}
	echo "=============================================="
	echo "${scan_result}"
	echo "=============================================="
}

# configurate wap_supplicant config
generate_wpa_conf()
{
	if [ -f ${CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return ;
	fi
	mkdir -p $DIR_CONFIG
	WPA_SCAN
	scan_entry=`echo "${scan_result}" | tr '\t' ' ' | grep " ${ssid}$" | tail -n 1`

	if [ "${scan_entry}" == "" ]; then
		echo "failed to detect SSID [${ssid}], please try to get close to the AP"
		exit 1
	fi

	echo "ctrl_interface=${CTRL_INTERFACE}" > ${CONFIG}
	echo "network={" >> ${CONFIG}
	echo "ssid=\"${ssid}\"" >> ${CONFIG}

	frequency=`echo "${scan_entry}" |awk '{print $2}'`
	if [ $frequency -gt 2484 ]; then
		channel=$((($frequency-5000)/5))
	else
		channel=$((($frequency-2407)/5))
	fi
	#echo "Frequency=${frequency}, Channel=${channel}"

	WEP=`echo "${scan_entry}" | grep WEP`
	WPA=`echo "${scan_entry}" | grep WPA`
	WPA2=`echo "${scan_entry}" | grep WPA2`
	CCMP=`echo "${scan_entry}" | grep CCMP`
	TKIP=`echo "${scan_entry}" | grep TKIP`

	if [ "${WPA}" != "" ]; then
		#WPA2-PSK-CCMP	(11n requirement)
		#WPA-PSK-CCMP
		#WPA2-PSK-TKIP
		#WPA-PSK-TKIP
		echo "key_mgmt=WPA-PSK" >> ${CONFIG}

		if [ "${WPA2}" != "" ]; then
			echo "proto=WPA2" >> ${CONFIG}
		else
			echo "proto=WPA" >> ${CONFIG}
		fi

		if [ "${CCMP}" != "" ]; then
			if [ "${TKIP}" != "" ]; then
				echo "pairwise=CCMP TKIP" >> ${CONFIG}
				echo "group=TKIP" >> ${CONFIG}
			else
				echo "pairwise=CCMP" >> ${CONFIG}
			fi
		else
			echo "pairwise=TKIP" >> ${CONFIG}
		fi
		echo "psk=\"${passwd}\"" >> ${CONFIG}
	fi

	if [ "${WEP}" != "" ] && [ "${WPA}" == "" ]; then
		echo "key_mgmt=NONE" >> ${CONFIG}
	    echo "wep_key0=${passwd}" >> ${CONFIG}
	    echo "wep_tx_keyidx=0" >> ${CONFIG}
	fi

	if [ "${WEP}" == "" ] && [ "${WPA}" == "" ]; then
		echo "key_mgmt=NONE" >> ${CONFIG}
	fi

	if [ $NO_SCAN -eq 1 ]; then
		echo "disabled=1" >> ${CONFIG}
	fi

	echo "}" >> ${CONFIG}

	generate_driver_conf
}

# start wpa_supplicant
start_wpa_supplicant()
{
	ifconfig ${DEVICE} up
	if [ ${wpa_supplicant_version} == "0.7.3" ]; then
		CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -B"
	else
		CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -f${WPA_LOG} -d -t -B"
	fi
	echo "CMD=${CMD}"
	$CMD
	wpa_cli -p${CTRL_INTERFACE} -i ${DEVICE} select_network 0
}

generate_dns_conf()
{
	echo "#  DO NOT EDIT THIS FILE BY HAND " > /etc/resolv.conf
	for i in "$@"; do
		echo "nameserver $i" >> /etc/resolv.conf
	done
}

# Generate ip.conf
#ip_addr:192.168.1.28
#Bcast:192.168.1.255
#Mask:255.255.255.0
#Gateway:192.168.1.1
generate_ip_conf()
{
	echo "$2"
	echo "ip_$2" > $IP_CONFIG
	echo "$3"
	echo "$3" >> $IP_CONFIG
	echo "$4"
	echo "$4" >> $IP_CONFIG
	gw=`route -n |grep ${DEVICE} |awk 'NR==1{print $2}'`
	echo "Gateway:$gw" >> $IP_CONFIG
}

# Get IP Address, 1. DHCP 2. Static IP
get_ip_address()
{
	if [ -f $IP_CONFIG ]; then
		LOCAL_IP=`cat ${IP_CONFIG} | grep ip_addr`
		LOCAL_IP=${LOCAL_IP##*:}
		BCAST=`cat ${IP_CONFIG} | grep Bcast`
		BCAST=${BCAST##*:}
		MASK=`cat ${IP_CONFIG} | grep Mask`
		MASK=${MASK##*:}
		GATEWAY=`cat ${IP_CONFIG} | grep Gateway`
		GATEWAY=${GATEWAY##*:}
		echo "Static IP: $LOCAL_IP "
		echo "Broad Cast: $BCAST "
		echo "Net Mask: $MASK "
		echo "Gate Way: $GATEWAY "
		ifconfig ${DEVICE} ${LOCAL_IP} broadcast ${BCAST} netmask ${MASK}
		route add default gw ${GATEWAY} ${DEVICE}
	else
		dns_server=`udhcpc -i${DEVICE} 2>&1 |grep dns|awk '{print $3}'`
		#generate_dns_conf $dns_server
		LOCAL_IP_CONF=`ifconfig ${DEVICE} | grep "inet addr" | cut -c 11-`
		generate_ip_conf ${LOCAL_IP_CONF}
	fi
}

# check wpa status
wpa_cli_sta_check_status()
{
	connect_num=0
	connect_sleep_num=$(($CONNECT_NUM_MAX/2))
	until [ $connect_num -gt $CONNECT_NUM_MAX ]
	do
		WPA_STATUS_CMD=`wpa_cli -p${CTRL_INTERFACE} -i${DEVICE} status`
		WPA_STATUS=${WPA_STATUS_CMD##*wpa_state=}
		WPA_STATUS=${WPA_STATUS:0:9}
		if [ "$WPA_STATUS" == "COMPLETED" ]; then
			if [ "$(pgrep wpa_supplicant)" == "" ]; then
				echo ">>>wpa_supplicant isn't running<<<"
				exit 1
			fi
			echo "Check times [${connect_num}], wpa_supplicant status=${WPA_STATUS}"
			get_ip_address
			echo ">>>wifi_setup OK<<<"
			exit 0
		fi
		connect_num=$((connect_num+1));
		if [ $connect_num -gt $connect_sleep_num ]; then
			sleep 1
		fi
	done

	kill_apps wpa_supplicant
	echo ">>>wifi_setup failed, password may be incorrect or ap not in range<<<"
	exit 1
}

# stat wpa_cli  server to listen evernt
wpa_cli_sta_action()
{
	wpa_cli -i${DEVICE} -a${STA_ACTION} -B
}

wpa_cli_sta()
{
	if [ -x $STA_ACTION ]; then
		wpa_cli_sta_action
	else
		wpa_cli_sta_check_status
	fi
}
################  AP #####################

generate_hostapd_conf()
{
	if [ -f ${HOST_CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return ;
	fi
	mkdir -p $DIR_CONFIG
	#generate hostapd.conf
	echo "interface=wlan0" > ${HOST_CONFIG}
	echo "ctrl_interface=${HOSTAP_CTRL_INTERFACE}" >> ${HOST_CONFIG}
	echo "beacon_int=100" >> ${HOST_CONFIG}
#	echo "dtim_period=1" >> ${HOST_CONFIG}
	echo "preamble=0" >> ${HOST_CONFIG}
	#WPS support
	echo "wps_state=2" >> ${HOST_CONFIG}
	echo "eap_server=1" >> ${HOST_CONFIG}
	echo "ap_pin=12345670" >> ${HOST_CONFIG}
	echo "config_methods=label display push_button keypad ethernet" >> ${HOST_CONFIG}
	echo "wps_pin_requests=/var/run/hostapd.pin-req" >> ${HOST_CONFIG}
	#general
	echo "ssid=${ssid}" >> ${HOST_CONFIG}
	echo "max_num_sta=${HOST_MAX_STA}" >> ${HOST_CONFIG}
	echo "channel=${channel}" >> ${HOST_CONFIG}

	case ${encryption} in
		open)
			;;
		wpa)
			echo "wpa=2" >> ${HOST_CONFIG}
			echo "wpa_pairwise=CCMP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${passwd}" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			;;
		*)
			;;
	esac

	#Realtek rtl8189es
	if [ ${MODULE_ID} == "RTL8189ES" ]; then
		echo "driver=nl80211" >> ${HOST_CONFIG}
		echo "hw_mode=g" >> ${HOST_CONFIG}
		echo "ieee80211n=1" >> ${HOST_CONFIG}
		echo "ht_capab=[SHORT-GI-20][SHORT-GI-40][HT40]" >> ${HOST_CONFIG}
		#echo "wme_enabled=1" >> ${HOST_CONFIG}
		#echo "wpa_group_rekey=86400" >> ${HOST_CONFIG}
	fi
}

start_hostapd_ap()
{
## Setup interface and set IP,gateway##
	ifconfig ${DEVICE} up
	ifconfig ${DEVICE} ${LOCAL_IP}
	route add default netmask ${LOCAL_NETMASK} gw ${LOCAL_IP}
## Start Hostapd ##
	CMD="hostapd ${HOST_CONFIG} -B"
	echo "CMD=${CMD}"
	$CMD

## Start DHCP Server ##
	mkdir -p /var/lib/misc
	dnsmasq --no-daemon --no-resolv --no-poll --dhcp-range=${DHCP_IP_START},${DHCP_IP_END},1h &
	echo "HostAP Setup Finished."
}

################  WiFi Direct #####################

generate_wpa_p2p_conf()
{
	if [ -f ${P2P_CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return ;
	fi
	mkdir -p $DIR_CONFIG
	device_name=${ssid}
	if [ "${device_name}" == "" ]; then
		postmac=`ifconfig ${DEVICE} | grep HWaddr | awk '{print $NF}' | sed 's/://g' | cut -c 6- | tr 'A-Z' 'a-z'`
		device_name=amba-${postmac}
	fi

	echo "device_name=${device_name}" > ${P2P_CONFIG}
	echo "ctrl_interface=${CTRL_INTERFACE}" >> ${P2P_CONFIG}
	echo "device_type=10-0050F204-5" >> ${P2P_CONFIG}
	echo "config_methods=display push_button keypad" >> ${P2P_CONFIG}
}

start_wpa_supplicant_p2p()
{
	CMD="wpa_supplicant -i${DEVICE} -c${P2P_CONFIG} -D${driver} -f${WPA_LOG} -B"
	echo "CMD=${CMD}"
	$CMD
	if [ -x ${P2P_ACTION} ]; then
		wpa_cli -p${CTRL_INTERFACE} -i${DEVICE} -a${P2P_ACTION}
	else
		echo "Not exist ${P2P_ACTION} file"
	fi
	wpa_cli -p${CTRL_INTERFACE} p2p_set ssid_postfix "_AMBA"
	wpa_cli -p${CTRL_INTERFACE} p2p_find
}

stop_wifi_app()
{
	kill_apps NetworkManager wpa_supplicant hostapd dnsmasq udhcpc
}

clear_config()
{
	rm -rf ${CONFIG}
	rm -rf ${HOST_CONFIG}
	rm -rf ${P2P_CONFIG}
	rm -rf ${IP_CONFIG}
}
################   Main  ###################

## Usage
if [ $# -lt 1 ]; then
	usages
	exit 1
fi

## Stop WiFi
if [ $# -eq 1 ] && [ ${mode} == "stop" ]; then
	stop_wifi_app
	ifconfig ${DEVICE} down
	exit 0
fi

## Start WiFi
if [ $# -gt 2 ]; then
	check_param
	clear_config
fi

if [ ${mode} == "sta" ]; then
	generate_wpa_conf
	start_wpa_supplicant
	wpa_cli_sta
elif [ ${mode} == "ap" ]; then
	freq=$(($channel*5 +2407))
	generate_hostapd_conf
	start_hostapd_ap
elif [ ${mode} == "p2p" ]; then
	generate_wpa_p2p_conf
	start_wpa_supplicant_p2p
else
	usage
fi

########################################