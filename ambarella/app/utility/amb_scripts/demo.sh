#!/bin/sh

config_dir=/sys/module/ambarella_config/parameters
if [ -d $config_dir ]; then
	if [ -r $config_dir/board_chip ]; then
		board_chip=$(cat $config_dir/board_chip)
		board_type=$(cat $config_dir/board_type)
		board_rev=$(cat $config_dir/board_rev)
	else
		config_file=/etc/ambarella.conf
		board_chip=$(grep "ARCH" $config_file | awk -F \= '{print $2}')
	fi
else
	config_dir=/etc/ambarella.conf
	board_chip=$(awk -F \= '{print $2}' $config_dir | grep "s2lm_kiwi")
	if [ "$board_chip"x != "s2lm_kiwi"x ]; then
		board_chip=$(awk -F \= '{print $2}' $config_dir | grep "hawthorn")
	fi
fi

demo_default()
{
	/usr/local/bin/init.sh $1
	/usr/local/bin/test_encode -i0 -V480p --hdmi -X --bsize 720p
}

demo_i1()
{
	case "$board_type" in
		1)
		init_cmd="--ov14810"
		test2_cmd="--idle -vwvga --lcd td043 --disable-csc --fb 0 -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
		2)
		init_cmd="--na"
		test2_cmd="--idle -vD480x800 --lcd 1p3831 --disable-csc --rotate --fb 0 -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
		*)
		echo "Unknown $board_type"
		init_cmd="--na"
		test2_cmd="--idle -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
	esac

	case "$board_rev" in
		*)
		echo $board_rev
		;;
	esac

	/usr/local/bin/init.sh $init_cmd
	/usr/local/bin/test2 $test2_cmd

	demo_filename=/tmp/mmcblk0p1/Ducks.Take.Off.1080p.QHD.CRF25.x264-CtrlHD.mkv

	sd_empty=1
	while [ $sd_empty -ne 0 ]; do
		if [ -r $demo_filename ]; then
			echo "SD ready"
			sd_empty=0
		else
			sleep 2
			echo "SD empty"
		fi
	done

	loopcount=0
	while true; do
		loopcount=`expr $loopcount + 1`
		/usr/local/bin/pbtest $demo_filename > /dev/null 2>&1
		echo "=====Finish loop $loopcount=====" > /dev/ttyS0
		echo 3 > /proc/sys/vm/drop_caches
	done

	#reboot
}

demo_s2lm()
{
	echo "try to load it66121 external HDMI driver for S2Lm EVK board"
	if [ -r /lib/modules/$kernel_ver/extra/it66121.ko ]; then
	modprobe it66121
	fi
	echo "try to enter 480p preview by HDMI, and default VIN"
	/usr/local/bin/init.sh --mn34220pl
	test_encode -i0 -v480p --lcd digital601 -K --btype prev --bsize 480p --bmaxsize 480p -J --btype off
}

demo_s2l()
{
	/usr/local/bin/init.sh --mn34220pl
	/usr/local/bin/test_encode -i0 -V480p --hdmi -X --bsize 720p
}

demo_s2e()
{
	/usr/local/bin/init.sh --imx172
	/usr/local/bin/test_encode -i1080p -V480p --hdmi -X --bsize 1080p
}

case "$board_chip" in
	7200)
	demo_i1
	;;

	s2lm_kiwi)
	echo $board_chip
	demo_s2lm
	;;

	hawthorn)
	echo $board_chip
	demo_s2l
	;;

	s2)
	echo $board_chip
	demo_s2e
	;;

	*)
	echo $board_chip
	demo_default
	;;
esac

