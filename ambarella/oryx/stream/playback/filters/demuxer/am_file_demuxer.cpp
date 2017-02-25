/*******************************************************************************
 * am_file_demuxer.cpp
 *
 * History:
 *   2014-8-27 - [ypchang] created file
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
#include "am_playback_info.h"
#include "am_file_demuxer_if.h"
#include "am_file_demuxer.h"
#include "am_file_demuxer_object.h"
#include "am_file_demuxer_config.h"
#include "am_file_demuxer_version.h"

#include "am_amf_packet_pool.h"
#include "am_mutex.h"
#include "am_event.h"
#include "am_file.h"

#include <unistd.h>

#ifdef BUILD_AMBARELLA_ORYX_DEMUXER_DIR
#define ORYX_DEMUXER_DIR ((const char*)BUILD_AMBARELLA_ORYX_DEMUXER_DIR)
#else
#define ORYX_DEMUXER_DIR ((const char*)"/usr/lib/oryx/demuxer")
#endif

static const char *demuxer_type_to_str[] =
{
  "Invalid",
  "Unknown",
  "AAC",
  "WAV",
  "OGG",
  "MP4",
  "TS",
  "ES",
  "RTP",
  "UNIX_DOMAIN"
};

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMFileDemuxer::create(engine, config,
                                              input_num, output_num);
}

AMIFileDemuxer* AMFileDemuxer::create(AMIEngine *engine,
                                     const std::string& config,
                                     uint32_t input_num,
                                     uint32_t output_num)
{
  AMFileDemuxer *result = new AMFileDemuxer(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }

  return ((AMIFileDemuxer*)result);
}

void* AMFileDemuxer::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIFileDemuxer) ? (AMIFileDemuxer*)this :
      inherited::get_interface(ref_iid);
}

void AMFileDemuxer::destroy()
{
  inherited::destroy();
}

void AMFileDemuxer::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

void AMFileDemuxer::purge()
{
  if (AM_LIKELY(m_demuxer)) {
    m_demuxer->enable(false);
    m_demuxer = nullptr;
  }
  inherited::purge();
}

AMIPacketPin* AMFileDemuxer::get_input_pin(uint32_t index)
{
  ERROR("%s doesn't have input pin!", m_name);
  return NULL;
}

AMIPacketPin* AMFileDemuxer::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_output_num) ? m_output_pins[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such output pin [index:%u]", index);
  }

  return pin;
}

AM_STATE AMFileDemuxer::add_media(const AMPlaybackUri& uri)
{
  AUTO_MEM_LOCK(m_demuxer_lock);
  AM_STATE         ret = AM_STATE_ERROR;
  AM_DEMUXER_TYPE type = check_media_type(uri);
  if (AM_LIKELY((AM_DEMUXER_INVALID != type) &&
                (AM_DEMUXER_UNKNOWN != type))) {
    switch(m_demuxer_mode) {
      case AM_DEMUXER_MODE_UNKNOWN: {
        switch(uri.type) {
          case AM_PLAYBACK_URI_FILE: {
            m_file_uri_q.push(uri);
            m_demuxer_mode = AM_DEMUXER_MODE_FILE;
            ret = AM_STATE_OK;
          }break;
          case AM_PLAYBACK_URI_RTP: {
            m_rtp_uri_q.push(uri);
            m_demuxer_mode = AM_DEMUXER_MODE_RTP;
            m_demuxer = get_demuxer_codec(type, 0);
            if (AM_UNLIKELY(!m_demuxer)) {
              ERROR("Failed to load demuxer for type %d", type);
            } else {
              m_demuxer->enable(true);
              if (AM_UNLIKELY(!m_demuxer->add_uri(m_rtp_uri_q.front_pop()))) {
                ERROR("Failed to add RTP media!");
                m_demuxer->enable(false);
                m_demuxer = nullptr;
              } else {
                ret = AM_STATE_OK;
              }
            }
          }break;
          case AM_PLAYBACK_URI_UNIX_DOMAIN: {
            m_uds_uri_q.push(uri);
            m_demuxer_mode = AM_DEMUXER_MODE_UDS;
            m_demuxer = get_demuxer_codec(type, 0);
            if (AM_UNLIKELY(!m_demuxer)) {
              ERROR("Failed to load demuxer for type %d", type);
            } else {
              m_demuxer->enable(true);
              if (AM_UNLIKELY(!m_demuxer->add_uri(m_uds_uri_q.front_pop()))) {
                ERROR("Failed to add UNIX domain media!");
                m_demuxer->enable(false);
                m_demuxer = nullptr;
              } else {
                ret = AM_STATE_OK;
              }
            }
          }break;
          default: {
            ERROR("Unknown URI type: %d", uri.type);
          }break;
        }
      }break;
      case AM_DEMUXER_MODE_FILE: {
        switch(uri.type) {
          case AM_PLAYBACK_URI_FILE: {
            m_file_uri_q.push(uri);
            m_demuxer_mode = AM_DEMUXER_MODE_FILE;
            ret = AM_STATE_OK;
          }break;
          case AM_PLAYBACK_URI_RTP: {
            if (AM_LIKELY(!m_demuxer && (m_file_uri_q.size() == 0))) {
              m_rtp_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_RTP;
              ret = AM_STATE_OK;
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
                ret = AM_STATE_ERROR;
              } else {
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_rtp_uri_q.front_pop()))) {
                  ERROR("Failed to add RTP media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                  ret = AM_STATE_ERROR;
                }
              }
            } else {
              ERROR("Demuxer currently is playing files, "
                    "RTP URI be added!");
            }
          }break;
          case AM_PLAYBACK_URI_UNIX_DOMAIN: {
            if (AM_LIKELY(!m_demuxer && (m_file_uri_q.size() == 0))) {
              m_uds_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_UDS;
              ret = AM_STATE_OK;
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
                ret = AM_STATE_ERROR;
              } else {
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_uds_uri_q.front_pop()))) {
                  ERROR("Failed to add UNIX domain media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                  ret = AM_STATE_ERROR;
                }
              }
            } else {
              ERROR("Demuxer currently is playing files, "
                    "UNIX domain URI cannot be added!");
            }
          }break;
          default: {
            ERROR("Unknown URI type: %d", uri.type);
          }break;
        }
      }break;
      case AM_DEMUXER_MODE_RTP: {
        switch(uri.type) {
          case AM_PLAYBACK_URI_FILE: {
            if (AM_LIKELY(!m_demuxer)) {
              m_file_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_FILE;
              ret = AM_STATE_OK;
            } else {
              ERROR("Demuxer currently is playing RTP media, "
                    "files cannot be added!");
            }
          }break;
          case AM_PLAYBACK_URI_RTP: {
            m_rtp_uri_q.push(uri);
            m_demuxer_mode = AM_DEMUXER_MODE_RTP;
            ret = AM_STATE_OK;
            if (AM_LIKELY(!m_demuxer)) {
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
              } else {
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_rtp_uri_q.front_pop()))) {
                  ERROR("Failed to add RTP media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                  ret = AM_STATE_ERROR;
                }
              }
            }
          }break;
          case AM_PLAYBACK_URI_UNIX_DOMAIN: {
            if (AM_LIKELY(!m_demuxer)) {
              m_uds_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_UDS;
              ret = AM_STATE_OK;
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
                ret = AM_STATE_ERROR;
              } else {
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_uds_uri_q.front_pop()))) {
                  ERROR("Failed to add UNIX domain media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                  ret = AM_STATE_ERROR;
                }
              }
            } else {
              ERROR("Demuxer currently is playing RTP media, "
                    "UNIX domain URI cannot be added!");
            }
          }break;
          default: {
            ERROR("Unknown URI type: %d", uri.type);
          }break;
        }
      }break;
      case AM_DEMUXER_MODE_UDS: {
        switch(uri.type) {
          case AM_PLAYBACK_URI_FILE: {
            if (AM_LIKELY(!m_demuxer)) {
              m_file_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_FILE;
              ret = AM_STATE_OK;
            } else {
              ERROR("Demuxer currently is playing UNIX domain URI, "
                    "files cannot be added!");
            }
          }break;
          case AM_PLAYBACK_URI_RTP: {
            if (AM_LIKELY(!m_demuxer)) {
              m_rtp_uri_q.push(uri);
              m_demuxer_mode = AM_DEMUXER_MODE_RTP;
              ret = AM_STATE_OK;
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
                ret = AM_STATE_ERROR;
              } else {
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_rtp_uri_q.front_pop()))) {
                  ERROR("Failed to add RTP media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                  ret = AM_STATE_ERROR;
                }
              }
            } else {
              ERROR("Demuxer currently is playing UNIX domain URI, "
                    "RTP URI cannot be added!");
            }
          }break;
          case AM_PLAYBACK_URI_UNIX_DOMAIN: {
            if (AM_LIKELY(!m_demuxer)) {
              m_demuxer = get_demuxer_codec(type, 0);
              if (AM_UNLIKELY(!m_demuxer)) {
                ERROR("Failed to load demuxer for type %d", type);
              } else {
                m_uds_uri_q.push(uri);
                m_demuxer_mode = AM_DEMUXER_MODE_UDS;
                m_demuxer->enable(true);
                if (AM_UNLIKELY(!m_demuxer->add_uri(m_uds_uri_q.front_pop()))) {
                  ERROR("Failed to add UNIX domain media!");
                  m_demuxer->enable(false);
                  m_demuxer = nullptr;
                } else {
                  ret = AM_STATE_OK;
                }
              }
            } else {
              ERROR("Last UNIX domain media is not finished! "
                    "Please stop it before adding new media!");
            }
          }break;
          default: {
            ERROR("Unknown URI type: %d", uri.type);
          }break;
        }
      }break;
    }
  } else {
    ERROR("Unsupported media type!");
  }
#if 0
  if (AM_LIKELY((AM_DEMUXER_INVALID != type) &&
                (AM_DEMUXER_UNKNOWN != type))) {
    if (AM_LIKELY(!m_demuxer)) {
      /* todo: Currently only 1 stream is supported */
      m_demuxer = get_demuxer_codec(type, 0);
      ret = (m_demuxer ? AM_STATE_OK : AM_STATE_ERROR);
    } else {
      if (AM_LIKELY(m_demuxer->get_demuxer_type() == type)) {
        ret = AM_STATE_OK;
      } else {
        if (AM_LIKELY(m_demuxer->is_play_list_empty())) {
          AM_DESTROY(m_demuxer);
          m_demuxer = get_demuxer_codec(type, 0);
          ret = (m_demuxer ? AM_STATE_OK : AM_STATE_ERROR);
        } else {
          if(type == AM_DEMUXER_RTP) {
            ERROR("Media type of rtp is different from the codec in use!");
          } else {
            ERROR("Media type of %s is different from the codec in use!",
                  uri.media.file);
          }
          ret = AM_STATE_ERROR;
        }
      }
    }
    if (AM_LIKELY(ret == AM_STATE_OK)) {
      m_demuxer->enable(true);
      ret = (m_demuxer->add_uri(uri) ? AM_STATE_OK : AM_STATE_ERROR);
    } else {
      ERROR("Failed to load demuxer plugin!");
    }
  } else {
    ERROR("Unsupportted media type!");
  }
