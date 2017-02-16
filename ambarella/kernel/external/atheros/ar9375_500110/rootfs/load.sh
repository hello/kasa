#!/bin/sh
#
#    WFA-start-usb-single.sh : Start script for Wi-Fi Direct Testing.
#

#
#    Parameters
#
TOPDIR=`pwd`

MODULE_PATH=${TOPDIR}/lib/modules

WPA_SUPPLICANT=${TOPDIR}/sbin/wpa_supplicant
WPA_CLI=${TOPDIR}/sbin/wpa_cli
SIGMA_DUT=${TOPDIR}/sbin/sigma_dut
IW=${TOPDIR}/sbin/iw

WFA_SCRIPTS_PATH=${TOPDIR}/home/atheros/Atheros-P2P/scripts
P2P_ACT_FILE=${WFA_SCRIPTS_PATH}/p2p-action.sh
P2P_DEV_CONF=${WFA_SCRIPTS_PATH}/p2pdev_dual.conf
WLAN_ACT_FILE=${WFA_SCRIPTS_PATH}/wlan-action.sh
WLAN_DEV_CONF=${WFA_SCRIPTS_PATH}/empty.conf
WPA_SUPPLICANT_ENTROPY_FILE=${WFA_SCRIPTS_PATH}/entropy.dat

ETHDEV=eth0
WLANPHY=
WLANDEV=
P2PDEV=p2p0

#
# some sanity checking
#
USER=`whoami`
if [ $USER != "root" ]; then
        echo You must be 'root' to run the command
        exit 1
fi

#
# detect the device existence
# Right now we assume this notebook has only one mmc bus
DEVICE=`lsusb | grep "0cf3:9374"`
DEVICE_9375=`lsusb | grep "0cf3:9375"`
DEVICE_9372=`lsusb | grep "0cf3:9372"`
if [ "$DEVICE" = "" -a "$DEVICE_9375" = "" -a "$DEVICE_9372" = "" ]; then
	echo You must insert device before running the command
	exit 2
fi

# disable rfkill
rfkill unblock all

#
# install driver
#
echo "=============Install Driver..."
insmod $MODULE_PATH/compat.ko
insmod $MODULE_PATH/compat_firmware_class.ko 2> /dev/null
insmod $MODULE_PATH/cfg80211.ko
insmod $MODULE_PATH/ath6kl_usb.ko ath6kl_p2p=0x19 debug_quirks=0x200
sleep 3

#
# detect the network device
#
if [ "$WLANDEV" = "" -a -e /sys/class/net ]; then
	for dev in `ls /sys/class/net/`; do
		if [ -e /sys/class/net/$dev/device/idProduct ]; then

			PID=`cat /sys/class/net/$dev/device/idProduct`
			if [ "$PID" = "9374" -o "$PID" = "9375" -o "$PID" = "9372" ]; then
				WLANDEV=$dev
			fi
		fi
		if [ -e /sys/class/net/$dev/phy80211/name ]; then
			WLANPHY=`cat /sys/class/net/$dev/phy80211/name`
		fi
	done
	if [ "$WLANDEV" = "" ]; then
		echo Fail to detect wlan device
		exit 3
	fi
fi


if [ "$WLANDEV" = "" ]; then
	WLANDEV=wlan0
	WLANPHY=phy0
fi

#${IW} dev ${WLANDEV} interface add ${P2PDEV} type managed

sleep 1
iwconfig $WLANDEV power off
iwconfig $P2PDEV  power off

#
# wlan device detected
#
echo WLAN_DEV:$WLANDEV 
echo P2P_DEV:$P2PDEV 
echo ETH_device: $ETHDEV

#
# configure detected wlan interface
ifconfig $WLANDEV up
${IW} dev $WLANDEV scan | grep SSID
${IW} dev $WLANDEV connect $1
sleep 5

# configure WLAN IP address
ifconfig $WLANDEV $2
#dhclient $WLANDEV

#
#    Start wpa_supplicant
#
#echo "=============Start wpa_supplicant..."

#echo "Start Command : ${WPA_SUPPLICANT} -Dnl80211 -i ${P2PDEV} -p use_multi_chan_concurrent=1 -c ${P2P_DEV_CONF} -N -Dnl80211 -i ${WLANDEV} -p use_multi_chan_concurrent=1 -c ${WLAN_DEV_CONF} -e ${WPA_SUPPLICANT_ENTROPY_FILE}"

#${WPA_SUPPLICANT} -Dnl80211 -i ${P2PDEV} -p use_multi_chan_concurrent=1 -c ${P2P_DEV_CONF} -N -Dnl80211 -i ${WLANDEV} -p use_multi_chan_concurrent=1 -c ${WLAN_DEV_CONF} -e ${WPA_SUPPLICANT_ENTROPY_FILE} &

sleep 1
echo "=============Done!"
