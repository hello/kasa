#!/bin/sh
audioclk=`cat /proc/ambarella/performance | grep Audio | awk '{print $2}'`
if [ $audioclk != 12288000 ]; then echo "audio clk is wrong" $audioclk ", must be 12288000! (48KHz sample rate)"
else
	 echo "audio clk is OK"
fi
