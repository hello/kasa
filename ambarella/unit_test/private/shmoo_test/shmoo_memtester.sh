#!/bin/sh
# Program:
#	shmoo DLL
# History:
#
#HowTo
#	build memster in unit_test->linux->benchmark
#	edit your start value dll_start, shmoo_step, memtester lengh and confirm your preview frequency
#	edit your test command  at the tail of this file
#	it support S2L now, after change register address and interrupt name, we can support S2

PATH=/bin:\
/sbin:\
/usr/bin:\
/usr/local/bin:\
/root:
export PATH

DLL0_REG=0xec170090
DLL1_REG=0xec170094
DLL2_REG=0xec1700f0
DLL3_REG=0xec1700f4

shmoo_result_path=/sdcard/
shmoo_result_name=_result

dll_start=0x000000
shmoo_step=0x1
FREQ=30
#memtester length MB
test_length=1

for i in $(seq 5);do
	if [ ! -d $shmoo_result_path ] && [ $i -ne 5 ]; then
		echo Waiting for sdcard mounting in loop $i ...> /dev/ttyS0
		sleep 1
	elif [ ! -d  $shmoo_result_path ] && [ $i -eq 5 ]; then
		echo No sdcard found! > /dev/ttyS0
		exit 1
	elif [ -d $shmoo_result_path ]; then
		echo sdcard is found! > /dev/ttyS0
		break
	fi
done

modprobe ambad
watchdog -t 5 /dev/watchdog

init_byte0(){
	dll_start=$(($dll_start & 0xffffff00))
	dll0_forward_start=$dll_start
	dll0_forward_end=$(($dll0_forward_start + 0x0000001f))
	dll0_negative_start=$(($dll_start + 0x00000020))
	dll0_negative_end=$(($dll0_negative_start + 0x0000001f))
	dll0_step=$(($shmoo_step << 0))
	echo dll0_forward_start:0x`printf "%x" $dll0_forward_start` > /dev/ttyS0
	echo dll0_forward_end:0x`printf "%x" $dll0_forward_end` > /dev/ttyS0
	echo dll0_negative_start:0x`printf "%x" $dll0_negative_start` > /dev/ttyS0
	echo dll0_negative_end:0x`printf "%x" $dll0_negative_end` > /dev/ttyS0
	echo dll0_step:0x`printf "%x" $dll0_step` > /dev/ttyS0
}

init_byte1(){
	dll_start=$(($dll_start & 0xffff00ff))
	dll1_forward_start=$dll_start
	dll1_forward_end=$(($dll1_forward_start + 0x00001f00))
	dll1_negative_start=$(($dll_start + 0x00002000))
	dll1_negative_end=$(($dll1_negative_start + 0x00001f00))
	dll1_step=$(($shmoo_step << 8))
	echo dll1_forward_start:0x`printf "%x" $dll1_forward_start` > /dev/ttyS0
	echo dll1_forward_end:0x`printf "%x" $dll1_forward_end` > /dev/ttyS0
	echo dll1_negative_start:0x`printf "%x" $dll1_negative_start` > /dev/ttyS0
	echo dll1_negative_end:0x`printf "%x" $dll1_negative_end` > /dev/ttyS0
	echo dll1_step:0x`printf "%x" $dll1_step` > /dev/ttyS0
}

init_byte2(){
	dll_start=$(($dll_start & 0xff00ffff))
	dll2_forward_start=$dll_start
	dll2_forward_end=$(($dll2_forward_start + 0x001f0000))
	dll2_negative_start=$(($dll_start + 0x00200000))
	dll2_negative_end=$(($dll2_negative_start + 0x001f0000))
	dll2_step=$(($shmoo_step << 16))
	echo dll2_forward_start:0x`printf "%x" $dll2_forward_start` > /dev/ttyS0
	echo dll2_forward_end:0x`printf "%x" $dll2_forward_end` > /dev/ttyS0
	echo dll2_negative_start:0x`printf "%x" $dll2_negative_start` > /dev/ttyS0
	echo dll2_negative_end:0x`printf "%x" $dll2_negative_end` > /dev/ttyS0
	echo dll2_step:0x`printf "%x" $dll2_step` > /dev/ttyS0
}

set_dll_memtester(){
	value=0x`printf "%x" $1`
	echo value:"$value" > /dev/ttyS0
	amba_debug -w $DLL0_REG -d $value
	amba_debug -w $DLL1_REG -d $value
	amba_debug -w $DLL2_REG -d $value
	amba_debug -w $DLL3_REG -d $value

	memtester-arm $test_length 1
	if [ "$?" -eq 0 ]; then
		return 0
	else
		return 1
	fi
}

