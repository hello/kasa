/*******************************************************************************
 * am_audio_decoder.cpp
 *
 * History:
 *   2014-9-24 - [ypchang] created file
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

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"
#include "am_audio_define.h"
#include "am_audio_decoder_if.h"
#include "am_audio_decoder.h"
#include "am_audio_decoder_config.h"
#include "am_audio_decoder_version.h"

#include "am_audio_codec_if.h"
#include "am_plugin.h"

#include <unistd.h>

#ifdef BUILD_AMBARELLA_ORYX_CODEC_DIR
#define ORYX_CODEC_DIR ((const char*)BUILD_AMBARELLA_ORYX_CODEC_DIR)
#else
#define ORYX_CODEC_DIR ((const char*)"/usr/lib/oryx/codec")
#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_CODEC_CONF_DIR \
  (const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/codec")
#else
#define ORYX_CODEC_CONF_DIR ((const char*)"/etc/oryx/stream/codec")
#endif

/*
 * AMAudioDecoderPacketPool
 */

AMAudioDecoderPacketPool* AMAudioDecoderPacketPool::create(
    const char *name, uint32_t count)
{
  AMAudioDecoderPacketPool *result = new AMAudioDecoderPacketPool();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name, count)))) {
    delete result;
    result = NULL;
  }
  return result;
}

bool AMAudioDecoderPacketPool::alloc_packet(AMPacket*& packet, uint32_t size)
{
  bool ret = false;
  if (AM_LIKELY(m_data_mem)) {
    ret = AMPacketPool::alloc_packet(packet, size);
  } else {
    ERROR("Data memory is not set!");
  }

  return ret;
}

AM_STATE AMAudioDecoderPacketPool::set_buffer(uint8_t *buf, uint32_t dataSize)
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY(buf && (dataSize > 0))) {
    m_data_mem = buf;
    for (uint32_t i = 0; i < m_packet_count; ++ i) {
      /*m_packet_mem[i].m_payload->m_data.m_buffer =
          (m_data_mem + (i * dataSize));
      */
      m_packet_mem[i].set_data_ptr((m_data_mem + (i * dataSize)));
    }
    state = AM_STATE_OK;
  } else {
    if (AM_LIKELY(!buf)) {
      ERROR("Invalid buffer %p", buf);
    }
    if (AM_LIKELY(dataSize == 0)) {
      ERROR("Invalid data size %u", dataSize);
    }
  }
  return state;
}

void AMAudioDecoderPacketPool::clear_buffer()
{
  m_data_mem = NULL;
  for (uint32_t i = 0; i < m_packet_count; ++ i) {
    m_packet_mem[i].set_data_ptr(NULL);
  }
}

AMAudioDecoderPacketPool::AMAudioDecoderPacketPool() :
    m_payload_mem(NULL),
    m_data_mem(NULL),
    m_packet_count(0)
{}

AMAudioDecoderPacketPool::~AMAudioDecoderPacketPool()
{
  delete[] m_payload_mem;
  DEBUG("~AMAudioDecoderPacketPool");
}
AM_STATE AMAudioDecoderPacketPool::init(const char *name, uint32_t count)
{
  AM_STATE state = inherited::init(name, count, sizeof(AMPacket));
  if (AM_LIKELY(AM_STATE_OK == state)) {
    m_packet_count = count;
    m_payload_mem = new AMPacket::Payload[count];
    if (AM_LIKELY(m_payload_mem)) {
      for (uint32_t i = 0; i < m_packet_count; ++ i) {
        m_packet_mem[i].set_payload(&m_payload_mem[i]);
      }
    } else {
      state = AM_STATE_NO_MEMORY;
    }
  }
  return state;
}


/*
 * AMAudioDecoder
 */
AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMAudioDecoder::create(engine, config,
                                               input_num, output_num);
}

AMIAudioDecoder* AMAudioDecoder::create(AMIEngine *engine,
                                       const std::string& config,
                                       uint32_t input_num,
                                       uint32_t output_num)
{
  AMAudioDecoder *result = new AMAudioDecoder(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }

  return (AMIAudioDecoder*)result;
}

