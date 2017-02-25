/*******************************************************************************
 * am_mp4_file_parser.h
 *
 * History:
 *   2015-09-06 - [ccjing] created file
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
#ifndef AM_MP4_PARSER_H_
#define AM_MP4_PARSER_H_

#include "am_mp4_file_parser_if.h"
#include <vector>
class Mp4FileReader;

struct SampleChunk
{
    uint32_t chunk_num;
    uint32_t sample_count;
};

class AMMp4FileParser : public AMIMp4FileParser
{
    typedef std::vector<SampleChunk*> ChunkList;
  public :
    static AMMp4FileParser* create(const char* file_path);
  private :
    AMMp4FileParser();
    bool init(const char* file_path);
    virtual ~AMMp4FileParser();
  public :
    virtual void destroy();
    virtual bool parse_file();
    virtual FileTypeBox* get_file_type_box();
    virtual MediaDataBox* get_media_data_box();
    virtual MovieBox* get_movie_box();
    virtual bool get_video_frame(FrameInfo& frame_info);
    virtual bool get_audio_frame(FrameInfo& frame_info);
    virtual bool get_gps_frame(FrameInfo& frame_info);
    virtual uint32_t get_video_frame_number();
    virtual uint32_t get_audio_frame_number();
    virtual uint32_t get_gps_frame_number();
  private :
    DISOM_BOX_TAG get_box_type();
    bool parse_file_type_box();
    bool parse_media_data_box();
    bool parse_movie_box();
    bool parse_free_box();
    bool create_chunk_sample_table();
  private :
    FileTypeBox   *m_file_type_box;
    FreeBox       *m_free_box;
    MediaDataBox  *m_media_data_box;
    MovieBox      *m_movie_box;
    Mp4FileReader *m_file_reader;
    ChunkList     *m_video_chunk_list;
    ChunkList     *m_audio_chunk_list;
    ChunkList     *m_gps_chunk_list;
    uint32_t       m_file_offset;
};

#endif /* AM_MP4_PARSER_H_ */
