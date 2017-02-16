#!/bin/sh

time dd if=/dev/zero of=/tmp/mmcblk0p1/empty.file bs=128k count=1k
sync
echo 1 > /proc/sys/vm/drop_caches
time dd if=/tmp/mmcblk0p1/empty.file of=/dev/null bs=128k count=1k
