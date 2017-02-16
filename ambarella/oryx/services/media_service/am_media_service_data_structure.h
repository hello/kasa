/*******************************************************************************
 * am_media_service_data_structure.h
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
#ifndef AM_MEDIA_SERVICE_DATA_STRUCTURE_H_
#define AM_MEDIA_SERVICE_DATA_STRUCTURE_H_
#include "am_api_media.h"

struct AudioFileList{
  public:
    enum {
      MAX_FILENAME_LENGTH = 490,
      MAX_FILE_NUMBER     = 2,
    };
    AudioFileList(){
      file_number = 0;
      for(uint32_t i = 0; i< MAX_FILE_NUMBER; ++ i) {
        memset(file_list[i], 0, sizeof(file_list[i]));
      }
    }
  public:
    char file_list[MAX_FILE_NUMBER][MAX_FILENAME_LENGTH];
    uint32_t file_number;
};

class AMApiPlaybackAudioFileList : public AMIApiPlaybackAudioFileList
{

  public:
    AMApiPlaybackAudioFileList();
    AMApiPlaybackAudioFileList(AMIApiPlaybackAudioFileList* audio_file);
    virtual ~AMApiPlaybackAudioFileList();
    virtual bool add_file(const std::string &file_name);
    virtual std::string get_file(uint32_t file_number);
    virtual uint32_t get_file_number();
    virtual bool is_full();
    virtual char* get_file_list();
    virtual uint32_t get_file_list_size();
    virtual void clear_file();
  private:
    AudioFileList m_list;
};

#endif /* AM_MEDIA_SERVICE_DATA_STRUCTURE_H_ */
