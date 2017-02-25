/*******************************************************************************
 * am_demuxer_ogg.cpp
 *
 * History:
 *   2014-11-11 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_file.h"
#include "am_mutex.h"
#include "am_audio_type.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_pool.h"

#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_demuxer_ogg.h"
#include "am_audio_define.h"

#include "am_queue.h"

#include <ogg/ogg.h>
#include <speex_header.h>
#include <speex.h>

#define PACKET_POOL_SIZE      32
#define OPUS_DATA_SIZE        4096
#define OGGSEEK_BYTES_TO_READ 8192

enum OGG_STATE
{
  OGG_STATE_OK,
  OGG_STATE_ERROR,
  OGG_STATE_CONTINUE,
  OGG_STATE_DONE,
};

class LogicalStream
{
    friend class Ogg;
  public:
    LogicalStream() :
      m_page_granule(0),
      m_page_granule_prev(0),
      m_spx_header(nullptr),
      m_packet_num(0),
      m_page_packet_count(0),
      /*m_spx_frame_size(0),*/
      m_serial_no(0),
      m_pre_skip_sample(0),
      /*m_spx_lookahead(0),*/
      m_codec(AM_AUDIO_CODEC_NONE)
    {
      memset(&m_stream_state, 0, sizeof(m_stream_state));
      memset(&m_audio_info, 0, sizeof(m_audio_info));
    }
    ~LogicalStream()
    {
      ogg_stream_clear(&m_stream_state);
      if (AM_LIKELY(m_spx_header)) {
        speex_header_free(m_spx_header);
      }
    }

    bool init(ogg_page* page)
    {
      m_serial_no = ogg_page_serialno(page);
      INFO("New logical stream: %08x", m_serial_no);
      return (0 == ogg_stream_init(&m_stream_state, m_serial_no));
    }

    void reset_serial(int serial)
    {
      ogg_stream_reset_serialno(&m_stream_state, serial);
    }

    OGG_STATE get_one_packet(ogg_packet* packet)
    {
      int val = ogg_stream_packetout(&m_stream_state, packet);
      OGG_STATE ret =
          (val < 0) ? OGG_STATE_ERROR :
                      ((val == 0) ? OGG_STATE_CONTINUE : OGG_STATE_OK);
      m_packet_num += (val < 0) ? 0 : 1;
      return ret;
    }

    OGG_STATE output_packet_from_page(ogg_page* page, ogg_packet* packet)
    {
      OGG_STATE ret = OGG_STATE_OK;

      if (AM_UNLIKELY(!input_page(page))) {
        ERROR("Failed reading Ogg bitstream data!");
        ret = OGG_STATE_ERROR;
      } else {
        int val = ogg_stream_packetout(&m_stream_state, packet);
        ret =
            (val < 0) ? OGG_STATE_ERROR :
                        ((val == 0) ? OGG_STATE_CONTINUE : OGG_STATE_OK);
        if (AM_UNLIKELY(OGG_STATE_ERROR == ret)) {
          ERROR("Failed to read packet!");
        }
      }

      return ret;
    }

    bool input_page(ogg_page* page)
    {
      bool ret = false;
      if (0 == ogg_stream_pagein(&m_stream_state, page)) {
        m_page_granule_prev = m_page_granule;
        m_page_granule = ogg_page_granulepos(page);
        m_packet_num = 0;
        m_page_packet_count = ogg_page_packets(page);
        ret = true;
      }
      return ret;
    }

    void read_opus_hdr(ogg_packet *packet)
    {
      oggpack_buffer opb;

      m_codec = AM_AUDIO_CODEC_OPUS;
      oggpack_readinit(&opb, packet->packet, packet->bytes);
      oggpack_adv(&opb, 64); /* Skip OpusHead  */
      oggpack_adv(&opb, 8);  /* Skip verion_id */
      m_audio_info.channels = oggpack_read(&opb, 8);
      m_audio_info.type = AM_AUDIO_OPUS;
      m_audio_info.sample_rate = 48000;
      m_audio_info.sample_size = sizeof(int16_t);
      m_pre_skip_sample = oggpack_read(&opb, 16);
      NOTICE("Pre-skip sample: %u", m_pre_skip_sample);
      /*mPreSkipSample = AM_MAX(80*48, mPreSkipSample);*/
    }

    void read_speex_hdr(ogg_packet *packet)
    {
      m_spx_header = speex_packet_to_header((char*)packet->packet,
                                                   packet->bytes);
     /* void *spx_state = NULL;
      const SpeexMode *spx_mode = NULL;
      spx_mode = speex_lib_get_mode(m_spx_header->mode);
      spx_state = speex_decoder_init(spx_mode);
      if (AM_LIKELY(spx_state)) {
        speex_decoder_ctl(spx_state, SPEEX_GET_FRAME_SIZE, &m_spx_frame_size);
        speex_decoder_ctl(spx_state, SPEEX_GET_LOOKAHEAD, &m_spx_lookahead);
        speex_decoder_destroy(spx_state);
      }*/
      m_codec = AM_AUDIO_CODEC_SPEEX;
      m_audio_info.channels = m_spx_header->nb_channels;
      m_audio_info.type = AM_AUDIO_SPEEX;
      m_audio_info.sample_rate = m_spx_header->rate;
      m_audio_info.sample_size = sizeof(int16_t);
      m_audio_info.codec_info = m_spx_header;
      m_pre_skip_sample = 0;
    }

  private:
    int64_t             m_page_granule;
    int64_t             m_page_granule_prev;
    SpeexHeader        *m_spx_header;
    /* Indicates the packet number in this page
     * This is increased every time a packet is out from a page */
    uint32_t            m_packet_num;
    uint32_t            m_page_packet_count;
    /*int32_t             m_spx_frame_size;*/
    int                 m_serial_no;
    int                 m_pre_skip_sample;
    /*int                 m_spx_lookahead;*/
    AM_AUDIO_CODEC_TYPE m_codec;
    ogg_stream_state    m_stream_state;
    AM_AUDIO_INFO       m_audio_info;
};

