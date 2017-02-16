/*******************************************************************************
 * h265.h
 *
 * History:
 *   2015-3-3 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_
/* Generic NAL Unit Header
 * +---------------+---------------+
 * |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |
 * +-------------+-----------------+
 */
#define H265_NAL_HEADER_SIZE          2
#define H265_FU_INDICATOR_SIZE        2
#define H265_PROFILE_TIER_LEVEL_SIZE 12

/*
 * FU Header
 * +---------------+
 * |7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+
 * |S|E|  FuType   |
 * +---------------+
 */

struct AMH265FUHeader
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 7     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t type :6; /* Bit 5 ~ 0 */
#else
    uint8_t type :6; /* Bit 0 ~ 5 */
    uint8_t e    :1; /* Bit 6     */
    uint8_t s    :1; /* Bit 7     */
#endif
};
typedef struct AMH265FUHeader H265_FU_HEADER;

enum H265_NALU_TYPE
{
  H265_TRAIL_N    = 0,
  H265_TRAIL_R    = 1,
  H265_TSA_N      = 2,
  H265_TSA_R      = 3,
  H265_STSA_N     = 4,
  H265_STSA_R     = 5,
  H265_RADL_N     = 6,
  H265_RADL_R     = 7,
  H265_BLA_W_LP   = 16,
  H265_BLA_W_RADL = 17,
  H265_BLA_N_LP   = 18,
  H265_IDR_W_RADL = 19,
  H265_IDR_N_LP   = 20,
  H265_CRA_NUT    = 21,
  H265_VPS        = 32,
  H265_SPS        = 33,
  H265_PPS        = 34,
  H265_AUD        = 35,
  H265_EOS_NUT    = 36,
  H265_EOB_NUT    = 37,
  H265_FD_NUT     = 38,
  H265_SEI_PREFIX = 39,
  H265_SEI_SUFFIX = 40,
};

struct AM_H265_NALU
{
    uint8_t  *addr;    /* start address of this NALU in the bit stream */
    int32_t   type;
    uint32_t  size;    /* Size of this NALU */
    uint32_t  pkt_num; /* How many RTP packets are needed for this NALU */
    AM_H265_NALU() :
      addr(nullptr),
      type(0),
      size(0),
      pkt_num(0)
    {}
    AM_H265_NALU(const AM_H265_NALU &n) :
      addr(n.addr),
      type(n.type),
      size(n.size),
      pkt_num(n.pkt_num)
    {}
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_ */
