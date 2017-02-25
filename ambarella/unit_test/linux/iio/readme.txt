root@/#lsiio
Device 000: e801d000.adc
Trigger 000: e801d000.adc-dev0



root@/#echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_voltage1_en 
root@/#echo 1 > /sys/bus/iio/devices/iio\:device0/scan_elements/in_voltage3_en
root@/#iio_generic_buffer -n e801d000.adc
iio device number being used is 0
iio trigger number being used is 0
/sys/bus/iio/devices/iio:device0 e801d000.adc-dev0
3299.194336 3299.194336
3299.194336 3299.194336
3299.194336 3299.194336
3299.194336 3299.194336
...




root@/#iio_event_monitor e801d000.adc
Found IIO device with name e801d000.adc with device number 0
Event: time: 968888270446955, type: voltage, channel: 3, evtype: thresh, direction: either
Event: time: 968888270454558, type: voltage, channel: 3, evtype: thresh, direction: either
Event: time: 968888270460012, type: voltage, channel: 3, evtype: thresh, direction: either
Event: time: 968888270495962, type: voltage, channel: 3, evtype: thresh, direction: either
Event: time: 968888270506679, type: voltage, channel: 3, evtype: thresh, direction: either
Event: time: 968888442227487, type: voltage, channel: 3, evtype: thresh, direction: either