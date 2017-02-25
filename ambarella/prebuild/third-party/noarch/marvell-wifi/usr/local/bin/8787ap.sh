#!/bin/sh
ssid=$1 
passwd=$2
DEVICE=uap00
 
len=${#passwd}


if [ "$#" -ne 2 ]; then
echo "usage:  $0  SSID PASSWORD"
exit 1
fi

if [ $len -lt 8 ]; then
echo "usage:  password length at least 8"
exit 1
fi

rm -rf tmp.conf
killall -9 udhcpd

ifconfig uap0 up

iwpriv uap0 apcfg "ASCII_CMD=AP_CFG,SSID=$ssid,SEC=WPA2-PSK,KEY=$passwd"
uaputl bss_start

echo "start           192.168.0.20" >>tmp.conf
echo "end             192.168.0.254" >>tmp.conf
echo "interface       uap0" >>tmp.conf
echo "option  subnet  255.255.255.0" >>tmp.conf
echo "opt     router  192.168.0.1" >>tmp.conf
echo "option  domain  local" >>tmp.conf
echo "option  lease   864000" >>tmp.conf

ifconfig uap0 192.168.0.1
udhcpd ./tmp.conf 
sleep 1

echo "8787ap_setup finished "
