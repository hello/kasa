/*******************************************************************************
 * test_mp4_processer.cpp
 *
 * History:
 *   2015-9-16 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
int main()
{
  int ret = 0;
  uint32_t begin_time_point = 0;
  uint32_t end_time_point = 0;
  std::string source_mp4_file_name;
  std::string new_mp4_file_name;
  do {
    PRINTF("Please input begin time point:");
    std::cin >> begin_time_point;
    PRINTF("Please input end time point:");
    std::cin >> end_time_point;
    PRINTF("Please input source mp4 file name:");
    std::cin >> source_mp4_file_name;
    PRINTF("Please input new mp4 file name:");
    std::cin >> new_mp4_file_name;
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
