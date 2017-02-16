#!/bin/sh
echo 1 > /proc/sys/vm/block_dump
while [ true ]
do
dmesg -c
echo -e "\n"
sleep 1
done
