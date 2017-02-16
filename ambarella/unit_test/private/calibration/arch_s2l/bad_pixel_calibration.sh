#!/bin/sh

## History:
##		2013/04/26
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

CALIB_UTILS_SCRIPT=`which calib_utils.sh`
source calib_utils.sh

bpc_init_setting()
{
	echo;
	echo "============================================"
	echo "Start to do bad pixel calibration";
	echo "============================================"
	init_setting

	CALIBRATION_FILE_NAME="$width"x"$height"_cali_bad_pixel.bin
	CALIBRATION_FILE_NAME=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME}

	CALIBRATION_FILE_NAME_1="$width"x"$height"_cali_bad_pixel_1.bin
	CALIBRATION_FILE_NAME_1=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME_1}
	CALIBRATION_FILE_NAME_2="$width"x"$height"_cali_bad_pixel_2.bin
	CALIBRATION_FILE_NAME_2=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME_2}

	if [ -e ${CALIBRATION_FILE_NAME} ]; then
		rm -f ${CALIBRATION_FILE_NAME}
	fi
}

bpc_enter_preview()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: enter preview."
	echo "============================================"
	echo;
	test_image -i 0 &
	sleep 2
	init_vinvout_mode0
	sleep 2
#	start_then_stop_test_image
	killall -9 test_image
}

restore_with_vinvout_mode0()
{
	sleep 2
	test_image -i 0 &
	sleep 2
	if [ $bwidth -ge "1920" ]; then
		echo "# test_encode -i"$width"x"$height" -f 0 "$vout"480p "$tv_args" --enc-mode 0 $buff_id --bsize 1080p --bmaxsize 1080p "$src_buf_type""
		$test_encode_cmd -i"$width"x"$height" -f 0 "$vout"480p $tv_args --enc-mode 0 $buff_id --bsize 1080p --bmaxsize 1080p $src_buf_type
	else
		echo "# test_encode -i "$width"x"$height" --enc-mode 0 $buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" "$src_buf_type""
		$test_encode_cmd -i"$width"x"$height" --enc-mode 0 $buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" $src_buf_type
	fi
	sleep 3
	killall -9 test_image
	$cali_bad_pix_cmd -A$agc_index
	$cali_bad_pix_cmd -S1012
	echo "# cali_bad_pixel -r $width,$height"
	$cali_bad_pix_cmd -r $width,$height
	sleep 2
}

# Step 5: do bad pixel detection
detect_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 2: Detect bright bad pixel."
	echo "============================================"
	echo; echo -n "Please put on the lens cap, and then press enter key..."
	read keypress

	echo "# cali_bad_pixel -d $width,$height,160,160,0,$high_thresh,$low_thresh,3,$agc_index,1012 -f ${CALIBRATION_FILE_NAME_1}"
	$cali_bad_pix_cmd -d $width,$height,160,160,0,$high_thresh,$low_thresh,3,$agc_index,1012 -f ${CALIBRATION_FILE_NAME_1}
	echo; echo "bright bad pixel detection finished, result saved in ${CALIBRATION_FILE_NAME_1}"

	check_feedback
}

# Step 6: verify the calibration result
verify_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 3: Switch to enc-mode 0, Verify bad pixel effect."
	echo "============================================"

	echo;echo -n "Press enter key to restore bad pixel effect..."
	read keypress

	restore_with_vinvout_mode0
	echo;echo;echo -n "Now you can see the bright bad pixel, press enter key to continue..."
	read keypress
}

# Step 7: correct bad pixels
correct_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 4: Correct bright bad pixel, Verify bad pixel correction."
	echo "============================================"

	echo;echo -n "Press enter key to correct bad pixels......"
	read keypress

	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME_1}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME_1}

	echo; echo -n "Check the screen if bright bad pixels disappear, Press enter key to continue..."
	read keypress
	echo;echo;echo;
	echo "============================================"
	echo; echo -n "Start dark bad pixel, Press enter key to continue....."
	read keypress
}


# Step 8: do bad pixel detection
detect_dark_bad_pixel()
{

	echo;echo;echo;
	echo "============================================"
	echo "Step 5: Detect dark bad pixel."
	echo "============================================"
	echo; echo -n "Please remove the lens cap, and make the lens faced to a light box, and then press enter key..."
	read keypress

	echo "# cali_bad_pixel -d $width,$height,16,16,1,20,40,3,0,1400 -f ${CALIBRATION_FILE_NAME_2}"
	$cali_bad_pix_cmd -d $width,$height,16,16,1,20,40,3,0,1400 -f ${CALIBRATION_FILE_NAME_2}
	echo; echo "bad pixel detection finished, result saved in ${CALIBRATION_FILE_NAME_2}"

	check_feedback
}

# Step 9: verify the calibration result
verify_dark_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 6: Switch to enc-mode 0, Verify bad pixel effect."
	echo "============================================"
	echo; echo -n "Press enter key to restore bad pixel effect..."
	read keypress

	restore_with_vinvout_mode0
	echo;echo;echo -n "Now you can see the dark bad pixel, press enter key to continue..."
	read keypress
}

# Step 10: correct bad pixels
correct_dark_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 7: Correct dark bad pixel, Verify bad pixel correction."
	echo "============================================"
	echo; echo -n "Press enter key to correct bad pixels......"
	read keypress
	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME_2}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME_2}

	echo; echo -n "check the screen if dark bad pixels disappear, Press enter key to continue..."
	read keypress
}

finish_calibration()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 8: Gather all the bad pixels"
	echo "============================================"
	echo "# bitmap_merger -o $CALIBRATION_FILE_NAME_1,$CALIBRATION_FILE_NAME_2,$CALIBRATION_FILE_NAME"
	$bitmap_merger_cmd -o $CALIBRATION_FILE_NAME_1,$CALIBRATION_FILE_NAME_2,$CALIBRATION_FILE_NAME
	rm $CALIBRATION_FILE_NAME_1 $CALIBRATION_FILE_NAME_2 -rf
	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME}

	echo; echo "Thanks for using Ambarella calibration tool. "
	echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
}

#####main
check_cmd BPC
bpc_init_setting
init_sensor
sleep 1
bpc_enter_preview
detect_bright_bad_pixel
verify_bright_bad_pixel
correct_bright_bad_pixel
bpc_enter_preview
detect_dark_bad_pixel
verify_dark_bad_pixel
correct_dark_bad_pixel
finish_calibration
