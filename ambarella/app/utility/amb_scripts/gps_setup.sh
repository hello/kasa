#!/bin/sh
##########################
# History:
#	2016/04/01 - [Tao Wu] Create file
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

GPS_SETUP_VERSION="0.1"

interface=$1
gpio_power=$2

RUN_DIR=/tmp/gps
APP=glgps_broadcom
APP_XML=gps_amba.xml

usages()
{
	echo "Version: ${GPS_SETUP_VERSION}"
	echo "This script used to Setup GPS"
	echo "usage:        $0 [Interface] [GPIO_PWR]"
	echo ""
	echo "Example:"
	echo "Setup GPS: $0 /dev/ttyUSB0 113"
	echo ""
	echo "NOTICE: Pleaset configurate the corresponding [PortName] [BaudRate] [GpioNStdbyPath] in /etc/gps/${APP_XML}."
	exit 0
}

check_env()
{
	if [ ! -e "${interface}" ]; then
		echo "${interface} is not exist."
		exit 1
	fi

	app_locate=$(which ${APP})
	if [ -z "${app_locate}" ]; then
		echo "${APP} is not exist."
		exit 1
	fi

	if [ ! -e /etc/gps/${APP_XML} ]; then
		echo "/etc/gps/${APP_XML} is not exist"
		exit 1
	fi
}

prepare_env()
{
	rm -rf ${RUN_DIR}
	mkdir ${RUN_DIR}
	mkdir ${RUN_DIR}/data
	mkdir ${RUN_DIR}/log
	mkdir ${RUN_DIR}/gnss
	mkdir ${RUN_DIR}/gnss/control
	chmod -R 777 ${RUN_DIR}
}

export_gpio_power()
{
	if [ ! -e /sys/class/gpio/gpio"$gpio_power" ]; then
		echo "$gpio_power" > /sys/class/gpio/export
		echo out > /sys/class/gpio/gpio"$gpio_power"/direction
		echo 0 > /sys/class/gpio/gpio"$gpio_power"/value
	fi
}

do_gps()
{
	ln -s "${app_locate}" ${RUN_DIR}/${APP}
	ln -s /etc/gps/${APP_XML} ${RUN_DIR}/${APP_XML}

	echo "============== ${RUN_DIR}/${APP_XML} ==============="
	head -n 7 ${RUN_DIR}/${APP_XML}| tail -n 3
	echo "=============================================="

	CMD="${RUN_DIR}/${APP} -c ${RUN_DIR}/${APP_XML} Periodic &"
	echo "CMD=${CMD}"
	cd ${RUN_DIR} ; $CMD
}
################   Main  ###################

## Show usage when no parameter
if [ $# -eq 0 ]; then
	usages
fi

check_env
prepare_env
export_gpio_power
do_gps

