/*******************************************************************************
 * am_upgrade.h
 *
 * History:
 *   2015-1-8 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    AM_MOUNT_STATUS is_mounted(const std::string &partition_path,
                               const std::string &mount_dst);
    AM_MOUNT_STATUS m_mount_partition(const std::string &partition_name,
                                      const std::string &mount_dst,
                                      const std::string &fs_type);
    bool is_fs_supported(const std::string &fs_type);
    void remove_previous_fw(const std::string &fw_path);
    bool copy_file(const std::string &src_file, const std::string &dst_file);
    bool copy_fw_to_ADC();
    bool save_fw_path(const std::string &fw_path);
    bool set_upgrade_status(const AM_UPGRADE_STATUS state);
    static void upgrade_thread(void *upgrade_args);
    AMFWUpgrade(AMFWUpgrade const &copy) = delete;
    AMFWUpgrade& operator=(AMFWUpgrade const &copy) = delete;

  private:
    std::string         m_fw_path;
    std::string         m_adc_dir;
    std::atomic_int     m_ref_counter;
    AM_UPGRADE_MODE     m_mode;
    AM_UPGRADE_STATUS   m_state;
    AMConfig            *m_config;
    AMDownload          *m_updl;
    bool                m_init;
    bool                m_dl_ready;
    bool                m_mounted;
    bool                m_in_progress;
    static AMFWUpgrade  *m_instance;
};

#endif /* ORYX_UPGRADE_INCLUDE_AM_UPGRADE_H_ */