read_s2l_venc_interrupt(){
        while read irqno irqcnt gicORvic irqname
        do
                if [ "$irqname" = "vin0_idsp_sof" ]; then
                        cnt=$((irqcnt));
                        echo $cnt > /dev/ttyS0;
                fi
        done</proc/interrupts
}

check_s2l_venc(){
	read_s2l_venc_interrupt
	cnt_start=$cnt
	sleep 1
	read_s2l_venc_interrupt
	cnt_enc=$cnt
	int=`expr $cnt_enc - $cnt_start`
	if [ "$int" -ge "$FREQ" ]; then
		echo "$int" > /dev/ttyS0
		return 0
	fi
	return 1
}

shmoo_forward(){
	## shmoo forward direction
	eval dll=$"$1_forward_start"
	eval dll_max=$"$1_forward_end"
	eval dll_step=$"$1_step"
	#echo dll:0x`printf "%x" $dll`
	while [ 1 ]
	do
		if [ "$dll" -gt "$dll_max" ]; then
			break;
		fi
		cat "$shmoo_result_path""$1""$shmoo_result_name" | grep `printf "0x%x" $dll` > /dev/null
		if [ "$?" -eq 0 ]; then
			echo dll:0x`printf "%x" $dll` tested > /dev/ttyS0
		else
			echo 0x`printf "%x" $dll` : ...... >> "$shmoo_result_path""$1""$shmoo_result_name"
			echo "shmoo value 0x`printf "%x" $dll`" > /dev/ttyS0
			sync
			sleep 1
			set_dll_memtester $dll
			if [ "$?" -eq "0" ]; then
				check_s2l_venc
				sed '$d' -i "$shmoo_result_path""$1""$shmoo_result_name"
				if [ "$?" -eq "1" ]; then
					echo 0x`printf "%x" $dll` : dsp fail >> "$shmoo_result_path""$1""$shmoo_result_name"
					sleep 1
					reboot
					exit 2
				else
					echo 0x`printf "%x" $dll` : pass >> "$shmoo_result_path""$1""$shmoo_result_name"
				fi
			else
				sed '$d' -i "$shmoo_result_path""$1""$shmoo_result_name"
				echo 0x`printf "%x" $dll` : memtester fail >> "$shmoo_result_path""$1""$shmoo_result_name"
				sleep 1
				reboot
				exit 1
			fi
		fi
		dll=$(($dll+$dll_step))

	done
}

shmoo_negative(){
	## shmoo forward direction
	eval dll=$"$1_negative_start"
	eval dll_max=$"$1_negative_end"
	eval dll_step=$"$1_step"
	#echo dll:0x`printf "%x" $dll`
	while [ 1 ]
	do
		if [ "$dll" -gt "$dll_max" ]; then
			break;
		fi
		cat "$shmoo_result_path""$1""$shmoo_result_name" | grep `printf "0x%x" $dll` > /dev/null
		if [ "$?" -eq 0 ]; then
			echo dll:0x`printf "%x" $dll` tested > /dev/ttyS0
		else
			echo 0x`printf "%x" $dll` : ...... >> "$shmoo_result_path""$1""$shmoo_result_name"
			echo "shmoo value 0x`printf "%x" $dll`" > /dev/ttyS0
			sync
			sleep 1
			set_dll_memtester $dll
			if [ "$?" -eq "0" ]; then
				check_s2l_venc
				sed '$d' -i "$shmoo_result_path""$1""$shmoo_result_name"
				if [ "$?" -eq "1" ]; then
					echo 0x`printf "%x" $dll` : dsp fail >> "$shmoo_result_path""$1""$shmoo_result_name"
					sleep 1
					reboot
					exit 1
				else
					echo 0x`printf "%x" $dll` : pass >> "$shmoo_result_path""$1""$shmoo_result_name"
				fi
			else
				sed '$d' -i "$shmoo_result_path""$1""$shmoo_result_name"
				echo 0x`printf "%x" $dll` : memtester fail >> "$shmoo_result_path""$1""$shmoo_result_name"
				sleep 1
				reboot
				exit 1
			fi
		fi
		dll=$(($dll+$dll_step))
	done
}

shmoo(){
	if [ -f "$shmoo_result_path""$1""$shmoo_result_name" ]; then
		echo "continue shmoo test" > /dev/ttyS0
	else
		touch "$shmoo_result_path""$1""$shmoo_result_name"
	fi

	case "$1" in
		"dll0")
			init_byte0
			;;
		"dll1")
			init_byte1
			;;
		"dll2")
			init_byte2
			;;
		*)
			echo "do not support this byte" > /dev/ttyS0
			;;
	esac


	shmoo_forward $1
	shmoo_negative $1


        case "$1" in
			"dll0")
				dll0_p_max=$dll_p_max
				dll0_n_max=$dll_n_max
				;;
			"dll1")
				dll1_p_max=$dll_p_max
				dll1_n_max=$dll_n_max
				;;
			"dll2")
				dll2_p_max=$dll_p_max
				dll2_n_max=$dll_n_max
				;;
			*)
				echo "do not support this byte range" > /dev/ttyS0
				;;
        esac
}

