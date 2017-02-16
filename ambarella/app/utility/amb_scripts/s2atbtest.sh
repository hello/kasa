#!/bin/sh

usbdev_atb_test=0
ethernet_atb_test=0
vin_atb_test=0
vout_atb_test=0
default_atb_test=0
uart1_protocol=0

adc7_path=/sys/class/hwmon/hwmon0/device/adc7
cfg_file_path=/tmp/mmcblk0p1/s2atbtest_cfg
md5_file_path=/tmp/mmcblk0p1/s2atbmd5

adcvalue=0
board_fpga=0
board_adv7619=0
uart1_timeout=10
sensor=""
vin_mode=""
vout_mode=""
encode_mode=""
#frame_rate=0
encode_frame_number_vin=0
encode_frame_number_vout=0

adv7619_timeout=10

led_0=2
led_1=207
led_2=17
led_3=5
led_4=76
led_5=36
led_6=32
led_7=143
led_8=0
led_9=4

led1_set()
{
	amba_debug -p i2c /dev/i2c-0 7 114 1 0 1 "$1"
	echo "$1"
}

led2_set()
{
	amba_debug -p i2c /dev/i2c-0 7 116 1 0 1 "$1"
}

test_usbdev()
{
	echo "usb device test start"
	led1_set "$led_1"
	dd if=/dev/zero of=/tmp/usb bs=1M count=64
	mkdir -p /mnt/usb
	mkfs.vfat -n atbusb /tmp/usb
	mount /tmp/usb /mnt/usb
	mkdir /mnt/usb/usb_test
	umount /mnt/usb
	modprobe g_mass_storage stall=0 removable=1 file=/tmp/usb
	case "$?" in
		0)
		;;
		*)
		echo "modprobe g_mass_storage failed!"
		return 1
		;;
	esac

	usbdev_timeout=10
	i=0
	while [ "$i" -lt "$usbdev_timeout" ]; do
		mount /tmp/usb /mnt/usb
		if [ -f /mnt/usb/usb_test/USB.OK ]; then
			break
		fi
		umount /mnt/usb
		i=$((i+1))
		sleep 1
	done

	if [ "$i" -ge "$usbdev_timeout" ]; then
		echo "can not get USB.OK file!"
		return 1
	fi

	echo "usb device test stop"
	return 0
}

test_ethernet()
{
	echo "ethernet test start"
	led1_set "$led_2"
	ifconfig eth0 10.0.0.2
	ping 10.0.0.1 -c 3 -W 2
	if [ "$?" != 0 ]; then
		return 1
	fi

	echo "ethernet test stop"
	return 0
}

set_gpio_direction()
{
	if [ -d /sys/class/gpio/gpio"$1" ]; then
		echo "$2" > /sys/class/gpio/gpio"$1"/direction
	else
		echo "$1" > /sys/class/gpio/export
		echo "$2" > /sys/class/gpio/gpio"$1"/direction
	fi
}

get_gpio_level()
{
	while read value
	do
		gpiolevel=$value
		#echo "$gpiolevel" "$value"
	done</sys/class/gpio/gpio"$1"/value
	
	if [ "$gpiolevel" -eq 0 ]; then
		return 0
	else
		return 1
	fi
}

set_gpio_level()
{
	echo "$2" > /sys/class/gpio/gpio"$1"/value
}

setboardtype()
{
if [ -f "$adc7_path" ]; then

	IFS_OLD=$IFS
	IFS=':'
	while read channel value
	do
		#echo "$channel is $value"
		adcvalue=$value
	done<"$adc7_path"
	#echo "output $adcvalue"
	#adcvalue=${adcvalue#* }
	adcvalue=$(($adcvalue))
	#echo "output $adcvalue"

	if [ $adcvalue -ge 4080 ]; then
		echo "fpga board"
		board_fpga=1
		if [ "$default_atb_test" = 1 ]; then
			usbdev_atb_test=1
			ethernet_atb_test=1
			vin_atb_test=1
			vout_atb_test=1
			uart1_protocol=1
		fi
	else
		echo "hdmi board"
		board_adv7619=1
		if [ "$default_atb_test" = 1 ]; then
			vout_atb_test=1
		fi
	fi
	IFS=$IFS_OLD
fi
}

config_uart1()
{
stty -F /dev/ttyS1 raw speed 115200
}

uart1_read()
{
	READ=`timeout -t "$uart1_timeout" dd if=/dev/ttyS1 count=5`
	if [ "$READ" = "$1" ]; then
		echo "#NEXT" > /dev/ttyS1
		return 0
	else
		echo "can not get DOSOK"
		return 1
	fi
}

uart1_write()
{
	echo "$1" > /dev/ttyS1
}

read_cfgfile()
{
	IFS_OLD=$IFS
	IFS=':'
	while read parameter value
	do
		#echo "$parameter" is "$value"
		case "$parameter" in
			sensor)
				sensor=$value
			;;
			vin_mode)
				vin_mode=$value
			;;
			vout_mode)
				vout_mode=$value
			;;
			encode_mode)
				encode_mode=$value
			;;
			encode_frame_number_vin)
				encode_frame_number_vin=$value
			;;
			encode_frame_number_vout)
				encode_frame_number_vout=$value
			;;
			*)
				echo "can not get correct parameter"
			;;
		esac
	done<"$cfg_file_path"
	IFS=$IFS_OLD
	#echo "$sensor" $vin_mode $vout_mode $encode_mode $encode_frame_number_vin $encode_frame_number_vout
}

