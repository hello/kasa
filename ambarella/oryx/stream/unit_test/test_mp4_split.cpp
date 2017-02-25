/*******************************************************************************
 * test_mp4_processer.cpp
 *
 * History:
 *   2015-9-16 - [ccjing] created file
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
#include "am_mp4_file_splitter_if.h"

bool test_mp4_split(uint32_t begin_time_point, uint32_t end_time_point,
                    const char* source_mp4_file_name,
                    const char* new_mp4_file_name);
int main(int argc, char*argv[])
{
  int ret = 0;
  uint32_t begin_time_point = 0;
  uint32_t end_time_point = 0;
  std::string source_mp4_file_name;
  std::string new_mp4_file_name;
  do {
    if(argc < 5) {
      PRINTF("Please input cmd as below : \n"
          "test_mp4_split source_mp4_file_name begin_time_point "
          "end_time_point new_mp4_file_name");
      break;
    }
    source_mp4_file_name = argv[1];
    begin_time_point = atoi(argv[2]);
    end_time_point = atoi(argv[3]);
    new_mp4_file_name = argv[4];
    NOTICE("begin time point is %u, end time point is %u, source file name is %s,"
        "new mp4 file name is %s", begin_time_point, end_time_point,
        source_mp4_file_name.c_str(), new_mp4_file_name.c_str());
    if(AM_UNLIKELY(!test_mp4_split(begin_time_point, end_time_point,
                                   source_mp4_file_name.c_str(),
                                   new_mp4_file_name.c_str()))) {
      ERROR("test mp4 split error.");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

bool test_mp4_split(uint32_t begin_time_point, uint32_t end_time_point,
                    const char* source_mp4_file_name,
                    const char* new_mp4_file_name)
{
  bool ret = true;
  AMIMp4FileSplitter* file_splitter = NULL;
  do {
    file_splitter = AMIMp4FileSplitter::create(source_mp4_file_name);
    if(file_splitter == NULL) {
      ERROR("Failed to create AMMp4FileSplitter");
      ret = false;
      break;
    }
    if(!file_splitter->get_proper_mp4_file(new_mp4_file_name,begin_time_point,
                                           end_time_point)) {
      ERROR("Failed to split mp4 file");
      ret = false;
      break;
    }
  }while(0);
  file_splitter->destroy();
  return ret;
}