static const char* codec_type_to_str(AM_AUDIO_CODEC_TYPE codec)
{
  const char *str = NULL;
  switch (codec) {
    case AM_AUDIO_CODEC_PASS:  str = "pass";  break;
    case AM_AUDIO_CODEC_AAC:   str = "aac";   break;
    case AM_AUDIO_CODEC_OPUS:  str = "opus";  break;
    case AM_AUDIO_CODEC_LPCM:  str = "lpcm";  break;
    case AM_AUDIO_CODEC_BPCM:  str = "bpcm";  break;
    case AM_AUDIO_CODEC_G726:  str = "g726";  break;
    case AM_AUDIO_CODEC_G711:  str = "g711";  break;
    case AM_AUDIO_CODEC_SPEEX: str = "speex"; break;
    default: str = "None"; break;
  }

  return str;
}

AM_STATE AMAudioDecoder::load_audio_codec(AM_AUDIO_CODEC_TYPE codec)
{
  AM_STATE state = (AM_AUDIO_CODEC_NONE != codec) ? AM_STATE_OK:AM_STATE_ERROR;
  if (AM_LIKELY((AM_AUDIO_CODEC_NONE != codec) &&
                (AM_AUDIO_CODEC_PASS != codec) &&
                (!m_audio_codec || (m_audio_codec &&
                    (codec != m_audio_codec->get_codec_type()))))) {
    /* Here we use file name to search for audio codec,
     * because this is simple and fast, although not smart enough
     */
    const char *codecstr = codec_type_to_str(codec);
    std::string codec = ORYX_CODEC_DIR;
    codec.append("/codec-").append(codecstr).append(".so");

    AM_DESTROY(m_audio_codec);
    AM_DESTROY(m_plugin);
    m_plugin = AMPlugin::create(codec.c_str());

    if (AM_LIKELY(m_plugin)) {
      AudioCodecNew get_audio_codec =
          (AudioCodecNew)m_plugin->get_symbol(AUDIO_CODEC_NEW);
      if (AM_LIKELY(get_audio_codec)) {
        std::string codecConf = ORYX_CODEC_CONF_DIR;
        codecConf.append("/codec-").append(codecstr).append(".acs");
        m_audio_codec = get_audio_codec(codecConf.c_str());
      } else {
        ERROR("Failed to get symbol (%s) from %s!",
              AUDIO_CODEC_NEW, codec.c_str());
      }
    }
    state = (m_audio_codec != NULL) ? AM_STATE_OK : AM_STATE_ERROR;
    m_is_codec_changed = true;
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      ERROR("Failed to create audio codec %s", codecstr);
    }
  } else if (AM_AUDIO_CODEC_NONE == codec) {
    ERROR("Invalid audio codec type %s!", codec_type_to_str(codec));
    m_is_codec_changed = true;
  } else if (AM_AUDIO_CODEC_PASS != codec) {
    NOTICE("Audio codec %s is already loaded!", codec_type_to_str(codec));
    m_is_codec_changed = false;
  } else {
    NOTICE("Audio type doesn't require a codec, just pass through!");
  }

  return state;
}

uint32_t AMAudioDecoder::version()
{
  return AUDIO_DECODER_VERSION_NUMBER;
}

void* AMAudioDecoder::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIAudioDecoder) ? (AMIAudioDecoder*)this :
      inherited::get_interface(refiid);
}

void AMAudioDecoder::destroy()
{
  inherited::destroy();
}

void AMAudioDecoder::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMAudioDecoder::get_input_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_input_num) ? m_input_pins[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such input pin [index: %u]", index);
  }
  return pin;
}

AMIPacketPin* AMAudioDecoder::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_output_num) ? m_output_pins[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such output pin [index: %u]", index);
  }
  return pin;
}

AM_STATE AMAudioDecoder::stop()
{
  AM_STATE state = AM_STATE_OK;

  if (AM_LIKELY(m_run.load())) {
    m_run = false;
    state = inherited::stop();
  }

  return state;
}

