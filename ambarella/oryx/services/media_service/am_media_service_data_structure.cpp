/*******************************************************************************
 * am_media_service_data_structure.cpp
 *
 * History:
 *   May 13, 2015 - [ccjing] created file
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
#include "am_media_service_data_structure.h"
AMIApiPlaybackAudioFileList* AMIApiPlaybackAudioFileList::create()
{
  AMApiPlaybackAudioFileList* m_list = new AMApiPlaybackAudioFileList();
  return m_list ? (AMIApiPlaybackAudioFileList*)m_list : NULL;
}

AMIApiPlaybackAudioFileList* AMIApiPlaybackAudioFileList::create(
    AMIApiPlaybackAudioFileList* audio_file)
{
  AMApiPlaybackAudioFileList* m_list = new AMApiPlaybackAudioFileList(audio_file);
  return m_list ? (AMIApiPlaybackAudioFileList*)m_list : NULL;
}

AMApiPlaybackAudioFileList::AMApiPlaybackAudioFileList()
{}

AMApiPlaybackAudioFileList::AMApiPlaybackAudioFileList(
    AMIApiPlaybackAudioFileList* audio_file)
{
  m_list.file_number = audio_file->get_file_number();
  for (uint32_t i = 0; i < m_list.file_number; ++ i) {
    memcpy(m_list.file_list[i],  audio_file->get_file(i).c_str(),
           sizeof(m_list.file_list[i]));
  }
}

AMApiPlaybackAudioFileList::~AMApiPlaybackAudioFileList()
{
  m_list.file_number = 0;
}

bool AMApiPlaybackAudioFileList::add_file(const std::string &file_name)
{
  bool ret = true;
  if(file_name.empty()) {
    ERROR("file_name is empty, can not add it to file list, drop it.");
  } else if (m_list.file_number >= AudioFileList::MAX_FILE_NUMBER) {
    ERROR("This file list is full, the max file number is %d, "
        "please create a new file list.", AudioFileList::MAX_FILE_NUMBER);
    ret = false;
  } else if (file_name.size() >= AudioFileList::MAX_FILENAME_LENGTH) {
    ERROR("file name is too length, the max file name length is %d",
          AudioFileList::MAX_FILENAME_LENGTH);
    ret = false;
  } else {
    memcpy(m_list.file_list[m_list.file_number], file_name.c_str(),
            file_name.size());
    ++ m_list.file_number;
  }
  return ret;
}

std::string AMApiPlaybackAudioFileList::get_file(uint32_t file_number)
{
  if (file_number > 0 && file_number <= m_list.file_number) {
    return std::string(m_list.file_list[file_number - 1]);
  } else {
    WARN("Do not have this file, return a empty string");
    return std::string("");
  }
}

uint32_t AMApiPlaybackAudioFileList::get_file_number()
{
  return m_list.file_number;
}

bool AMApiPlaybackAudioFileList::is_full()
{
  return (m_list.file_number >= AudioFileList::MAX_FILE_NUMBER) ? true : false;
}

char* AMApiPlaybackAudioFileList::get_file_list()
{
  return (char*)&m_list;
}

uint32_t AMApiPlaybackAudioFileList::get_file_list_size()
{
  return sizeof(AudioFileList);
}

void AMApiPlaybackAudioFileList::clear_file()
{
  for (uint32_t i = 0; i < AudioFileList::MAX_FILE_NUMBER; ++ i) {
    memset(m_list.file_list[i], 0, sizeof(m_list.file_list[i]));
  }
  m_list.file_number = 0;
}

