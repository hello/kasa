#!/bin/sh -x
DIR=.output
case $1 in
    unloadall )
    echo "..unloading all"
    /sbin/rmmod $DIR/usdio.ko
    /sbin/rmmod $DIR/sdio_pcistd_hcd.ko
    /sbin/rmmod $DIR/sdio_busdriver.ko
    /sbin/rmmod $DIR/sdio_lib.ko
    ;;
    *)
    /sbin/insmod $DIR/sdio_lib.ko debuglevel=0
    /sbin/insmod $DIR/sdio_busdriver.ko RequestListSize=300 debuglevel=0
    /sbin/insmod $DIR/sdio_pcistd_hcd.ko debuglevel=0
    /sbin/insmod $DIR/usdio.ko
    rm -f /dev/usdio0
    mknod -m 777 /dev/usdio0 c 65 0
    ;;
esac