class Ogg
{
    friend class AMDemuxerOgg;
    typedef AMSafeQueue<LogicalStream*> OggStreamQ;

  public:
    static Ogg* create()
    {
      Ogg* result = new Ogg();
      if (AM_UNLIKELY(result && !result->init())) {
        delete result;
        result = NULL;
      }
      return result;
    }

  public:
    void destroy()
    {
      delete this;
    }

    void reset()
    {
      ogg_sync_reset(&m_sync_state);
      stream_release();
      m_pkt_count = 0;
    }

    bool get_packet(AMPacket*& packet, AMFile& ogg)
    {
      ogg_packet op;
      ogg_page og;
      bool ret = true;
      OGG_STATE opState = OGG_STATE_ERROR;
      bool got_packet = false;

      while (!got_packet && ret) {
        while (!m_stream ||
            (OGG_STATE_OK != (opState = m_stream->get_one_packet(&op)))) {
          OGG_STATE state = read_page(ogg, &og);
          switch (state) {
            case OGG_STATE_OK: {
              if (AM_UNLIKELY(ogg_page_bos(&og))) {
                LogicalStream* newSt = NULL;
                do {
                  if (AM_LIKELY(m_stream)) {
                    /* This BOS means this stream is chained without an EOS */
                    if ((AM_UNLIKELY((m_pkt_count > 0) &&
                                     (m_stream->m_serial_no ==
                                         ogg_page_serialno(&og))))) {
                      ERROR("Chaining without changing serial number: "
                          "Last: 0x%08x, Current: 0x%08x",
                          m_stream->m_serial_no, ogg_page_serialno(&og));
                      break;
                    } else if (0 == m_pkt_count) {
                      m_pkt_count = 0;
                      del_logical_stream(m_stream);
                      m_stream = NULL;
                    } else {
                      ERROR("Multiplexed Opus OGG stream, "
                          "only chained stream is supported!");
                      break;
                    }
                  }
                  newSt = new LogicalStream();
                  if (AM_UNLIKELY(!newSt)) {
                    ERROR("Failed to create new stream!");
                    state = OGG_STATE_ERROR;
                    ret = false;
                    break;
                  }
                  if (AM_UNLIKELY(!newSt->init(&og))) {
                    ERROR("Failed to initialize Ogg stream!");
                    state = OGG_STATE_ERROR;
                    break;
                  }
                  if (AM_LIKELY(add_logical_stream(newSt))) {
                    m_stream = newSt;
                    m_stream->input_page(&og);
                  }
                }while(0);

                if (AM_UNLIKELY(state == OGG_STATE_ERROR)) {
                  delete newSt;
                  newSt = NULL;
                  ret = false;
                }
              } else if (AM_UNLIKELY(ogg_page_eos(&og))) {
                state = OGG_STATE_DONE;
                opState = OGG_STATE_DONE;
                ret = true;
                got_packet = true;
                packet->set_data_size(0);
              } else if (AM_LIKELY(m_stream->m_serial_no ==
                  ogg_page_serialno(&og))) {
                m_stream->input_page(&og);
              }
            }break;
            case OGG_STATE_ERROR: {
              ret = false;
            }break;
            case OGG_STATE_DONE: {
              packet->set_data_size(0);
              opState = OGG_STATE_DONE;
              ret = true;
            }break;
            default:break;
          }
          if (AM_UNLIKELY(OGG_STATE_OK != state)) {
            reset();
            break;
          }
        }
        if (AM_LIKELY(OGG_STATE_OK == opState)) {
          ++ m_pkt_count;
          switch(op.packetno) {
            case 0: { /* Header */
              if (AM_LIKELY((op.bytes >= 8) &&
                            (0 == memcmp(op.packet, "OpusHead", 8)))) {
                if (AM_UNLIKELY((OGG_STATE_OK ==
                    m_stream->get_one_packet(&op)) ||
                                (og.header[og.header_len - 1] == 255))) {
                  ERROR("Extra packets on initial header page!");
                  ret = false;
                } else {
                  m_stream->read_opus_hdr(&op);
                  ret = true;
                }
              } else if (AM_LIKELY((op.bytes >= 8) &&
                                   (0 == memcmp(op.packet, "Speex   ", 8)))) {
                m_stream->read_speex_hdr(&op);
                ret = true;
              } else {
                ERROR("OGG stream not supported: "
                    "Only OggOpus/OggSpeex are supported!");
                ret = false;
              }
              if (AM_LIKELY(ret)) {
                memcpy(((AM_AUDIO_INFO*)packet->get_data_ptr()),
                       &m_stream->m_audio_info,
                       sizeof(m_stream->m_audio_info));
                packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
                packet->set_frame_type(m_stream->m_codec);
                packet->set_data_size(sizeof(AM_AUDIO_INFO));
                got_packet = true;
              }
            }break;
            case 1: {
              char *header = NULL;
              int length = 0;
              switch(m_stream->m_codec) {
                case AM_AUDIO_CODEC_OPUS: {
                  if (AM_LIKELY((op.bytes > 8) &&
                                (0 == memcmp(op.packet, "OpusTags", 8)))) {
                    header = (char*)op.packet + 8;
                    length = op.bytes - 8;
                  }
                }break;
                case AM_AUDIO_CODEC_SPEEX:
                default: {
                  header = (char*)op.packet;
                  length = op.bytes;
                }break;
              }
              print_comments_header(header, length);
              if (AM_UNLIKELY((OGG_STATE_OK ==
                  m_stream->get_one_packet(&op)) ||
                              (og.header[og.header_len - 1] == 255))) {
                ERROR("Extra packets on initial tags page! Invalid stream!");
                ret = false;
              }
            }break;
            default: { /* Data */
              memcpy(packet->get_data_ptr(), op.packet, op.bytes);
              packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
              packet->set_frame_type(m_stream->m_codec);
              packet->set_data_size(op.bytes);
              ret = true;
              got_packet = true;
              switch(m_stream->m_codec) {
                case AM_AUDIO_CODEC_SPEEX: {
                  if (AM_UNLIKELY(op.packetno <=
                                  (1+m_stream->m_spx_header->extra_headers))) {
                    /* Skip extra headers */
                    got_packet = false;
                  } else {
                    packet->set_data_offset(m_stream->m_pre_skip_sample);
                  }
                }break;
                case AM_AUDIO_CODEC_OPUS: {
                  packet->set_data_offset(
                      (op.packetno == 2) ? m_stream->m_pre_skip_sample : 0);
                }break;
                default: {
                  packet->set_data_offset(0);
                }break;
              }
            }break;
          }
        }
      }

      return ret;
    }

