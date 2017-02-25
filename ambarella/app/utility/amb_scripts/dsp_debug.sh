#!/bin/sh
#
# The check file can help the user debug the issue that dsp crash.
# It's used for S2(S2E), S2L, S3L, S5, and S5L.
#
# History:
#	2012/05/05 - [Jian Tang] Created file
#	2015/07/07 - [Jingyang Qiu] Add S3L support
#	2016/05/31 - [Jian Tang] Add S5 support
#	2016/12/13 - [Jian Tang] Add S5L support
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

IRQ_LAST_PIXEL="vin0_idsp_last_pixel"
IRQ_VCAP="vcap"
IRQ_VDSP="vdsp"

DSPLOG_FOLDER="/tmp"
REGISTER_FOLDER="/home/ambarella"
DSPLOG_AMBADEBUG_FILE="dsplog_ambadebug"
DSPLOG_BEFORE_FILE="dsplog_capture_before_hang"
DSPLOG_AFTER_FILE="dsplog_capture_after_hang"
DSPLOG_FILE_SIZE=10000000

KERNEL_VERSION=$(uname -r)
CHIP=$(grep AMBARELLA_ARCH /etc/ambarella.conf | cut -d'=' -f 2 | tr '[:lower:]' '[:upper:]')

glo_var=0

func_get_interrupts()
{
	local irq=$(grep "$1" /proc/interrupts | cut -d':' -f2 | grep -o "[0-9]*[0-9] ")
	local tmp=0
	for i in $irq ; do
		tmp=$((tmp+i))
	done
	glo_var=$tmp
}

func_dump_reg_error()
{
	echo "Invalid Chip ID! Do nothing."
}

func_dump_reg_s2()
{
	amba_debug -w 0x70118000 -d 0x1000 && amba_debug -r 0x70110000 -s 0x120 > ${REGISTER_FOLDER}/vin_register.txt
	amba_debug -r 0x70107C00 -s 0x120 > ${REGISTER_FOLDER}/0x70107C00_0x120.txt
	amba_debug -r 0x70118040 -s 0x220 > ${REGISTER_FOLDER}/0x70118040_0x220.txt
	# DSP PC address:
	amba_debug -r 0x70160020 -s 0x20 > ${REGISTER_FOLDER}/0x70160020_0x20.txt
	amba_debug -r 0x70160400 -s 0x410 > ${REGISTER_FOLDER}/0x70160400_0x410.txt
	amba_debug -r 0x70140028 -s 0x20 > ${REGISTER_FOLDER}/0x70140028_0x20.txt
	amba_debug -r 0x70150028 -s 0x20 > ${REGISTER_FOLDER}/0x70150028_0x20.txt
	amba_debug -r 0x70140800 -s 0x210 > ${REGISTER_FOLDER}/0x70140800_0x210.txt
	amba_debug -r 0x70150800 -s 0x210 > ${REGISTER_FOLDER}/0x70150800_0x210.txt
	dmesg > ${REGISTER_FOLDER}/dmesg.txt
	if [ "$KERNEL_VERSION" = "3.8.8" ] || [ "$KERNEL_VERSION" = "3.8.8+" ]; then
		cat /proc/ambarella/mode > ${REGISTER_FOLDER}/mode.txt
	else
		cat /proc/ambarella/clock > ${REGISTER_FOLDER}/clock.txt
	fi

	sleep 10
	amba_debug -r 0x80000 -s 0x20000 -f ${DSPLOG_FOLDER}/${DSPLOG_AMBADEBUG_FILE}.bin
	dsplog_cap -i ${DSPLOG_FOLDER}/${DSPLOG_AMBADEBUG_FILE}.bin -f ${REGISTER_FOLDER}/${DSPLOG_AMBADEBUG_FILE}.txt
}

func_dump_reg_s2l()
{
	amba_debug -w 0xec118000 -d 0x1000 && amba_debug -r 0xec110000 -s 0x120 > ${REGISTER_FOLDER}/vin_register.txt

	amba_debug -r 0xec160020 -s 0x20 > ${REGISTER_FOLDER}/0xec160020_0x20.txt
	amba_debug -r 0xec160200 -s 0x200 > ${REGISTER_FOLDER}/0xec160200_0x200.txt
	amba_debug -r 0xec150028 -s 0x20 > ${REGISTER_FOLDER}/0xec150028_0x20.txt
	amba_debug -r 0xec101c00 -s 0x80 > ${REGISTER_FOLDER}/0xec101c00_0x80.txt

	cat /proc/ambarella/iav > ${REGISTER_FOLDER}/iav.txt
	cat /proc/ambarella/clock > ${REGISTER_FOLDER}/clock.txt
}