void AMAudioDecoder::on_run()
{
  AMPacketQueueInputPin *input_pin = NULL;
  AMPacket              *input_pkt = NULL;
  bool                  need_break = false;
  bool             data_need_break = false;
  uint32_t             error_count = 0;

  ack(AM_STATE_OK);
  m_run = true;

  INFO("%s starts to run!", m_name);
  while (m_run.load()) {
    if (AM_UNLIKELY(!wait_input_packet(input_pin, input_pkt))) {
      if (AM_LIKELY(!m_run.load())) {
        NOTICE("Stop is called!");
      } else {
        NOTICE("Filter is aborted!");
      }
      break;
    }

    if (AM_LIKELY(input_pkt && (input_pkt->get_attr() ==
                                AMPacket::AM_PAYLOAD_ATTR_AUDIO))) {
      switch(input_pkt->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          need_break = (AM_STATE_OK != on_info(input_pkt));
        }break;
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          data_need_break = (AM_STATE_OK != on_data(input_pkt));
        }break;
        case AMPacket::AM_PAYLOAD_TYPE_EOL:
        case AMPacket::AM_PAYLOAD_TYPE_EOF: {
          need_break = (AM_STATE_OK != on_eof(input_pkt));
        }break;
        default: {
          ERROR("Invalid packet type!");
        }break;
      }
      input_pkt->release();
      if (AM_UNLIKELY((need_break || data_need_break) && m_run.load())) {
        if (AM_LIKELY(need_break)) {
          break;
        }
        if (AM_LIKELY(data_need_break)) {
          ++ error_count;
          NOTICE("Data decoder get errors, error count: %u", error_count);
          if (AM_LIKELY(error_count >=
                        m_decoder_config->decode_error_threshold)) {
            ERROR("Data decoder error count exceeds threshold, abort!");
            break;
          }
        }
      } else {
        error_count = 0;
      }
    } else if (AM_LIKELY(input_pkt)) {
      ERROR("%s received no-audio packet!", m_name);
    } else {
      ERROR("Invalid packet!");
    }
  }
  if (AM_LIKELY(m_audio_codec)) {
    m_audio_codec->finalize();
  }

  if (AM_UNLIKELY((need_break || data_need_break) && m_run)) {
    NOTICE("%s posts ENG_MSG_ABORT!", m_name);
    post_engine_msg(AMIEngine::ENG_MSG_ABORT);
  }
  INFO("%s exits mainloop!", m_name);
}

AM_AUDIO_CODEC_TYPE AMAudioDecoder::audio_type_to_codec_type(AM_AUDIO_TYPE type)
{
  AM_AUDIO_CODEC_TYPE codecType = AM_AUDIO_CODEC_NONE;
  switch(type) {
    case AM_AUDIO_NULL:    codecType = AM_AUDIO_CODEC_NONE;  break;
    case AM_AUDIO_LPCM:
    case AM_AUDIO_BPCM:
    case AM_AUDIO_G711A:
    case AM_AUDIO_G711U:   codecType = AM_AUDIO_CODEC_PASS;  break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: codecType = AM_AUDIO_CODEC_G726;  break;
    case AM_AUDIO_AAC:     codecType = AM_AUDIO_CODEC_AAC;   break;
    case AM_AUDIO_OPUS:    codecType = AM_AUDIO_CODEC_OPUS;  break;
    case AM_AUDIO_SPEEX:   codecType = AM_AUDIO_CODEC_SPEEX; break;
    default:break;
  }
  return codecType;
}

static const char* audio_type_to_str(AM_AUDIO_TYPE type)
{
  const char *ret = "Unknown";
  switch(type) {
    case AM_AUDIO_LPCM:    ret = "lpcm";  break;
    case AM_AUDIO_BPCM:    ret = "bpcm";  break;
    case AM_AUDIO_G711A:   ret = "alaw";  break;
    case AM_AUDIO_G711U:   ret = "ulaw";  break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: ret = "g726";  break;
    case AM_AUDIO_AAC:     ret = "aac";   break;
    case AM_AUDIO_OPUS:    ret = "opus";  break;
    case AM_AUDIO_SPEEX:   ret = "speex"; break;
    default: break;
  }

  return ret;
}