  private:
    OGG_STATE read_page(AMFile& ogg, ogg_page* page)
    {
      OGG_STATE ret = OGG_STATE_OK;
      char* buffer = NULL;
      ssize_t readSize = 0;

      while (ogg_sync_pageout(&m_sync_state, page) != 1) {
        buffer = ogg_sync_buffer(&m_sync_state, OGGSEEK_BYTES_TO_READ);
        if (AM_UNLIKELY(!buffer)) {
          ret = OGG_STATE_ERROR;
          break;
        }
        if (AM_LIKELY((readSize = ogg.read(buffer,
                                           OGGSEEK_BYTES_TO_READ)) <= 0)) {
          ret = ((readSize == 0) ? OGG_STATE_DONE : OGG_STATE_ERROR);
          break;
        }
        ogg_sync_wrote(&m_sync_state, readSize);
      }

      return ret;
    }

    bool add_logical_stream(LogicalStream* stream)
    {
      bool added = false;
      size_t count = m_ogg_stream_q->size();
      for (size_t i = 0; i < count; ++ i) {
        LogicalStream* st = m_ogg_stream_q->front_pop();
        m_ogg_stream_q->push(st);
        if (AM_UNLIKELY(st->m_serial_no == stream->m_serial_no)) {
          added = true;
          break;
        }
      }
      if (AM_LIKELY(!added)) {
        m_ogg_stream_q->push(stream);
      } else {
        ERROR("Logical Stream with serial No.: 0x%08x is already added!",
              stream->m_serial_no);
      }
      return !added;
    }

