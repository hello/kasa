/*******************************************************************************
 * test_am_file_sink.cpp
 *
 * History:
 *   2014-12-15 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include <iostream>
#include <fstream>

#include "am_file_sink_if.h"

bool test_file_writer();
bool test_file_reader();
//bool test_file_get_line();

int main(int argc, char* argv[])
{
  int ret = 0;
  do {
    bool result = true;
    if (AM_UNLIKELY(!(result = test_file_writer()))) {
      ERROR("Test file writer error!");
      ret = -1;
      break;
    }
    if (AM_UNLIKELY(!(result = test_file_reader()))) {
      ERROR("Test file reader error!");
      ret = -1;
      break;
    }

  } while (0);

  return ret;
}

bool test_file_writer()
{
  bool ret = true;
  std::string write_string = "test_am_file_sink.cpp, this is the test program"
                             " of the am_file_sink";
  const char* file_name = "/tmp/test_am_file_sink";
  AMIFileWriter* file_writer = AMIFileWriter::create();
  do {
    if (AM_UNLIKELY(!file_writer)) {
      ERROR("Failed to create file_writer and file_reader!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->create_file(file_name))) {
      ERROR("Failed to create file!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_file(write_string.c_str(),
                                             write_string.length()))) {
      ERROR("Failed to write data to file!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->set_buf(512 * 1024))) {
      ERROR("Failed to set IO buffer!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->seek_file(3, SEEK_SET))) {
      ERROR("Failed to seek file!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_file("////", 4))) {
      ERROR("Failed to write file!");
      ret = false;
      break;
    }
  } while (0);

  file_writer->close_file();
  file_writer->destroy();

  return ret;
}

bool test_file_reader()
{
  bool ret = true;
  char * read_string = NULL;
  const char* file_name = "/proc/self/mounts";
  uint64_t size = 0;
  int read_size = 0;
  read_string = new char[10];
  AMIFileReader* file_reader = AMIFileReader::create();

  do {
    if(AM_UNLIKELY(!file_reader)){
      ERROR("Failed to create file_reader!");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_reader->open_file(file_name))) {
      ERROR("Failed to open file: %s", file_name);
      ret = false;
      break;
    }
    size = file_reader->get_file_size();
    PRINTF("File size = %llu", size);
    read_size = file_reader->read_file(read_string, 5);
    read_string[5] = '\0';
    PRINTF("read data: %s, size = %d", read_string, read_size);
    if (AM_UNLIKELY(!file_reader->seek_file(3))) {
      ERROR("Failed to seek file!");
      ret = false;
      break;
    }
    memset(read_string, 0, 10);
    read_size = file_reader->read_file(read_string, 5);
    read_string[5] = '\0';
    PRINTF("read data: %s, size = %d", read_string, read_size);
    read_size = file_reader->read_file(read_string, 5);
    read_string[5] = '\0';
    PRINTF("read data: %s, size = %d", read_string, read_size);

    uint64_t curr_position = 0;
    if(!file_reader->tell_file(curr_position)) {
      ERROR("Failed to tell file.");
      ret = false;
      break;
    }
    PRINTF("Current position: %llu", curr_position);
    if (AM_UNLIKELY(!file_reader->advance_file(2))) {
      ERROR("Failed to advance file!");
      ret = false;
      break;
    }
    memset(read_string, 0, 10);
    read_size = file_reader->read_file(read_string, 5);
    read_string[5] = '\0';
    PRINTF("read data: %s, size = %d", read_string, read_size);
  } while (0);

  file_reader->close_file();
  delete read_string;
  file_reader->destroy();

  return ret;
}
