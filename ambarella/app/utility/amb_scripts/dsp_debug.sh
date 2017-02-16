#!/bin/sh
# The check file can help the user debug the issue that dsp crash.
# It's used for S2(S2E) and S2L.
#

IRQ_LAST_PIXEL="vin0_idsp_last_pixel"
IRQ_VDSP="vdsp"

DSPLOG_FOLDER="/tmp"
REGISTER_FOLDER="/home/ambarella"

KERNEL_VERSION=`uname -r`
CHIP=`grep AMBARELLA_ARCH /etc/ambarella.conf | cut -d'=' -f 2 | tr [:lower:] [:upper:]`

glo_var=0
get_interrupts()
{
	irq=`cat /proc/interrupts | grep $1 | cut -d':' -f2 | grep -o "[0-9]*[0-9] "`
	tmp=0
	for i in $irq ; do
		tmp=$(($tmp+$i))
	done
	glo_var=$tmp
}

caplog_s2()
{
	amba_debug -w 0x70118000 -d 0x1000 && amba_debug -r 0x70110000 -s 0x100 > ${REGISTER_FOLDER}/vin_register.txt
    amba_debug -r 0x70118040 -s 0x90 > ${REGISTER_FOLDER}/0x70118040_0x90.txt
    amba_debug -r 0x70107C00 -s 0xB0 > ${REGISTER_FOLDER}/0x70107C00_0xB0.txt
    #DSP PC address:
    amba_debug -r 0x70160020 -s 0x20 > ${REGISTER_FOLDER}/0x70160020_0x20.txt
	dmesg > ${REGISTER_FOLDER}/dmesg.txt
	if [ $KERNEL_VERSION = "3.8.8" ] || [ $KERNEL_VERSION = "3.8.8+" ]; then
		cat /proc/ambarella/mode > ${REGISTER_FOLDER}/mode.txt
	else
		cat /proc/ambarella/clock > ${REGISTER_FOLDER}/clock.txt
	fi

	sleep 10
	amba_debug -r 0x80000 -s 0x20000 -f ${REGISTER_FOLDER}/dsplog_ambadebug.bin
}

caplog_s2l()
{
	amba_debug -w 0xec118000 -d 0x1000 && amba_debug -r 0xec110000 -s 0x120 > ${REGISTER_FOLDER}/vin_register.txt

	amba_debug -r 0xec160020 -s 0x20 > ${REGISTER_FOLDER}/0xec160020_0x20.txt
	amba_debug -r 0xec160200 -s 0x200 > ${REGISTER_FOLDER}/0xec160200_0x200.txt
	amba_debug -r 0xec150028 -s 0x20 > ${REGISTER_FOLDER}/0xec150028_0x20.txt
	amba_debug -r 0xec101c00 -s 0x80 > ${REGISTER_FOLDER}/0xec101c00_0x80.txt
	amba_debug -r 0x08696240 -s 0x240 > ${REGISTER_FOLDER}/sec1_0x08696240.bin
	amba_debug -r 0x08696000 -s 0x240 > ${REGISTER_FOLDER}/sec1_0x08696000.bin
	amba_debug -r 0xC8696240 -s 0x240 > ${REGISTER_FOLDER}/sec1_0xC8696240.bin
	amba_debug -r 0xC8696000 -s 0x240 > ${REGISTER_FOLDER}/sec1_0xC8696000.bin
	cat /proc/ambarella/iav > ${REGISTER_FOLDER}/iav.txt
	cat /proc/ambarella/clock > ${REGISTER_FOLDER}/clock.txt
}

check_interrupt()
{

	get_interrupts ${IRQ_LAST_PIXEL}
	last_pixel=$glo_var
	get_interrupts ${IRQ_VDSP}
	vdsp=$glo_var

	sleep 1

	get_interrupts ${IRQ_LAST_PIXEL}
	last_pixel2=$glo_var
	get_interrupts ${IRQ_VDSP}
	vdsp2=$glo_var

	diff_last_pixel=$(($last_pixel2-$last_pixel))
	diff_vdsp=$(($vdsp2-$vdsp))

	p2=${diff_last_pixel#-}
	p3=${diff_vdsp#-}


	echo "last_pixel in 1s: $p2"
	echo "vdsp in 1s: $p3"

	if [ $p2 -eq 0 ] || [ $p3 -eq 0 ]; then
		cat /proc/interrupts > ${REGISTER_FOLDER}/interrupt.txt
		sleep 5
		killall  dsplog_cap
		date > ${REGISTER_FOLDER}/stop_date.txt
		dsplog_cap -m all -b all -l 3 -o ${DSPLOG_FOLDER}/dsplog_capture_after_hang.bin -p 5000000 &
		cat /proc/interrupts > ${REGISTER_FOLDER}/interrupt.txt
		sleep 1
		cat /proc/interrupts >> ${REGISTER_FOLDER}/interrupt.txt

		echo "Get ${CHIP} DSP info"
		if [ ${CHIP} = "S2L" ]; then
			caplog_s2l
		else
			caplog_s2
		fi

		dmesg > ${REGISTER_FOLDER}/dmesg.txt
		cat /proc/meminfo > ${REGISTER_FOLDER}/meminfo.txt
		lsmod > ${REGISTER_FOLDER}/lsmod.txt
		cat /proc/interrupts >> ${REGISTER_FOLDER}/interrupt.txt
		test_encode --show-a > ${REGISTER_FOLDER}/test_encode_show_a.txt &
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
		killall dsplog_cap
		dsplog_cap -i ${DSPLOG_FOLDER}/dsplog_capture_before_hang.bin.1 -f ${REGISTER_FOLDER}/dsplog_capture_before_hang.1.txt
		dsplog_cap -i ${DSPLOG_FOLDER}/dsplog_capture_before_hang.bin.2 -f ${REGISTER_FOLDER}/dsplog_capture_before_hang.2.txt
		dsplog_cap -i ${DSPLOG_FOLDER}/dsplog_capture_after_hang.bin.1 -f ${REGISTER_FOLDER}/dsplog_capture_after_hang.1.txt

		date > ${REGISTER_FOLDER}/end_date.txt
		exit 1
	fi
}

mkdir -p ${REGISTER_FOLDER}
date > ${REGISTER_FOLDER}/start_date.txt

dsplog_cap -m all -b all -l 3 -o ${DSPLOG_FOLDER}/dsplog_capture_before_hang.bin -p 5000000 &

while [ true ]
do

check_interrupt
done

