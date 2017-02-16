#!/bin/sh
# example: use the add partition <add:ipcam not use it > 
#
#
#  mtdparts=ambnand:70m@0xf220000"
#
	/sbin/mdev -s

PartitionName="add"
VolumeName="ambarella_fs"
DeviceName="ubi1"
FolderName="/ambarella"

MtdNum=`cat /proc/mtd | grep $PartitionName | awk '{print $1}' | sed -e s/^mtd// | sed -e s/\://`

# format
if [ -r /dev/$DeviceName ]; then
	echo "	$DeviceName exists"
else
	/usr/sbin/ubiattach /dev/ubi_ctrl -m $MtdNum -d 1
	Check_UBI=$?
if [ $Check_UBI != "0" ]; then
	/usr/sbin/ubiformat /dev/mtd$MtdNum
	/usr/sbin/ubiattach /dev/ubi_ctrl -m $MtdNum -d 1
fi
	/usr/sbin/ubimkvol /dev/$DeviceName -N $VolumeName -m
fi

UbiCount=`ubinfo |grep Count | awk '{print $5 }'`

if [	"$UbiCount" == "1" ]; then
	/usr/sbin/ubiattach /dev/ubi_ctrl -m $MtdNum -d 1
else
	echo "	$DeviceName was attached"
fi

#create Directory
if [ -r $FolderName ]; then
	echo "	$FolderName exists"
else
	echo "create Directory:$FolderName"
	mkdir $FolderName
fi

#mount
if [	"` df | grep $DeviceName`"	!= ""	]; then
	echo "	$DeviceName was mounted"
else
	echo "	mount -t ubifs $DeviceName:$VolumeName $FolderName "
	mount -t ubifs $DeviceName:$VolumeName $FolderName
fi