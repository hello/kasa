Introduction
	The original motivation for YAFFS 2 was to add support for the new NAND with 2kB pages instead of 512-byte pages and strictly sequential page writing order.
	To achieve this, a new design is used which also realises the following benefits:
	      zero page rewrites means faster operation. (YAFFS1 uses a single rewrite in the spare area to delete a page).
	      ability to exploit simultaneous page programming on some chips.
	      improves performance relative to YAFFS1 speed(write:1.5x to 5x, delete: 4x, garbage collection: 2x)
	      lower RAM footprint (approx. 25% to 50% of YAFFS1).
	      Can support Toshiba/Sandisk MLC parts.
	Most of YAFFS and YAFFS2 code is common, therefore the code will likely be kept common with YAFFS vs YAFFS2 being run-time selected.

Use yaffs as rootfs
	make menuconfig
	select: Ambarella Firmware Configuration  --->Amboot(boot loader)  --->Booting parameter set
		"root=/dev/mtdblock11 rootfstype=yaffs2 init=linuxrc console=ttyS0 mtdparts=ambnand:-(USRFS)"
	select: Ambarella File System Configuration  --->Linux Root File System  --->   YAFFS

	make menuconfig_public_linux
	select: File systems  ---> Miscellaneous filesystems  ---> YAFFS2 file system support

How to test
	a) First Time (The mtd patition is "USRFS")
		flash_eraseall /dev/mtd14
		mkdir /tmp/ffs
		mount -t yaffs2 /dev/mtdblock14 /tmp/ffs
		(copy a file to the ffs directory)
		umount /tmp/ffs
		reboot

	b) After a)
		mkdir /tmp/ffs
		mount -t yaffs2 /dev/mtdblock14 /tmp/ffs
		test_ffs /tmp/ffs/[filename]
		umount /tmp/ffs
	(note, 14 is the mtd number in flash partition, the latest yaffs2 code can be got from http://www.yaffs.net/)

Test result
	# test_ffs ./test.bin
	start time      [1073898151s:924888us]
	end time        [1073898154s:483544us]
	spend [2558656us] to read 16 MB data
	flash fs read speed is 6.253283 MB/s
	start time      [1073898154s:792306us]
	end time        [1073898160s:400006us]
	spend [5607700us] to write 16 MB data
	flash fs write speed is 2.853220 MB/s
(Note, the file is buffered in the memory after the first test, so you should reboot before you do the test again, or we can close the vfs to prevent the file system buffering? )

												2009/12/21 Qiao
