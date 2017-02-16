#!/bin/sh
##
## unit_test/private/iav_test/auto_test/script_encmode.sh
##
## History:
##    2014/08/05 - [Jian Tang] Created file
##
## Copyright (C) 2014-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.

TEST_ENCODE="/usr/local/bin/test_encode"

###### Common commands
CMD_MODE_0=$TEST_ENCODE" -i 0 -V480p -f30 --hdmi -X --bsize 1080p --bmaxsize 1080p -Y --bmaxsize 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmaxsize 720p -K --btype off --bsize 0x0  --enc-mode 0 -A --smaxsize 1080p"
CMD_MODE_0_B=$CMD_MODE_0		# Basic commands
CMD_MODE_0_H=$CMD_MODE_0		# HDR commands
CMD_MODE_1=$TEST_ENCODE"  -i0 -V480p --hdmi --enc-mode 1 -X --bsize 1080p --bmaxsize 1080p -w 1920 -W 1920 -P --bsize 1920x1080 --bins 1080p --enc-rotate-poss 0"
CMD_MODE_4_B=$TEST_ENCODE" -i 0 -V 480p --hdmi -X --bsize 1080p --bmax 1080p -J --btype prev --bsize 480p --bmax 480p -Y --bsize 480p --bmax 576p -K --btype enc --bsize 480p --bmax 480p --enc-mode 4 -A --smax 1080p"
CMD_MODE_4_H=$TEST_ENCODE" -i 1080p --hdr-mode 1 -V 480p --hdmi -X --bsize 1080p --bmax 1080p -J --btype prev --bsize 480p --bmax 480p -Y --bsize 480p --bmax 576p -K --btype enc --bsize 480p --bmax 480p --enc-mode 4 -A --smax 1080p"
CMD_MODE_5_2X=$TEST_ENCODE" -i 1080p --hdr-mode 1 -V480p --hdmi --mixer 0 --enc-mode 5 --hdr-expo 2 -f30 -X --bmaxsize 1080p --bsize 1080p -Y --bmaxsize 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmaxsize 720p -K --btype off --bsize 0x0 -A --smaxsize 1080p"
CMD_MODE_5_3X=$TEST_ENCODE" -i 1080p --hdr-mode 2 -V480p --hdmi --mixer 0 --enc-mode 5 --hdr-expo 3 -f30 -X --bmaxsize 1080p --bsize 1080p -Y --bmaxsize 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmaxsize 720p -K --btype off --bsize 0x0 -A --smaxsize 1080p"

####### OV5658 commands
OV5658_MODE_0_B=$TEST_ENCODE" -i 0 -f30 -V480p --hdmi -J --btype off -X --bsize 5m --bmax 5m -Y --bmax 576p --bsize 480p --btype enc -K --btype prev --bsize 480p  --enc-mode 0 -A --smax 5m"
OV5658_MODE_0_H=$OV5658_MODE_0_B
OV5658_MODE_1=$TEST_ENCODE"  -i0 -V480p --hdmi --enc-mode 1 -X --bsize 1080p --bmaxsize 1080p -w 1920 -W 1920 -P --bsize 1920x1080 --bins 2592x1944 --enc-rotate-poss 0"
OV5658_MODE_4=$TEST_ENCODE"  -i0 -V 480p --hdmi -X --bsize 5m --bmax 5m -J --btype prev --bsize 480p --bmax 480p -Y --bsize 480p --bmax 576p -K --btype enc --bsize 480p --bmax 480p --enc-mode 4 -A --smax 5m"
OV5658_MODE_5=$OV5658_MODE_0_B

####### OV4689 commands
OV4689_MODE_0_B=$TEST_ENCODE" -i 0 -V480p --hdmi -J --btype prev -X --bsize 2560x1440 --bmax 2560x1440 -Y --bmax 576p --bsize 480p --btype enc -K --btype enc --bsize 480p --enc-mode 0 -A --smax 2560x1440"
OV4689_MODE_0_H=$TEST_ENCODE" -i 2560x1440 --hdr-mode 1 -V480p --hdmi --hdr-expo 2 -X --bsize 2560x1440 --bmax 2560x1440 -Y --bmax 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmax 720p -K --btype off --bsize 0x0 --enc-mode 0 -A --smax 2560x1440"
OV4689_MODE_0_S=$TEST_ENCODE" -i 0 -V480p --hdmi -X --bsize 2560x1440 --bmax 2560x1440 -Y --bmax 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmax 720p -K --btype off --bsize 0x0 --enc-mode 0 -A --smax 2560x1440"
OV4689_MODE_1=$TEST_ENCODE"  -i0 -V480p --hdmi --enc-mode 1 -X --bsize 1080p --bmaxsize 1080p -w 1920 -W 1920 -P --bsize 1920x1080 --bins 2688x1512 --enc-rotate-poss 0"
OV4689_MODE_4_B=$TEST_ENCODE" -i 0 -V480p --hdmi -J --btype prev -X --bsize 2560x1440 --bmax 2560x1440 -Y --bmax 576p --bsize 480p --btype enc -K --btype enc --bsize 480p --enc-mode 4 -A --smax 2560x1440"
OV4689_MODE_4_H=$TEST_ENCODE" -i 2560x1440 --hdr-mode 1 -V480p --hdmi --hdr-expo 2 -X --bsize 2560x1440 --bmax 2560x1440 -Y --bmax 576p --bsize 480p --btype enc -J --btype prev --bsize 480p --bmax 720p -K --btype off --bsize 0x0 --enc-mode 4 -A --smax 2560x1440"
OV4689_MODE_5=$CMD_MODE_5_2X

