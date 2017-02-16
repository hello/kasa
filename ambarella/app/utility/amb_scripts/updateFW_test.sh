#!/bin/sh

#####################################################################
## the default parameters of mtd script is A5s ipcam
## if the board type is different,
## customer must following the virtual status to modify it.
## if the MTD tool is diff, it must necessory to modify the update part.
#####################################################################
ORIGNAL_ROOTFS="lnx"
SECOND_ROOTFS="add"
KERNEL_MTD="pri"
KERNEL_START_ADDR="0xc0208000"
MTD_ERASE="flash_eraseall"
MTD_WRITE="nandwrite"
MTD_CMDLINE_OPTION="cmd"

####################################################################
## the default image file path in current directory
####################################################################

DEFAULT_KERNEL_IMAGE_PATH="Image"
DEFAULT_ROOTFS_IMAGE_PATH="ubifs"


##############################################
## check whether files exist
##############################################
check_kernel_file()
{
	if [ ! -r ${KERNEL_IMAGE_PATH} ] ; then
		echo "can't find Kernel image:${KERNEL_IMAGE_PATH}"
		exit 1
	fi
}

check_rootfs_file()
{
	if [ ! -r ${ROOTFS_IMAGE_PATH} ] ; then
		echo "can't find Rootfs image:${ROOTFS_IMAGE_PATH}"
		exit 1
	fi
}

##############################################
## check rootfs size
##############################################
check_rootfs_size()
{
	MTD_CMD=`nandwrite --show_info | grep cmdline`
	MTD_CMD=`echo ${MTD_CMD:10}`
	ROOTFS_MTD=`echo ${MTD_CMD} | awk '{print $2}' | sed -e s/^ubi.mtd=//`
	PREV_ROOTFS=${ROOTFS_MTD}
	if [ ${ROOTFS_MTD} == ${ORIGNAL_ROOTFS} ] ; then
		ROOTFS_MTD=${SECOND_ROOTFS}
	else
		ROOTFS_MTD=${ORIGNAL_ROOTFS}
	fi

	ROOTFS_MTD_NUM=`cat /proc/mtd | grep ${ROOTFS_MTD} | awk '{print $1}' | sed -e s/://`
	echo "${ROOTFS_MTD} is ${ROOTFS_MTD_NUM}"

	ROOTFS_SIZE=`cat /proc/mtd | grep ${ROOTFS_MTD} | awk '{print $2}'`
	echo $((ROOTFS_SIZE=0x${ROOTFS_SIZE}))
	ROOTFS_SIZE=`expr ${ROOTFS_SIZE} / 1024 / 1024`

	echo "${ROOTFS_MTD} size is ${ROOTFS_SIZE} M"

	ROOTFS_IMAGE_SIZE=`du -m ${ROOTFS_IMAGE_PATH} | awk '{print $1}'`
	echo "${ROOTFS_IMAGE_PATH} size is ${ROOTFS_IMAGE_SIZE} M"

	if [ ${ROOTFS_IMAGE_SIZE} -gt ${ROOTFS_SIZE} ] ; then
		echo "${ROOTFS_IMAGE_PATH} is larger than the partition size"
		exit 1
	fi
}

##############################################
##  check kernel size
##############################################
check_kernel_size()
{
	KERNEL_MTD_NUM=`cat /proc/mtd | grep ${KERNEL_MTD} | awk '{print $1}' | sed -e s/://`
	echo "${KERNEL_MTD} is ${KERNEL_MTD_NUM}"

	KERNEL_SIZE=`cat /proc/mtd | grep ${KERNEL_MTD} | awk '{print $2}'`
	echo $((KERNEL_SIZE=0x${KERNEL_SIZE}))
	KERNEL_SIZE=`expr ${KERNEL_SIZE} / 1024 / 1024`

	echo "${KERNEL_MTD} size is ${KERNEL_SIZE} M"

	KERNEL_IMAGE_SIZE=`du -m ${KERNEL_IMAGE_PATH} | awk '{print $1}'`
	echo "${KERNEL_IMAGE_PATH} size is ${KERNEL_IMAGE_SIZE} M"

	if [ ${KERNEL_IMAGE_SIZE} -gt ${KERNEL_SIZE} ] ; then
		echo "${KERNEL_IMAGE_PATH} is larger than the partition size"
		exit 1
	fi
}

