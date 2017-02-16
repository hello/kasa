#!/bin/sh
echo "Start uploading to amba cloud" > /dev/ttyS0
/usr/bin/apps_launcher &
/usr/local/bin/amba_cloud_agent --disableaudio --enableencryption --stream 0