    void del_logical_stream(LogicalStream*& stream)
    {
      size_t count = m_ogg_stream_q->size();
      for (size_t i = 0; i < count; ++ i) {
        LogicalStream *st = m_ogg_stream_q->front_pop();
        if (AM_LIKELY(st == stream)) {
          delete stream;
          stream = NULL;
          break;
        } else {
          m_ogg_stream_q->push(st);
        }
      }
    }
#define readint(buf, base)            \
    (((buf[base+3]<<24)&0xff000000) | \
     ((buf[base+2]<<16)&0x00ff0000) | \
     ((buf[base+1]<< 8)&0x0000ff00) | \
     (buf[base]&0x000000ff))

    void print_comments_header(char *comments, int length)
    {
      do {
        char *head = comments;
        char *end  = comments + length;
        int len = 0;
        int fields = 0;
        std::string str;

        if (AM_UNLIKELY(!comments || (0 == length))) {
          break;
        }
        if (AM_LIKELY(length < 8)) {
          NOTICE("Invalid or corrupted comments!");
          break;
        }
        len = readint(head, 0);
        head += sizeof(int);

        if (AM_UNLIKELY((len < 0) || ((head + len) > end))) {
          NOTICE("Invalid or corrupted comments!");
          break;
        }
        str = std::string(head, len);
        INFO("%s", str.c_str());
        head += len;

        if (AM_UNLIKELY((head + sizeof(int)) > end)) {
          NOTICE("Invalid or corrupted comments!");
          break;
        }

        fields = readint(head, 0);
        head += sizeof(int);

        for (int i = 0; i < fields; ++ i) {
          std::size_t pos;
          if (AM_UNLIKELY((head + sizeof(int)) > end)) {
            NOTICE("Invalid or corrupted comments!");
            break;
          }
          len = readint(head, 0);
          head += sizeof(int);
          if (AM_UNLIKELY((len < 0) || ((head + len) > end))) {
            NOTICE("Invalid or corrupted comments!");
            break;
          }
          str = std::string(head, len);
          pos = str.find("=");

          INFO("%20s: %s",
               str.substr(0, pos).c_str(),
               str.substr(pos + 1).c_str());
          head += len;
        }
      }while(0);
    }