##############################################
##  update kernel image
##############################################
updatea_kernel_image()
{
	${MTD_ERASE} /dev/${KERNEL_MTD_NUM}
	echo "${MTD_WRITE} -p --${KERNEL_MTD} -L ${KERNEL_START_ADDR} /dev/${KERNEL_MTD_NUM} ${KERNEL_IMAGE_PATH}"
	${MTD_WRITE} -p --${KERNEL_MTD} -L ${KERNEL_START_ADDR} /dev/${KERNEL_MTD_NUM} ${KERNEL_IMAGE_PATH}
}

##############################################
## update rootfs image
##############################################
update_rootfs_image()
{
	if [ ${ROOTFS_MTD} == ${SECOND_ROOTFS} ] ; then
		echo "${MTD_ERASE} /dev/${ROOTFS_MTD_NUM}"
		${MTD_ERASE} /dev/${ROOTFS_MTD_NUM}
		echo "${MTD_WRITE} -p --${SECOND_ROOTFS} /dev/${ROOTFS_MTD_NUM} ${ROOTFS_IMAGE_PATH} -F 1"
		${MTD_WRITE} -p --${SECOND_ROOTFS} /dev/${ROOTFS_MTD_NUM} ${ROOTFS_IMAGE_PATH} -F 1
	else
		echo "${MTD_ERASE} /dev/${ROOTFS_MTD_NUM}"
		${MTD_ERASE} /dev/${ROOTFS_MTD_NUM}
		echo "${MTD_WRITE} -p --${ORIGNAL_ROOTFS} /dev/${ROOTFS_MTD_NUM} ${ROOTFS_IMAGE_PATH} -F 1"
		${MTD_WRITE} -p --${ORIGNAL_ROOTFS} /dev/${ROOTFS_MTD_NUM} ${ROOTFS_IMAGE_PATH} -F 1
	fi
}

##############################################
## 	update cmdline
##############################################
update_cmdline()
{
	MTD_CMD=`echo ${MTD_CMD/${PREV_ROOTFS}/${ROOTFS_MTD}}`
	echo "${MTD_WRITE} --${MTD_CMDLINE_OPTION} ${MTD_CMD}"
	${MTD_WRITE} --${MTD_CMDLINE_OPTION} "${MTD_CMD}"
}

##############################################
## update kernel function
##############################################
update_kernel()
{
	check_kernel_file
	check_kernel_size
	updatea_kernel_image
}

##############################################
## update filesystem function
##############################################
update_rootfs()
{
	check_rootfs_file
	check_rootfs_size
	update_rootfs_image
	update_cmdline
}

##############################################
## Load the image files
##############################################
if [ "$1" == "" ] ; then
	echo "usage:updateFW_test.sh [Kernel full image] [Rootfs image full path]"
	echo "example:"
	echo "	updateFW_test.sh /tmp/mmcblk0p1/Image /tmp/mmcblk0p1/ubifs"
	echo "	updateFW_test.sh --default	:Image and ubifs in the current directory"
	exit 1
else
	if [ "$1" == "--default" ] ; then
		KERNEL_IMAGE_PATH=${DEFAULT_KERNEL_IMAGE_PATH}
		ROOTFS_IMAGE_PATH=${DEFAULT_ROOTFS_IMAGE_PATH}
	else
		KERNEL_IMAGE_PATH=$1
		if [ "$2" != "" ] ; then
			ROOTFS_IMAGE_PATH=$2
		else
			ROOTFS_IMAGE_PATH=${DEFAULT_ROOTFS_IMAGE_PATH}
		fi
	fi
fi


#####################################################################
## main function
##########################################################################
update_kernel
update_rootfs

##############################################
## must reboot after finished update FW
##############################################
reboot
