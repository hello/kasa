#!/bin/sh
##########################
# History:
#	2014/04/01 - [Tao Wu] Create file
#	2016/05/17 - [Tao Wu] Update file
#
# Copyright (c) 2016 Ambarella, Inc.
#
# This file and its contents ("Software") are protected by intellectual
# property rights including, without limitation, U.S. and/or foreign
# copyrights. This Software is also the confidential and proprietary
# information of Ambarella, Inc. and its licensors. You may not use, reproduce,
# disclose, distribute, modify, or otherwise prepare derivative works of this
# Software or any portion thereof except pursuant to a signed license agreement
# or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
# In the absence of such an agreement, you agree to promptly notify and return
# this Software to Ambarella, Inc.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
# MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
###############

WIFI_SETUP_VERSION="1.3.4"

mode=$1
driver=$2

############# Configuration it ###############
WOWLAN=1
DEVICE=wlan0
DEVICE_AP=wlan1
DEVICE_P2P=wlan1
DIR_CONFIG=/tmp/config

BG_CHECK_STA_STATUS=0

############# Exit Error Number ###############
ERRNO_OK=0
ERRNO_PARAM=1
ERRNO_ENV=2
ERRNO_SSID_NOT_FOUND=3
ERRNO_PASSWORD_WRONG=4

############# Broadcom WiFi module bcmdhd CONFIG ###############
BCMDHD_CONFIG=$DIR_CONFIG/dhd.conf

############# WPA CONFIG ###############
CTRL_INTERFACE=/var/run/wpa_supplicant
CONFIG=$DIR_CONFIG/wpa_supplicant.conf
HOST_CONFIG=$DIR_CONFIG/wpa_supplicant_ap.conf
WPA_LOG=/var/log/wpa_supplicant.log
STA_ACTION=/usr/local/bin/sta_action.sh

## Used for wpa_supplicant's param
WPA_EXT_PARAM=""
## Used for connect ap without scan
NO_SCAN=1
## Used for sta auto connect to ap when ap down and up
STA_AUTOCONNECTION=1
## User for update wpa_supplicant config after save_config
UPDATE_CONFIG_VALUE=1

wpa_supplicant_version="unknown"

DEFAULT_CHANNEL=1
sta_channel=$DEFAULT_CHANNEL
sta_frequency=$((DEFAULT_CHANNEL*5 + 2407))
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

############### Function ################

usages()
{
	echo "Version: ${WIFI_SETUP_VERSION}"
	echo "This script used to Setup/Stop WiFi STA/AP/CON/P2P mode with wpa_supplicant"
	echo "usage:                      $0 [sta|ap|wps|con|p2p] [wext|nl80211] <SSID> <Password|0> [open|wpa2|wpa|wpawpa2|wep|0] <Channel>"
	echo ""
	echo "Example:"
	echo "Connect To AP[open]:        $0 sta nl80211 <SSID>"
	echo "Connect To AP[encrypt]:     $0 sta nl80211 <SSID> <Password>"
	echo "Connect To AP-BSSID[open]:  $0 sta nl80211 <BSSID>"
	echo "Connect To Hidden[open]:    $0 sta nl80211 <SSID>     0      open"
	echo "Connect To Hidden[encrypt]: $0 sta nl80211 <SSID> <Password> [wpa2|wpa|wpawpa2|wep]"
	echo "Connect To AP[wps_pbc]:     $0 wps nl80211 <BSSID|any>     0       wps_pbc"
	echo "Connect To AP[wps_pin]:     $0 wps nl80211 <BSSID|any> <Pincode|0> wps_pin"
	echo "Setup AP[open]:             $0 ap  nl80211 <SSID>     0      open <Channel>"
	echo "Setup AP[encrypt]:          $0 ap  nl80211 <SSID> <Password> [wpa2|wpa|wpawpa2]  <Channel>"
	echo "Setup Concurrent:           $0 con nl80211 <STA-SSID> <Password|0> [open|wpa2|wpa|wpawpa2|wep|0] <AP-SSID> <Password|0> [open|wpa]"
	echo "Setup P2P:                  $0 p2p nl80211 <SSID>"
	echo "Stop STA/AP/CON/P2P mode:   $0 stop"
	echo ""
	echo "NOTICE: Using interface STA:[${DEVICE}], AP[${DEVICE_AP}], P2P[${DEVICE_P2P}] by default, change it if necessary."
	echo "Configuration: WOWLAN, DEVICE/DEVICE_AP/DEVICE_P2P, DIR_CONFIG value in $0 ."

	exit $ERRNO_OK
}

