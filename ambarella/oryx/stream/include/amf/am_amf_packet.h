/*******************************************************************************
 * am_amf_packet.h
 *
 * History:
 *   2014-7-23 - [ypchang] created file
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
#ifndef AM_AMF_PACKET_H_
#define AM_AMF_PACKET_H_

/*******************************************************************************
 * AMPacket
 * |-------0-------|-------1-------|-------2-------|-------3-------|
 * +----------------------- Payload Header-------------------------+
 * |---------------------------------------------------------------|
 * |              Type             |              Attr             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * |-------0-------|-------1-------|-------2-------|-------3-------|
 * +--------------------------Payload Data-------------------------+
 * |---------------------------------------------------------------|
 * |                      Data PTS Low 32bit                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      Data PTS High 32bit                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Stream ID  |  Event  ID    |   Video Type  |  Frame Type   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Data Frame Number                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Data Frame Count                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        Frame    Attribute                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Data Size                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Data Offset                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Data Pointer                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ******************************************************************************/

#include <atomic>

class AMPacketPool;
class AMIPacketPool;
class AMSimplePacketPool;
class AMFixedPacketPool;
class AMPacket
{
    friend class AMPacketPool;
    friend class AMSimplePacketPool;
    friend class AMFixedPacketPool;

  public:
    enum AM_PAYLOAD_TYPE {
      AM_PAYLOAD_TYPE_NONE  = 0,
      AM_PAYLOAD_TYPE_INFO  = 1,
      AM_PAYLOAD_TYPE_DATA  = 2,
      AM_PAYLOAD_TYPE_EOF   = 3,
      AM_PAYLOAD_TYPE_EOS   = 4,
      AM_PAYLOAD_TYPE_EOL   = 5,
      AM_PAYLOAD_TYPE_EVENT = 6
    };

    enum AM_PAYLOAD_ATTR {
      AM_PAYLOAD_ATTR_NONE      = 0,
      AM_PAYLOAD_ATTR_VIDEO     = 1,
      AM_PAYLOAD_ATTR_AUDIO     = 2,
      AM_PAYLOAD_ATTR_SEI       = 3,
      AM_PAYLOAD_ATTR_EVENT_EMG = 4,
      AM_PAYLOAD_ATTR_EVENT_MD  = 5,
      AM_PAYLOAD_ATTR_GPS       = 6,
      AM_PAYLOAD_ATTR_GSENSOR   = 7,
    };

    enum AM_PACKET_TYPE {
      AM_PACKET_TYPE_NONE   = 0,
      AM_PACKET_TYPE_NORMAL = 1 << 0,
      AM_PACKET_TYPE_EVENT  = 1 << 1,
      AM_PACKET_TYPE_STOP   = 1 << 2,
      AM_PACKET_TYPE_SYNC   = 1 << 3,
    };

    struct PayloadHeader
    {
        uint16_t m_payload_type; /* Info | Data | EOS */
        uint16_t m_payload_attr; /* Video | Audio | Sei */
        explicit PayloadHeader() :
          m_payload_type(AM_PAYLOAD_TYPE_NONE),
          m_payload_attr(AM_PAYLOAD_ATTR_NONE){}
        explicit PayloadHeader(const PayloadHeader &header) :
          m_payload_type(header.m_payload_type),
          m_payload_attr(header.m_payload_attr){}
        virtual ~PayloadHeader(){}
    };

