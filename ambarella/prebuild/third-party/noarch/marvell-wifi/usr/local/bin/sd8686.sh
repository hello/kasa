#!/bin/sh

version=$(uname -r)

modprobe libertas
insmod /lib/modules/$version/kernel/drivers/net/wireless/libertas/libertas_sdio.ko helper_name=mrvl/helper_sd.bin fw_name=mrvl/sd8686.bin