#endif
  return ret;
}

AM_STATE AMFileDemuxer::play(AMPlaybackUri* uri)
{
  AM_STATE ret = (uri == NULL) ? AM_STATE_OK : add_media(*uri);
  AUTO_MEM_LOCK(m_demuxer_lock);
  if (AM_LIKELY((AM_STATE_OK == ret) && !m_run.load())) {
    ret = start();
  }

  return ret;
}

AM_STATE AMFileDemuxer::start()
{
  m_run = true;
  m_demuxer_event->signal();
  return AM_STATE_OK;
}

AM_STATE AMFileDemuxer::stop()
{
  AM_STATE ret = AM_STATE_OK;
  if (AM_UNLIKELY(!m_started.load())) {
    m_demuxer_event->signal();
  }
  if (AM_LIKELY(m_run.load())) {
    m_demuxer_lock.lock();
    m_run = false;
    m_demuxer_lock.unlock();
    ret = inherited::stop();
  }
  return ret;
}

uint32_t AMFileDemuxer::version()
{
  return DEMUXER_VERSION_NUMBER;
}

void AMFileDemuxer::on_run()
{
  uint32_t count = 0;
  AM_DEMUXER_STATE state = AM_DEMUXER_NONE;
  m_started = false;
  ack(AM_STATE_OK);
  m_demuxer_event->clear();
  m_demuxer_event->wait();
  m_started = true;
  INFO("%s starts to run!", m_name);
  while (m_run.load()) {
    AMPacket *packet = nullptr;
    PlaybackUriQ *uri_list = nullptr;

    switch(m_demuxer_mode) {
      case AM_DEMUXER_MODE_FILE: {
        uri_list = &m_file_uri_q;
      }break;
      case AM_DEMUXER_MODE_RTP: {
        uri_list = &m_rtp_uri_q;
      }break;
      case AM_DEMUXER_MODE_UDS: {
        uri_list = &m_uds_uri_q;
      }break;
      case AM_DEMUXER_MODE_UNKNOWN: {
        usleep(m_demuxer_config->file_empty_timeout);
      }break;
    }
    if (AM_LIKELY(!uri_list)) {
      continue;
    }

    if (AM_LIKELY(!uri_list->empty() &&
                  (m_demuxer_mode == AM_DEMUXER_MODE_FILE))) {
      AUTO_MEM_LOCK(m_demuxer_lock);
      AMPlaybackUri uri = uri_list->front();
      AM_DEMUXER_TYPE type = check_media_type(uri);
      if (AM_LIKELY(m_demuxer && (state == AM_DEMUXER_NONE) &&
                    (type != m_demuxer->get_demuxer_type()) &&
                    (m_demuxer_mode == AM_DEMUXER_MODE_FILE))) {
        m_demuxer = nullptr;
      }
      if (AM_UNLIKELY(!m_demuxer)) {
        m_demuxer = get_demuxer_codec(type, 0);
        if (AM_UNLIKELY(!m_demuxer)) {
          ERROR("Failed to load demuxer for type %d", type);
        }
      }
      if (AM_LIKELY(m_demuxer && (type == m_demuxer->get_demuxer_type()) &&
                    (uri.type == AM_PLAYBACK_URI_FILE))) {
        bool added = false;
        m_demuxer->enable(true);
        while (type == check_media_type(uri)) {
          if (AM_UNLIKELY(!m_demuxer->add_uri(uri))) {
            ERROR("Failed to add %s", uri.media.file);
            break;
          } else {
            added = true;
            uri_list->pop();
            if (AM_LIKELY(!uri_list->empty())) {
              uri = uri_list->front();
            } else {
              break;
            }
          }
        }
        if (AM_UNLIKELY(!added)) {
          ERROR("Failed to add media to demuxer!");
          m_demuxer = nullptr;
        }
      }
    }
    m_demuxer_lock.lock();
    if (AM_LIKELY(m_demuxer)) {
      if (AM_UNLIKELY(m_demuxer->is_play_list_empty())) {
        if (AM_LIKELY(++count < m_demuxer_config->wait_count)) {
          m_demuxer_lock.unlock();
          usleep(m_demuxer_config->file_empty_timeout);
          continue;
        }
        if (AM_LIKELY(state == AM_DEMUXER_NO_FILE)) {
          if (m_packet_pool->alloc_packet(packet, 0)) {
            NOTICE("Timeout! no files found! Send %s packet!",
                   uri_list->empty() ? "EOL" : "EOF");
            packet->set_type(uri_list->empty() ?
                AMPacket::AM_PAYLOAD_TYPE_EOL :
                AMPacket::AM_PAYLOAD_TYPE_EOF);
            packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
            packet->set_stream_id(m_demuxer->stream_id());
            send_packet(packet);
            state = AM_DEMUXER_NONE;
          } else {
            NOTICE("Failed to allocate packet. Is packet pool disabled?");
            m_run = false;
          }
        }
        count = 0;
        m_demuxer_lock.unlock();
        usleep(m_demuxer_config->file_empty_timeout);
        continue;
      }
      count = 0;
      state = m_demuxer->get_packet(packet);
      switch(state) {
        case AM_DEMUXER_OK: {
          send_packet(packet);
        }break;
        case AM_DEMUXER_NO_PACKET: {
          usleep(m_demuxer_config->packet_empty_timeout);
        }break;
        case AM_DEMUXER_NO_FILE:
        default: break;
      }
    }
    m_demuxer_lock.unlock();
    /* Just make m_demuxer_lock can be obtained by other thread */
    usleep(1000);
  }
  if (AM_LIKELY(m_demuxer)) {
    m_demuxer->enable(false);
    m_demuxer = nullptr;
  }
  if (AM_UNLIKELY(m_run.load())) {
    NOTICE("%s posts ENG_MSG_ABORT!", m_name);
    post_engine_msg(AMIEngine::ENG_MSG_ABORT);
  }

  INFO("%s exits mainloop!", m_name);
}

