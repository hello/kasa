/*******************************************************************************
 * rtp.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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

#define RTP_HEADER_SIZE          sizeof(AMRtpHeader)
#define RTP_PAYLOAD_TYPE_PCMU    0
#define RTP_PAYLOAD_TYPE_PCMA    8
#define RTP_PAYLOAD_TYPE_JPEG    26
#define RTP_PAYLOAD_TYPE_H264    96
#define RTP_PAYLOAD_TYPE_H265    97
#define RTP_PAYLOAD_TYPE_OPUS    98
#define RTP_PAYLOAD_TYPE_AAC     99
#define RTP_PAYLOAD_TYPE_G726    100
#define RTP_PAYLOAD_TYPE_G726_16 101
#define RTP_PAYLOAD_TYPE_G726_24 102
#define RTP_PAYLOAD_TYPE_G726_32 103
#define RTP_PAYLOAD_TYPE_G726_40 104
#define RTP_PAYLOAD_TYPE_SPEEX   105

#endif /* RTP_H_ */
