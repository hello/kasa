/*******************************************************************************
 * test_mp4_fixer.cpp
 *
 * History:
 *   2015-12-15 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_log.h"
#include "am_define.h"
#include "am_mp4_file_fixer_if.h"
int main(int argc, char*argv[])
{
  int ret = 0;
  std::string index_file_name;
  std::string damaged_file_name;
  std::string fixed_file_name;
  do {
    if (argc > 4) {
      PRINTF("Please input cmd as below : \n"
             "test_mp4_fixer index_file_name damaged_file_name fixed_file_name"
             "If want to use default arguments, input cmd : test_mp4_fixer");
      break;
    }
    switch (argc) {
      case 4: {
        fixed_file_name = argv[3];
        INFO("fixed file name is %s", fixed_file_name.c_str());
      }
      case 3: {
        damaged_file_name = argv[2];
        INFO("damaged_file_name is %s", damaged_file_name.c_str());
      }
      case 2: {
        index_file_name = argv[1];
        INFO("index file name is %s", index_file_name.c_str());
      } break;
      case 1:
        break;
      default: {
        ERROR("argc error.");
        ret = -1;
      } break;
    }
    if(ret < 0) {
      break;
    }
    AMIMp4FileFixer* mp4_file_fixer = AMIMp4FileFixer::create(
        ((index_file_name.size() > 0) ? index_file_name.c_str() : nullptr),
        ((damaged_file_name.size() > 0) ? damaged_file_name.c_str() : nullptr),
        ((fixed_file_name.size() > 0) ? fixed_file_name.c_str() : nullptr));
    if(!mp4_file_fixer) {
      ERROR("Failed to create mp4 file fixer.");
      ret = -1;
      break;
    }
    if(!mp4_file_fixer->fix_mp4_file()) {
      ERROR("Failed to fix mp4_file.");
      ret = -1;
    }
    mp4_file_fixer->destroy();
  } while (0);
  return ret;
}