AM_DEMUXER_TYPE AMFileDemuxer::check_media_type(const AMPlaybackUri& uri)
{
  AM_DEMUXER_TYPE type = AM_DEMUXER_UNKNOWN;
  switch (uri.type) {
    case AM_PLAYBACK_URI_FILE: {
      const std::string &file_name = uri.media.file;
      if (AM_LIKELY(!file_name.empty() && AMFile::exists(file_name.c_str()))) {
        AMFile media(file_name.c_str());
        if (AM_LIKELY(media.open(AMFile::AM_FILE_READONLY))) {
          char buf[16] = {0};
          ssize_t size = media.read(buf, 12);
          media.close();
          if (12 == size) {
            uint16_t     high = buf[0];
            uint16_t      low = buf[1];
            uint32_t  chunkId = buf[0]|(buf[1]<<8)|(buf[ 2]<<16)|(buf[ 3]<<24);
            uint32_t riffType = buf[8]|(buf[9]<<8)|(buf[10]<<16)|(buf[11]<<24);
            if (buf[0] == 0x47) {
              type = AM_DEMUXER_TS;
            } else if (0x0FFF == ((high << 4) | (low >> 4))) {
              type = AM_DEMUXER_AAC;
            } else if ((buf[4] == 'f') && (buf[5] == 't') &&
                (buf[6] == 'y') && (buf[7] == '[')) {
              type = AM_DEMUXER_MP4;
            } else if (0x00000001 ==
                ((buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3])) {
              type = AM_DEMUXER_ES;
            } else if ((0x46464952 == chunkId) && /* RIFF */
                (0x45564157 == riffType)) {/* wave */
              type = AM_DEMUXER_WAV;
            } else if (0x5367674F == chunkId) {   /* OggS */
              type = AM_DEMUXER_OGG;
            }
          } else {
            ERROR("Failed reading file %s!", file_name.c_str());
          }
        } else {
          ERROR("Failed to open %s for reading!", file_name.c_str());
        }
      } else {
        if (AM_LIKELY(file_name.empty())) {
          ERROR("Empty uri!");
        } else {
          ERROR("%s doesn't exist!", file_name.c_str());
        }
        type = AM_DEMUXER_INVALID;
      }
    }break;
    case AM_PLAYBACK_URI_RTP: {
      switch(uri.media.rtp.audio_type) {
        case AM_AUDIO_G711A:
        case AM_AUDIO_G711U:
        case AM_AUDIO_G726_40:
        case AM_AUDIO_G726_32:
        case AM_AUDIO_G726_24:
        case AM_AUDIO_G726_16:
        case AM_AUDIO_OPUS: {
          type = AM_DEMUXER_RTP;
        }break;
        default: {
          type = AM_DEMUXER_INVALID;
        }break;
      }
    }break;
    case AM_PLAYBACK_URI_UNIX_DOMAIN : {
      type = AM_DEMUXER_UNIX_DOMAIN;
    } break;
    default:
      ERROR("uri type error.");
      break;
  }

  return type;
}

