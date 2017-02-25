#!/bin/sh
#
# History:
#	2016/03/23 - [ypxu] created file
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

config_dir=/sys/module/ambarella_config/parameters
if [ -d $config_dir ]; then
	if [ -r $config_dir/board_chip ]; then
		board_chip=$(cat $config_dir/board_chip)
#		board_type=$(cat $config_dir/board_type)
#		board_rev=$(cat $config_dir/board_rev)
	else
		config_file=/etc/ambarella.conf
		board_chip=$(grep "ARCH" $config_file | awk -F = '{print $2}')
	fi
else
	config_dir=/etc/ambarella.conf
	board_chip=$(awk -F = '{print $2}' $config_dir | grep "s3lm_antman")
	if [ "$board_chip"x != "s3lm_antman"x ]; then
		printf "%b\n" "\e[1;31m =====not s3lm antman board=====\e[0m" > /dev/ttyS0
	fi
fi

demo_default()
{
	/usr/local/bin/init.sh "$1"
	/usr/local/bin/test_encode -i0 -V480p --cvbs -X --bsize 720p
}

demo_s3lm_antman()
{
	/usr/local/bin/init.sh --ov2718_mipi
	if [ $? -eq 0 ]; then
		printf "%b\n" "\e[1;31m =====init ov2718 success=====\e[0m" > /dev/ttyS0
	else
		printf "%b\n" "\e[1;31m =====init ov2718 error=====\e[0m" > /dev/ttyS0
		exit 0;
	fi
	/usr/local/bin/test_tuning -a &
	/usr/local/bin/test_encode -i0 -V480i --cvbs --me0-scale 1 --enc-dummy-latency 5 -f30 -X --bsize 1080p --bmaxsize 1080p
	if [ $? -eq 0 ]; then
		printf "%b\n" "\e[1;31m =====enter preview success=====\e[0m" > /dev/ttyS0
	else
		printf "%b\n" "\e[1;31m =====enter preview error=====\e[0m" > /dev/ttyS0
		exit 0;
	fi
	/bin/sleep 1
	/usr/local/bin/test_encode -A -H 1080p --smaxsize 1080p
	/usr/local/bin/rtsp_server &
	/usr/local/bin/test_encode -A -e
	if [ $? -eq 0 ]; then
		printf "%b\n" "\e[1;31m =====enter encoding success=====\e[0m" > /dev/ttyS0
	else
		printf "%b\n" "\e[1;31m =====enter encoding error=====\e[0m" > /dev/ttyS0
		exit 0;
	fi
	/bin/sleep 1
	/usr/local/bin/test_smartrc -A -q1 -t0 -a1 -l1 &
	if [ $? -eq 0 ]; then
		printf "%b\n" "\e[1;31m =====enter smartrc success=====\e[0m" > /dev/ttyS0
	else
		printf "%b\n" "\e[1;31m =====enter smartrc error=====\e[0m" > /dev/ttyS0
		exit 0;
	fi
}

case "$board_chip" in

	s3lm_antman)
	echo "$board_chip"
	demo_s3lm_antman
	;;

	*)
	echo "$board_chip"
	demo_default "$@"
	;;
esac