  private:
    Ogg() :
      m_ogg_stream_q(nullptr),
      m_stream(nullptr),
      m_pkt_count(0)
    {}

    virtual ~Ogg()
    {
      stream_release();
      delete m_ogg_stream_q;
      DEBUG("~Ogg");
    }

    bool init()
    {
      bool ret = false;
      do {
        if (0 != ogg_sync_init(&m_sync_state)) {
          ERROR("Failed to initialize ogg_sync_state!");
          break;
        }
        if (AM_UNLIKELY(NULL == (m_ogg_stream_q = new OggStreamQ))) {
          ERROR("Failed to allocate OggStreamQueue!");
          break;
        }
        ret = true;
      } while (0);

      return ret;
    }
    void stream_release()
    {
      while (m_ogg_stream_q && !m_ogg_stream_q->empty()) {
        delete m_ogg_stream_q->front_pop();
      }
      m_stream = NULL;
    }

  private:
    OggStreamQ    *m_ogg_stream_q;
    LogicalStream *m_stream;
    uint32_t       m_pkt_count;
    ogg_sync_state m_sync_state;
};
/*
 * AMDemuxerOgg
 */

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_OGG;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerOgg::create(streamid);
}

AMIDemuxerCodec* AMDemuxerOgg::create(uint32_t streamid)
{
  AMDemuxerOgg *demuxer = new AMDemuxerOgg(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = NULL;
  }

  return demuxer;
}

bool AMDemuxerOgg::is_drained()
{
  return (m_packet_pool->get_avail_packet_num() == PACKET_POOL_SIZE);
}

AM_DEMUXER_STATE AMDemuxerOgg::get_packet(AMPacket *&packet)
{
  AM_DEMUXER_STATE state = AM_DEMUXER_OK;

  packet = NULL;
  while (NULL == packet) {
    if (AM_UNLIKELY(NULL == m_media)) {
      m_is_new_file = (NULL != (m_media = get_new_file()));
      if (AM_UNLIKELY(!m_media)) {
        state = AM_DEMUXER_NO_FILE;
        break;
      }
      if (AM_LIKELY(m_is_new_file)) {
        m_ogg->reset(); /* Reset OGG state when new file is get */
      }
    }

    if (AM_LIKELY(m_media && (m_media->is_open() ||
                              m_media->open(AMFile::AM_FILE_READONLY)))) {
      if (AM_LIKELY(allocate_packet(packet))) {
        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->set_stream_id(m_stream_id);
        packet->set_pts(0);

        if (AM_LIKELY(m_ogg->get_packet(packet, *m_media))) {
          if (AM_LIKELY(packet->get_data_size() > 0)) {
            break; /* Data is get, break out */
          } else {
            INFO("%s EOF", m_media->name());
          }
        }
        delete m_media;
        m_media = NULL;
        packet->release();
        packet = NULL;
      } else {
        state = AM_DEMUXER_NO_PACKET;
        break;
      }
    }
  }

  return state;
}

void AMDemuxerOgg::destroy()
{
  enable(false);
  inherited::destroy();
}

AMDemuxerOgg::AMDemuxerOgg(uint32_t streamid) :
    inherited(AM_DEMUXER_OGG, streamid),
    m_ogg(nullptr),
    m_audio_codec_type(AM_AUDIO_CODEC_NONE),
    m_is_new_file(true)
{
}

AMDemuxerOgg::~AMDemuxerOgg()
{
  AM_DESTROY(m_ogg);
  DEBUG("~AMDemuxerOgg");
}

AM_STATE AMDemuxerOgg::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    state = inherited::init();
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      break;
    }

    m_packet_pool = AMFixedPacketPool::create("OggDemuxerPacketPool",
                                              PACKET_POOL_SIZE,
                                              OPUS_DATA_SIZE);
    if (AM_UNLIKELY(!m_packet_pool)) {
      ERROR("Failed to create packet pool for OGG demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    if (AM_UNLIKELY(NULL == (m_ogg = Ogg::create()))) {
      ERROR("Failed to create OGG demuxer!");
      state = AM_STATE_ERROR;
      break;
    }

  }while(0);

  return state;
}
