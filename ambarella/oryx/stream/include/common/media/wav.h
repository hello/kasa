/*******************************************************************************
 * wav.h
 *
 * History:
 *   2014-11-10 - [ypchang] created file
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
#ifndef WAV_H_
#define WAV_H_

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT  COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')

struct WavRiffHdr {
    uint32_t chunk_id;       /* 'RIFF'       */
    uint32_t chunk_size;     /* FileSize - 8 */
    uint32_t type;           /* 'WAVE'       */
    WavRiffHdr() :
      chunk_id(0),
      chunk_size(0),
      type(0){}
    bool is_chunk_ok() {
      return ((chunk_id == WAV_RIFF) && (type == WAV_WAVE));
    }
};

struct WavChunkHdr {
    uint32_t chunk_id;
    uint32_t chunk_size;
    WavChunkHdr() :
      chunk_id(0),
      chunk_size(0){}
};

struct WavFmtBody {
    uint16_t audio_fmt;       /* 1 indicates PCM    */
    uint16_t channels;        /* Mono: 1, Stereo: 2 */
    uint32_t sample_rate;     /* 44100, 48000, etc  */
    uint32_t byte_rate;       /* sample_rate * channels * bits_per_sample / 8 */
    uint16_t block_align;     /* channels * bits_per_sample / 8 */
    uint16_t bits_per_sample;
};

struct WavFmtHdr: public WavChunkHdr {
    /* ChunkId 'fmt ' */
    WavFmtHdr() :
      WavChunkHdr(){}
    bool is_chunk_ok() {
      return ((chunk_id == WAV_FMT) && (chunk_size >= sizeof(WavFmtBody)));
    }
};

struct WavFmt {
    WavFmtHdr  fmt_header;
    WavFmtBody fmt_body;
};

struct WavDataHdr: public WavChunkHdr {
    /* ChunkId 'data' */
    WavDataHdr() :
      WavChunkHdr(){}
    bool is_chunk_ok() {
      return (chunk_id == WAV_DATA);
    }
};

struct WavChunkData: public WavChunkHdr {
    char* chunk_data;
    WavChunkData() :
      WavChunkHdr(),
      chunk_data(0){}
    ~WavChunkData()
    {
      delete[] chunk_data;
    }
    char* get_chunk_data(uint32_t size)
    {
      delete[] chunk_data;
      if (size) {
        chunk_data = new char[size];
      }
      return chunk_data;
    }
    bool is_fmt_chunk()
    {
      return (chunk_id == WAV_FMT);
    }
    bool is_data_chunk()
    {
      return (chunk_id == WAV_DATA);
    }
};
#endif /* WAV_H_ */