echo "edit test command" > /dev/ttyS0
init.sh --imx178
cat /proc/ambarella/iav | grep S2LM
if [ "$?" -eq "0" ];then
	test_encode -i0 -f $FREQ -V480i --cvbs
else
	test_encode -i0 -f $FREQ -V480p --hdmi
fi
sleep 2
test_encode -A -h1080p -b0 --smaxsize 1080p -B -h480p -b1
test_tuning -a&
#test_stream --nofile&
test_encode -A -e -B -e
test_privacymask -r 8&
test_overlay -r 3&
#while [ 1 ]; do bandwidth-arm --quick; done > /dev/null &
echo "test command end" > /dev/ttyS0

shmoo dll0
shmoo dll1
shmoo dll2


shift=$(amba_debug -r 0xec170420 | awk '{print $2}')
echo shift value $shift  > /dev/ttyS0
positive_max_range=31
negative_max_range=$((0x3f - $shift))
echo positive max:$positive_max_range negitive max:0x`printf "%x" $negative_max_range`  > /dev/ttyS0

get_range(){
        while [ 1 ]
        do
                case "$1" in
                "dll0")
                        let "dll_check=$dll<<0"
                        ;;
                "dll1")
                        let "dll_check=$dll<<8"
                        ;;
                "dll2")
                        let "dll_check=$dll<<16"
                        ;;
                *)
                        ;;
                esac

                if [ "$dll" -gt "$2" ]; then
                        break;
                fi


                #cat "$shmoo_result_path"dll0"$shmoo_result_name" | grep "$dll : pass"
                cat /tmp/mmcblk0p1/$1_result | grep "`printf "0x%x" $dll_check` : pass" >> /dev/zero
                if [ "$?" -eq "0" ]; then
                        dll_max=$dll
                fi
                dll=$(($dll+1))
        done
}

dll=0
dll_max=0
get_range dll0 $positive_max_range
dll0_p_max=$dll_max
#echo dll0_p_max:$dll0_p_max

dll=32
dll_max=0
get_range dll0 $negative_max_range
dll0_n_max=$dll_max
#echo dll0_n_max:$dll0_n_max

dll=0
dll_max=0
get_range dll1 $positive_max_range
dll1_p_max=$dll_max
#echo dll1_p_max:$dll1_p_max

dll=32
dll_max=0
get_range dll1 $negative_max_range
dll1_n_max=$dll_max
#echo dll1_n_max:$dll1_n_max

dll=0
dll_max=0
get_range dll2 $positive_max_range
dll2_p_max=$dll_max
#echo dll2_p_max:$dll2_p_max

dll=32
dll_max=0
get_range dll2 $negative_max_range
dll2_n_max=$dll_max
#echo dll2_n_max:$dll2_n_max

dll0_n_max=$(($dll0_n_max - 0x20))
if [ $dll0_p_max -gt $dll0_n_max ]; then
	dll0_m=$(($dll0_p_max - $dll0_n_max))
	let "dll0_m=$dll0_m>>1"
else
	dll0_m=$(($dll0_n_max - $dll0_p_max))
	let "dll0_m=$dll0_m>>1"
	dll0_m=$(($dll0_m + 0x20))
fi
#echo dll0_m:0x`printf "%x" $dll0_m`

dll1_n_max=$(($dll1_n_max - 0x20))
if [ $dll1_p_max -gt $dll1_n_max ]; then
	dll1_m=$(($dll1_p_max - $dll1_n_max))
	let "dll1_m=$dll1_m>>1"
else
	dll1_m=$(($dll1_n_max - $dll1_p_max))
	let "dll1_m=$dll1_m>>1"
	dll1_m=$(($dll1_m + 0x20))
fi
#echo dll1_m:0x`printf "%x" $dll1_m`

dll2_n_max=$(($dll2_n_max - 0x20))
if [ $dll2_p_max -gt $dll2_n_max ]; then
        dll2_m=$(($dll2_p_max - $dll2_n_max))
        let "dll2_m=$dll2_m>>1"
else
        dll2_m=$(($dll2_n_max - $dll2_p_max))
        let "dll2_m=$dll2_m>>1"
        dll2_m=$(($dll2_m + 0x20))
fi
echo dll2_m:0x`printf "%x" $dll2_m` > /dev/ttyS0
echo dll1_m:0x`printf "%x" $dll1_m` > /dev/ttyS0
echo dll0_m:0x`printf "%x" $dll0_m` > /dev/ttyS0
