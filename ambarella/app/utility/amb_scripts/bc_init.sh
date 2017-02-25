#!/bin/sh

AMBARELLA_CONF=ambarella.conf

mkdir /mnt/SD
mount -t vfat /dev/mmcblk0p1 /mnt/SD

[ -r /etc/$AMBARELLA_CONF ] && . /etc/$AMBARELLA_CONF

kernel_ver=$(uname -r)

if [ -d /sys/module/ambarella_config/parameters ]; then
	config_dir=/sys/module/ambarella_config/parameters
fi

if [ -x /tmp/mmcblk0p1/ext-pro.sh ]; then
echo "==============================================="
echo "Exec: /tmp/mmcblk0p1/ext-pro.sh"
echo "==============================================="
/tmp/mmcblk0p1/ext-pro.sh
elif [ -x /home/default/ext-pro.sh ]; then
echo "==============================================="
echo "Exec: /home/default/ext-pro.sh"
echo "==============================================="
/home/default/ext-pro.sh
fi
case "$?" in
    0)
	;;
    *)
  	echo "Exec ext-pro.sh failed!"
	exit 1
	;;
esac

if [ -d /tmp/mmcblk0p1/lib/modules ]; then
echo "==============================================="
echo "Using ext modules: /tmp/mmcblk0p1/lib/modules"
echo "==============================================="
mount --bind /tmp/mmcblk0p1/lib/modules /lib/modules
elif [ -d /home/default/lib/modules ]; then
echo "==============================================="
echo "Using ext modules: /home/default/lib/modules"
echo "==============================================="
mount --bind /home/default/lib/modules /lib/modules
fi
case "$?" in
    0)
	;;
    *)
  	echo "Mount ext modules failed!"
	exit 1
	;;
esac

if [ -d /tmp/mmcblk0p1/lib/firmware ]; then
echo "==============================================="
echo "Using ext firmware: /tmp/mmcblk0p1/lib/firmware"
echo "==============================================="
mount --bind /tmp/mmcblk0p1/lib/firmware /lib/firmware
elif [ -d /home/default/lib/firmware ]; then
echo "==============================================="
echo "Using ext firmware: /home/default/lib/firmware"
echo "==============================================="
mount --bind /home/default/lib/firmware /lib/firmware
elif [ -d /lib/firmware/still ] && [ "$AMBARELLA_CALI" = "1" ]; then
echo "==============================================="
echo "Using stillcap firmware: /lib/firmware/still"
echo "==============================================="
mount --bind /lib/firmware/still /lib/firmware
fi
case "$?" in
    0)
	;;
    *)
  	echo "Mount ext firmware failed!"
	exit 1
	;;
esac

if [ "$SYS_BOARD_BSP" = "a2ipcam" ]; then
	echo "Note: iav module param can be changed to enable slow shutter and disable osd insert"
	modprobe iav disable_osd_insert=0
else
	modprobe iav
fi
case "$?" in
    0)
	;;
    *)
  	echo "Modprobe IAV failed!"
	exit 1
	;;
esac

modprobe ambtve
case "$?" in
    0)
	;;
    *)
  	echo "Modprobe ambtve failed!"
	exit 1
	;;
esac

if [ -r /lib/modules/"$kernel_ver"/extra/ambdbus.ko ]; then
	modprobe ambdbus
	case "$?" in
	    0)
		;;
	    *)
	  	echo "Modprobe ambdbus failed!"
		;;
	esac
fi

if [ -r /lib/modules/"$kernel_ver"/extra/ambhdmi.ko ]; then
        modprobe ambhdmi
        case "$?" in
            0)
                ;;
            *)
                echo "Modprobe ambhdmi failed!"
                ;;
        esac
fi

if [ -r /lib/modules/"$kernel_ver"/extra/vin.ko ]; then
        modprobe vin
        case "$?" in
            0)
              ;;
            *)
               echo "Modprobe vin failed!"
               exit 1
               ;;
        esac
fi

default_vin=gs2970a

if [ "$SYS_BOARD_BSP" = "s2ipcam" ]; then
	#export ext gpio(10) to use vin gpio reset
	echo 168 > /sys/class/gpio/export
fi

