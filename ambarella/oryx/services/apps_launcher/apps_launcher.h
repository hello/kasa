/*******************************************************************************
 * apps_launcher.h
 *
 * History:
 *   2014-10-21 - [lysun] created file
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
#ifndef APPS_LAUNCHER_H_
#define APPS_LAUNCHER_H_
#include "commands/am_service_impl.h"
#include <list>

typedef std::list<am_service_attribute> AMServiceAttrList;

class AMAppsLauncher
{
  public:
    static AMAppsLauncher  *get_instance();

  public:
    int load_config();
    int init();
    int start();
    int stop();
    int clean();
    void destroy();
    const AMServiceAttrList& get_service_list();

  protected:
    AMAppsLauncher();
    virtual ~AMAppsLauncher();

  private:
    void get_ubi_node(const std::string &index_str,
                      std::string &ubi_node);
    bool get_partition_index_str(const std::string &partition_name,
                                 std::string &index_str);
    bool is_mount(const std::string &mount_dst);
    bool umount_dir(const std::string &mount_dst);
    bool mount_partition(const std::string &device_name,
                         const std::string &mount_dst,
                         const std::string &fs_type);
    bool check_partition_exist(const std::string partition_name);
    bool touch_file(const std::string filename);
    bool restore_config(const std::string &config_name,
                        const std::string &config_path,
                        const std::string &extract_path);
    bool backup_config(const std::string &config_name,
                       const std::string &config_list,
                       const std::string &path_base,
                       const std::string &store_path);
    bool check_config(const std::string &config_path);
    bool init_prepare();

  private:
    static AMAppsLauncher *m_instance;
    AMServiceManagerPtr    m_service_manager = nullptr;
    AMAPIProxyPtr          m_api_proxy       = nullptr;
    AMServiceAttrList      m_service_list;
};

#endif /* APPS_LAUNCHER_H_ */
