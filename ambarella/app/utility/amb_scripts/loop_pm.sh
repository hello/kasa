#!/bin/bash
##########################
# History:
#	2016/05/20 - [Tao Wu] Create file
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
###############

LOOP_PM_VERSION="0.2"

mode=$1
sleep_value=$2
gpio_id=$3

LOG_FILE=/root/count.log
MAX_NUM=9999999
num=0

usages()
{
	echo "Version: ${LOOP_PM_VERSION}"
	echo "This script used to do loop test in suspend/resume"
	echo "usage:      $0 [suspend|resume] <Sleep in sencond> <GPIO ID>"

	echo ""
	echo "Example:"
	echo "Suspend:    $0 suspend 1"
	echo "Resume :    $0 resume 5 120"

	echo ""
	echo "Notice: need find one free <GPIO ID> when do resume"
	exit 0
}

do_cmd_suspend()
{
	echo "suspend..."
	echo mem > /sys/power/state
	sleep "$sleep_value"
}

do_cmd_resume()
{
	if [ ! -e /sys/class/gpio/gpio"${gpio_id}" ]; then
		echo "${gpio_id}" > /sys/class/gpio/export
		if [ $? -ne 0 ]; then
			echo "Export GPIO[${gpio_id}] failed, please check it whether is used by other device"
			exit 1
		fi
		echo "Export GPIO[${gpio_id}] OK"
	fi
	if [ "$(cat /sys/class/gpio/gpio"${gpio_id}"/direction)" != "out" ]; then
		echo out > /sys/class/gpio/gpio"${gpio_id}"/direction
		echo "Set GPIO[${gpio_id}] as OUT OK"
	fi

	echo "resume..."
	echo 0 > /sys/class/gpio/gpio"${gpio_id}"/value
	sleep 1
	echo 1 > /sys/class/gpio/gpio"${gpio_id}"/value
	sleep "$sleep_value"
}

do_cmd()
{
	case ${mode} in
		suspend) do_cmd_suspend;;
		resume) do_cmd_resume;;
		*)
		echo "Unkown mode [${mode}], please select mode in [suspend|resume] "
		exit 1
	esac
}

## Main ##
if [ $# -eq 0 ]; then
	usages
fi

echo "Save log file[$LOG_FILE], Max test times[$MAX_NUM]. Starting..."
while [ $num -le $MAX_NUM ]; do

	echo "Count [${num}]"
	echo "Count [${num}]" > ${LOG_FILE}
	do_cmd

	num=$((num+1));
done
echo "Stop at count [${num}]"

