/*******************************************************************************
 * am_config_set.cpp
 *
 * History:
 *   Apr 7, 2016 - [longli] created file
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

#include "am_base_include.h"
#include <dirent.h>
#include "am_log.h"
#include "am_config_set.h"

using namespace std;

AMConfigSet::AMConfigSet(const string &cfg_set_path):
    m_archive(nullptr),
    m_cfg_set_path(cfg_set_path)
{
}

AMConfigSet::~AMConfigSet()
{
  delete m_archive;
  m_archive = nullptr;
}

bool AMConfigSet::init()
{
  bool ret = true;

  m_archive = AMIArchive::create();
  if (!m_archive) {
    ERROR("Failed to create AMIArchive instance.\n");
    ret = false;
  }

  return ret;
}

AMConfigSet* AMConfigSet::create(const string &cfg_set_path)
{
  AMConfigSet *ins = new AMConfigSet(cfg_set_path);

  if (ins && !ins->init()) {
    ERROR("Failed to create AMDec7z instance.");
    delete ins;
    ins = nullptr;
  }

  return ins;
}


bool AMConfigSet::create_config(const std::string &cfg_name,
                                const std::string &src_config_list,
                                const std::string &base_dir)
{
  bool ret = true;
  string cfg_path(cfg_name);

  if (cfg_name.find_first_of('/') == string::npos) {
    cfg_path = m_cfg_set_path + "/" + cfg_name;
  }

  ret = m_archive->compress(cfg_path, src_config_list,
                            AM_COMPRESS_XZ, base_dir);

  return ret;
}

bool AMConfigSet::get_config_list(string &name_list)
{
  bool ret = true;
  DIR *dirp;
  struct dirent *direntp;

  do {
    if ((dirp = opendir(m_cfg_set_path.c_str())) == nullptr) {
      ERROR("Failed to open folder %s.", m_cfg_set_path.c_str());
      ret = false;
      break;
    }

    INFO("Configure file list(under %s):\n", m_cfg_set_path.c_str());
    name_list.clear();
    while ((direntp = readdir(dirp)) != nullptr) {
      if (direntp->d_type == DT_REG) {
            INFO("--%s\n", direntp->d_name);
            name_list = name_list + direntp->d_name + ",";
      }
    }
    closedir(dirp);
  } while (0);

  return ret;
}

bool AMConfigSet::extract_config(const string &config_name,
                                 const string &cfg_dst_path)
{
  bool ret = true;
  string cfg_set_path(config_name);

  do {
    if (access(cfg_set_path.c_str(), F_OK)) {
      cfg_set_path = m_cfg_set_path + "/"+ config_name;
      if (access(cfg_set_path.c_str(), F_OK)) {
        cfg_set_path += ".tar.xz";
        if (access(cfg_set_path.c_str(), F_OK)) {
          INFO("config_file: %s\n", cfg_set_path.c_str());
          PRINTF("%s not found\n", config_name.c_str());
          ret = false;
          break;
        }
      }
    }

    ret = m_archive->decompress(cfg_set_path, cfg_dst_path, AM_COMPRESS_XZ);

  } while (0);

  return ret;
}
