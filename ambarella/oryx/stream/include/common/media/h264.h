/*******************************************************************************
 * h264.h
 *
 * History:
 *   2014-12-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_

/* Generic Nal header
 *
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */

struct AMH264HeaderOctet
{
#ifdef BIGENDIAN
    uint8_t f    :1; /* Bit 7     */
    uint8_t nri  :2; /* Bit 6 ~ 5 */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t nri  :2; /* Bit 5 ~ 6 */
    uint8_t f    :1; /* Bit 7     */
#endif
};

typedef struct AMH264HeaderOctet H264_NAL_HEADER;
typedef struct AMH264HeaderOctet H264_FU_INDICATOR;
typedef struct AMH264HeaderOctet H264_STAP_HEADER;
typedef struct AMH264HeaderOctet H264_MTAP_HEADER;

/*  FU header
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |S|E|R|  Type   |
 *  +---------------+
 */

struct AMH264FUHeader
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 7     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t r    :1; /* Bit 5     */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t r    :1; /* Bit 5     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t s    :1; /* Bit 7     */
#endif
};

typedef struct AMH264FUHeader H264_FU_HEADER;

enum H264_NALU_TYPE
{
  H264_IBP_HEAD = 0x01,
  H264_IDR_HEAD = 0x05,
  H264_SEI_HEAD = 0x06,
  H264_SPS_HEAD = 0x07,
  H264_PPS_HEAD = 0x08,
  H264_AUD_HEAD = 0x09,
};

struct AM_H264_NALU
{
    uint8_t        *addr;    /* Start address of this NALU in the bit stream */
    H264_NALU_TYPE  type;
    uint32_t        size;    /* Size of this NALU */
    uint32_t       pkt_num; /* How many RTP packets are needed for this NALU */
    AM_H264_NALU() :
      addr(nullptr),
      type(H264_IBP_HEAD),
      size(0),
      pkt_num(0)
    {}
    AM_H264_NALU(const AM_H264_NALU &n) :
      addr(n.addr),
      type(n.type),
      size(n.size),
      pkt_num(n.pkt_num)
    {}
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_ */