AM_STATE AMAudioDecoder::on_info(AMPacket *packet)
{
  AM_AUDIO_INFO *srcAudioInfo = (AM_AUDIO_INFO*)(packet->get_data_ptr());
  AM_AUDIO_CODEC_TYPE codecType = audio_type_to_codec_type(srcAudioInfo->type);
  AM_STATE state = load_audio_codec(codecType);

  m_is_info_sent = false; /* INFO packet should be sent every time */
  m_is_pass_through = (AM_AUDIO_CODEC_PASS == codecType);

  INFO("\nAudio %s Information"
       "\n      Sample Rate: %u"
       "\n         Channels: %u"
       "\npkt_pts_increment: %u"
       "\n      sample_size: %u"
       "\n       chunk_size: %u"
       "\n    sample_format: %s",
       audio_type_to_str(srcAudioInfo->type),
       srcAudioInfo->sample_rate,
       srcAudioInfo->channels,
       srcAudioInfo->pkt_pts_increment,
       srcAudioInfo->sample_size,
       srcAudioInfo->chunk_size,
       sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT(
           srcAudioInfo->sample_format)).c_str());

  if (AM_LIKELY((AM_STATE_OK == state)) && !m_is_pass_through) {
    state = m_audio_codec->initialize(srcAudioInfo,
                                      AM_AUDIO_CODEC_MODE_DECODE) ?
                                          AM_STATE_OK : AM_STATE_ERROR;
    if (AM_LIKELY(m_audio_codec->is_initialized() && m_is_codec_changed)) {
      uint32_t dataSize = m_audio_codec->get_codec_output_size();
      if (AM_LIKELY(m_audio_data_size < dataSize)) {
        m_audio_data_size = dataSize;
        /* Wait down stream to release all the packets,
         * so that we can change the buffer of the packet pool
         */
        NOTICE("Current buffer size is not sufficient enough!");
        NOTICE("Wait all the packets to be released by down streams!");
        while (m_packet_pool->get_avail_packet_num() !=
               m_decoder_config->packet_pool_size) {
          usleep(10000);
        }
        NOTICE("Re-allocating buffer to %u(bytes) x %u(count) = %u(bytes)!",
               dataSize, m_decoder_config->packet_pool_size,
               dataSize * m_decoder_config->packet_pool_size);
        delete[] m_buffer;
        m_packet_pool->clear_buffer();
        m_buffer = new uint8_t[dataSize * m_decoder_config->packet_pool_size];
        if (AM_LIKELY(m_buffer)) {
          state = m_packet_pool->set_buffer(m_buffer, dataSize);
        } else {
          ERROR("Failed to allocate buffer for %s decoder!",
                m_audio_codec->get_codec_name().c_str());
          AM_DESTROY(m_audio_codec);
          state = AM_STATE_NO_MEMORY;
        }
      }
    }
  } else if (AM_LIKELY(m_is_pass_through)) {
    packet->add_ref();
    send_packet(packet);
  }

  return state;
}

