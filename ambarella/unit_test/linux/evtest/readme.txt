https://cgit.freedesktop.org/evtest/

For IR-keys, you have to select the decoder first. E.g.
1) modprobe ir-ambarella
2) cat /sys/class/rc/rc0/protocols
3) echo nec > /sys/class/rc/rc0/protocols

Note: if you want to see the "Repeat" Key, you have to setup the key map first.