check_driver()
{
	if [ "${driver}" != "wext" ] && [ "${driver}" != "nl80211" ]; then
		echo "Please Select Driver [wext|nl80211]"
		exit $ERRNO_PARAM
	fi
}

check_iface()
{
	iface=$1
	if [ "$(ifconfig "$iface")" = "" ]; then
		echo "Not Found Interface $iface"
		exit $ERRNO_ENV
	fi
}

check_wpa_supplicant()
{
	if [ "$(which wpa_supplicant)" = "" ]; then
		echo "Not Found wpa_supplicant"
		exit $ERRNO_ENV
	fi
	wpa_version=$(wpa_supplicant -v | head -n 1)
	wpa_supplicant_version=${wpa_version##*wpa_supplicant }
	echo "wpa_supplicant version=${wpa_supplicant_version}"
}

kill_apps()
{
	#the max times to kill app
	local KILL_NUM_MAX=10

	for app in "$@"
	do
		local kill_num=0
		while [ "$(pgrep "${app}")" != "" ]
		do
			if [ $kill_num -ge $KILL_NUM_MAX ]; then
				echo "Please try execute \"killall ${app}\" by yourself"
				exit $ERRNO_ENV
			else
				killall -9 "${app}"
				sleep 1
			fi
			kill_num=$((kill_num + 1));
		done
	done
}

stop_wifi_app()
{
	kill_apps udhcpc dnsmasq NetworkManager wpa_cli wpa_supplicant
	if [ -x $STA_ACTION ]; then
		kill_apps $STA_ACTION
	fi
	ifconfig ${DEVICE} down
	ifconfig ${DEVICE_AP} down
	ifconfig ${DEVICE_P2P} down
}

################ Driver #####################

get_device_id()
{
	MODULE_ID=UNKNOWN
	if [ -e /sys/module/bcmdhd ]; then
		MODULE_ID=BCMDHD
	elif [ -e /sys/module/8189es ]; then
		MODULE_ID=RTL8189ES
	fi
}

generate_bcmdhd_conf()
{
	local local_ssid=$1
	local local_channel=$2

	if [ -f ${BCMDHD_CONFIG} ]; then
		grep -v 'roam_cache_ssid' ${BCMDHD_CONFIG} | grep -v 'roam_cache_channel' > ${BCMDHD_CONFIG}.tmp
		mv ${BCMDHD_CONFIG}.tmp ${BCMDHD_CONFIG}
	fi
	echo "roam_cache_ssid=${local_ssid}" >> ${BCMDHD_CONFIG}
	echo "roam_cache_channel=${local_channel}" >> ${BCMDHD_CONFIG}
}

generate_driver_conf()
{
	if [ "${MODULE_ID}" = "BCMDHD" ]; then
		generate_bcmdhd_conf "$1" "$2"
	fi
}

################ STA #####################

show_scan_result()
{
	echo "=============================================="
	echo "${scan_result}"
	echo "=============================================="
}

WPA_SCAN()
{
	#the max times to wait scan result
	local SCAN_CHECK_MAX=10

	local local_ssid=$1

	wpa_supplicant -i${DEVICE} -D"${driver}" -C${CTRL_INTERFACE} -B
	wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} scan
	sleep 1

	local scan_matched_num=0
	local scan_check=0
	local ssid_line_no=1

	is_bssid=$(echo "${local_ssid}" | grep -E "[0-9a-f]{2}([-:])[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$")
	scan_result=""
	scan_entry=""

	while [ $scan_check -lt $SCAN_CHECK_MAX ]
	do
		scan_result=$(wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} scan_result)
		if  [ "${is_bssid}" ]; then
			scan_entry=$(echo "${scan_result}" | tr '\t' ' ' | grep -F "${local_ssid}")
		else
			ssid_line_no=$(echo "${scan_result}" |cut -f 5-| sed 's/\\\\/\\/g' | sed 's/\\\"/\"/g' | grep -n -x -F "${local_ssid}" |  cut -d ':' -f 1 )
			scan_matched_num=$(echo "${ssid_line_no}" | wc -l)
			ssid_line_no=$(echo "${ssid_line_no}" | head -n 1)
			scan_entry=$(echo "${scan_result}" | tr '\t' ' ' | awk 'NR=="'${ssid_line_no}'" {print $0}' | sed 's/\\\\/\\/g' | sed 's/\\\"/\"/g')
		fi

		if [ "${scan_entry}" != "" ]; then
			echo "Scan [${scan_check}] times and get entry [${scan_matched_num}]=${scan_entry}"
			break
		fi

		scan_check=$((scan_check + 1))
		sleep 1
	done

	kill_apps wpa_supplicant
	rm -r ${CTRL_INTERFACE}

	if [ "${scan_entry}" = "" ]; then
		show_scan_result
		echo "Scan [${scan_check}] times and can not find SSID [${local_ssid}]"
		exit $ERRNO_SSID_NOT_FOUND
	fi
	if [ "$scan_matched_num" -gt 1 ]; then
		show_scan_result
		echo "Found [${scan_matched_num}] AP with the same SSID [${local_ssid}]"
	fi
}