AMFileDemuxer::DemuxerList* AMFileDemuxer::load_codecs()
{
  AMFileDemuxer::DemuxerList *list = NULL;
  std::string demuxer_dir(ORYX_DEMUXER_DIR);
  std::string *codecs = NULL;

  int number = AMFile::list_files(demuxer_dir, codecs);
  if (AM_LIKELY(number > 0)) {
    list = new AMFileDemuxer::DemuxerList();
    for (int i = 0; i < number; ++ i) {
      AMFileDemuxerObject *object = new AMFileDemuxerObject(codecs[i]);
      if (AM_LIKELY(object)) {
        if (AM_LIKELY(object->open())) {
          list->push(object);
        } else {
          delete object;
        }
      }
    }
  } else {
    ERROR("No codecs found!");
  }
  delete[] codecs;

  return list;
}

AMIDemuxerCodec* AMFileDemuxer::get_demuxer_codec(AM_DEMUXER_TYPE type,
                                                  uint32_t stream_id)
{
  AMFileDemuxerObject *obj = NULL;
  uint32_t size = m_demuxer_list->size();
  for (uint32_t i = 0; i < size; ++ i) {
    obj = m_demuxer_list->front_pop();
    m_demuxer_list->push(obj);
    if (AM_LIKELY(obj->m_type == type)) {
      break;
    }
    obj = NULL;
  }
  if (AM_UNLIKELY(!obj)) {
    ERROR("Failed to get demuxer for media type: %s",
          demuxer_type_to_str[type]);
  }

  return (obj ? obj->get_demuxer(stream_id) : NULL);
}

