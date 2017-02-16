Introduction
	JFFS2 is a log-structured file system designed for use on flash devices in embedded systems. Rather than using a kind of translation layer on flash devices to emulate a normal hard drive, as is the case with older flash solutions, it places the filesystem directly on the flash chips.

	JFFS2 was developed by Red Hat, based on the work started in the original JFFS by Axis Communications, AB.


Use jffs2 as rootfs
	make menuconfig
	select: Ambarella Firmware Configuration  --->Amboot(boot loader)  --->Booting parameter set
		"root=/dev/mtdblock11 rootfstype=jffs2 init=linuxrc console=ttyS0 mtdparts=ambnand:-(USRFS)"
	select: Ambarella File System Configuration  --->Linux Root File System  --->   JFFS2

	make menuconfig_public_linux
	select: File systems  ---> Miscellaneous filesystems  ---> JFFS2 file system support

How to test
	a) First Time (The mtd patition is "USRFS")
		flash_eraseall /dev/mtd14
		mkdir /tmp/ffs
		mount -t jffs2 /dev/mtdblock14 /tmp/ffs
		(copy a file to the ffs directory)
		umount /tmp/ffs
		reboot

	b) After a)
		mkdir /tmp/ffs
		mount -t jffs2 /dev/mtdblock14 /tmp/ffs
		test_ffs /tmp/ffs/[filename]
		umount /tmp/ffs
	(note, 14 is the mtd number in flash partition, the latest jffs2 code can be got from http://www.jffs.net/)

Test result
	# test_ffs ./test.bin
	start time      [1073742197s:632487us]
	end time        [1073742199s:925855us]
	spend [2293368us] to read 11 mb data
	flash fs read speed is 5.084147 mb/s
	start time      [1073742199s:942400us]
	end time        [1073742204s:497159us]
	spend [4554759us] to write 16 mb data
	flash fs write speed is 3.512809 mb/s
(Note, the file is buffered in the memory after the first test, so you should reboot before you do the test again, or we can close the vfs to prevent the file system buffering? )

												2009/12/21 Qiao
