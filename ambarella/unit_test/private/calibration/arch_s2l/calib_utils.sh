#!/bin/sh

## History:
##		2013/04/26
##		Author: Shawn Hu
##
## Copyright (C) 2011-2015, Ambarella, Inc.

CALIBRATION_FILES_PATH=/ambarella/calibration

## Amba Common cmds
test_encode_cmd=`which test_encode`
amba_debug_cmd=`which amba_debug`
test_image_cmd=`which test_image`

## BPC cmds
cali_bad_pix_cmd=`which cali_bad_pixel`
bitmap_merger_cmd=`which bitmap_merger`

## Lens Shading Calib cmds
cali_lens_shading_cmd=`which cali_lens_shading`

## Fish Eye Calib cmds
test_yuv_cmd=`which test_yuvcap`
cali_center_cmd=`which cali_fisheye_center`

## Piris Calib cmds
cali_piris_cmd=`which cali_piris`

## Awb Calib cmds
cali_awb_cmd=`which cali_awb`

# @param1: cmd path
# @param2: cmd name
exit_if_not_have()
{
	if [ "x$1" == "x" ]
	then
		echo "ABORT: could not find command $2 !!!"
		exit 1
	fi
}
# @param1: calibration type
check_cmd()
{
	case "$1" in
		BPC)
			exit_if_not_have $test_encode_cmd       "test_encode"
			exit_if_not_have $cali_bad_pix_cmd      "cali_bad_pixel"
			exit_if_not_have $bitmap_merger_cmd     "bitmap_merger"
			;;
		LSC)
			exit_if_not_have $test_encode_cmd       "test_encode"
			exit_if_not_have $cali_lens_shading_cmd "cali_lens_shading"
			;;
		FEC)
			exit_if_not_have $test_encode_cmd       "test_encode"
			exit_if_not_have $test_image_cmd        "test_image"
			exit_if_not_have $test_yuv_cmd          "test_yuv"
			exit_if_not_have $cali_center_cmd       "cali_center"
			;;
		PC)
			exit_if_not_have $test_encode_cmd       "test_encode"
			exit_if_not_have $test_image_cmd        "test_image"
			exit_if_not_have $cali_piris_cmd        "cali_piris"
			;;
		AWB)
			exit_if_not_have $test_encode_cmd       "test_encode"
			exit_if_not_have $cali_awb_cmd          "cali_awb"
			;;
		*)
			echo "$0: what kind of calibration?"
			exit 2
			;;
	esac

	mount -o remount,rw /
	mkdir -p $CALIBRATION_FILES_PATH
}

check_feedback()
{
	if [ $? -gt 0 ]; then
		exit 1
	fi
}

init_setting()
{
	accept=0
	while [ $accept != 1 ]
	do
		echo "1:  S2L(Hawthorn)"
		echo "2:  S2L-M(Kiwi)"
		echo; echo -n "Choose IDSP type:"
		read platform
		case $platform in
			1)
				dsp_type="S2L"
				accept=1
				tv_args="--hdmi"
				buff_id="-X"
				vout="-V"
				src_buf_type=
				;;
			2)
				dsp_type="S2LM"
				accept=1
				tv_args="--lcd digital601"
				buff_id="-K"
				vout="-v"
				src_buf_type="--btype prev"
				;;
			*)
				accept=0
				echo "please input valid DSP type"
				;;
		esac
	done

	accept=0
	while [ $accept != 1 ]
	do
		echo "1:  IMX123 3.2M sensor"
		echo "2:  IMX104 1M sensor"
		echo "3:  IMX178 6M sensor"
		echo "4:  MN34220 2M sensor"
		echo "5:  OV4689 4M sensor"
		echo "6:  IMX224 sensor"
		echo "7:  AR0230 sensor"
		echo; echo -n "choose sensor type:"
		read sensor_type
		case $sensor_type in
			1|2|3|4|5|6|7)
				accept=1
				;;
			*)
				accept=0
				echo;
				echo "Please input valid number, without any other keypress(backsapce or delete)!"
				;;
		esac
	done

	case "$sensor_type" in
		1	)
			sensor_name=imx123
			high_thresh=200
			low_thresh=200
			agc_index=640		# 30dB
			;;
		2	)
			sensor_name=imx104
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		3	)
			sensor_name=imx178
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		4	)
			sensor_name=mn34220pl
			high_thresh=200
			low_thresh=200
			agc_index=100
			;;
		5	)
			sensor_name=ov4689_2lane
			high_thresh=200
			low_thresh=200
			agc_index=256
			;;
		6	)
			sensor_name=imx224
			high_thresh=200
			low_thresh=200
			agc_index=256
			;;
		7	)
			sensor_name=ar0230
			high_thresh=200
			low_thresh=200
			agc_index=256
			;;
		*	)
			echo "sensor type unrecognized!"
			exit 1
			;;
	esac

	accept=0
	while [ $accept != 1 ]
	do
		echo "1:  2592x1944 (5MP)"
		echo "2:  2048x1536 (3MP)"
		echo "3:  1080p (2MP)"
		echo "4:  720p (1MP)"
		echo "5:  3840x2160 (8MP)"
		echo "6:  4096x2160 (8MP)"
		echo "7:  4000x3000 (12MP)"
		echo "8:  2304x1296 (3MP)"
		echo "9:  2304x1536 (3.4MP)"
		echo "a:  3072x2048 (6MP)"
		echo "b:  2560x2048 (5MP)"
		echo "c:  1536x1024 (1.6MP)"
		echo "d:  2688x1512 (4MP)"
		echo "e:  2048x1536 (3.2MP)"
		echo; echo -n "choose vin mode:"
		read vin_mode
		case $vin_mode in
			1|2|3|4|5|6|7|8|9|a|b|c|d|e)
				accept=1
				;;
			*)
				accept=0
				echo "Please input valid number, without \
any other keypress(backsapce or delete)!"
				;;
		esac
	done

	case "$vin_mode" in
		1	)
			width="2592"
			height="1944"
			bwidth="1920"
			bheight="1080"
			;;
		2	)
			width="2048"
			height="1536"
			bwidth="1920"
			bheight="1080"
			;;
		3	)
			width="1920"
			height="1080"
			bwidth="1920"
			bheight="1080"
			;;
		4	)
			width="1280"
			height="720"
			bwidth="1280"
			bheight="720"
			;;
		5	)
			width="3840"
			height="2160"
			bwidth="3840"
			bheight="2160"
			;;
		6	)
			width="4096"
			height="2160"
			bwidth="3840"
			bheight="2160"
			;;
		7	)
			width="4000"
			height="3000"
			bwidth="3840"
			bheight="2160"
			;;
		8	)
			width="2304"
			height="1296"
			bwidth="2304"
			bheight="1296"
			;;
		9	)
			width="2304"
			height="1536"
			bwidth="2304"
			bheight="1536"
			;;
		a	)
			width="3072"
			height="2048"
			bwidth="3072"
			bheight="2048"
			;;
		b	)
			width="2560"
			height="2048"
			bwidth="1920"
			bheight="1080"
			;;
		c	)
			width="1536"
			height="1024"
			bwidth="1536"
			bheight="1024"
			;;
		d	)
			width="2688"
			height="1512"
			bwidth="1920"
			bheight="1080"
			;;
		e	)
			width="2048"
			height="1536"
			bwidth="1920"
			bheight="1080"
			;;
		*	)
			echo "vin mode unrecognized!"
			exit 1
			;;
	esac

	if [ $vout == "-v" ]
	then
		bwidth="720"
		bheight="480"
	fi
}