generate_wpa_scan_conf()
{
	local local_ssid=$1
	local local_passwd=$2

	if [ -f ${CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return
	fi
	mkdir -p $DIR_CONFIG

	WPA_SCAN "${local_ssid}"

	sta_frequency=$(echo "${scan_entry}" | awk '{print $2}')
	if [ "$sta_frequency" -gt 2484 ]; then
		sta_channel=$(((sta_frequency-5000)/5))
	else
		sta_channel=$(((sta_frequency-2407)/5))
	fi

	if  [ "${is_bssid}" ]; then
		echo "STA-BSSID=${local_ssid}, Frequency=${sta_frequency}, Channel=${sta_channel}"
	else
		echo "STA-SSID=${local_ssid}, Frequency=${sta_frequency}, Channel=${sta_channel}"
	fi
	echo "ctrl_interface=${CTRL_INTERFACE}" > ${CONFIG}
	echo "update_config=${UPDATE_CONFIG_VALUE}" >> ${CONFIG}

	## This for WiFi suspend/resume, otherwise wpa will send disconnect ctl when suspend
	if [ $WOWLAN -eq 1 ]; then
		echo "wowlan_triggers=any" >> ${CONFIG}
	fi

	echo "network={" >> ${CONFIG}

	if  [ "${is_bssid}" ]; then
		echo "bssid=${local_ssid}" >> ${CONFIG}
	else
		echo "ssid=\"${local_ssid}\"" >> ${CONFIG}
	fi

	WEP=$(echo "${scan_entry}" | grep WEP)
	WPA=$(echo "${scan_entry}" | grep WPA)
	WPA2=$(echo "${scan_entry}" | grep WPA2)
	CCMP=$(echo "${scan_entry}" | grep CCMP)
	TKIP=$(echo "${scan_entry}" | grep TKIP)

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
		local len_passwd=${#local_passwd}
		if [ "$len_passwd" -eq 64 ]; then
			echo "passphrase length is 64, using hex type"
			echo "psk=${local_passwd}" >> ${CONFIG}
		elif [ "$len_passwd" -ge 8 ] && [ "$len_passwd" -le 63 ]; then
			echo "psk=\"${local_passwd}\"" >> ${CONFIG}
		else
			echo "Invalid passphrase length ${len_passwd} (expected: 8..63 or 64 hex)"
			rm -rf ${CONFIG}
			exit $ERRNO_PARAM
		fi
	fi

	if [ "${WEP}" != "" ] && [ "${WPA}" = "" ]; then
		{
			echo "key_mgmt=NONE"
			echo "wep_key0=${local_passwd}"
			echo "wep_tx_keyidx=0"
		} >> ${CONFIG}
	fi

	if [ "${WEP}" = "" ] && [ "${WPA}" = "" ]; then
		echo "key_mgmt=NONE" >> ${CONFIG}
	elif [ "${local_passwd}" = "" ]; then
		echo "Please input password for encrypt AP"
		rm -rf ${CONFIG}
		exit $ERRNO_PARAM
	fi

	if [ $NO_SCAN -eq 1 ]; then
		echo "disabled=1" >> ${CONFIG}
	fi

	echo "}" >> ${CONFIG}

	generate_driver_conf "${local_ssid}" ${sta_channel}
}

generate_wpa_hidden_conf()
{
	local local_ssid=$1
	local local_passwd=$2
	local local_encrypt=$3

	if [ -f ${CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return
	fi
	mkdir -p $DIR_CONFIG

	is_bssid=$(echo "${local_ssid}" | grep -E "[0-9a-f]{2}([-:])[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$")
	if  [ "${is_bssid}" ]; then
		echo "Hidden STA-BSSID=${local_ssid}, Encryption=${local_encrypt}"
	else
		echo "Hidden STA-SSID=${local_ssid}, Encryption=${local_encrypt}"
	fi

	echo "ctrl_interface=${CTRL_INTERFACE}" > ${CONFIG}
	echo "ap_scan=2" >> ${CONFIG}
	echo "update_config=${UPDATE_CONFIG_VALUE}" >> ${CONFIG}

	## This for WiFi suspend/resume, otherwise wpa will send disconnect ctl when suspend
	if [ $WOWLAN -eq 1 ]; then
		echo "wowlan_triggers=any" >> ${CONFIG}
	fi

	echo "network={" >> ${CONFIG}

	if  [ "${is_bssid}" ]; then
		echo "bssid=${local_ssid}" >> ${CONFIG}
	else
		echo "ssid=\"${local_ssid}\"" >> ${CONFIG}
	fi

	if [ "${local_encrypt}" != "open" ]; then
		local len_passwd=${#local_passwd}
		local local_passwd_value=""

		if [ "$len_passwd" -eq 64 ]; then
			echo "passphrase length is 64, using hex type"
			local_passwd_value="${local_passwd}"
		elif [ "$len_passwd" -ge 8 ] && [ "$len_passwd" -le 63 ]; then
			local_passwd_value="\"${local_passwd}\""
		else
			echo "Invalid passphrase length ${len_passwd} (expected: 8..63 or 64 hex)"
			rm -rf ${CONFIG}
			exit $ERRNO_PARAM
		fi
	fi

	case ${local_encrypt} in
		open)
			echo "key_mgmt=NONE">> ${CONFIG}
			;;
		wpa2)
			echo "key_mgmt=WPA-PSK">> ${CONFIG}
			echo "proto=WPA2" >> ${CONFIG}
			echo "pairwise=CCMP" >> ${CONFIG}
			echo "psk=${local_passwd_value}" >> ${CONFIG}
			;;
		wpa)
			echo "key_mgmt=WPA-PSK">> ${CONFIG}
			echo "proto=WPA" >> ${CONFIG}
			echo "pairwise=TKIP" >> ${CONFIG}
			echo "psk=${local_passwd_value}" >> ${CONFIG}
			;;
		wpawpa2)
			echo "key_mgmt=WPA-PSK">> ${CONFIG}
			echo "proto=WPA2" >> ${CONFIG}
			echo "pairwise=CCMP TKIP" >> ${CONFIG}
			echo "group=TKIP" >> ${CONFIG}
			echo "psk=${local_passwd_value}" >> ${CONFIG}
			;;
		wep)
			echo "key_mgmt=NONE" >> ${CONFIG}
			echo "wep_key0=${local_passwd}" >> ${CONFIG}
			echo "wep_tx_keyidx=0" >> ${CONFIG}
			;;
		*)
			echo "Please select hidden SSID encryption [open|wpa2|wpa|wpawpa2|wep]"
			rm -rf ${CONFIG}
			exit $ERRNO_PARAM
			;;
	esac

	if [ $NO_SCAN -eq 1 ]; then
		echo "disabled=1" >> ${CONFIG}
	fi

	echo "}" >> ${CONFIG}
}

