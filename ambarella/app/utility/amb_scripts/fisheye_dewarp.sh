#!/bin/sh

/usr/local/bin/init.sh --imx172;
/bin/cp /etc/camera/fisheye.conf.imx172.focusafe125  /etc/camera/fisheye.conf ;
/usr/local/bin/rtsp_server & ;
/usr/local/bin/test_fisheyecam 360 h1280 ;