init_sensor()
{
	echo "# init.sh --"$sensor_name""
	init.sh --"$sensor_name"
}

# enter preview with enc-mode=0
init_vinvout_mode0()
{
	# tweak: for s2lm first cvbs then hdmi to enter preview
	if [ $dsp_type == "S2LM" ]
	then
		$test_encode_cmd \
			-i"$width"x"$height" \
			-V480i \
			--cvbs \
			-X --bsize "$width"x"$height" --bmaxsize "$width"x"$height"
		sleep 4
	fi

	if [ $bwidth -gt "1920" ]; then
		echo "# test_encode \
-i"$width"x"$height" \
"$vout"480p "$tv_args" --enc-mode 0 \
"$buff_id" --bsize 1080p --bmaxsize 1080p \
--raw-capture 1 "$src_buf_type""

		$test_encode_cmd \
			-i"$width"x"$height" \
			"$vout"480p $tv_args --enc-mode 0 \
			$buff_id --bsize 1080p --bmaxsize 1080p \
			--raw-capture 1 $src_buf_type
	else
		echo "# test_encode \
-i"$width"x"$height" \
"$vout"480p "$tv_args" --enc-mode 0 \
"$buff_id" --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
--raw-capture 1 "$src_buf_type""

		$test_encode_cmd \
			-i"$width"x"$height" \
			"$vout"480p $tv_args --enc-mode 0 \
			$buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
			--raw-capture 1 $src_buf_type
	fi

	check_feedback
	sleep 1
}

# enter preview with enc-mode=2
init_vinvout_mode2()
{
	# tweak: for s2lm first cvbs then hdmi to enter preview
	if [ $dsp_type == "S2LM" ]
	then
		$test_encode_cmd \
			-i"$width"x"$height" \
			-V480i \
			--cvbs \
			-X --bsize "$width"x"$height" --bmaxsize "$width"x"$height"
		sleep 4
	fi

	echo "# test_encode \
-i"$width"x"$height" \
"$vout"480p "$tv_args" --enc-mode 2 \
"$buff_id" --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
--raw-capture 1 "$src_buf_type""

	$test_encode_cmd \
		-i"$width"x"$height" \
		"$vout"480p $tv_args --enc-mode 2 \
		$buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
		--raw-capture 1 $src_buf_type

	check_feedback
	sleep 1
}

# enter preview with enc-mode=3
init_vinvout_mode3()
{
	if [ $bwidth -ge "1920" ]; then
		echo \
"# test_encode \
-i"$width"x"$height" -f 15 \
"$vout"480p "$tv_args" --enc-mode 3 \
"$buff_id" --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
"$src_buf_type""

		$test_encode_cmd \
			-i"$width"x"$height" -f 15 \
			"$vout"480p $tv_args --enc-mode 3 \
			$buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
			$src_buf_type
	else
		echo \
"# test_encode \
-i"$width"x"$height" \
"$vout"480p "$tv_args" --enc-mode 3 \
"$buff_id" --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
"$src_buf_type""

		$test_encode_cmd \
			-i"$width"x"$height" \
			"$vout"480p $tv_args --enc-mode 3 \
			$buff_id --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" \
			$src_buf_type
	fi

	check_feedback
}

# start then stop test_image
start_then_stop_test_image()
{
	test_image -i 0 &
	sleep 3
	killall -9 test_image
}

