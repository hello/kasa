#!/bin/sh
#
#  example: use the add partition <add:ipcam not use it >
#    mtdparts=ambnand:70m@0xf220000"
#
# History:
#	2012/05/05 - [Jingyang Qiu] created file
#
# Copyright (c) 2015 Ambarella, Inc.
#
# This file and its contents ("Software") are protected by intellectual
# property rights including, without limitation, U.S. and/or foreign
# copyrights. This Software is also the confidential and proprietary
# information of Ambarella, Inc. and its licensors. You may not use, reproduce,
# disclose, distribute, modify, or otherwise prepare derivative works of this
# Software or any portion thereof except pursuant to a signed license agreement
# or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
# In the absence of such an agreement, you agree to promptly notify and return
# this Software to Ambarella, Inc.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
# MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

/sbin/mdev -s

PartitionName="add"
VolumeName="ambarella_fs"
DeviceName="ubi1"
FolderName="/ambarella"

MtdNum=$(grep $PartitionName /proc/mtd | awk '{print $1}' | sed -e s/^mtd// | sed -e s/://)

# format
if [ -r /dev/$DeviceName ]; then
	echo "	$DeviceName exists"
else
	/usr/sbin/ubiattach /dev/ubi_ctrl -m "$MtdNum" -d 1
	Check_UBI=$?
if [ $Check_UBI != "0" ]; then
	/usr/sbin/ubiformat /dev/mtd"$MtdNum"
	/usr/sbin/ubiattach /dev/ubi_ctrl -m "$MtdNum" -d 1
fi
	/usr/sbin/ubimkvol /dev/$DeviceName -N $VolumeName -m
fi

UbiCount=$(ubinfo |grep Count | awk '{print $5 }')

if [	"$UbiCount" = "1" ]; then
	/usr/sbin/ubiattach /dev/ubi_ctrl -m "$MtdNum" -d 1
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
if [	"$( df | grep $DeviceName)"	!= ""	]; then
	echo "	$DeviceName was mounted"
else
	echo "	mount -t ubifs $DeviceName:$VolumeName $FolderName "
	mount -t ubifs $DeviceName:$VolumeName $FolderName
fi
