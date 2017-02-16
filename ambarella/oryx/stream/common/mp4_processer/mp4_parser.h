/*******************************************************************************
 * mp4_parser.h
 *
 * History:
 *   2015-09-06 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MP4_PARSER_H_
#define AM_MP4_PARSER_H_

#include "iso_base_defs.h"
class Mp4FileReader;
struct FrameInfo {
    uint32_t frame_number;
    uint32_t frame_size;
    uint32_t buffer_size;
    uint8_t  *frame_data;
    FrameInfo();
    ~FrameInfo();
};

 class AMMp4Parser
 {
   public :
     static AMMp4Parser* create(const char* file_path);

   private :
     AMMp4Parser();
     bool init(const char* file_path);
     virtual ~AMMp4Parser();

   public :
     void destroy();
     bool parse_file();
     FileTypeBox* get_file_type_box();
     MediaDataBox* get_media_data_box();
     MovieBox* get_movie_box();
     bool get_video_frame(FrameInfo& frame_info);
     bool get_audio_frame(FrameInfo& frame_info);
   private :
     DISOM_BOX_TAG get_box_type();
     bool parse_file_type_box();
     bool parse_media_data_box();
     bool parse_movie_box();
     bool parse_free_box();

   private :
     uint32_t m_file_offset;
     FileTypeBox *m_file_type_box;
     FreeBox      *m_free_box;
     MediaDataBox *m_media_data_box;
     MovieBox     *m_movie_box;

     Mp4FileReader *m_file_reader;
 };

#endif /* AM_MP4_PARSER_H_ */
