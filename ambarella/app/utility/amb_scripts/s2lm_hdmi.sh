#!/bin/sh
kernel_ver=$(uname -r)
echo "try to load it66121 external HDMI driver for S2Lm EVK board"
if [ -r /lib/modules/$kernel_ver/extra/it66121.ko ]; then
modprobe it66121
fi
echo "try to enter 480p preview by HDMI, and default VIN"
test_encode -i0 -v480p --lcd digital601 -K --bsize 480p --bmaxsize 480p