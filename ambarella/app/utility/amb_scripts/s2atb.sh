#!/bin/sh

atbtest_script_name=s2atbtest.sh
atbtest_script_path=/tmp/mmcblk0p1

if [ -f "$atbtest_script_path"/"$atbtest_script_name" ]; then
        #cd "$atbtest_script_path"
        cp "$atbtest_script_path"/"$atbtest_script_name" /usr/local/bin
        /usr/local/bin/"$atbtest_script_name" --default
        rm /usr/local/bin/"$atbtest_script_name"
else
        echo can not get "$atbtest_script_path"/"$atbtest_script_name"
        #exit 1
fi



