/*******************************************************************************
 * am_mp4_file_splitter.h
 *
 * History:
 *   2015-09-10 - [ccjing] created file
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
#ifndef MP4_FILE_SPLITTER_H_
#define MP4_FILE_SPLITTER_H_
#include "am_mp4_file_splitter_if.h"

class AMIMp4FileParser;
struct SplitInfo
{
    uint64_t duration;
    uint32_t video_begin_frame_num;
    uint32_t audio_begin_frame_num;
    uint32_t gps_begin_frame_num;
    uint32_t video_end_frame_num;
    uint32_t audio_end_frame_num;
    uint32_t gps_end_frame_num;
};
class AMMp4FileSplitter : public AMIMp4FileSplitter
{
  public :
    static AMMp4FileSplitter* create(const char *source_mp4_file_path);
  public :
    virtual void destroy();
    virtual bool get_proper_mp4_file(const char* file_path,
                                        uint32_t begin_point_time,
                                        uint32_t end_point_time);
    virtual ~AMMp4FileSplitter();
  private :
    AMMp4FileSplitter();
    bool init(const char *source_mp4_file_path);
    bool find_split_frame_num(SplitInfo& split_info, uint32_t begin_time,
                                uint32_t end_time);
  private :
    AMIMp4FileParser *m_mp4_parser;
};




#endif /* MP4_FILE_SPLITTER_H_ */