AM_STATE AMAudioDecoder::on_data(AMPacket *packet)
{
  uint32_t    dataSize = packet->get_data_size();

  if (AM_LIKELY(!m_is_pass_through)) {
    uint32_t    usedSize = 0;

    if (AM_LIKELY(dataSize > 0)) {
      uint32_t dataPreSkip = 0;
      bool isSkipSet = false;

      while(dataSize > 0) {
        AMPacket *outputPkt = NULL;
        if (AM_UNLIKELY(!m_packet_pool->alloc_packet(outputPkt, 0))) {
          if (AM_UNLIKELY(m_packet_pool->is_enable())) {
            ERROR("Failed to allocate output packet!");
            m_run = false;
          } else {
            NOTICE("Output pin of %s is disabled!", m_name);
          }
          break;
        } else {
          uint32_t outSize = 0;
          usedSize = m_audio_codec->decode(packet->get_data_ptr() + usedSize,
                                           dataSize,
                                           outputPkt->get_data_ptr(),
                                           &outSize);
          if (AM_UNLIKELY(usedSize == ((uint32_t)-1))) {
            NOTICE("Decoder error: failed to decode %u bytes!", dataSize);
            outputPkt->release();
            break;
          } else {
            AM_AUDIO_INFO* decodedAudioInfo =
                m_audio_codec->get_codec_audio_info();
            if (AM_UNLIKELY(!m_is_info_sent)) {
              AMPacket *audioInfoPkt = NULL;
              if (AM_UNLIKELY(!m_packet_pool->alloc_packet(audioInfoPkt, 0))) {
                NOTICE("Failed to allocate output packet for "
                    "decoded audio information!");
                m_run = false;
                outputPkt->release();
                break;
              } else {
                AM_AUDIO_INFO *outAudioInfo =
                    ((AM_AUDIO_INFO*)audioInfoPkt->get_data_ptr());
                memcpy(outAudioInfo, decodedAudioInfo, sizeof(*outAudioInfo));
                audioInfoPkt->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
                audioInfoPkt->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
                audioInfoPkt->set_data_size((sizeof(decodedAudioInfo)));
                audioInfoPkt->set_stream_id(packet->get_stream_id());
                audioInfoPkt->set_frame_type(
                    (uint16_t)m_audio_codec->get_codec_type());
                send_packet(audioInfoPkt);
                m_is_info_sent = true;
              }
            }
            if (AM_LIKELY(!isSkipSet)) {
              dataPreSkip = packet->get_data_offset() *
                  decodedAudioInfo->sample_size * decodedAudioInfo->channels;
            }
            if (AM_UNLIKELY(dataPreSkip >= outSize)) {
              /* Opus decoder may generate some data needs to be skipped,
               * we need to skip them
               */
              outputPkt->release();
              dataPreSkip -= outSize;
            } else {
              outputPkt->set_data_offset(dataPreSkip);
              outputPkt->set_data_size(outSize);
              outputPkt->set_pts(packet->get_pts());
              outputPkt->set_stream_id(packet->get_stream_id());
              outputPkt->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
              outputPkt->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
              outputPkt->set_frame_type(
                  (uint16_t)m_audio_codec->get_codec_type());
              send_packet(outputPkt);
              isSkipSet = true;
              dataPreSkip = 0;
            }
            dataSize -= usedSize;
          }
        }
      }
    } else {
      NOTICE("Invalid data size: %u!", dataSize);
      dataSize = ((uint32_t)-1);
    }
  } else {
    packet->add_ref();
    send_packet(packet);
  }

  return ((m_is_pass_through || (dataSize == 0)) ?
      AM_STATE_OK : AM_STATE_ERROR);
}

AM_STATE AMAudioDecoder::on_eof(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  if (AM_UNLIKELY(m_is_pass_through)) {
    packet->add_ref();
    send_packet(packet);
    INFO("%s sent %s to stream%hu", m_name,
         (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOF) ? "EOF" : "EOL",
         packet->get_stream_id());
  } else {
    AMPacket *out_packet = NULL;
    if (AM_UNLIKELY(!m_packet_pool->alloc_packet(out_packet, 0))) {
      if (AM_LIKELY(!m_run.load())) {
        NOTICE("Stop is called!");
      } else {
        NOTICE("Failed to allocate output packet for sending EOF!");
        state = AM_STATE_ERROR;
      }
    } else {
      out_packet->set_data_size(0);
      out_packet->set_pts(0);
      out_packet->set_stream_id(packet->get_stream_id());
      out_packet->set_type(packet->get_type());
      out_packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
      out_packet->set_frame_type((uint8_t)m_audio_codec->get_codec_type());
      send_packet(out_packet);
      INFO("%s sent %s to stream%hu, remain packet %u",
           m_name,
           (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOF)? "EOF" : "EOL",
           packet->get_stream_id(),
           m_packet_pool->get_avail_packet_num());
    }
  }

  return state;
}

AMAudioDecoder::AMAudioDecoder(AMIEngine *engine) :
    inherited(engine),
    m_plugin(nullptr),
    m_audio_codec(nullptr),
    m_input_pins(nullptr),
    m_output_pins(nullptr),
    m_packet_pool(nullptr),
    m_config(nullptr),
    m_decoder_config(nullptr),
    m_buffer(nullptr),
    m_audio_data_size(0),
    m_input_num(0),
    m_output_num(0),
    m_run(false),
    m_is_info_sent(false),
    m_is_pass_through(false),
    m_is_codec_changed(true)
{
}

