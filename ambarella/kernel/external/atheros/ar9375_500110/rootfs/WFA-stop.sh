#!/bin/sh
#
#    WFA-stop.sh : Stop script for Wi-Fi Direct Testing.
#

#
#    Parameters
#

USER=`whoami`

if [ $USER != "root" ]; then
        echo You must be 'root' to run the command
        exit 1
fi


#
#    Stop WFD automation & wpa_cli action control
#
echo "=============Stop DHCP..."
killall -q udhcpd
killall -q udhcpc
echo "=============Stop wpa_cli action control..."
killall -q wpa_cli
echo "=============Stop WFD automation"
killall -s KILL -q sigma_dut
sleep 1

#
#    Stop wpa_supplicant
#
echo "=============Stop wpa_supplicant..."
killall -q wpa_supplicant
sleep 1
killall -q hostapd
sleep 1

#
echo "=============Uninstall Driver..."
rmmod ath6kl_usb  2> /dev/null
rmmod ath6kl_sdio 2> /dev/null
rmmod cfg80211 2> /dev/null
rmmod compat_firmware_class.ko 2> /dev/null
rmmod compat 2> /dev/null

echo "=============Done!"