case $1 in
    --imx178 ) insmod /lib/modules/"$kernel_ver"/extra/imx178.ko ;;
    --imx178_spi ) insmod /lib/modules/"$kernel_ver"/extra/imx178.ko spi=1 bus_addr=0 ;;
    --gs2970 )
	printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/"$kernel_ver"/extra/gs2970.ko ;;
    --gs2970a )
        if [ -e $config_dir/board_vin0_vsync_irq_gpio ];then
	    printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
        fi
	insmod /lib/modules/"$kernel_ver"/extra/gs2970a.ko ;;
    --dummyfpga )
	printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/"$kernel_ver"/extra/dummyfpga.ko ;;
    --adv7441 )
	printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
	if [  -e /lib/firmware/EDID.bin ]; then
		insmod /lib/modules/"$kernel_ver"/extra/adv7443.ko edid_data=EDID.bin
	else
		insmod /lib/modules/"$kernel_ver"/extra/adv7443.ko
	fi
	;;
    --adv7441a )
	printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
	if [  -e /lib/firmware/EDID.bin ]; then
		insmod /lib/modules/"$kernel_ver"/extra/adv7441a.ko edid_data=EDID.bin
	else
		insmod /lib/modules/"$kernel_ver"/extra/adv7441a.ko
	fi
	;;

    --yuv480i ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=1 video_bits=8 video_mode=65520 cap_cap_w=720 cap_cap_h=480 video_phase=3 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=1;;
    --yuv480p ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65522 cap_cap_w=720 cap_cap_h=480 video_phase=3 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv576i ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=1 video_bits=8 video_mode=65521 cap_cap_w=720 cap_cap_h=576 video_phase=3 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=1;;
    --yuv576p ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65523 cap_cap_w=720 cap_cap_h=576 video_phase=3 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv720p ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65524 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=8541875 video_yuv_mode=3;;
    --yuv720p50 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65525 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv720p30 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=40 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv720p25 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=39 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv720p24 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=38 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=21333333 video_yuv_mode=3;;
    --yuv1080i ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_format=1 video_type=6 video_mode=65526 cap_cap_w=1920 cap_cap_h=1080 video_phase=2 video_yuv_mode=3 video_fps=8541875 ;;
    --yuv1080i50 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_format=1 video_bits=16 video_mode=65527 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv1080p30 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_mode=65533 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17066667 ;;
    --yuv1080p25 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_format=2 video_bits=16 video_mode=65532 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv1080p ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_mode=65528 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=148500000;;
    --yuv1080p50 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_mode=65529 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=10240000 video_pixclk=148500000;;
    --yuv1600p30 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_mode=12 cap_cap_w=1600 cap_cap_h=1200 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=27000000;;
    --mdin380 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=6 video_mode=12 cap_cap_w=1600 cap_cap_h=1200 video_phase=3 video_yuv_mode=3 video_fps=17083750 ;;
    --yuv1280_960p30 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=16 cap_cap_w=1280 cap_cap_h=960 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv1280_960p25 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=16 cap_cap_w=1280 cap_cap_h=960 video_phase=3 video_fps=20480000 video_yuv_mode=3;;

    --yuv480i_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=1 video_bits=8 video_mode=65520 cap_start_x=64 cap_start_y=18 cap_cap_w=720 cap_cap_h=480 video_phase=0 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=0;;
    --yuv480p_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65522 cap_start_x=60 cap_start_y=36 cap_cap_w=720 cap_cap_h=480 video_phase=0 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv576i_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=1 video_bits=8 video_mode=65521 cap_start_x=60 cap_start_y=22 cap_cap_w=720 cap_cap_h=576 video_phase=0 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=0;;
    --yuv576p_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65523 cap_start_x=66 cap_start_y=44 cap_cap_w=720 cap_cap_h=576 video_phase=0 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv720p_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65524 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=8541875 video_yuv_mode=3;;
    --yuv720p50_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65525 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv720p30_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=40 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv720p25_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=39 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv720p24_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=38 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=21333333 video_yuv_mode=3;;
    --yuv1080i_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_format=1 video_type=1 video_mode=65526 cap_start_x=192 cap_start_y=20 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=8541875 ;;
    --yuv1080i50_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=1 video_bits=16 video_mode=65527 cap_start_x=192 cap_start_y=20 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv1080p30_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_mode=65533 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17066667 ;;
    --yuv1080p25_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65532 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv1080p_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_mode=65528 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=148500000;;
    --yuv1080p50_601 ) insmod /lib/modules/"$kernel_ver"/extra/ambdd.ko video_type=1 video_mode=65529 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=10240000 video_pixclk=148500000;;

    --na )
	printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
	echo "Init without VIN drivers..." ;;
    --gps )
    echo " Load default Touch Screen Driver "
    insmod /lib/modules/"$kernel_ver"/kernel/drivers/input/touchscreen/ak4183.ko
	insmod /lib/modules/"$kernel_ver"/extra/$default_vin.ko
    ;;
    --ak4183 )
    echo " Load ak4183 Touch Screen Driver "
    insmod /lib/modules/"$kernel_ver"/kernel/drivers/input/touchscreen/ak4183.ko
	insmod /lib/modules/"$kernel_ver"/extra/$default_vin.ko
    ;;
    --chacha_mt4d )
    echo " Load chacha_mt4d Touch Screen driver "
    insmod /lib/modules/"$kernel_ver"/kernel/drivers/input/touchscreen/chacha_mt4d.ko
	insmod /lib/modules/"$kernel_ver"/extra/$default_vin.ko
    ;;
    *)
        if [ -e $config_dir/board_vin0_vsync_irq_gpio ];then
	    printf "%d\n" -1 > $config_dir/board_vin0_vsync_irq_gpio
        fi
	echo "Default method to init without VIN drivers..."
	#echo "Use $default_vin as default."
	echo "Cat this file to find out what VIN can be supported."
	#insmod /lib/modules/"$kernel_ver"/extra/$default_vin.ko
	;;
