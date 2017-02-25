/*******************************************************************************
 * rtcp.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_RTCP_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_RTCP_H_

/* RTCP Sender Report
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    RC   |   PT=SR=200   |             length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         SSRC of sender                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |              NTP timestamp, most significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             NTP timestamp, least significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         RTP timestamp                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     sender's packet count                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      sender's octet count                     |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

/*
 * This structure works only in little endian system
 */

/* #define byteswap16(a) ((((a) & 0xff00u) >> 8) | (((a) & 0x00ffu) << 8))

#define byteswap32(a)            \
  ((((a) & 0x000000ffu) << 24) | \
   (((a) & 0x0000ff00u) <<  8) | \
   (((a) & 0x00ff0000u) >>  8) | \
   (((a) & 0xff000000u) >> 24))
*/

#define byteswap16(a) __builtin_bswap16(a)
#define byteswap32(a) __builtin_bswap32(a)

#define h2ns(a) byteswap16(a)
#define h2nl(a) byteswap32(a)
#define n2hs(a) byteswap16(a)
#define n2hl(a) byteswap32(a)

enum AM_RTCP_TYPE
{
  AM_RTCP_SR   = 200,
  AM_RTCP_RR   = 201,
  AM_RTCP_SDES = 202,
  AM_RTCP_BYE  = 203,
  AM_RTCP_APP  = 204,
};

struct AMRtcpHeader
{
    uint8_t  count   :5; /* 0 */
    uint8_t  padding :1; /* 0 */
    uint8_t  version :2; /* 2 */
    uint8_t  pkt_type;   /* SR = 200 */
    uint16_t length;     /* 28 bytes */
    uint32_t ssrc;
    AMRtcpHeader(uint8_t type = 200) :
      count(0),
      padding(0),
      version(2),
      pkt_type(type),
      length(0),
      ssrc(0)
    {}

    uint8_t get_count()
    {
      return count;
    }

    void set_packet_type(uint8_t type)
    {
      pkt_type = type;
    }

    void set_packet_length(uint16_t len)
    {
      length = h2ns(len);
    }

    void set_ssrc(uint32_t _ssrc)
    {
      ssrc = h2nl(_ssrc);
    }

    uint32_t get_ssrc()
    {
      return n2hl(ssrc);
    }
};

struct AMRtcpSRPayload
{
    uint32_t ntp_time_h32   = 0;
    uint32_t ntp_time_l32   = 0;
    uint32_t rtp_time_stamp = 0;
    uint32_t packet_count   = 0;
    uint32_t packet_bytes   = 0;

    void set_ntp_time_h32(uint32_t h32)
    {
      ntp_time_h32 = h2nl(h32);
    }

    void set_ntp_time_l32(uint32_t l32)
    {
      ntp_time_l32 = h2nl(l32);
    }

    void set_time_stamp(uint32_t ts)
    {
      rtp_time_stamp = h2nl(ts);
    }

    void set_packet_count(uint32_t pc)
    {
      packet_count = h2nl(pc);
    }

    void add_packet_count(uint32_t pc)
    {
      packet_count = h2nl((n2hl(packet_count) + pc));
    }

    void set_packet_bytes(uint32_t pb)
    {
      packet_bytes = h2nl(pb);
    }

    uint32_t get_ntp_time_h32()
    {
      return n2hl(ntp_time_h32);
    }

    uint32_t get_ntp_time_l32()
    {
      return n2hl(ntp_time_l32);
    }

    uint32_t get_time_stamp()
    {
      return n2hl(rtp_time_stamp);
    }

    uint32_t get_packet_count()
    {
      return n2hl(packet_count);
    }

    uint32_t get_packet_bytes()
    {
      return n2hl(packet_bytes);
    }

    void add_packet_bytes(uint32_t pb)
    {
      packet_bytes = h2nl((n2hl(packet_bytes) + pb));
    }

};

/* RTCP Receiver Report
 * |       0       |       1       |       2       |       3       |
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    RC   |   PT=SR=200   |             length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         SSRC of sender                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                 SSRC_1 (SSRC of first source)                 | report
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
 * | fraction lost |       cumulative number of packets lost       |   1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           extended highest sequence number received           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      interarrival jitter                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         last SR (LSR)                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   delay since last SR (DLSR)                  |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

struct AMRtcpRRPayload {
    uint32_t sourcessrc;
    uint32_t fractionlost   :8;
    uint32_t packetslost    :24;
    uint32_t sequencenum;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;

    uint32_t get_source_ssrc()
    {
      return n2hl(sourcessrc);
    }

    uint8_t get_fraction_lost()
    {
      return (uint8_t)fractionlost;
    }

    int32_t get_packet_lost()
    {
      uint32_t tmp = n2hl(packetslost << 8);
      return (int32_t)((0x00800000 & tmp) ? (0xFF000000 | tmp) : tmp);
    }

    uint32_t get_sequence_number()
    {
      return n2hl(sequencenum);
    }

    uint32_t get_jitter()
    {
      return n2hl(jitter);
    }

    uint32_t get_lsr()
    {
      return n2hl(lsr);
    }

    uint32_t get_dlsr()
    {
      return n2hl(dlsr);
    }
};

#define RTCP_SR_PACKET_SIZE \
  ((uint16_t)(sizeof(AMRtcpHeader) + sizeof(AMRtcpSRPayload)))

#define RTCP_RR_PACKET_SIZE \
  ((uint16_t)(sizeof(AMRtcpHeader) + sizeof(AMRtcpRRPayload)))

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_RTCP_H_ */
