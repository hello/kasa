#!/bin/sh
#
# History:
#	2013/06/24 - [Tao Wu] Create file
#	2016/06/22 - [Tao Wu] Update file
#
# Copyright (c) 2015 Ambarella, Inc.
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

HOSTAP_VERSION="1.1.2"

encryption=$1
ssid=$2
passwd=$3
channel=$4

DEVICE=wlan0
DIR_CONFIG=/tmp/config

############# HOSTAP CONFIG ###############
WOWLAN=1
HOSTAP_CTRL_INTERFACE=/var/run/hostapd
HOST_CONFIG=$DIR_CONFIG/hostapd.conf
DEFAULT_CHANNEL=1
HOST_MAX_STA=5
AP_PIN=12345670
#DRIVER=nl80211

wpa_group_rekey=120
#############  DHCP Server ###############
LOCAL_IP=192.168.42.1
LOCAL_NETMASK=255.255.255.0
DHCP_IP_START=192.168.42.2
DHCP_IP_END=192.168.42.20

############# Exit Error Number ###############
ERRNO_OK=0
ERRNO_PARAM=1
ERRNO_ENV=2
#ERRNO_SSID_NOT_FOUND=3
#ERRNO_PASSWORD_WRONG=4

#####################################
usages()
{
	echo "Version: ${HOSTAP_VERSION}"
	echo "This script used to Setup/Stop WiFi AP mode with hostapd"
	echo "usage:             $0 [open|wpa2|wpa|wpawpa2|wps] <SSID> <Password> <Channel>"
	echo ""
	echo "Example:"
	echo "Setup AP[Open]:    $0  open               <SSID>     0      <Channel>"
	echo "Setup AP[Encrypt]: $0 [wpa2|wpa|wpawpa2]  <SSID> <Password> <Channel>"
	echo "Setup AP[WPA2+WPS]:$0  wps                <SSID> <Password> <Channel>"
	echo "      [Control AP] # hostapd_cli -i<interface> [wps_pbc | wps_pin any <PIN>] (PIN:${AP_PIN})"
	echo "Stop AP mode:      $0 stop"
	echo ""
	echo "NOTICE: Using interface AP[${DEVICE}] by default, change it if necessary."

	exit $ERRNO_OK
}

# kill process
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
	kill_apps udhcpc dnsmasq NetworkManager hostapd_cli hostapd
	ifconfig ${DEVICE} down
}

