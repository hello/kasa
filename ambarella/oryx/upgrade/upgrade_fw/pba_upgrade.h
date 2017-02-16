/*******************************************************************************
 * pba_upgrade.h
 *
 * History:
 *   2015-1-15 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_
#define ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_

#define PROC_MTD_FILE               "/proc/mtd"
#define MTDBLOCK_DEVICE_PATH        "/dev/mtdblock"
#define PROC_FILESYSTEMS            "/proc/filesystems"
#define DEFAULT_MOUNT_FS_TYPE       "jffs2"
#define KERNERL_NAME                "Image"
#define UBIFS_NAME                  "ubifs"
#define SIGN_SUFFIX                 ".sign"
#define PUBLIC_KEY_SUFFIX           "public.pem"
#define UPGRADE_ADC_PARTITION       "adc"
#define ADC_MOUNT_DIR               "/dev/adc"
#define UPGRADE_CFG_PARTITION_NAME  "add"
#define ADD_MOUNT_DIR               "/dev/add"
#define UPGRADE_CFG_FILE_NAME       "upgrade_cfg.acs"
#define FW_FILE_PATH                "filepath"
#define UPGRADE_STATE_NAME          "status"
#define ENTER_PBA_CMDLINE           "upgrade_partition -S 2 -C"


#endif /* ORYX_UPGRADE_UPGRADE_FW_PBA_UPGRADE_H_ */
