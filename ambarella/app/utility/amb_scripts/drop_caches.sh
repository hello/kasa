#!/bin/sh
while [ true ]
do
echo 1 > /proc/sys/vm/drop_caches
sleep 1
done
