#!/bin/sh
#
# History:
#	2016/01/05 - [Bin Wang] created file
#
# Copyright (c) 2016 Ambarella, Inc.
#
# This file and its contents ("Software") are protected by intellectual
# property rights including, without limitation, U.S. and/or foreign
# copyrights. This Software is also the confidential and proprietary
# information of Ambarella, Inc. and its licensors. You may not use, reproduce,
# disclose, distribute, modify, or otherwise prepare derivative works of this
# Software or any portion thereof except pursuant to a signed license agreement
# or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
# In the absence of such an agreement, you agree to promptly notify and return
# this Software to Ambarella, Inc.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
# MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
echo "Start to test img_svc! Make sure you run #apps_launcher -w firstly."
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "Part A. AE parameters test:"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo
#################################################
echo -e "\033[32m ********A.1 stop/start AE********\033[0m"
echo "# test_image_service_air_api --ae-enable 0"
test_image_service_air_api --ae-enable 0
echo "Now AE should stop"
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-enable 1"
test_image_service_air_api --ae-enable 1
echo "Now AE should start"
echo "......Press Enter to continue......"
read
#################################################
ae_metering_mode_begin=0
ae_metering_mode_end=3
echo -e "\033[32m ********A.2 AE metering mode********\033[0m"
echo "AE metering mode. 0: spot 1: center 2: average 3: custom."
ae_metering_mode=$ae_metering_mode_begin
while [ true ]
do
	echo "# test_image_service_air_api --ae-metering-mode $ae_metering_mode"
	test_image_service_air_api --ae-metering-mode $ae_metering_mode
	ae_metering_mode=$(($ae_metering_mode+1))
	echo "......Press Enter to continue......"
	read
	if [ $ae_metering_mode -gt $ae_metering_mode_end ]; then
		echo "# test_image_service_air_api --ae-metering-mode 2"
		test_image_service_air_api --ae-metering-mode 2
		echo "......Press Enter to continue......"
		read
		break;
	fi
