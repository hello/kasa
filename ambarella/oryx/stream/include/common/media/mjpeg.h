/*******************************************************************************
 * mjpeg.h
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_

#include <vector>

struct AMJpegHeader
{
    uint8_t typeSpecific;
    uint8_t fregOffset3;
    uint8_t fregOffset2;
    uint8_t fregOffset1;
    uint8_t type;
    uint8_t qfactor;
    uint8_t width;
    uint8_t height;
};

struct AMJpegQuantizationHeader
{
    uint8_t mbz;
    uint8_t precision;
    uint8_t length2;
    uint8_t length1;
};

struct AMJpegQtable
{
    uint8_t *data;
    uint32_t len;
    AMJpegQtable() :
      data(nullptr),
      len(0)
    {}
    AMJpegQtable(const AMJpegQtable &qtable) :
      data(qtable.data),
      len(qtable.len)
    {}
};

typedef std::vector<AMJpegQtable> AMJpegQtableList;
struct AMJpegParams
{
    uint8_t         *data;
    uint32_t         len;
    uint32_t         offset;
    uint32_t         q_table_total_len;
    uint16_t         width;
    uint16_t         height;
    uint16_t         precision;
    uint16_t         type;
    AMJpegQtableList q_table_list;
    void clear()
    {
      q_table_list.clear();
      data = nullptr;
      len  = 0;
      offset = 0;
      q_table_total_len = 0;
      width = 0;
      height = 0;
      precision = 0;
      type = 0;
    }
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_ */