esac
case "$?" in
    0)
	;;
    *)
  	echo "Insmod $1 failed!"
	exit 1
	;;
esac

case "$2" in
    --m13vp288ir ) insmod /lib/modules/"$kernel_ver"/extra/m13vp288ir.ko ;;
    *)
        echo "Default init without lens driver"
        ;;
esac
case "$?" in
    0)
        ;;
    *)
        echo "Insmod $2 failed!"
        exit 1
        ;;
esac

if [ -r /lib/modules/"$kernel_ver"/extra/ambad.ko ]; then
	modprobe ambad
fi

if [ -r /lib/modules/"$kernel_ver"/extra/fdet.ko ]; then
	modprobe fdet
fi

if [ -r /lib/modules/"$kernel_ver"/extra/mpu6000.ko ]; then
	modprobe mpu6000
fi

if [ -r /lib/modules/"$kernel_ver"/extra/eis.ko ]; then
	modprobe eis
fi

case $2 in
	--high )
	/usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110100 && /usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110101 && /usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110100
	echo "Change iDSP to 147 MHz"
	/usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111000 && /usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111001 && /usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111000
	echo "Change DDR2 to 348 MHz"
	;;
	*)
	echo "Use default settings"
	;;
esac

LOAD_UCODE=$(which load_ucode)
if [  -e "$LOAD_UCODE" ]; then
$LOAD_UCODE /lib/firmware
case "$?" in
	0)
	;;
	*)
  	echo "Load ucode failed!"
	exit 1
	;;
esac
fi


if [  -e /usr/local/bin/iav_server ]; then
echo "Start IAV Server ..."
/usr/local/bin/iav_server &
fi

if [  -e /tmp/mmcblk0p1/ext-post.sh ]; then
echo "==============================================="
echo "Exec: /tmp/mmcblk0p1/ext-post.sh"
echo "==============================================="
/tmp/mmcblk0p1/ext-post.sh
elif [  -e /home/default/ext-post.sh ]; then
echo "==============================================="
echo "Exec: /home/default/ext-post.sh"
echo "==============================================="
/home/default/ext-post.sh
fi
case "$?" in
    0)
	;;
    *)
  	echo "Exec ext-post.sh failed!"
	exit 1
	;;
esac

#add dsp log driver
if [ -r /lib/modules/"$kernel_ver"/extra/dsplog.ko ]; then
	echo "load dsplog driver "
	modprobe dsplog
fi

if [ -r /usr/local/bin/ImageServerDaemon.sh ]; then
	if [ -x /usr/local/bin/ImageServerDaemon.sh ]; then
		echo "==============================================="
		echo "Exec: /usr/local/bin/ImageServerDaemon.sh"
		echo "==============================================="
		/usr/local/bin/ImageServerDaemon.sh start
	fi
fi


BC_DSP_INIT=$(which bc_dsp_init)
if [  -e "$BC_DSP_INIT" ]; then
case $2 in
	--par )
          "$BC_DSP_INIT" --par "$3";
        case "$?" in
	    0)
	      ;;
	    *)
  	      echo "bc_dsp_init failed!"
	      exit 1
	      ;;
            esac
        ;;
        *)

        ;;
esac
fi

exit $?
