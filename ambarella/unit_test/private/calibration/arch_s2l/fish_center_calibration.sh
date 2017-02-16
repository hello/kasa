#!/bin/sh

## History:
##    2013/08/09 [Qian Shen] created file
##    2013/12/05 [Shupeng Ren] modified file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

source calib_utils.sh

fec_init_setting()
{
	echo;
	echo "============================================"
	echo "Start to do fisheye center calibration";
	echo "============================================"

  init_setting

	CALIBRATION_FILE_PREFIX=cali_fisheye_center
	CALIBRATION_FILE_PREFIX=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_PREFIX}
	CALIBRATION_FILE_NAME=${CALIBRATION_FILE_PREFIX}_prev_M_${width}x${height}.yuv

	if [ -e ${CALIBRATION_FILE_PREFIX} ]; then
		rm -f ${CALIBRATION_FILE_PREFIX}
	fi

}

fec_init_vinvout2()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: switch to encode mode 2."
	echo;echo "if VIN >= 8M, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;

 init_vinvout_mode2

{
$test_image_cmd -i1 <<EOF
e
e
120
EOF
} &
	sleep 3
}

# Step 5: capture yuv file
capture_yuv()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 2: Capture YUV file"
	echo "============================================"
	echo; echo -n "Please make sure the image is focused and cover the lens inside the light box, then press enter key to continue..."
	read keypress

	exe_cmd=$test_yuv_cmd" -b 0 -Y -f "$CALIBRATION_FILE_PREFIX
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 6: do circle center detection
detect_center()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 3: Detect circel center"
	echo "============================================"
	echo;
	exe_cmd=$cali_center_cmd" -f "${CALIBRATION_FILE_NAME}" -w "$width" -h "$height
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 7: Decide if do the test one more time
loop_calibration()
{
	again=1
	while [ $again != 0 ]
	do
		accept=0
		while [ $accept != 1 ]
		do
			echo; echo "Run one more test? (Y/N)"
			echo; echo -n "please choose:"
			read action
			case $action in
				y|Y)
					accept=1
					;;
				n|N)
					accept=1
					again=0
					;;
				*)
					;;
			esac
		done
		if [ $again != 0 ]; then
			capture_yuv
			detect_center
		fi
	done
}

finish_calibration()
{
	killall test_idsp
	killall test_image
	echo;echo;
	echo "============================================"
	echo "Step 4: Get the center."
	echo "============================================"
	echo;
	echo "If failed to find center, please check:"
	echo "	1. if the lens is a fisheye lens."
	echo "	2. if the lens is focused or mounted in the right way."
	echo "	3. if the light is not bright enough."
	echo; echo "Thanks for using Ambarella calibration tool. "
	echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
}

#####main
check_cmd FEC
fec_init_setting
init_sensor
sleep 1
fec_init_vinvout2
capture_yuv
detect_center
loop_calibration
finish_calibration