done
#################################################
echo -e "\033[32m ********A.3 AE slow shutter enable/disable********\033[0m"
echo "# test_image_service_air_api --slow-shutter-mode 0"
test_image_service_air_api --slow-shutter-mode 0
echo "Now slow shutter should disable"
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --slow-shutter-mode 1"
test_image_service_air_api --slow-shutter-mode 1
echo "Now slow shutter should enable"
echo "......Press Enter to continue......"
read
#################################################
echo -e "\033[32m ********A.4 AE anti-flicker 50Hz/60Hz********\033[0m"
echo "AE anti flicker. 0: 50Hz, 1: 60Hz."
echo "# test_image_service_air_api --anti-flicker 0"
test_image_service_air_api --anti-flicker 0
echo "Now anti flicker should be 50Hz"
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --anti-flicker 1"
test_image_service_air_api --anti-flicker 1
echo "Now anti flicker should be 60Hz."
echo "......Press Enter to continue......"
read
#################################################
echo -e "\033[32m ********A.5 AE target ratio[25~400]********\033[0m"
echo "# test_image_service_air_api --ae-target-ratio 25"
test_image_service_air_api --ae-target-ratio 25
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-target-ratio 50"
test_image_service_air_api --ae-target-ratio 50
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-target-ratio 100"
test_image_service_air_api --ae-target-ratio 100
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-target-ratio 200"
test_image_service_air_api --ae-target-ratio 200
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-target-ratio 400"
test_image_service_air_api --ae-target-ratio 400
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ae-target-ratio 100"
test_image_service_air_api --ae-target-ratio 100
echo "......Press Enter to continue......"
read
#################################################
echo -e "\033[32m ********A.6 AE backlight compensation********\033[0m"
echo "backlight compensation enable/disable. 0: disable, 1: enable."
echo "# test_image_service_air_api --back-light-comp 1"
test_image_service_air_api --back-light-comp 1
echo "Now backlight compensation should be enabled"
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --back-light-comp 0"
test_image_service_air_api --back-light-comp 0
echo "Now backlight compensation should be disabled"
echo "......Press Enter to continue......"
read
#################################################
echo -e "\033[32m ********A.7 Local exposure********\033[0m"
echo "Local exposure. 0: disable, 1~3: strength. 3 is the strongest."
echo "# test_image_service_air_api --local-exposure 1"
test_image_service_air_api --local-exposure 1
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --local-exposure 2"
test_image_service_air_api --local-exposure 2
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --local-exposure 3"
test_image_service_air_api --local-exposure 3
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --local-exposure 0"
test_image_service_air_api --local-exposure 0
echo "......Press Enter to continue......"
read
#################################################
echo -e "\033[32m ********A.8 AE DC-Iris********\033[0m"
echo "DC-Iris enable/disable. 0: disable, 1: enable."
echo "# test_image_service_air_api --dc-iris-enable 1"
test_image_service_air_api --dc-iris-enable 1
echo "Now DC-Iris should be enabled"
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --dc-iris-enable 0"
test_image_service_air_api --dc-iris-enable 0
echo "Now DC-Iris should be disabled"
echo "......Press Enter to continue......"
read
#################################################
sensor_gain_max_begin=0
sensor_gain_max_end=48
sensor_gain_max_inc_step=6
echo -e "\033[32m ********A.9 AE sensor gain max********\033[0m"
echo "AE sensor gain max [0-48]."
sensor_gain_max=$sensor_gain_max_begin
while [ true ]
do
	echo "# test_image_service_air_api --sensor-gain-max $sensor_gain_max"
	test_image_service_air_api --sensor-gain-max $sensor_gain_max
	sensor_gain_max=$(($sensor_gain_max+$sensor_gain_max_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $sensor_gain_max -gt $sensor_gain_max_end ]; then
		echo
		break
	fi
done
#################################################
shutter_time_min_begin=8000
shutter_time_min_end=1000
shutter_time_min_inc_step=1000
echo -e "\033[32m ********A.10 AE shutter time min********\033[0m"
echo "AE shutter time min [1-]. Minimum sensor shutter time. 8000 means 1/8000 seconds"
shutter_time_min=$shutter_time_min_begin
while [ true ]
do
	echo "# test_image_service_air_api --shutter-time-min $shutter_time_min"
	test_image_service_air_api --shutter-time-min $shutter_time_min
	shutter_time_min=$(($shutter_time_min-$shutter_time_min_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $shutter_time_min -lt $shutter_time_min_end ]; then
		echo "# test_image_service_air_api --shutter-time-min 200"
		test_image_service_air_api --shutter-time-min 200
		echo "......Press Enter to continue......"
		read
		echo "# test_image_service_air_api --shutter-time-min 8000"
		test_image_service_air_api --shutter-time-min 8000
		echo "......Press Enter to continue......"
		read
		break
	fi
done
#################################################
shutter_time_max_begin=5
shutter_time_max_end=100
shutter_time_max_inc_step=10
echo -e "\033[32m ********A.11 AE shutter time max********\033[0m"
echo "AE shutter time max [1-]. Maximum sensor shutter time. 100 means 1/100 seconds"
shutter_time_max=$shutter_time_max_begin
while [ true ]
do
	echo "# test_image_service_air_api --shutter-time-max $shutter_time_max"
	test_image_service_air_api --shutter-time-max $shutter_time_max
	shutter_time_max=$(($shutter_time_max+$shutter_time_max_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $shutter_time_max -gt $shutter_time_max_end ]; then
		echo "# test_image_service_air_api --shutter-time-max 100"
		test_image_service_air_api --shutter-time-max 100
		echo "......Press Enter to continue......"
		read
		echo "# test_image_service_air_api --shutter-time-max 10"
		test_image_service_air_api --shutter-time-max 10
		echo "......Press Enter to continue......"
		read
		break
	fi
done
#################################################
echo -e "\033[32m ********A.12 IR LED mode********\033[0m"
echo "IR LED mode. 0: off, 1: on, 2: auto."
echo "# test_image_service_air_api --ir-led-mode 0"
test_image_service_air_api --ir-led-mode 0
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ir-led-mode 1"
test_image_service_air_api --ir-led-mode 1
echo "......Press Enter to continue......"
read
echo "# test_image_service_air_api --ir-led-mode 2"
test_image_service_air_api --ir-led-mode 2
echo "......Press Enter to continue......"
read
#################################################
sensor_gain_manual_begin=0
sensor_gain_manual_end=36
sensor_gain_manual_inc_step=6
echo -e "\033[32m ********A.13 AE sensor gain manual********\033[0m"
echo "sensor gain manual [0-36]."
echo "Disable AE first"
echo "# test_image_service_air_api --ae-enable 0"
test_image_service_air_api --ae-enable 0
sensor_gain_manual=$sensor_gain_manual_begin
while [ true ]
do
	echo "# test_image_service_air_api --sensor-gain-manual $sensor_gain_manual"
	test_image_service_air_api --sensor-gain-manual $sensor_gain_manual
	sensor_gain_manual=$(($sensor_gain_manual+$sensor_gain_manual_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $sensor_gain_manual -gt $sensor_gain_manual_end ]; then
		echo "Enable AE"
		echo "# test_image_service_air_api --ae-enable 1"
		test_image_service_air_api --ae-enable 1
		echo
		break
	fi
done
#################################################
shutter_time_manual_begin=100
shutter_time_manual_end=1000
shutter_time_manual_inc_step=100
echo -e "\033[32m ********A.14 AE shutter time manual********\033[0m"
echo "shutter time manual [1-8000]."
echo "Disable AE first"
echo "# test_image_service_air_api --ae-enable 0"
test_image_service_air_api --ae-enable 0
shutter_time_manual=$shutter_time_manual_begin
while [ true ]
do
	echo "# test_image_service_air_api --shutter-time-manual $shutter_time_manual"
	test_image_service_air_api --shutter-time-manual $shutter_time_manual
	shutter_time_manual=$(($shutter_time_manual+$shutter_time_manual_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $shutter_time_manual -gt $shutter_time_manual_end ]; then
		echo "Enable AE"
		echo "# test_image_service_air_api --ae-enable 1"
		test_image_service_air_api --ae-enable 1
		echo
		break
	fi
done

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "Part B. AWB parameters test:"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo
#################################################
awb_mode_begin=0
awb_mode_end=9
awb_mode_inc_step=1
echo -e "\033[32m ********B.1 AWB mode********\033[0m"
echo "awb-mode [0-9]. WB mode. 0:MW_WB_AUTO, 1:INCANDESCENT(2800K), 2:D4000, 3:D5000, 4:SUNNY(6500K), 5:CLOUDY(7500K), 6:FLASH, 7:FLUORESCENT, 8:FLUORESCENT_H, 9:UNDERWATER."
awb_mode=$awb_mode_begin
while [ true ]
do
	echo "# test_image_service_air_api --awb-mode $awb_mode"
	test_image_service_air_api --awb-mode $awb_mode
	awb_mode=$(($awb_mode+$awb_mode_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $awb_mode -gt $awb_mode_end ]; then
		echo "test_image_service_air_api --awb-mode 0"
		test_image_service_air_api --awb-mode 0
		echo
		break
	fi
done
#################################################

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "Part C. Image style test:"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo
#################################################
mctf_strength_begin=0
mctf_strength_end=11
mctf_strength_inc_step=1
echo -e "\033[32m ********C.1 MCTF strength********\033[0m"
echo "MCTF strength[0,11]. 0: off 1: weakest 11: strongest. "
mctf_strength=$mctf_strength_begin
while [ true ]
do
	echo "# test_image_service_air_api --mctf-strength $mctf_strength"
	test_image_service_air_api --mctf-strength $mctf_strength
	mctf_strength=$(($mctf_strength+$mctf_strength_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $mctf_strength -gt $mctf_strength_end ]; then
		echo "test_image_service_air_api --mctf-strength 6"
		test_image_service_air_api --mctf-strength 6
		echo
		break
	fi
done
#################################################
hue_begin=-2
hue_end=2
hue_inc_step=1
echo -e "\033[32m ********C.2 Hue********\033[0m"
echo "hue[-2,2]. -2: weakest 2: strongest. "
hue=$hue_begin
while [ true ]
do
	echo "# test_image_service_air_api --hue $hue"
	test_image_service_air_api --hue $hue
	hue=$(($hue+$hue_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $hue -gt $hue_end ]; then
		echo "test_image_service_air_api --hue 0"
		test_image_service_air_api --hue 0
		echo
		break
	fi
done
#################################################
saturation_begin=-2
saturation_end=2
saturation_inc_step=1
echo -e "\033[32m ********C.3 saturation********\033[0m"
echo "saturation[-2,2]. -2: weakest 2: strongest. "
saturation=$saturation_begin
while [ true ]
do
	echo "# test_image_service_air_api --saturation $saturation"
	test_image_service_air_api --saturation $saturation
	saturation=$(($saturation+$saturation_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $saturation -gt $saturation_end ]; then
		echo "test_image_service_air_api --saturation 0"
		test_image_service_air_api --saturation 0
		echo
		break
	fi
done
#################################################
sharpness_begin=-2
sharpness_end=2
sharpness_inc_step=1
echo -e "\033[32m ********C.4 sharpness********\033[0m"
echo "sharpness[-2,2]. -2: weakest 2: strongest. "
sharpness=$sharpness_begin
while [ true ]
do
	echo "# test_image_service_air_api --sharpness $sharpness"
	test_image_service_air_api --sharpness $sharpness
	sharpness=$(($sharpness+$sharpness_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $sharpness -gt $sharpness_end ]; then
		echo "test_image_service_air_api --sharpness 0"
		test_image_service_air_api --sharpness 0
		echo
		break
	fi
done
#################################################
brightness_begin=-2
brightness_end=2
brightness_inc_step=1
echo -e "\033[32m ********C.5 brightness********\033[0m"
echo "brightness[-2,2]. -2: weakest 2: strongest. "
brightness=$brightness_begin
while [ true ]
do
	echo "# test_image_service_air_api --brightness $brightness"
	test_image_service_air_api --brightness $brightness
	brightness=$(($brightness+$brightness_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $brightness -gt $brightness_end ]; then
		echo "test_image_service_air_api --brightness 0"
		test_image_service_air_api --brightness 0
		echo
		break
	fi
done
#################################################
contrast_begin=-2
contrast_end=2
contrast_inc_step=1
echo -e "\033[32m ********C.6 contrast********\033[0m"
echo "contrast[-2,2]. -2: weakest 2: strongest. "
contrast=$contrast_begin
while [ true ]
do
	echo "# test_image_service_air_api --contrast $contrast"
	test_image_service_air_api --contrast $contrast
	contrast=$(($contrast+$contrast_inc_step))
	echo "......Press Enter to continue......"
	read
	if [ $contrast -gt $contrast_end ]; then
		echo "test_image_service_air_api --contrast 0"
		test_image_service_air_api --contrast 0
		echo
		break
	fi
done
##################################################

echo "Test Done!"


