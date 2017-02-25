/*******************************************************************************
 * am_upgrade.h
 *
 * History:
 *   2015-1-8 - [longli] created file
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
#ifndef ORYX_UPGRADE_INCLUDE_AM_UPGRADE_H_
#define ORYX_UPGRADE_INCLUDE_AM_UPGRADE_H_

#include "am_upgrade_if.h"
#include "am_download.h"
#include "am_configure.h"
#include <atomic>

class AMFWUpgrade: public AMIFWUpgrade
{
    friend class AMIFWUpgrade;

  public:
    virtual bool set_mode(AM_UPGRADE_MODE mode) override;
    virtual bool set_use_sdcard(const uint32_t flag) override;
    virtual bool set_fw_url(const std::string &fw_url,
                            const std::string &fw_save_path) override;
    virtual bool set_fw_path(const std::string &fw_path) override;
    /* set download firmware authentication if needed */
    virtual bool set_dl_user_passwd(const std::string &user,
                                    const std::string &passwd) override;
    virtual bool set_dl_connect_timeout(uint32_t timeout) override;
    virtual AM_MOUNT_STATUS mount_partition(const std::string &partition_name,
                                            const std::string &mount_dst,
                                            const std::string &fs_type)override;
    virtual bool umount_dir(const std::string &umount_dir) override;
    virtual bool upgrade() override;
    virtual void run_upgrade_thread(void *paras, uint32_t size) override;
    virtual AM_UPGRADE_STATUS get_upgrade_status() override;
    virtual bool is_in_progress() override;

  protected:
    static AMFWUpgrade *get_instance();
    virtual void release() override;
    virtual void inc_ref() override;

  private:
    AMFWUpgrade();
    virtual ~AMFWUpgrade();
    bool init();
    bool init_config_file(const std::string &parti_name,
                          const std::string &dst_dir,
                          const std::string &fs_type,
                          const std::string &config_file);
    bool download_init();
    bool set_upgrade_cmd();
    bool create_dir(const std::string &dir_path);
    bool check_dl_file();
    bool get_partition_index_str(const std::string &partition_name,
                                 std::string &index_str);
    void get_ubi_node(const std::string &index_str, std::string &ubi_node);
    AM_MOUNT_STATUS is_mounted(const std::string &mtd_index,
                               const std::string &mount_dst,
                               const std::string &mount_fs_type);
    AM_MOUNT_STATUS m_mount_partition(const std::string &partition_name,
                                      const std::string &mount_dst,
                                      const std::string &fs_type);
    bool is_fs_supported(const std::string &fs_type);
    void remove_previous_fw(const std::string &fw_path);
    bool copy_file(const std::string &src_file, const std::string &dst_file);
    bool copy_fw_to_ADC();
    bool save_fw_path(const std::string &fw_path);
    bool set_upgrade_status(const AM_UPGRADE_STATUS state);
    static void upgrade_thread(AMUpgradeArgs upgrade_args);
    AMFWUpgrade(AMFWUpgrade const &copy) = delete;
    AMFWUpgrade& operator=(AMFWUpgrade const &copy) = delete;

  private:
    AMConfig           *m_config;
    AMDownload         *m_updl;
    AM_UPGRADE_MODE     m_mode;
    AM_UPGRADE_STATUS   m_state;
    uint32_t            m_use_sdcard;
    bool                m_init;
    bool                m_dl_ready;
    bool                m_mounted;
    bool                m_in_progress;
    std::string         m_fw_path;
    std::string         m_adc_dir;
    std::atomic_int     m_ref_counter;
    static AMFWUpgrade  *m_instance;
};

#endif /* ORYX_UPGRADE_INCLUDE_AM_UPGRADE_H_ */
