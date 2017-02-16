#!/bin/sh
export PATH="$PATH:/bin:/sbin"
demoFile="/usr/local/bin/demo.sh"
sleep 1
echo "This is an example of systemd autorun." > /dev/ttyS0
if [ -f "$demoFile" ]; then
/usr/local/bin/demo.sh
fi