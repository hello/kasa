#!/bin/bash
#
# History:
#	2012/05/05 - [Jingyang Qiu] created file
#
# Copyright (c) 2015 Ambarella, Inc.
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

get_dsp()
{
	dsp=$(echo "$vin0_idsp" | cut -d':' -f2 | grep -o "[0-9]*[0-9] ")
	tmp=0
	for i in $dsp ; do
		tmp=$((tmp+i))
	done
	return $tmp
}

get_dsp2()
{
	dsp2=$(echo "$vin0_idsp" | cut -d':' -f3 | grep -o "[0-9]*[0-9] ")
	tmp=0
	for i in $dsp2 ; do
		tmp=$((tmp+i))
	done
	return $tmp
}

help()
{
	echo "USAGE:$0 threshhold value"
	echo "Example: $0 10"
	exit
}

creat_dir()
{
	if [ ! -d /home/ambarella ];then
		mkdir -p /home/ambarella
	fi
}

check_and_creat_file()
{
	random=$(cut -d'.' -f0 /proc/uptime)
	redirect=dsplog_$random
	while [ -e /home/ambarella/"$redirect" ];do
		echo "$redirect Exit file ,re-create!"
		random=$(cut -d'.' -f0 /proc/uptime)
		redirect=dsplog_$random
	done
	return 0
}

if [ -z "$1" ];then
	help
fi

if [ "$1" = "-h" ];then
	help
fi

echo "$1" | egrep '^[0-9]+$'
notdit=$?
if [ $notdit -eq 1 ];then
	echo "Please enter the number!"
	help
fi

redirect=
creat_dir
check_and_creat_file
redirect=/home/ambarella/$redirect
touch "$redirect"

flag=0
xflag=0
max_gap=0
uptime=$(cut -d' ' -f1 /proc/uptime | cut -d'.' -f1)
current=$(cut -d' ' -f1 /proc/uptime | cut -d'.' -f1)
while true
do
vin0_idsp=$(grep vin0_idsp /proc/interrupts)
if [ $? -eq 0 ]; then
	get_dsp
	dsp=$?
	get_dsp2
	dsp2=$?
	if [ $flag -eq 0 ];then
		if [ $dsp -gt $dsp2 ];then
			diff=$((dsp-dsp2))
			diff2=0
		else
			diff2=$((dsp2-dsp))
			diff=0
		fi
		echo "Start diff is ${diff#-}"
		flag=1
	fi
	dsp=$((dsp+diff2))
	dsp2=$((dsp2+diff))
	n=$((dsp-dsp2))
	p=${n#-}
	if [ $p -gt "$1" ];then
		xflag=0
		echo "CARE !! Difference between vin0_idsp and vin0_idsp_last_pixel is more than $1!!" >> "$redirect"
		cat /proc/interrupts >> "$redirect"
	fi
	current=$(cut -d' ' -f1 /proc/uptime| cut -d'.' -f1)
	if [ $p -gt $max_gap ]; then
		max_gap=$p
		echo "Current max diff is $max_gap" >> "$redirect"
		uptime=$(cut -d' ' -f1 /proc/uptime| cut -d'.' -f1)
	fi
	timediff=$((current-uptime))
	timediff=${timediff#-}
	if [ $timediff -gt 3600 ]; then
		if [ $xflag -eq 0 ]; then
			xflag=1
			echo "The intr counts diffs cannot increase after running for a long time" >> "$redirect"
		fi
	fi
else
	flag=0
fi
sleep 2
done
