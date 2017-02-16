Introduction
	UBIFS is a new flash file system developed by Nokia engineers with help of the University of Szeged. In a way, UBIFS may be considered as the next generation of the JFFS2 file-system.

	JFFS2 file system works on top of MTD devices, but UBIFS works on top of UBI volumes and cannot operate on top of MTD devices. In other words, there are 3 subsystems involved:

	1) MTD subsystem, which provides uniform interface to access flash chips. MTD provides an notion of MTD devices (e.g., /dev/mtd0) which basically represents raw flash;
	2) UBI subsystem, which is a wear-leveling and volume management system for flash devices; UBI works on top of MTD devices and provides a notion of UBI volumes; UBI volumes are higher level entities than MTD devices and they are devoid of many unpleasant issues MTD devices have (e.g., wearing and bad blocks); see here for more information;
	3) UBIFS file system, which works on top of UBI volumes.

Use ubifs as rootfs
	make menuconfig
	select: Ambarella Firmware Configuration  --->Amboot(boot loader)  --->Booting parameter set "console=ttyS0 ubi.mtd=11 root=ubi0:rootfs rootfstype=ubifs init=/linuxrc mtdparts=ambnand:-(USRFS)"
	select: Ambarella File System Configuration  --->Linux Root File System (UBIFS)  --->   UBIFS
	make menuconfig_public_linux
	select: Device Drivers  --->Memory Technology Device (MTD) support  --->UBI - Unsorted block images  --->  Enable UBI
	select: File systems  ---> Miscellaneous filesystems  ---> UBIFS file system support

How to test
	a) First Time (The mtd patition is ubifs fs)
		flash_eraseall /dev/mtd14
		./ubiattach /dev/ubi_ctrl -m 14
		./ubimkvol  /dev/ubi1 -s 60MiB -N usr_a
		./ubimkvol  /dev/ubi1 -s 20MiB -N usr_b

		mkdir /tmp/ubi_v
		mount -t ubifs ubi1_1 /tmp/ubi_v
		umount /tmp/ubi_v
		reboot

	b) After a)
		./ubiattach /dev/ubi_ctrl -m 14
		./ubinfo -a

		mkdir /tmp/ubi_v
		mount -t ubifs ubi1_1 /tmp/ubi_v
		ls /tmp/ubi_v -la
		umount /tmp/ubi_v
	(note, 14 is the mtd number in flash partition, the ubi utils is compiled from mtd-utils project, you can get them from http://elinux.org/CompilingMTDUtils)

Test result
	# test_ubifs
	start time      [1073778784s:285420us]
	end time        [1073778785s:690860us]
	spend [1405440us] to read 16 MB data
	UBIFS read speed is 11.384336 MB/s
	start time      [1073778785s:718474us]
	end time        [1073778787s:335196us]
	spend [1616722us] to write 16 MB data
	UBIFS write speed is 9.896568 MB/s
(Note, the file is buffered in the memory after the first test, so you should reboot before you do the test again, or we can close the vfs to prevent the file system buffering? )

												2009/12/21 Qiao
