/*******************************************************************************
 * test_mp4_combine.cpp
 *
 * History:
 *   2015-10-13 - [ccjing] created file
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
#include "am_log.h"
#include "am_define.h"
#include <iostream>
#include "am_mp4_file_combiner_if.h"

bool test_mp4_combine(const char* first_mp4_file,
                 const char* second_mp4_file, const char* combined_mp4_file);
int main(int argc, char* argv[])
{
  int ret = 0;
  std::string first_mp4_file;
  std::string second_mp4_file;
  std::string combined_mp4_file;
  do {
    if(argc < 4) {
      PRINTF("Please input cmd as below : \n"
          "test_mp4_combine first_mp4_file second_mp4_file combined_mp4_file");
      break;
    }
    first_mp4_file = argv[1];
    second_mp4_file = argv[2];
    combined_mp4_file = argv[3];
    NOTICE("first mp4 file name is %s, second mp4 file name is %s, "
        "combined mp4 file name is %s,", first_mp4_file.c_str(),
        second_mp4_file.c_str(), combined_mp4_file.c_str());
    if(AM_UNLIKELY(!test_mp4_combine(first_mp4_file.c_str(),
                                   second_mp4_file.c_str(),
                                   combined_mp4_file.c_str()))) {
      ERROR("test mp4 combine error.");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

bool test_mp4_combine(const char* first_mp4_file,
                    const char* second_mp4_file, const char* combined_mp4_file)
{
  bool ret = true;
  AMIMp4FileCombiner* file_combiner = NULL;
  do {
    file_combiner = AMIMp4FileCombiner::create(first_mp4_file, second_mp4_file);
    if(file_combiner == NULL) {
      ERROR("Failed to create AMMp4FileCombiner");
      ret = false;
      break;
    }
    if(!file_combiner->get_combined_mp4_file(combined_mp4_file)) {
      ERROR("Failed to combine mp4 file");
      ret = false;
      break;
    }
  }while(0);
  file_combiner->destroy();
  return ret;
}
