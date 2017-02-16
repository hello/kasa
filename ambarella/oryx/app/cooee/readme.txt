Following seven source files are provided by broadcom INC.

common.h  cooee.c  cooee.h  cooee_inter.c  cooee_inter.h  scan.h  scan.c

test_cooee.c is just a demo to call interface of cooee and store
wifi configuration, including ssid and password, into a specified file.

Specifically, cooee_interface (char *file_name) will wait a message
sent by an app of mobile device. Because cooee_interface will return
if it can't detect message in certain time, test_cooee.c define
MAX_TIME_TO_GET_WIFI_CONF to specify how many times to try to get wifi configuration.

