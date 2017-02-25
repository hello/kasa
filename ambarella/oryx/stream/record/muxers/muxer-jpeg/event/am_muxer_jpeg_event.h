/*******************************************************************************
 * am_muxer_jpeg_event.h
 *
 * History:
 *   2016-1-27 - [ccjing] created file
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
#ifndef AM_MUXER_JPEG_EVENT_H_
#define AM_MUXER_JPEG_EVENT_H_
#include "am_muxer_jpeg_base.h"
#include "am_record_event_sender.h"
struct EventInfo
{
    bool    enable           = false;
    uint8_t pre_jpeg_num     = 0;
    uint8_t pre_jpeg_count   = 0;
    uint8_t after_jpeg_num   = 0;
    uint8_t after_jpeg_count = 0;
};
struct AMFixedPacketPool;
class AMMuxerJpegEvent : public AMMuxerJpegBase
{
    typedef AMMuxerJpegBase inherited;
  public :
    static AMMuxerJpegEvent* create(const char* config_name);
    virtual AM_MUXER_ATTR get_muxer_attr();
    virtual void feed_data(AMPacket* packet);

  private :
    AMMuxerJpegEvent();
    virtual ~AMMuxerJpegEvent();
    virtual AM_STATE init(const char* config_name);
    virtual AM_STATE generate_file_name(char file_name[]);
    virtual void main_loop();
    virtual AM_STATE on_data_packet(AMPacket* packet);

  private :
    bool process_packet(AMPacket* packet);
    void clear_event_jpeg_params();
    AM_STATE on_event_packet(AMPacket* packet);
    bool update_filename_time_stamp();
    bool parse_event_packet(AMPacket* event_packet);

  private :
    int64_t              m_pts_diff         = 0;
    int64_t              m_last_pts         = 0;
    AMPacket            *m_event_packet     = nullptr;
    AMFixedPacketPool   *m_packet_pool      = nullptr;
    uint32_t             m_packet_size      = 0;
    uint8_t              m_packet_count     = 0;
    uint8_t              m_time_stamp_count = 0;
    EventInfo            m_event_info;
    std::string          m_last_time_stamp;
    packet_queue         m_buf_pkt_queue;
};

#endif /*AM_MUXER_JPEG_EVENT_H_*/
