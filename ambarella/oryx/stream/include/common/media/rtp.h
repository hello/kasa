/*******************************************************************************
 * rtp.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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
#ifndef RTP_H_
#define RTP_H_

/* RTP Header
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|   CC  |M|      PT     |         sequence number       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            synchronization source (SSRC) identifier           |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

/*
 * This structure works only in little endian system
 */
struct AMRtpHeader
{
    uint8_t csrccount :4;
    uint8_t extension :1;
    uint8_t padding   :1;
    uint8_t version   :2;
    uint8_t payloadtype :7;
    uint8_t marker      :1;
    uint16_t sequencenumber;
    uint32_t timestamp;
    uint32_t ssrc;

    void set_mark(bool m)
    {
      marker = (m ? 1 : 0);
    }

    void set_type(uint8_t pt)
    {
      payloadtype = pt;
    }

    void set_version(uint8_t ver)
    {
      version = ver;
    }

    void set_seq_num(uint16_t seqnum)
    {
      sequencenumber = ((seqnum & 0xff00u) >> 8) | ((seqnum & 0x00ffu) << 8);
    }

    void set_time_stamp(uint32_t ts)
    {
      timestamp = ((ts & 0x000000ffu) << 24) |
                  ((ts & 0x0000ff00u) <<  8) |
                  ((ts & 0x00ff0000u) >>  8) |
                  ((ts & 0xff000000u) >> 24);
    }

    void set_ssrc(uint32_t num)
    {
      ssrc = ((num & 0x000000ffu) << 24) |
             ((num & 0x0000ff00u) <<  8) |
             ((num & 0x00ff0000u) >>  8) |
             ((num & 0xff000000u) >> 24);
    }
};

struct AMRtpTcpHeader
{
    uint8_t magic_num;
    uint8_t channel;
    uint16_t data_len;

    void set_magic_num()
    {
      magic_num = 0x24; /* '$' */
    }

    void set_channel_id(uint8_t id)
    {
      channel = id;
    }

    void set_data_length(uint16_t len)
    {
      data_len = ((len & 0xff00u) >> 8) | ((len & 0x00ffu) << 8);
    }
};

enum AM_RTP_PAYLOAD_TYPE
{
  RTP_PAYLOAD_TYPE_PCMU    = 0,
  RTP_PAYLOAD_TYPE_PCMA    = 8,
  RTP_PAYLOAD_TYPE_JPEG    = 26,
  RTP_PAYLOAD_TYPE_H264    = 96,
  RTP_PAYLOAD_TYPE_H265    = 97,
  RTP_PAYLOAD_TYPE_OPUS    = 98,
  RTP_PAYLOAD_TYPE_AAC     = 99,
  RTP_PAYLOAD_TYPE_G726    = 100,
  RTP_PAYLOAD_TYPE_G726_16 = 101,
  RTP_PAYLOAD_TYPE_G726_24 = 102,
  RTP_PAYLOAD_TYPE_G726_32 = 103,
  RTP_PAYLOAD_TYPE_G726_40 = 104,
  RTP_PAYLOAD_TYPE_SPEEX   = 105,
};

#define RTP_HEADER_SIZE          sizeof(AMRtpHeader)

#endif /* RTP_H_ */
