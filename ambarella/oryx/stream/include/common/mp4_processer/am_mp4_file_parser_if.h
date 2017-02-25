/*******************************************************************************
 * am_mp4_file_parser_if.h
 *
 * History:
 *   2016-1-26 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef AM_MP4_FILE_PARSER_IF_H_
#define AM_MP4_FILE_PARSER_IF_H_
#include "version.h"
#include "iso_base_defs.h"
struct FrameInfo
{
    uint8_t  *frame_data = nullptr;
    uint32_t  frame_number = 0;
    uint32_t  frame_size = 0;
    uint32_t  buffer_size = 0;
    ~FrameInfo()
    {
      delete[] frame_data;
    }
};

class AMIMp4FileParser
{
  public :
    static AMIMp4FileParser* create(const char* file_path);
    virtual void destroy()      = 0;
    virtual bool parse_file()   = 0;
    virtual FileTypeBox* get_file_type_box()   = 0;
    virtual MediaDataBox* get_media_data_box() = 0;
    virtual MovieBox* get_movie_box()          = 0;
    virtual bool get_video_frame(FrameInfo& frame_info) = 0;
    virtual bool get_audio_frame(FrameInfo& frame_info) = 0;
    virtual bool get_gps_frame(FrameInfo& frame_info)   = 0;
    virtual uint32_t get_video_frame_number() = 0;
    virtual uint32_t get_audio_frame_number() = 0;
    virtual uint32_t get_gps_frame_number()   = 0;
  protected:
    virtual ~AMIMp4FileParser() {}
};
#endif /* AM_MP4_FILE_PARSER_IF_H_ */