generate_wpa_sta_conf()
{
	local local_is_hidden=$3
	if [ "${local_is_hidden}" ] && [ "${local_is_hidden}" != "0" ]; then
		generate_wpa_hidden_conf "$1" "$2" "$3"
	else
		generate_wpa_scan_conf "$1" "$2"
	fi
}

start_wpa_supplicant_sta()
{
	ifconfig ${DEVICE} up
	if [ "${wpa_supplicant_version}" = "0.7.3" ]; then
		CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -B ${WPA_EXT_PARAM}"
	else
		CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -f${WPA_LOG} -B ${WPA_EXT_PARAM}"
	fi
	echo "CMD=${CMD}"
	$CMD

	wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} select_network 0
	if [ $STA_AUTOCONNECTION -eq 1 ]; then
		wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} sta_autoconnect 1
	fi
}

generate_dns_conf()
{
	echo "#  DO NOT EDIT THIS FILE BY HAND " > /etc/resolv.conf
	for i in "$@"; do
		echo "nameserver $i" >> /etc/resolv.conf
	done
}

generate_ip_conf()
{
# Generate ip.conf
#ip_addr:192.168.1.28
#Bcast:192.168.1.255
#Mask:255.255.255.0
#Gateway:192.168.1.1
	echo "$2"
	echo "ip_$2" > $IP_CONFIG
	echo "$3"
	echo "$3" >> $IP_CONFIG
	echo "$4"
	echo "$4" >> $IP_CONFIG
	gw=$(route -n |grep ${DEVICE} |awk 'NR==1{print $2}')
	echo "Gateway:$gw" >> $IP_CONFIG
}