AMFileDemuxer::AMFileDemuxer(AMIEngine *engine) :
  inherited(engine)
{
}

AMFileDemuxer::~AMFileDemuxer()
{
  delete m_config;
  AM_DESTROY(m_demuxer_event);
  AM_DESTROY(m_packet_pool);
  for (uint32_t i = 0; i < m_output_num; ++ i) {
    AM_DESTROY(m_output_pins[i]);
  }
  delete[] m_output_pins;
  while (m_demuxer_list && !m_demuxer_list->empty()) {
    delete m_demuxer_list->front_pop();
  }
  delete m_demuxer_list;
  DEBUG("~AMFileDemuxer");
}

AM_STATE AMFileDemuxer::init(const std::string& config,
                             uint32_t input_num,
                             uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;

  m_input_num  = input_num;
  m_output_num = output_num;
  do {
    m_config = new AMFileDemuxerConfig();
    if (AM_UNLIKELY(NULL == m_config)) {
      ERROR("Failed to create config module for FileDemuxer filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_demuxer_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_demuxer_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    } else {
      state = inherited::init((const char*)m_demuxer_config->name.c_str(),
                              m_demuxer_config->real_time.enabled,
                              m_demuxer_config->real_time.priority);
      if (AM_LIKELY(AM_STATE_OK == state)) {
        if (AM_UNLIKELY(m_input_num)) {
          WARN("%s should not have input, but input num is set to %u, "
               "reset to 0", m_name, m_input_num);
          m_input_num = 0;
        }

        if (AM_UNLIKELY(0 == m_output_num)) {
          WARN("%s should have at least 1 output, but output is 0, reset to 1!",
               m_name);
          m_output_num = 1;
        }
        m_packet_pool = AMFixedPacketPool::create(
            "DemuxerPacketPool",
            m_demuxer_config->packet_pool_size,
            m_demuxer_config->packet_size);
        if (AM_UNLIKELY(NULL == m_packet_pool)) {
          ERROR("Failed to create packet pool for %s", m_name);
          state = AM_STATE_NO_MEMORY;
          break;
        }
        m_demuxer_event = AMEvent::create();
        if (AM_UNLIKELY(!m_demuxer_event)) {
          ERROR("Failed to create AMEvent!");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        m_output_pins = new AMFileDemuxerOutput*[m_output_num];
        if (AM_UNLIKELY(NULL == m_output_pins)) {
          ERROR("Failed to allocate memory for output pin pointers");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        memset(m_output_pins, 0, sizeof(AMFileDemuxerOutput*) * m_output_num);
        for (uint32_t i = 0; i < m_output_num; ++ i) {
          m_output_pins[i] = AMFileDemuxerOutput::create(this);
          if (AM_UNLIKELY(!m_output_pins[i])) {
            ERROR("Failed to create output pin[%u] for %s", i, m_name);
            state = AM_STATE_ERROR;
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
        if (AM_UNLIKELY(NULL == (m_demuxer_list = load_codecs()))) {
          ERROR("Failed to load demuxers plugin!");
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
  }while(0);

  return state;
}

void AMFileDemuxer::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    for (uint32_t i = 0 ; i < m_output_num; ++ i) {
      packet->add_ref();
      m_output_pins[i]->send_packet(packet);
    }
    packet->release();
  }
}
