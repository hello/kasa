/**
 * am_utils.h
 *
 * History:
 *	2012/2/29 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_UTILS_H__
#define __AM_UTILS_H__

inline uint16_t GetBe16(uint8_t* pData)
{
  return (pData[1] | (pData[0]<<8));
}

inline uint32_t GetBe32(uint8_t* pData)
{
  return (pData[3] | (pData[2]<<8) | (pData[1]<<16) | (pData[0]<<24));
}

#define DBEW64(x, p) do { \
  p[0] = (x >> 56) & 0xff; \
  p[1] = (x >> 48) & 0xff; \
  p[2] = (x >> 40) & 0xff; \
  p[3] = (x >> 32) & 0xff; \
  p[4] = (x >> 24) & 0xff; \
  p[5] = (x >> 16) & 0xff; \
  p[6] = (x >> 8) & 0xff; \
  p[7] = x & 0xff; \
} while(0)

#endif