get_ip_address()
{
# Get IP Address, 1. DHCP 2. Static IP
	if [ -f $IP_CONFIG ]; then
		LOCAL_IP=$(grep ip_addr ${IP_CONFIG})
		LOCAL_IP=${LOCAL_IP##*:}
		BCAST=$(grep Bcast ${IP_CONFIG})
		BCAST=${BCAST##*:}
		MASK=$(grep Mask ${IP_CONFIG})
		MASK=${MASK##*:}
		GATEWAY=$(grep Gateway ${IP_CONFIG})
		GATEWAY=${GATEWAY##*:}
		echo "Static IP: $LOCAL_IP "
		echo "Broad Cast: $BCAST "
		echo "Net Mask: $MASK "
		echo "Gate Way: $GATEWAY "
		ifconfig ${DEVICE} "${LOCAL_IP}" broadcast "${BCAST}" netmask "${MASK}"
		route add default gw "${GATEWAY}" ${DEVICE}
	else
		dns_server=$(udhcpc -i${DEVICE} 2>&1 |grep dns|awk '{print $3}')
		generate_dns_conf "${dns_server}"
		local_ip_config=$(ifconfig ${DEVICE} | grep "inet addr" | cut -c 11-)
		generate_ip_conf "${local_ip_config}"
	fi
}

wpa_cli_sta_check_status()
{
	## the max times to check wpa status
	local CONNECT_NUM_MAX=80
	local CHECK_INTERVAL=0.5

	local connect_num=0
	local connect_sleep_num=$((CONNECT_NUM_MAX/2))

	# OK                        : SCANNING -> 4WAY_HAND -> COMPLETED
	# Password Mismatch: SCANNING -> 4WAY_HAND -> DISCONNEC
	local WPA_STATUS_CMD=""
	local WPA_STATUS=""
	local WPA_STATUS_DID_AUTH=0

	while [ $connect_num -lt $CONNECT_NUM_MAX ]
	do
		WPA_STATUS_CMD=$(wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} status | grep wpa_state)
		WPA_STATUS=${WPA_STATUS_CMD#*wpa_state=}
		# echo "current wpa_status=${WPA_STATUS}"

		if [ "${WPA_STATUS}" = "COMPLETED" ]; then
			if [  "$(pgrep wpa_supplicant)" = "" ]; then
				echo "wpa_supplicant is not running"
				exit $ERRNO_ENV
			fi

			echo "Check time [${connect_num}/${CONNECT_NUM_MAX} x ${CHECK_INTERVAL}], wpa_status=${WPA_STATUS}"
			get_ip_address
			echo ">>>wifi_setup OK<<<"
			exit $ERRNO_OK
		fi

		# If status change from authenticating to scan, then password is incorrect
		if [ ${WPA_STATUS_DID_AUTH} -eq 0 ]; then
			if [ "${WPA_STATUS}" = "4WAY_HANDSHAKE" ] || [ "${WPA_STATUS}" = "GROUP_HANDSHAKE" ]; then
				echo "authenticating wpa_status=${WPA_STATUS}"
				WPA_STATUS_DID_AUTH=1
			fi
		else
			if [ "${WPA_STATUS}" = "SCANNING" ] || [ "${WPA_STATUS}" = "DISCONNECTED" ]; then
				echo "Status change from authenticating to ${WPA_STATUS}, password is incorrect"
				break
			fi
		fi

		connect_num=$((connect_num + 1));
		if [ $connect_num -gt $connect_sleep_num ]; then
			printf '.'
			sleep $CHECK_INTERVAL
		fi
	done

	kill_apps wpa_supplicant
	echo "wifi_setup failed"
	exit $ERRNO_PASSWORD_WRONG
}

wpa_cli_sta_action()
{
# stat wpa_cli  server to listen evernt
	wpa_cli -i${DEVICE} -a${STA_ACTION} -B
}

wpa_cli_sta()
{
	if [ $BG_CHECK_STA_STATUS -eq 1 ] && [ -x $STA_ACTION ]; then
		wpa_cli_sta_action
	else
		wpa_cli_sta_check_status
	fi
}

################  WPS ( PBC and PinCode ) #####################

generate_wpa_wps_conf()
{
	if [ -f ${CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return
	fi

	mkdir -p $DIR_CONFIG

	echo "ctrl_interface=${CTRL_INTERFACE}" > ${CONFIG}
	echo "update_config=${UPDATE_CONFIG_VALUE}" >> ${CONFIG}
}

start_wpa_supplicant_sta_wps()
{
	local local_wps_method=$1
	local local_wps_bssid=$2
	local local_wps_pincode=$3

	if [ "${local_wps_method}" != "wps_pbc" ] && [ "${local_wps_method}" != "wps_pin" ]; then
		echo "Unknown WPS method, should select [wps_pbc|wps_pin]"
		exit $ERRNO_PARAM
	fi

	is_bssid=$(echo "${local_wps_bssid}" | grep -E "[0-9a-f]{2}([-:])[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$")
	if [ "${local_wps_bssid}" != "any" ] && [ ! "${is_bssid}" ]; then
		echo "Invalid param, should input [<BSSID>|any]"
		exit $ERRNO_PARAM
	fi

	if [ "${local_wps_pincode}" -eq 0 ]; then
		local_wps_pincode=""
	else
		local len_pincode=${#local_wps_pincode}
		if [ $len_pincode -gt 8 ] || [ $len_pincode -lt 4 ]; then
			echo "Invalid WPS Pincode length ${local_wps_pincode} (expected: 4..8)"
			exit $ERRNO_PARAM
		fi
	fi

	echo "WPS-BSSID=${local_wps_bssid}, method=${local_wps_method}"

	ifconfig ${DEVICE} up
	CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -f${WPA_LOG} -B ${WPA_EXT_PARAM}"
	echo "CMD=${CMD}"
	$CMD

	ret_cmd=$(wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} "${local_wps_method}" "${local_wps_bssid}" ${local_wps_pincode})
	if [ "${local_wps_method}" = "wps_pin" ]; then
		echo "Pincode[${ret_cmd}]"
	fi

	if [ $STA_AUTOCONNECTION -eq 1 ]; then
		wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} sta_autoconnect 1
	fi
}
################  AP #####################

channel_to_frequency()
{
	local local_channel=$1
	## return gobal value sta_frequency and sta_channel

	if [ "${local_channel}" ]; then
		if [ "${local_channel}" -gt 196 ] || [ "${local_channel}" -lt 1 ]; then
			echo "Your Channel is wrong (1 ~ 196), using Channel ${DEFAULT_CHANNEL} by default."
			local_channel=$DEFAULT_CHANNEL
		fi
	else
		echo "Using Channel ${DEFAULT_CHANNEL} by default."
		local_channel=$DEFAULT_CHANNEL
	fi

	sta_channel=$local_channel
	if [ "$local_channel" -gt 14 ]; then
		sta_frequency=$((local_channel*5 + 5000))
	else
		sta_frequency=$((local_channel*5 + 2407))
	fi
}

generate_wpa_ap_conf()
{
	local	local_ssid=$1
	local	local_passwd=$2
	local	local_encrypt=$3
	local	local_frequency=$4

	if [ -f ${HOST_CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return
	fi

	mkdir -p $DIR_CONFIG
	echo "AP-SSID=${local_ssid}, Encryption=${local_encrypt}, Frequence=${local_frequency}"

	echo "ctrl_interface=${CTRL_INTERFACE}" > ${HOST_CONFIG}
	echo "ap_scan=2" >> ${HOST_CONFIG}

	## This for WiFi suspend/resume
	if [ $WOWLAN -eq 1 ]; then
		echo "wowlan_triggers=any" >> ${HOST_CONFIG}
	fi

	{
		echo "network={"
		echo "ssid=\"${local_ssid}\""
		echo "mode=2"
		echo "frequency=${local_frequency}"
	} >> ${HOST_CONFIG}

	if [ "${local_encrypt}" != "open" ]; then
		local len_passwd=${#local_passwd}
		local local_passwd_value=""

		if [ "$len_passwd" -eq 64 ]; then
			echo "passphrase length is 64, using hex type"
			local_passwd_value="${local_passwd}"
		elif [ "$len_passwd" -ge 8 ] && [ "$len_passwd" -le 63 ]; then
			local_passwd_value="\"${local_passwd}\""
		else
			echo "Invalid passphrase length ${len_passwd} (expected: 8..63 or 64 hex)"
			rm -rf ${HOST_CONFIG}
			exit $ERRNO_PARAM
		fi
	fi

	case ${local_encrypt} in
		open)
			echo "key_mgmt=NONE" >> ${HOST_CONFIG}
			;;
		wpa2)
			echo "key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "proto=WPA2" >> ${HOST_CONFIG}
			echo "pairwise=CCMP" >> ${HOST_CONFIG}
			echo "psk=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		wpa)
			echo "key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "proto=WPA" >> ${HOST_CONFIG}
			echo "pairwise=TKIP" >> ${HOST_CONFIG}
			echo "psk=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		wpawpa2)
			echo "key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "proto=WPA2" >> ${HOST_CONFIG}
			echo "pairwise=CCMP TKIP" >> ${HOST_CONFIG}
			echo "group=TKIP" >> ${HOST_CONFIG}
			echo "psk=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		*)
			echo "Please select AP encryption [open|wpa2|wpa|wpawpa2]"
			rm -rf ${HOST_CONFIG}
			exit $ERRNO_PARAM
			;;
	esac

	echo "}" >> ${HOST_CONFIG}
}

start_dhcp_server()
{
## Start DHCP Server ##
	mkdir -p /var/lib/misc
	mkdir -p /etc/dnsmasq.d
	dnsmasq -i${DEVICE_AP} --no-daemon --no-resolv --no-poll --dhcp-range=${DHCP_IP_START},${DHCP_IP_END},1h &
}

start_wpa_supplicant_ap()
{
## Setup interface and set IP,gateway##
	ifconfig ${DEVICE_AP} "${LOCAL_IP}" up
	route add default netmask ${LOCAL_NETMASK} gw "${LOCAL_IP}"
	CMD="wpa_supplicant -i${DEVICE_AP} -D${driver} -c${HOST_CONFIG} -f${WPA_LOG} -B ${WPA_EXT_PARAM}"
	echo "CMD=${CMD}"
	$CMD

	start_dhcp_server
	echo "HostAP Setup Finished."
}

################  Concurrent #####################

start_wpa_supplicant_concurrent()
{
	ifconfig ${DEVICE} up
	ifconfig ${DEVICE_AP} "${LOCAL_IP}" up
	route add default netmask ${LOCAL_NETMASK} gw "${LOCAL_IP}"
	CMD="wpa_supplicant -i${DEVICE} -D${driver} -c${CONFIG} -N -i${DEVICE_AP} -D${driver} -c${HOST_CONFIG} -f${WPA_LOG} -B ${WPA_EXT_PARAM}"
	echo "CMD=${CMD}"
	$CMD

	wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} select_network 0
	if [ $STA_AUTOCONNECTION -eq 1 ]; then
		wpa_cli -i${DEVICE} -p${CTRL_INTERFACE} sta_autoconnect 1
	fi
	start_dhcp_server
	echo "Concurrent Setup Finished."
}

start_bcmdhd_concurrent_ap()
{
	local	local_ssid=$1
	local	local_passwd=$2
	local	local_encrypt=$3
	local	local_channel=$4

	if [ "$(which dhd_helper)" = "" ]; then
		echo "Not Found dhd_helper tools"
		exit $ERRNO_ENV
	fi

	case ${local_encrypt} in
		open)
			CMD="dhd_helper iface ${DEVICE} ssid "${local_ssid}" hidden n chan ${local_channel} amode open emode none "
			;;
		wpa)
			local len_passwd=${#local_passwd}
			if [ "$len_passwd" -lt 8 ] || [ "$len_passwd" -gt 64 ]; then
				echo "Invalid passphrase length ${len_passwd} (expected: 8..63 or 64 hex)"
				exit $ERRNO_PARAM
			fi
			CMD="dhd_helper iface ${DEVICE} ssid "${local_ssid}" chan ${local_channel} amode wpapsk emode tkip key "${local_passwd}" "
			;;
		*)
			echo "Please select AP encryption [open|wpa]"
			exit $ERRNO_PARAM
			;;
	esac

	ifconfig ${DEVICE} up
	echo "CMD=${CMD}"
	$CMD

	## Change AP interface to wl0.1 when use broadcom Wifi
	DEVICE_AP=wl0.1
	ifconfig ${DEVICE_AP} "${LOCAL_IP}" up
	route add default netmask ${LOCAL_NETMASK} gw "${LOCAL_IP}"
	start_dhcp_server
}
################  WiFi Direct #####################

generate_wpa_p2p_conf()
{
	local local_ssid=$1

	if [ -f ${P2P_CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return
	fi
	mkdir -p $DIR_CONFIG
	local local_device_name=${local_ssid}
	if [ "${local_device_name}" = "" ]; then
		local_postmac=$(ifconfig ${DEVICE_P2P} | grep HWaddr | awk '{print $NF}' | sed 's/://g' | cut -c 6- | tr '[:upper:]' '[:lower:]')
		local_device_name=amba-${local_postmac}
	fi

	echo "P2P Device Name=${local_device_name}"
	echo "device_name=${local_device_name}" > ${P2P_CONFIG}
	{
		echo "ctrl_interface=${CTRL_INTERFACE}"
		echo "device_type=10-0050F204-5"
		echo "config_methods=display push_button keypad"
	} >> ${P2P_CONFIG}
}

start_wpa_supplicant_p2p()
{
	CMD="wpa_supplicant -i${DEVICE_P2P} -c${P2P_CONFIG} -D${driver} -f${WPA_LOG} -B ${WPA_EXT_PARAM}"
	echo "CMD=${CMD}"
	$CMD

	if [ -x ${P2P_ACTION} ]; then
		wpa_cli -p${CTRL_INTERFACE} -i${DEVICE_P2P} -a${P2P_ACTION}
	else
		echo "Not exist ${P2P_ACTION} file"
	fi
	wpa_cli -p${CTRL_INTERFACE} p2p_set ssid_postfix "_AMBA"
	wpa_cli -p${CTRL_INTERFACE} p2p_find
}

clear_config()
{
	rm -rf ${CONFIG}
	rm -rf ${HOST_CONFIG}
	rm -rf ${P2P_CONFIG}
	rm -rf ${IP_CONFIG}

	rm -rf ${WPA_LOG}
}

################   Main  ###################

## Show usage when no parameter
if [ $# -eq 0 ]; then
	usages
fi

## Check driver of wext or nl80211
if [ $# -gt 1 ]; then
	check_driver
fi

## Setup new config before clear old one
if [ $# -gt 2 ]; then
	clear_config
fi

echo "Version: ${WIFI_SETUP_VERSION}"
check_wpa_supplicant
get_device_id

case ${mode} in
	"stop")
		stop_wifi_app
		;;
	sta)
		sta_ssid=$3
		sta_passwd=$4
		sta_encrypt=$5
		check_iface "${DEVICE}"
		generate_wpa_sta_conf "${sta_ssid}" "${sta_passwd}" "${sta_encrypt}"
		start_wpa_supplicant_sta
		wpa_cli_sta
		;;
	wps)
		wps_bssid=$3
		wps_pincode=$4
		wps_method=$5
		check_iface "${DEVICE}"
		generate_wpa_wps_conf
		start_wpa_supplicant_sta_wps "${wps_method}" "${wps_bssid}" "${wps_pincode}"
		wpa_cli_sta
		;;
	ap)
		ap_ssid=$3
		ap_passwd=$4
		ap_encrypt=$5
		ap_channel=$6
		check_iface "${DEVICE_AP}"
		channel_to_frequency "${ap_channel}"
		generate_wpa_ap_conf "${ap_ssid}" "${ap_passwd}" "${ap_encrypt}" "${sta_frequency}"
		start_wpa_supplicant_ap
		;;
	con)
		sta_ssid=$3
		sta_passwd=$4
		sta_encrypt=$5
		ap_ssid=$6
		ap_passwd=$7
		ap_encrypt=$8
		check_iface "${DEVICE}"
		check_iface "${DEVICE_AP}"
		if [ "${MODULE_ID}" = "BCMDHD" ]; then
			## In Broadcom Wifi, AP channel will swith to the same channel as STA.
			generate_wpa_sta_conf "${sta_ssid}" "${sta_passwd}" "${sta_encrypt}"
			start_bcmdhd_concurrent_ap "${ap_ssid}" "${ap_passwd}" "${ap_encrypt}" ${sta_channel}
			start_wpa_supplicant_sta
			wpa_cli_sta
		else
			## AP Use the same Channel as STA
			generate_wpa_sta_conf "${sta_ssid}" "${sta_passwd}" "${sta_encrypt}"
			generate_wpa_ap_conf "${ap_ssid}" "${ap_passwd}" "${ap_encrypt}" ${sta_frequency}
			start_wpa_supplicant_concurrent
			wpa_cli_sta
		fi
		;;
	p2p)
		ap_ssid=$3
		check_iface "${DEVICE_P2P}"
		generate_wpa_p2p_conf "${ap_ssid}"
		start_wpa_supplicant_p2p
		;;
	*)
		echo "Please Select Mode [sta|ap|wps|con|p2p] or stop"
		exit $ERRNO_PARAM
esac
########################################
