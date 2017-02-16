#!/bin/sh

version=$(uname -r)

insmod /lib/modules/$version/extra/mlan.ko 
insmod /lib/modules/$version/extra/sd8787.ko 