func_dump_reg_s3l()
{
	amba_debug -w 0xec118000 -d 0x1000 && amba_debug -r 0xec110000 -s 0x120 > ${REGISTER_FOLDER}/vin_register.txt

	amba_debug -r 0xec160020 -s 0x20 > ${REGISTER_FOLDER}/0xec160020_0x20.txt
	amba_debug -r 0xec160400 -s 0x200 > ${REGISTER_FOLDER}/0xec160400_0x200.txt
	amba_debug -r 0xec150028 -s 0x20 > ${REGISTER_FOLDER}/0xec150028_0x20.txt
	amba_debug -r 0xec101c00 -s 0x100 > ${REGISTER_FOLDER}/0xec101c00_0x100.txt

	cat /proc/ambarella/iav > ${REGISTER_FOLDER}/iav.txt
	cat /proc/ambarella/clock > ${REGISTER_FOLDER}/clock.txt
}

func_dump_reg_s5()
{
	amba_debug -w 0xf4008000 -d 0x100 && amba_debug -r 0xf4000000 -s 0x120 > ${REGISTER_FOLDER}/vin_register.txt

	amba_debug -r 0xec130010 -s 0x40  > ${REGISTER_FOLDER}/0xec130010_0x40.txt
	amba_debug -r 0xec130800 -s 0x800 > ${REGISTER_FOLDER}/0xec130800_0x800.txt
	amba_debug -r 0xec010010 -s 0x40  > ${REGISTER_FOLDER}/0xec010010_0x40.txt
	amba_debug -r 0xec010800 -s 0x800 > ${REGISTER_FOLDER}/0xec010800_0x800.txt
	amba_debug -r 0xec160010 -s 0x1c0 > ${REGISTER_FOLDER}/0xec160010_0x1c0.txt
	amba_debug -r 0xec160800 -s 0x600 > ${REGISTER_FOLDER}/0xec160800_0x600.txt
	amba_debug -r 0xec06f000 -s 0x400 > ${REGISTER_FOLDER}/0xec06f000_0x400.txt
}

func_dump_reg_s5l()
{
	## S5L is almost same as S3L, reuse S3L function call
	func_dump_reg_s3l
}

func_dump_info()
{
	cat /proc/interrupts > ${REGISTER_FOLDER}/interrupt.txt
	killall  dsplog_cap
	date > ${REGISTER_FOLDER}/stop_date.txt
	dsplog_cap -m all -b all -l 3 -o ${DSPLOG_FOLDER}/${DSPLOG_AFTER_FILE}.bin -p ${DSPLOG_FILE_SIZE} &
	cat /proc/interrupts > ${REGISTER_FOLDER}/interrupt.txt
	sleep 2
	cat /proc/interrupts >> ${REGISTER_FOLDER}/interrupt.txt

	echo "Get ${CHIP} DSP info"
	if [ "$CHIP" = "S2" ] ; then
		func_dump_reg_s2
	elif [ "$CHIP" = "S2L" ] ; then
		func_dump_reg_s2l
	elif [ "$CHIP" = "S3L" ] ; then
		func_dump_reg_s3l
	elif [ "$CHIP" = "S5" ] ; then
		func_dump_reg_s5
	elif [ "$CHIP" = "S5L" ] ; then
		func_dump_reg_s5l
	else
		func_dump_reg_error
	fi

	cat /proc/meminfo > ${REGISTER_FOLDER}/meminfo.txt
	lsmod > ${REGISTER_FOLDER}/lsmod.txt
	cat /proc/interrupts >> ${REGISTER_FOLDER}/interrupt.txt
	test_encode --show-all > ${REGISTER_FOLDER}/test_encode_show_a.txt &
	load_ucode /lib/firmware/ > ${REGISTER_FOLDER}/ucode.txt
	cat /proc/interrupts >> ${REGISTER_FOLDER}/interrupt.txt
	echo "================== ${CHIP} DSP HANG =================="
	echo " DSP log binary by dsplog_cap is ${DSPLOG_FOLDER}/dsplog_capture_*****.bin "
	echo " All the debug info have been captured on the /home/ "
	echo " Please help to parse the ${DSPLOG_FOLDER}/dsplog_capture.bin in the follow command : "
	echo " dsplog_cap -i ${DSPLOG_FOLDER}/dsplog_capture.bin -f ${REGISTER_FOLDER}/dsplog_capture.txt "
	echo " Then send us the ${REGISTER_FOLDER}/dsplog_capture.txt and all the files on the /home "
	echo "================== DSP HANG =================="
	sleep 2
	dmesg > ${REGISTER_FOLDER}/dmesg.txt
	killall dsplog_cap
	dsplog_cap -i ${DSPLOG_FOLDER}/${DSPLOG_BEFORE_FILE}.bin.1 -f ${REGISTER_FOLDER}/${DSPLOG_BEFORE_FILE}.1.txt
	dsplog_cap -i ${DSPLOG_FOLDER}/${DSPLOG_BEFORE_FILE}.bin.2 -f ${REGISTER_FOLDER}/${DSPLOG_BEFORE_FILE}.2.txt
	dsplog_cap -i ${DSPLOG_FOLDER}/${DSPLOG_AFTER_FILE}.bin.1 -f ${REGISTER_FOLDER}/${DSPLOG_AFTER_FILE}.1.txt

	date > ${REGISTER_FOLDER}/end_date.txt
}