md5_check()
{
	check=0
	md5_checkfile=""

	md5sum atb_encode.h264 > atb_encode.h264.md5
	while read checksum filename
	do
		echo encode md5 "$checksum"
		check=$checksum
	done<./atb_encode.h264.md5

	if [ -f "$md5_file_path" ]; then
		while read checksum filename
		do
			#echo "$checksum"
			if [ "$check" = "$checksum" ]; then
				echo "match!"
				return 0
			fi
		done<"$md5_file_path"
	else
		echo "can not get md5 file in sdcard"
	fi
	echo "no match md5 value"
	return 1
}

test_vin_fpga()
{
	led1_set "$led_3"
	init.sh --"$sensor"
	test_encode -i"$vin_mode" -V"$vout_mode" --hdmi -A -h"$encode_mode" --smaxsize "$encode_mode" -X --bsize "$encode_mode" --bmax "$encode_mode" --enc-mode 6 --sharpen-b 0
	test_image -l
	test_stream -f atb_encode --md5test "$encode_frame_number_vin" --verbose --only-filename&
	test_encode -A -e
	wait
	test_encode -A -s
}

config_vout()
{
	led1_set "$led_4"
	init.sh --na
	test2 -V2160p30 --hdmi --cs rgb
	amba_debug -M hdmi -w 0xc -d 0x7
	amba_debug -M rct -w 0x33c -d 0x08000000
	amba_debug -M hdmi -w 0x388 -d 0xff000006
}

test_vin_adv7619()
{
	led1_set "$led_5"
	init.sh --adv7619
	test_encode -i1920x2160 -V480p --hdmi -X --bsize 1920x2160 --bmax 1920x2160 --enc-mode 6 --raw-cap 1&
	wait
	test_encode -A -h1080p
	test_stream -f atb_encode --md5test "$encode_frame_number_vout" --verbose --only-filename&
	test_encode -A -e
	wait
	test_encode -A -s
}

usage()
{
	echo usage
	echo "-u	usbtest"
	echo "-e	ethernet"
	echo "--vin	vinencode test"
	echo "--vout	vouttest"
	echo "--default	s2atb test"
}

#echo param "$*" "$#"

if [ "$#" -lt 1 ]; then
	usage
	exit 1
fi
	

loop=0
while [ $# -ne 0 ]
do
	case "$1" in
		-u)
			usbdev_atb_test=1
		;;
		-e)
			ethernet_atb_test=1
		;;
		--vin)
			vin_atb_test=1
		;;
		--vout)
			vout_atb_test=1
		;;
		--default)
			default_atb_test=1
		;;
		*)
			echo "wrong"
		;;
	esac
	shift
done

modprobe ambad

set_gpio_direction 88 out
set_gpio_direction 89 out
set_gpio_direction 90 in
set_gpio_direction 91 in

setboardtype

if [ "$uart1_protocol" = 1 ]; then
	config_uart1
	uart1_write START
	uart1_read DOSOK
	if [ "$?" != 0 ]; then
		uart1_write uartfail
		exit 1
	fi
fi

if [ "$board_adv7619" = 1 ]; then
	echo "board adb7619"
elif [ "$board_fpga" = 1 ]; then
	echo "board fpga"
else
	echo "wrong board type"
	uart1_write adcwrong
	exit 1
fi

if [ "$usbdev_atb_test" = 1 ]; then
	test_usbdev
	if [ "$?" != 0 ]; then
		uart1_write usbfail
		exit 1
	fi
fi

if [ "$ethernet_atb_test" = 1 ]; then
	test_ethernet
	if [ "$?" != 0 ]; then
		uart1_write ethnetfail
		exit 1
	fi
fi

if [ -f "$cfg_file_path" ]; then
	read_cfgfile
else
	echo "can not get s2atbtest_cfg"
	uart1_write sdcardfail
	exit 1
fi

if [ "$vin_atb_test" = 1 ]; then
	test_vin_fpga
	md5_check
	if [ "$?" != 0 ]; then
		uart1_write vinfail
		exit 1
	fi
fi


if [ "$vout_atb_test" = 1 ]; then
	if [ "$board_fpga" = 1 ]; then
		config_vout
		i=0
		ret=0
		while [ "$i" -lt "$adv7619_timeout" ]; do
			get_gpio_level 91
			if [ "$?" -eq 1 ]; then
				echo gpio91
				ret=1
				break
			fi

			get_gpio_level 90
			if [ "$?" -eq 1 ]; then
				echo gpio90
				ret=0
				break
			fi

			i=$((i+1))
			sleep 1
		done
		
		if [ "$i" -eq "$adv7619_timeout" ]; then
			echo "get adv7619 return timeout"
			ret=1
		fi
		
		if [ "$ret" -eq 1 ]; then
			uart1_write adv7619vinfail
			exit 1
		fi
	elif [ "$board_adv7619" = 1 ]; then
		test_vin_adv7619
		md5_check
		if [ "$?" != 0 ]; then
			set_gpio_level 88 1
			exit 1
		else
			set_gpio_level 89 1
		fi
	else
		echo "no vin test for this board"
	fi
fi

led1_set "$led_6"
uart1_write PASS




