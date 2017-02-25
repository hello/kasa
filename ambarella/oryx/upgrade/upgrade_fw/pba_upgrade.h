/*******************************************************************************
 * pba_upgrade.h
 *
 * History:
 *   2015-1-15 - [longli] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
#ifndef ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_
#define ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_

#define PROC_MTD_FILE               "/proc/mtd"
#define MTDBLOCK_DEVICE_PATH        "/dev/mtdblock"
#define PROC_FILESYSTEMS            "/proc/filesystems"
#define DEFAULT_MOUNT_FS_TYPE       "ubifs"
#ifdef BUILD_AMBARELLA_ORYX_UPGRADE_MTD_MOUNT_FS_TYPE
#define MOUNT_FS_TYPE ((const char*)BUILD_AMBARELLA_ORYX_UPGRADE_MTD_MOUNT_FS_TYPE)
#else
#define MOUNT_FS_TYPE (DEFAULT_MOUNT_FS_TYPE)
#endif
#define UBIFS_FS_TYPE               "ubifs"
#define MOUNT_SD_FS_TYPE_EXFAT      "exfat"
#define BLD_NAME                    "bld_release.bin"
#define DTS_FILE_SUFFIX             ".dts.dtb"
#define KERNEL_NAME                 "Image"
#define UBIFS_NAME                  "ubifs"
#define SIGN_SUFFIX                 ".sign"
#define PUBLIC_KEY_SUFFIX           "public.pem"
#define UPGRADE_ADC_PARTITION       "adc"
#define ADC_MOUNT_DIR               "/dev/adc"
#define UPGRADE_CFG_PARTITION_NAME  "add"
#define ADD_MOUNT_DIR               "/dev/add"
#define SD_MOUNT_DIR                "/sdcard"
#define UPGRADE_CFG_FILE_NAME       "upgrade_cfg.acs"
#define FW_FILE_PATH                "filepath"
#define UPGRADE_STATE_NAME          "status"
#define USE_SDCARD                  "sdcard"
#define ENTER_PBA_CMDLINE           "/usr/local/bin/upgrade_partition -S 2 -C"


#endif /* ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_ */