    struct PayloadData
    {
        int64_t  m_payload_pts;
        uint8_t *m_buffer;          /* Data buffer */
        uint32_t m_stream_id  : 8;
        uint32_t m_event_id   : 8;
        uint32_t m_video_type : 8;  /* Used to specify video type (H.264) */
        uint32_t m_frame_type : 8;  /* Used to specify video/audio frame type */
        uint32_t m_frame_attr;      /* Used to specify data attribute */
        uint32_t m_frame_num;       /* Frame number */
        uint32_t m_frame_count;     /* Frame count in this payload */
        uint32_t m_size;            /* Size of data pointed by mBuffer */
        uint32_t m_offset;          /* Data offset, used in playback */
        uint32_t m_addr_offset;     /* Address offset, used in video bsinfo */
        explicit PayloadData() :
          m_payload_pts(0LL),
          m_buffer(nullptr),
          m_stream_id(0),
          m_event_id(0),
          m_video_type(0xFF),
          m_frame_type(0),
          m_frame_attr(0),
          m_frame_num(0),
          m_frame_count(0),
          m_size(0),
          m_offset(0),
          m_addr_offset(0)
          {}
        explicit PayloadData(const PayloadData &data) :
          m_payload_pts(data.m_payload_pts),
          m_stream_id(data.m_stream_id),
          m_event_id(data.m_event_id),
          m_video_type(data.m_video_type),
          m_frame_type(data.m_frame_type),
          m_frame_attr(data.m_frame_attr),
          m_frame_num(data.m_frame_num),
          m_frame_count(data.m_frame_count),
          m_size(data.m_size),
          m_offset(data.m_offset),
          m_addr_offset(data.m_addr_offset) {
          if (m_buffer) {
            memcpy(m_buffer, data.m_buffer, data.m_size);
          } else {
            m_buffer = data.m_buffer;
          }
        }
        virtual ~PayloadData(){}
    };

    struct Payload
    {
        PayloadHeader m_header;
        PayloadData   m_data;
        explicit Payload(){}
        explicit Payload(const Payload &payload) {
          m_header = payload.m_header;
          m_data = payload.m_data;
        }
        Payload& operator=(const Payload &payload) {
          m_header = payload.m_header;
          m_data   = payload.m_data;
          return *this;
        }
        virtual ~Payload(){}
    };

  public:
    explicit AMPacket();
    AMPacket(const AMPacket &packet) = delete;
    AMPacket& operator=(const AMPacket &packet) = delete;
    virtual ~AMPacket(){}

  public:
    void add_ref();
    void release();

    AMPacket::Payload* get_payload(AMPacket::AM_PAYLOAD_TYPE type,
                                   AMPacket::AM_PAYLOAD_ATTR attr);
    AMPacket::Payload* get_payload();
    void set_payload(void *payload);
    void set_data_ptr(uint8_t *ptr);
    uint8_t* get_data_ptr();
    uint32_t get_data_size();
    void set_data_size(uint32_t size);
    uint32_t get_data_offset();
    void set_data_offset(uint32_t offset);
    uint32_t get_addr_offset();
    void set_addr_offset(uint32_t offset);
    uint32_t get_packet_type();
    void set_packet_type(uint32_t type);
    AMPacket::AM_PAYLOAD_TYPE get_type();
    void set_type(AMPacket::AM_PAYLOAD_TYPE type);
    AMPacket::AM_PAYLOAD_ATTR get_attr();
    void set_attr(AMPacket::AM_PAYLOAD_ATTR attr);
    int64_t get_pts();
    void set_pts(int64_t pts);
    uint8_t get_event_id();
    void set_event_id(uint8_t id);
    uint8_t get_stream_id();
    void set_stream_id(uint8_t id);
    uint8_t get_video_type();
    void set_video_type(uint8_t type);
    uint8_t get_frame_type();
    void set_frame_type(uint8_t type);
    uint32_t get_frame_attr();
    void set_frame_attr(uint32_t attr);
    uint32_t get_frame_number();
    void set_frame_number(uint32_t num);
    uint32_t get_frame_count();
    void set_frame_count(uint32_t count);

  protected:
    AMIPacketPool  *m_pool;
    AMPacket       *m_next;
    Payload        *m_payload;
    uint32_t        m_packet_type;
    std::atomic_int m_ref_count;
};

#endif /* AM_AMF_PACKET_H_ */