################ Driver #####################
:'
get_device_id()
{
	MODULE_ID=UNKNOWN
	if [ -e /sys/module/bcmdhd ]; then
		MODULE_ID=BCMDHD
	elif [ -e /sys/module/8189es ]; then
		MODULE_ID=RTL8189ES
	fi
}
'
################ AP Mode #####################
check_encrypt()
{
	if [ "${encryption}" != "open" ]; then
		len=${#passwd}
		if [ "$len" -lt 8 ]; then
			echo "Password length at least 8"
			exit 1
		fi
	fi
}

check_channel()
{
	if [ ${#channel} -gt 0 ]; then
		if [ "${channel}" -gt 196 ] || [ "${channel}" -lt 1 ]; then
			echo "Your Channel is wrong(1 ~ 196), using Channel ${DEFAULT_CHANNEL} by default."
			channel=$DEFAULT_CHANNEL
		fi
	else
		echo "No specified channel, using default channel ${DEFAULT_CHANNEL}."
		channel=$DEFAULT_CHANNEL
	fi
}

generate_hostapd_conf()
{
	if [ -f ${HOST_CONFIG} ]; then
		## Use the saved config, Do not need generate new config except failed to connect.
		return ;
	fi
	mkdir -p $DIR_CONFIG

	check_encrypt
	check_channel
	echo "AP: SSID[${ssid}], Password[${passwd}], Encryption[${encryption}], Channel[${channel}]."

	echo "interface=${DEVICE}" > ${HOST_CONFIG}
	echo "ctrl_interface=${HOSTAP_CTRL_INTERFACE}" >> ${HOST_CONFIG}
	#echo "driver=${DRIVER}" >> ${HOST_CONFIG}

	## This for WiFi suspend/resume
	if [ $WOWLAN -eq 1 ]; then
		echo "wowlan_triggers=any" >> ${HOST_CONFIG}
	fi

	{
	echo "beacon_int=100"
	echo "dtim_period=1"
	echo "preamble=0"
	echo "ssid=${ssid}"
	echo "max_num_sta=${HOST_MAX_STA}"
	} >> ${HOST_CONFIG}

	if [ ${channel} -gt 14 ]; then
		echo "hw_mode=a" >> ${HOST_CONFIG}
	fi
	echo "channel=${channel}" >> ${HOST_CONFIG}

	if [ "${encryption}" != "open" ]; then
		local len_passwd=${#passwd}
		local local_passwd_value=""

		if [ "$len_passwd" -eq 64 ]; then
			echo "passphrase length is 64, using hex type"
			local_passwd_value="${passwd}"
		elif [ "$len_passwd" -ge 8 ] && [ "$len_passwd" -le 63 ]; then
			local_passwd_value="${passwd}"
		else
			echo "Invalid passphrase length ${len_passwd} (expected: 8..63 or 64 hex)"
			rm -rf ${HOST_CONFIG}
			exit $ERRNO_PARAM
		fi
	fi

	case ${encryption} in
		open)
			;;
		wpa2)
			echo "wpa=2" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "wpa_pairwise=CCMP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		wpa)
			echo "wpa=1" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "wpa_pairwise=TKIP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		wpawpa2)
			echo "wpa=2" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "wpa_pairwise=TKIP CCMP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${local_passwd_value}" >> ${HOST_CONFIG}
			;;
		wps)
			echo "wpa=2" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			echo "wpa_pairwise=CCMP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${local_passwd_value}" >> ${HOST_CONFIG}

			echo "wps_state=2" >> ${HOST_CONFIG}
			echo "eap_server=1" >> ${HOST_CONFIG}
			echo "ap_pin=${AP_PIN}" >> ${HOST_CONFIG}
			echo "config_methods=label display push_button keypad ethernet" >> ${HOST_CONFIG}
			echo "wps_pin_requests=/var/run/hostapd.pin-req" >> ${HOST_CONFIG}
			;;
		*)
			echo "Not support encryption [$encryption]"
			exit $ERRNO_PARAM
			;;
	esac

	echo "wpa_group_rekey=$wpa_group_rekey" >> ${HOST_CONFIG}
	#echo "ignore_broadcast_ssid=0" >> ${HOST_CONFIG}
	#echo "ap_setup_locked=0" >> ${HOST_CONFIG}
}

start_dhcp_server()
{
## Start DHCP Server ##
	mkdir -p /var/lib/misc
	mkdir -p /etc/dnsmasq.d
	dnsmasq -i${DEVICE} --no-daemon --no-resolv --no-poll --dhcp-range=${DHCP_IP_START},${DHCP_IP_END},1h &
}

hostapd_start_ap()
{
	## Setup interface and set IP,gateway##
	echo "Using Interface AP:[${DEVICE}]"
	ifconfig ${DEVICE} ${LOCAL_IP} up
	route add default netmask ${LOCAL_NETMASK} gw ${LOCAL_IP}
	## Start Hostapd ##
	CMD="hostapd ${HOST_CONFIG} -B"
	echo "CMD=${CMD}"
	$CMD

	start_dhcp_server
	echo "HostAP Setup Finished."
}

clear_config()
{
	rm -rf ${HOST_CONFIG}
}

################   Main  ###################

## Show usage when no parameter
if [ $# -eq 0 ]; then
	usages
fi

## Start WiFi
if [ $# -gt 1 ]; then
	clear_config
fi

case ${encryption} in
	"stop")
		stop_wifi_app
		;;
	open|wpa2|wpa|wpawpa2|wps)
		generate_hostapd_conf
		hostapd_start_ap
		;;
	*)
		echo "Please Select Encryption [open|wpa2|wpa|wpawpa2|wps] or stop"
		exit $ERRNO_PARAM
		;;
esac
########################################