func_check_interrupt_s2l()
{
	func_get_interrupts ${IRQ_LAST_PIXEL}
	local last_pixel=$glo_var

	func_get_interrupts ${IRQ_VDSP}
	local vdsp=$glo_var

	sleep 1

	func_get_interrupts ${IRQ_LAST_PIXEL}
	local last_pixel2=$glo_var

	func_get_interrupts ${IRQ_VDSP}
	local vdsp2=$glo_var

	local diff_last_pixel=$((last_pixel2-last_pixel))
	local diff_vdsp=$((vdsp2-vdsp))

	local p2=${diff_last_pixel#-}
	local p3=${diff_vdsp#-}

	echo "last_pixel in 1s: $p2"
	echo "vdsp in 1s: $p3"

	if [ "$p2" -eq 0 ] || [ "$p3" -eq 0 ]; then
		func_dump_info
		exit 1
	fi
}

func_check_interrupt_others()
{
	func_get_interrupts ${IRQ_LAST_PIXEL}
	local last_pixel=$glo_var

	func_get_interrupts ${IRQ_VCAP}
	local vcap=$glo_var

	func_get_interrupts ${IRQ_VDSP}
	local vdsp=$glo_var

	sleep 1

	func_get_interrupts ${IRQ_LAST_PIXEL}
	local last_pixel2=$glo_var

	func_get_interrupts ${IRQ_VCAP}
	local vcap2=$glo_var

	func_get_interrupts ${IRQ_VDSP}
	local vdsp2=$glo_var

	local diff_last_pixel=$((last_pixel2-last_pixel))
	local diff_vcap=$((vcap2-vcap))
	local diff_vdsp=$((vdsp2-vdsp))

	local p2=${diff_last_pixel#-}
	local p3=${diff_vcap#-}
	local p4=${diff_vdsp#-}

	echo "last_pixel in 1s: $p2"
	echo "vcap in 1s: $p3"
	echo "vdsp in 1s: $p4"

	if [ "$p2" -eq 0 ] || [ "$p3" -eq 0 ] || [ "$p4" -eq 0 ]; then
		func_dump_info
		exit 1
	fi
}

func_check_interrupt()
{
	if [ "$CHIP" = "S2L" ]; then
		func_check_interrupt_s2l
	else
		func_check_interrupt_others
	fi
}


mkdir -p ${REGISTER_FOLDER}
date > ${REGISTER_FOLDER}/start_date.txt

dsplog_cap -m all -b all -l 3 -o ${DSPLOG_FOLDER}/${DSPLOG_BEFORE_FILE}.bin -p ${DSPLOG_FILE_SIZE} &

while true
do
func_check_interrupt
done

