/*******************************************************************************
 * am_demuxer_wav.cpp
 *
 * History:
 *   2014-11-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
#include "am_demuxer_wav.h"
#include "am_audio_define.h"
#include "wav.h"

#include <queue>
#include <unistd.h>

#define PACKET_POOL_SIZE 32
#define PCM_DATA_SIZE    2048
#define G726_DATA_SIZE   960
#define G711_DATA_SIZE   960

typedef std::queue<WavChunkData*> WavChunkDataQ;

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_WAV;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerWav::create(streamid);
}

AMIDemuxerCodec* AMDemuxerWav::create(uint32_t streamid)
{
  AMDemuxerWav *demuxer = new AMDemuxerWav(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = NULL;
  }
  return demuxer;
}

AM_DEMUXER_STATE AMDemuxerWav::get_packet(AMPacket *&packet)
{
  AM_DEMUXER_STATE ret = AM_DEMUXER_OK;

  packet = NULL;
  while (NULL == packet) {
    if (AM_UNLIKELY(NULL == m_media)) {
      m_is_new_file = (NULL != (m_media = get_new_file()));
      if (AM_UNLIKELY(!m_media)) {
        ret = AM_DEMUXER_NO_FILE;
        break;
      }
    }

    if (AM_LIKELY(m_media && (m_media->is_open() ||
                              m_media->open(AMFile::AM_FILE_READONLY)))) {
      if (AM_LIKELY(allocate_packet(packet))) {
        char* buffer = (char*)packet->get_data_ptr();

        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->set_stream_id(m_stream_id);
        packet->set_pts(0LL);

        if (AM_UNLIKELY(m_is_new_file)) {
          m_is_new_file = false;
          if (wav_file_parser(*((AM_AUDIO_INFO*)buffer), *m_media)) {
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
            packet->set_frame_type(m_audio_codec_type);
            packet->set_data_size(sizeof(AM_AUDIO_INFO));
            continue;
          } else {
            ERROR("Invalid WAV file: %s, skip!", m_media->name());
          }
        } else {
          ssize_t readSize = m_media->read(buffer, m_read_data_size);
          if (AM_UNLIKELY(readSize <= 0)) {
            if (readSize < 0) {
              ERROR("%s: %s! Skip!", m_media->name(), strerror(errno));
            } else {
              INFO("%s EOF", m_media->name());
            }
          } else {
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
            packet->set_frame_type(m_audio_codec_type);
            packet->set_data_size(readSize);
            packet->set_data_offset(0);
            continue;
          }
        }
        delete m_media;
        m_media = NULL;
        packet->release();
        packet = NULL;
      } else {
        ret = AM_DEMUXER_NO_PACKET;
        break;
      }
    }
  }

  return ret;
}

void AMDemuxerWav::destroy()
{
  enable(false);
  inherited::destroy();
}

AMDemuxerWav::AMDemuxerWav(uint32_t streamid) :
    inherited(AM_DEMUXER_WAV, streamid),
    m_is_new_file(true),
    m_audio_codec_type(AM_AUDIO_CODEC_NONE),
    m_read_data_size(0)
{}

AMDemuxerWav::~AMDemuxerWav()
{
  DEBUG("~AMDemuxerWav");
}

AM_STATE AMDemuxerWav::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    state = inherited::init();
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      break;
    }

    m_packet_pool = AMFixedPacketPool::create("WavDemuxerPacketPool",
                                              PACKET_POOL_SIZE,
                                              PCM_DATA_SIZE);
    if (AM_UNLIKELY(!m_packet_pool)) {
      ERROR("Failed to create packet pool for WAV demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}

bool AMDemuxerWav::wav_file_parser(AM_AUDIO_INFO& audioInfo, AMFile& wav)
{
  bool ret = false;

  memset(&audioInfo, 0, sizeof(audioInfo));
  if (AM_LIKELY(wav.is_open())) {
    WavRiffHdr wavRiff;
    ssize_t readSize = wav.read((char*)&wavRiff, sizeof(wavRiff));
    if (AM_LIKELY((readSize == sizeof(wavRiff)) && wavRiff.is_chunk_ok())) {
      WavChunkDataQ wavChunkDataQ;
      WavChunkData* chunkData = NULL;
      WavChunkHdr*  chunkHdr = NULL;
      do {
        chunkData = new WavChunkData();
        chunkHdr = (WavChunkHdr*)chunkData;
        if (AM_LIKELY(chunkHdr)) {
          readSize = wav.read((char*)chunkHdr, sizeof(*chunkHdr));
          if (AM_UNLIKELY(readSize != sizeof(*chunkHdr))) {
            delete chunkData;
            chunkData = NULL;
          } else {
            char* id = (char*)&chunkHdr->chunk_id;
            INFO("Found WAV chunk: \"%c%c%c%c\", header size: %u",
                 id[0], id[1], id[2], id[3], chunkHdr->chunk_size);
            if (AM_LIKELY(!chunkData->is_data_chunk())) {
              char* data = chunkData->get_chunk_data(chunkHdr->chunk_size);
              if (AM_LIKELY(data)) {
                readSize = wav.read(data, chunkHdr->chunk_size);
                if (AM_UNLIKELY(readSize != (ssize_t)(chunkHdr->chunk_size))) {
                  delete chunkData;
                  chunkData = NULL;
                } else {
                  wavChunkDataQ.push(chunkData);
                }
              } else {
                delete chunkData;
                chunkData = NULL;
              }
            } else {
              wavChunkDataQ.push(chunkData);
              ret = true; /* WAV Header parse done */
            }
          }
        }
      } while(chunkData && !chunkData->is_data_chunk());
      if (AM_LIKELY(ret)) {
        WavChunkData* wavChunkData = NULL;
        size_t count = wavChunkDataQ.size();

        for (uint32_t i = 0; i < count; ++ i) {
          wavChunkData = wavChunkDataQ.front();
          wavChunkDataQ.pop();
          wavChunkDataQ.push(wavChunkData);
          if (AM_LIKELY(wavChunkData->is_fmt_chunk())) {
            break;
          }
          wavChunkData = NULL;
        }
        if (AM_LIKELY(wavChunkData)) {
          WavFmtBody* wavFmtBody = (WavFmtBody*)(wavChunkData->chunk_data);
          audioInfo.channels = wavFmtBody->channels;
          audioInfo.sample_rate = wavFmtBody->sample_rate;
          switch(wavFmtBody->audio_fmt) {
            case WAV_FORMAT_LPCM  : {
              audioInfo.type = AM_AUDIO_LPCM;
              audioInfo.sample_size = wavFmtBody->bits_per_sample / 8;
              audioInfo.sample_format = AM_SAMPLE_S16LE;
              m_audio_codec_type = AM_AUDIO_CODEC_LPCM;
              m_read_data_size = PCM_DATA_SIZE;
              audioInfo.chunk_size = m_read_data_size;
            }break;
            case WAV_FORMAT_G726 : {
              switch(wavFmtBody->bits_per_sample) {
                case 2: audioInfo.type = AM_AUDIO_G726_16; break;
                case 3: audioInfo.type = AM_AUDIO_G726_24; break;
                case 4: audioInfo.type = AM_AUDIO_G726_32; break;
                case 5: audioInfo.type = AM_AUDIO_G726_40; break;
                default:
                  break;
              }
              audioInfo.sample_size = wavFmtBody->bits_per_sample / 8;
              m_audio_codec_type = AM_AUDIO_CODEC_G726;
              m_read_data_size = G726_DATA_SIZE;
              audioInfo.chunk_size = m_read_data_size;
            }break;
            case WAV_FORMAT_MS_ALAW: {
              audioInfo.sample_size = wavFmtBody->bits_per_sample / 8;
              audioInfo.type = AM_AUDIO_G711A;
              audioInfo.sample_format = AM_SAMPLE_ALAW;
              m_audio_codec_type = AM_AUDIO_CODEC_G711;
              m_read_data_size = G711_DATA_SIZE;
              audioInfo.chunk_size = m_read_data_size;
            }break;
            case WAV_FORMAT_MS_ULAW: {
              audioInfo.sample_size = wavFmtBody->bits_per_sample / 8;
              audioInfo.type = AM_AUDIO_G711U;
              audioInfo.sample_format = AM_SAMPLE_ULAW;
              m_audio_codec_type = AM_AUDIO_CODEC_G711;
              m_read_data_size = G711_DATA_SIZE;
              audioInfo.chunk_size = m_read_data_size;
            }break;
            default: {
              audioInfo.type = AM_AUDIO_NULL;
            }break;
          }
        } else {
          ERROR("Cannot find FMT chunk in %s", wav.name());
          ret = false;
        }
        for (uint32_t i = 0; i < count; ++ i) {
          delete wavChunkDataQ.front();
          wavChunkDataQ.pop();
        }
      } else {
        ERROR("Cannot find data chunk in %s", wav.name());
      }
    } else if (AM_UNLIKELY(readSize < 0)) {
      ERROR("Read %s error: %s", wav.name(), strerror(errno));
    }
  } else {
    ERROR("File %s is not open!", wav.name());
  }
  INFO("sample rate : %u, channels : %u, pkt_pts_increment : %u, "
      "sample size : %u, chunk size : %u, sample format : %d, type : %d",
      audioInfo.sample_rate,
      audioInfo.channels,
      audioInfo.pkt_pts_increment,
      audioInfo.sample_size,
      audioInfo.chunk_size,
      audioInfo.sample_format,
      audioInfo.type);
  return ret;
}
