#!/bin/sh
export PATH="$PATH:/bin:/sbin"
echo "wait ethernet connect" > /dev/ttyS0
while true;do
	ping 10.0.0.1 -c 3
	if [ "$?" = 0 ]; then
		break;
	fi
	sleep 2
done

echo "run bandwidth-arm in background" > /dev/ttyS0
/usr/local/bin/bandwidth-arm > /dev/zero &
/sbin/watchdog -T 5 /dev/watchdog
/usr/local/bin/init.sh $1
/usr/local/bin/test_shmoo --default