AMAudioDecoder::~AMAudioDecoder()
{
  AM_DESTROY(m_audio_codec);
  AM_DESTROY(m_plugin);
  AM_RELEASE(m_packet_pool);
  for (uint32_t i = 0; i < m_input_num; ++ i) {
    AM_DESTROY(m_input_pins[i]);
  }
  for (uint32_t i = 0; i < m_output_num; ++ i) {
    AM_DESTROY(m_output_pins[i]);
  }
  delete m_config;
  delete[] m_buffer;
  delete[] m_input_pins;
  delete[] m_output_pins;
  DEBUG("~AMAudioDecoder");
}

AM_STATE AMAudioDecoder::init(const std::string& config,
                              uint32_t input_num,
                              uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num  = input_num;
  m_output_num = output_num;

  do {
    m_config = new AMAudioDecoderConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create AMAudioDecoderConfig!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_decoder_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_decoder_config)) {
      ERROR("Cannot get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    } else {
      state = inherited::init((const char*)m_decoder_config->name.c_str(),
                              m_decoder_config->real_time.enabled,
                              m_decoder_config->real_time.priority);
      if (AM_LIKELY(AM_STATE_OK == state)) {
        if (AM_UNLIKELY(!m_input_num)) {
          ERROR("%s doesn't have input! Invalid configuration! Abort!", m_name);
          state = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(!m_output_num)) {
          ERROR("%s doesn't have output! Invalid configuration! Abort!",
                m_name);
          state = AM_STATE_ERROR;
          break;
        }
        m_input_pins = new AMAudioDecoderInput*[m_input_num];
        if (AM_UNLIKELY(!m_input_pins)) {
          ERROR("Failed to allocate memory for input pin pointers!");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        memset(m_input_pins, 0, sizeof(AMAudioDecoderInput*) * m_input_num);
        for (uint32_t i = 0; i < m_input_num; ++ i) {
          m_input_pins[i] = AMAudioDecoderInput::create(this);
          if (AM_UNLIKELY(!m_input_pins[i])) {
            ERROR("Failed to create input pin[%u] for %s!", i, m_name);
            state = AM_STATE_ERROR;
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }

        m_packet_pool = AMAudioDecoderPacketPool::create(
            "AudioDecoderPacketPool", m_decoder_config->packet_pool_size);
        if (AM_UNLIKELY(!m_packet_pool)) {
          ERROR("Failed to create audio decoder packet pool!");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        m_packet_pool->add_ref();

        m_output_pins = new AMAudioDecoderOutput*[m_output_num];
        if (AM_UNLIKELY(!m_output_pins)) {
          ERROR("Failed to allocate memory for output pin pointers!");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        memset(m_output_pins, 0, sizeof(AMAudioDecoderOutput*) * m_output_num);
        for (uint32_t i = 0; i < m_output_num; ++ i) {
          m_output_pins[i] = AMAudioDecoderOutput::create(this);
          if (AM_UNLIKELY(!m_output_pins[i])) {
            ERROR("Failed to create output pin[%u] for %s", i, m_name);
            state = AM_STATE_ERROR;
            break;
          }
          m_output_pins[i]->set_packet_pool(m_packet_pool);
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
      }
    }
  }while(0);

  return state;
}

void AMAudioDecoder::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    for (uint32_t i = 0; i < m_output_num; ++ i) {
      packet->add_ref();
      m_output_pins[i]->send_packet(packet);
    }
    packet->release();
  }
}

std::string AMAudioDecoder::sample_format_to_string(
                             AM_AUDIO_SAMPLE_FORMAT format)
{
  std::string format_str = "";
  switch(format) {
    case AM_SAMPLE_U8:
      format_str = "AM_SAMPLE_U8";
      break;
    case AM_SAMPLE_ALAW :
      format_str = "AM_SAMPLE_ALAW";
      break;
    case AM_SAMPLE_ULAW :
      format_str = "AM_SAMPLE_ULAW";
      break;
    case AM_SAMPLE_S16LE :
      format_str = "AM_SAMPLE_S16LE";
      break;
    case AM_SAMPLE_S16BE :
      format_str = "AM_SAMPLE_S16BE";
      break;
    case AM_SAMPLE_S32LE :
      format_str = "AM_SAMPLE_S32LE";
      break;
    case AM_SAMPLE_S32BE :
      format_str = "AM_SAMPLE_S32BE";
      break;
    case AM_SAMPLE_INVALID :
    default :
      format_str = "invalid";
      break;
  }
  return format_str;
}
