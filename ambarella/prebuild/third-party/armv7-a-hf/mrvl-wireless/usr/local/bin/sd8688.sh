#!/bin/sh

version=$(uname -r)

modprobe bluetooth
insmod /lib/modules/$version/extra/wlan.ko helper_name=mrvl/helper_sd.bin fw_name=mrvl/sd8688.bin
insmod /lib/modules/$version/extra/bt.ko helper_name=mrvl/helper_sd.bin fw_name=mrvl/sd8688.bin

