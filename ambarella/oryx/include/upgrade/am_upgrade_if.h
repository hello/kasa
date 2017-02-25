/*******************************************************************************
 * am_upgrade_if.h
 *
 * History:
 *   2015-1-12 - [longli] created file
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
/*! @file am_upgrade_if.h
 *  @brief This file defines AMIFWUpgrade
 */
#ifndef ORYX_INCLUDE_UPGRADE_AM_UPGRADE_IF_H_
#define ORYX_INCLUDE_UPGRADE_AM_UPGRADE_IF_H_

#include "am_pointer.h"

enum AM_UPGRADE_MODE
{
  AM_MODE_NOT_SET         = -1,//!< AM_MODE_NOT_SET
  AM_DOWNLOAD_ONLY        = 0, //!< AM_DOWNLOAD_ONLY
  AM_UPGRADE_ONLY         = 1, //!< AM_UPGRADE_ONLY
  AM_DOWNLOAD_AND_UPGRADE = 2, //!< AM_DOWNLOAD_AND_UPGRADE
};

enum AM_MOUNT_STATUS
{
  AM_NOT_MOUNTED          = 0,    //not mounted
  AM_MOUNTED              = 1,    //mounted to device we want
  AM_ALREADY_MOUNTED      = 2,    //already mounted to device we want
  AM_MOUNTED_OTHER        = 3,    //mounted to another device
  AM_UNKNOWN              = 4,    //unknown
};

enum AM_UPGRADE_STATUS
{
  AM_NOT_KNOWN                = 0,
  AM_UPGRADE_PREPARE,
  AM_UPGRADE_PREPARE_FAIL,
  AM_DOWNLOADING_FW,
  AM_DOWNLOAD_FW_COMPLETE,
  AM_DOWNLOAD_FW_FAIL,
  AM_INIT_PBA_SYS_DONE,
  AM_INIT_PBA_SYS_FAIL,
  AM_UNPACKING_FW,
  AM_UNPACK_FW_DONE,
  AM_UNPACK_FW_FAIL,
  AM_FW_INVALID,
  AM_WRITEING_FW_TO_FLASH,
  AM_WRITE_FW_TO_FLASH_DONE,
  AM_WRITE_FW_TO_FLASH_FAIL,
  AM_INIT_MAIN_SYS,
  AM_UPGRADE_SUCCESS,
};

struct AMUpgradeArgs {
    char path_to_upgrade_file[128];
    /* when download firmware need authentication */
    char user_name[32];
    char passwd[32];
    /* set whether upgrade from sdcard or not */
    uint32_t use_sdcard;
    /* set timeout if can not connect to server  */
    uint32_t timeout;
    /* AM_UPGRADE_ONLY mode: NULL or store the path of firmware
     * AM_DOWNLOAD_ONLY mode: store the url of the firmware(can not be null)
     * AM_DOWNLOAD_AND_UPGRADE: store the url of the firmware(can not be null)
     */
    AM_UPGRADE_MODE upgrade_mode;//u32 type;
};

class AMIFWUpgrade;
typedef AMPointer<AMIFWUpgrade> AMIFWUpgradePtr;

class AMIFWUpgrade
{
    friend AMIFWUpgradePtr;

  public:
    static AMIFWUpgradePtr get_instance();
    virtual bool set_mode(AM_UPGRADE_MODE mode) = 0;
    /* set whether firmware is stored in sdcard or ADC partition */
    virtual bool set_use_sdcard(const uint32_t flag) = 0;
    /* Download firmware from fw_url and save it to fw_save_path.
     * If fw_save_path is directory, file name is parse from fw_url,
     * or the downloaded firmware is renamed to fw_save_path.
     */
    virtual bool set_fw_url(const std::string &fw_url,
                            const std::string &fw_save_path) = 0;
    /* this is only used for AM_UPGRADE_ONLY mode */
    virtual bool set_fw_path(const std::string &fw_path) = 0;
    /* set download firmware authentication if needed */
    virtual bool set_dl_user_passwd(const std::string &user,
                                 const std::string &passwd) = 0;
    virtual bool set_dl_connect_timeout(uint32_t timeout) = 0;
    virtual AM_MOUNT_STATUS mount_partition(const std::string &partition_name,
                                            const std::string &mount_dst,
                                            const std::string &fs_type) = 0;
    virtual bool umount_dir(const std::string &umount_dir) = 0;
    /* run in current process */
    virtual bool upgrade() = 0;
    /* used by system  service */
    virtual void run_upgrade_thread(void *paras, uint32_t size) = 0;
    virtual AM_UPGRADE_STATUS get_upgrade_status() = 0;
    virtual bool is_in_progress() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIFWUpgrade(){};
};

/*! @example test_upgrade.cpp
 *  This file is the test program of AMIFWUpgrade.
 */
#endif /* ORYX_INCLUDE_UPGRADE_AM_UPGRADE_IF_H_ */
