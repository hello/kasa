#!/bin/sh

version=$(uname -r)

insmod /lib/modules/$version/extra/mlan.ko 
insmod /lib/modules/$version/extra/sd8787.ko mfg_mode=1 drv_mode=1 fw_name=mrvl/SD8787.bin

