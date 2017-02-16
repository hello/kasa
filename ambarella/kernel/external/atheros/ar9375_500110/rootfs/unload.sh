#!/bin/sh

USER=`whoami`

if [ $USER != "root" ]; then
        echo You must be 'root' to run the command
        exit 1
fi

echo "Stop DHCP..."
killall -q udhcpd
killall -q udhcpc

#
#    Stop wpa_supplicant
#
echo "Stop wpa_supplicant..."
killall -q wpa_supplicant
sleep 1
killall -q hostapd
sleep 1

#
echo "Uninstall WLAN Driver..."

rmmod b43 2> /dev/null
rmmod iwlagn 2> /dev/null

rmmod ath9k 2>/dev/null
rmmod ath9k_common 2>/dev/null
rmmod ath9k_hw 2>/dev/null
rmmod ath 2>/dev/null

rmmod mac80211 2>/dev/null
rmmod cfg80211 2>/dev/null

rmmod ath6kl_usb  2> /dev/null
rmmod cfg80211 2> /dev/null
rmmod compat_firmware_class.ko 2> /dev/null
rmmod compat 2> /dev/null

echo "Stop network manager..."
service network-manager stop 2>/dev/null

echo "Uninstallation completely!"