####### MN34220 commands
MN34220_MODE_0_B=$TEST_ENCODE" -i 0 -V 480p --hdmi -J --btype prev -X --bsize 1080p --bmax 1080p -Y --bmax 576p --bsize 576p --btype enc -K --btype prev --bsize 480p --bmax 480p --enc-mode 0 -A --smax 1080p"
MN34220_MODE_0_H=$TEST_ENCODE" -i 0 --hdr-mode 1 -V 480p --hdmi --hdr-expo 2 -X --bsize 1080p --bmax 1080p -Y --bmax 576p --bsize 576p --btype enc -J --btype prev --bsize 480p -K --btype off --enc-mode 0 -A --smax 1080p"
MN34220_MODE_0_S=$MN34220_MODE_0_B


###### Global variables
RED="\033[31m "
GREEN="\033[32m "
MODE_NUM=6

shuffle_flag=0
sensor_mode=0		# 0 for sensor linear, 1 for sensor HDR
count=1


###### Global functions
func_usage()
{
	echo "Usage: $0 [sensor] [enc mode] [sensor mode] [count] "
	echo "Example:"
	echo "	OV4689 enter encode mode 1 once:        $0 ov4689  1"
	echo "	OV4689 enter encode mode 1 30 times:    $0 ov4689  1  linear  30"
	echo "	OV4689 Shuffle encode mode:             $0 ov4689  all  hdr  100"
	echo "	MN34220pl Shuffle encode mode:            $0 mn34220  all  linear  120"
	exit 0
}

func_print_result()
{
	if [ $1 -eq 0 ]
	then

		color=$GREEN
		result="PASS"
	else
		color=$RED
		result="FAIL"
	fi
	echo -e $color"Encode Mode Test $count: \tencode mode [$2]\t\t==>	$result \033[0m"
}

func_run_check()
{
	if [ $1 -eq 0 ]
	then
		if [ $2 -eq 0 ]
		then
			cmd=$CMD_MODE_0_B
		else
			cmd=$CMD_MODE_0_H
		fi
		mode=0
	elif [ $1 -eq 1 ]
	then
		if [ $2 -eq 0 ]
		then
			cmd=$CMD_MODE_1
			mode=1
		else
			cmd=$CMD_MODE_0_H
			mode=0
		fi
	elif [ $1 -eq 2 ]
	then
		cmd=$CMD_MODE_0_B
		mode=0
	elif [ $1 -eq 3 ]
	then
		cmd=$CMD_MODE_0_B
		mode=0
	elif [ $1 -eq 4 ]
	then
		if [ $2 -eq 0 ]
		then
			cmd=$CMD_MODE_4_B
		else
			cmd=$CMD_MODE_4_H
		fi
		mode=4
	elif [ $1 -eq 5 ]
	then
		if [ $2 -eq 0 ]
		then
			cmd=$CMD_MODE_0_B
			mode=0
		else
			cmd=$CMD_MODE_5_2X
			mode=5
		fi
	elif [ $1 -eq 6 ]
	then
		cmd=$CMD_MODE_5_3X
		mode=0
	else
		echo "Unknown encode mode $1"
		exit 255
	fi

	echo $cmd
	$cmd 1>/dev/null 2>&1

	if [ $? -eq 0 ]
	then
		func_print_result 0 $mode
	else
		func_print_result 1 $mode
		exit 255
	fi
}

func_main()
{
	while [ $count -gt 0 ]
	do
		if [ $shuffle_flag -eq 1 ]
		then
			id=`expr $RANDOM % $MODE_NUM`

			if [ $id -eq 0 ]
			then
				encode_mode=0
			elif [ $id -eq 1 ]
			then
				encode_mode=0
			elif [ $id -eq 2 ]
			then
				encode_mode=0
			elif [ $id -eq 3 ]
			then
				encode_mode=0
			elif [ $id -eq 4 ]
			then
				encode_mode=4
			elif [ $id -eq 5 ]
			then
				encode_mode=5
			elif [ $id -eq 6 ]
			then
				encode_mode=0
			fi
		fi
		func_run_check  $encode_mode  $sensor_mode
		count=`expr $count - 1`
	done
	exit 0
}

if [ $# -lt 2 ]
then
	func_usage
elif [ $# -gt 3 ]
then
	if [ $3 = linear ]
	then
		sensor_mode=0
	else
		sensor_mode=1
	fi

	if [ $# -eq 4 ]
	then
		count=$4
	fi
fi

if [ $1 = ov4689 ]
then
	CMD_MODE_0_B=$OV4689_MODE_0_B
	CMD_MODE_0_H=$OV4689_MODE_0_B
	CMD_MODE_1=$OV4689_MODE_1
	CMD_MODE_4_B=$OV4689_MODE_4_B
	CMD_MODE_4_H=$OV4689_MODE_4_H
	CMD_MODE_5_2X=$OV4689_MODE_5
	CMD_MODE_5_3X=$OV4689_MODE_5
elif [ $1 = ov5658 ]
then
	CMD_MODE_0_B=$MN34220_MODE_0_B
	CMD_MODE_0_H=$MN34220_MODE_0_H
	CMD_MODE_4_B=$OV5658_MODE_4
	CMD_MODE_4_H=$OV5658_MODE_4
elif [ $1 = mn34220 ]
then
	CMD_MODE_0_B=$MN34220_MODE_0_B
	CMD_MODE_0_H=$MN34220_MODE_0_H
fi

if [ $2 = all ]
then
	shuffle_flag=1
	if [ $# -lt 4 ]
	then
		func_usage
	fi
else
	shuffle_flag=0
	encode_mode=$2
fi

func_main

