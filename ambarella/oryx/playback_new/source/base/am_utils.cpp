/**
 * am_utils.cpp
 *
 * History:
 *    2015/07/28 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_network_al.h"

#include "am_internal_streaming.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "mpeg_ts_defs.h"

typedef struct {
  TChar format_name[8];
  StreamFormat format;
} SFormatStringPair;

typedef struct {
  char entropy_name[8];
  EntropyType type;
} SEntropyTypeStringPair;

typedef struct {
  char container_name[8];
  ContainerType container_type;
} SContainerStringPair;

typedef struct {
  char sampleformat_name[8];
  AudioSampleFMT sampleformat;
} SSampleFormatStringPair;

static SFormatStringPair g_FormatStringPair[] = {
  {"h264", StreamFormat_H264},
  {"avc", StreamFormat_H264},
  {"aac", StreamFormat_AAC},
  {"adpcm", StreamFormat_ADPCM},
};

static SEntropyTypeStringPair g_EntropyTypeStringPair[] = {
  {"cabac", EntropyType_H264_CABAC},
  {"cavlc", EntropyType_H264_CAVLC},
};

static SContainerStringPair g_ContainerStringPair[] = {
  {"mp4", ContainerType_MP4},
  {"3gp", ContainerType_3GP},
  {"ts", ContainerType_TS},
  {"mov", ContainerType_MOV},
  {"mkv", ContainerType_MKV},
  {"avi", ContainerType_AVI},
  {"amr", ContainerType_AMR},
};

static SSampleFormatStringPair g_SampleFormatStringPair[] = {
  {"u8", AudioSampleFMT_U8},
  {"s16", AudioSampleFMT_S16},
  {"s32", AudioSampleFMT_S32},
  {"float", AudioSampleFMT_FLT},
  {"double", AudioSampleFMT_DBL},
};

StreamFormat gfGetStreamFormatFromString(TChar *str)
{
  TUint i, tot;
  tot = sizeof(g_FormatStringPair) / sizeof(SFormatStringPair);
  for (i = 0; i < tot; i++) {
    if (0 == strncmp(str, g_FormatStringPair[i].format_name, strlen(g_FormatStringPair[i].format_name))) {
      return g_FormatStringPair[i].format;
    }
  }
  return StreamFormat_Invalid;
}

EntropyType gfGetEntropyTypeFromString(char *str)
{
  TUint i, tot;
  tot = sizeof(g_EntropyTypeStringPair) / sizeof(SEntropyTypeStringPair);
  for (i = 0; i < tot; i++) {
    if (0 == strncmp(str, g_EntropyTypeStringPair[i].entropy_name, strlen(g_EntropyTypeStringPair[i].entropy_name))) {
      return g_EntropyTypeStringPair[i].type;
    }
  }
  return EntropyType_NOTSet;
}

ContainerType gfGetContainerTypeFromString(char *str)
{
  TUint i, tot;
  tot = sizeof(g_ContainerStringPair) / sizeof(SContainerStringPair);
  for (i = 0; i < tot; i++) {
    if (0 == strncmp(str, g_ContainerStringPair[i].container_name, strlen(g_ContainerStringPair[i].container_name))) {
      return g_ContainerStringPair[i].container_type;
    }
  }
  return ContainerType_Invalid;
}

AudioSampleFMT gfGetSampleFormatFromString(char *str)
{
  TUint i, tot;
  tot = sizeof(g_SampleFormatStringPair) / sizeof(SSampleFormatStringPair);
  for (i = 0; i < tot; i++) {
    if (0 == strncmp(str, g_SampleFormatStringPair[i].sampleformat_name, strlen(g_SampleFormatStringPair[i].sampleformat_name))) {
      return g_SampleFormatStringPair[i].sampleformat;
    }
  }
  //default s16
  return AudioSampleFMT_S16;
}

const TChar *gfGetContainerStringFromType(ContainerType type)
{
  switch (type) {
    case ContainerType_MP4://default
      return "mp4";
    case ContainerType_3GP:
      return "3gp";
    case ContainerType_TS:
      return "ts";
    case ContainerType_MOV:
      return "mov";
    case ContainerType_AVI:
      return "avi";
    case ContainerType_AMR:
      return "amr";
    case ContainerType_MKV:
      return "mkv";
    default:
      LOG_FATAL("unknown container type.\n");
      break;
  }
  return "???";
}

IScheduler *gfGetNetworkReceiverScheduler(const volatile SPersistMediaConfig *p, TUint index)
{
  DASSERT(p);
  if (p) {
    TUint num = 1;
    DASSERT(p->number_scheduler_network_reciever);
    DASSERT(p->number_scheduler_network_reciever <= DMaxSchedulerGroupNumber);
    if (p->number_scheduler_network_reciever > DMaxSchedulerGroupNumber) {
      num = 1;
      LOG_FATAL("BAD p->number_scheduler_network_reciever %d\n", p->number_scheduler_network_reciever);
    } else {
      num = p->number_scheduler_network_reciever;
      if (!num) {
        num = 1;
        LOG_FATAL("BAD p->number_scheduler_network_reciever %d\n", p->number_scheduler_network_reciever);
      }
    }
    index = index % num;
    return p->p_scheduler_network_reciever[index];
  } else {
    LOG_FATAL("NULL p\n");
  }
  return NULL;
}

IScheduler *gfGetNetworkReceiverTCPScheduler(const volatile SPersistMediaConfig *p, TUint index)
{
  DASSERT(p);
  if (p) {
    TUint num = 1;
    DASSERT(p->number_scheduler_network_tcp_reciever);
    DASSERT(p->number_scheduler_network_tcp_reciever <= DMaxSchedulerGroupNumber);
    if (p->number_scheduler_network_tcp_reciever > DMaxSchedulerGroupNumber) {
      num = 1;
      LOG_FATAL("BAD p->number_scheduler_network_tcp_reciever %d\n", p->number_scheduler_network_tcp_reciever);
    } else {
      num = p->number_scheduler_network_tcp_reciever;
      if (!num) {
        num = 1;
        LOG_FATAL("BAD p->number_scheduler_network_tcp_reciever %d\n", p->number_scheduler_network_tcp_reciever);
      }
    }
    index = index % num;
    return p->p_scheduler_network_tcp_reciever[index];
  } else {
    LOG_FATAL("NULL p\n");
  }
  return NULL;
}

IScheduler *gfGetFileIOWriterScheduler(const volatile SPersistMediaConfig *p, TU32 index)
{
  DASSERT(p);
  if (p) {
    TUint num = 1;
    DASSERT(p->number_scheduler_io_writer);
    DASSERT(p->number_scheduler_io_writer <= DMaxSchedulerGroupNumber);
    if (p->number_scheduler_io_writer > DMaxSchedulerGroupNumber) {
      num = 1;
      LOG_FATAL("BAD p->number_scheduler_io_writer %d\n", p->number_scheduler_io_writer);
    } else {
      num = p->number_scheduler_io_writer;
      if (!num) {
        num = 1;
        LOG_FATAL("BAD p->number_scheduler_io_writer %d\n", p->number_scheduler_io_writer);
      }
    }
    index = index % num;
    return p->p_scheduler_io_writer[index];
  } else {
    LOG_FATAL("NULL p\n");
  }
  return NULL;
}

static const TU8 g_dayOfMonth[12] = {
  31,
  28,
  31,
  30,
  31,
  30,
  31,
  31,
  30,
  31,
  30,
  31
};

static const TU8 g_dayOfMonthLeapYear[12] = {
  31,
  29,
  31,
  30,
  31,
  30,
  31,
  31,
  30,
  31,
  30,
  31
};

static TUint isLeapYear(TU32 year)
{
  if (DLikely(year & 0x3)) {
    return 0;
  }
  TU32 mode_400 = year % 400;
  if (DUnlikely((100 == mode_400) || (200 == mode_400) || (300 == mode_400))) {
    return 0;
  }
  return 1;
}

static TU8 getDayOfMonth(TU8 month, TU32 year)
{
  if (DUnlikely(2 == month)) {
    if (DUnlikely(1 == isLeapYear(year))) {
      return 29;
    }
    return 28;
  } else if (DUnlikely(12 < month)) {
    LOG_FATAL("month %d > 12\n", month);
    return 30;
  }
  return g_dayOfMonth[month - 1];
}

static TU8 getDayOfMonthInLeapYear(TU8 month)
{
  if (DUnlikely(12 < month)) {
    LOG_FATAL("month %d > 12\n", month);
    return 30;
  }
  return g_dayOfMonthLeapYear[month - 1];
}

static TU8 getDayOfMonthInNormalYear(TU8 month)
{
  if (DUnlikely(12 < month)) {
    LOG_FATAL("month %d > 12\n", month);
    return 30;
  }
  return g_dayOfMonth[month - 1];
}

static TU32 getDayOfYear(TU32 year)
{
  if (DUnlikely(1 == isLeapYear(year))) {
    return 366;
  }
  return 365;
}

#if 0
static TU32 getDayOfLeapYear()
{
  return 366;
}

static TU32 getDayOfNormalYear()
{
  return 365;
}
#endif

// 1: later, 0 : earlier, 2 : equal
TU32 gfDateIsLater(SDateTime &current_date, SDateTime &start_date)
{
  if (DLikely(current_date.year == start_date.year)) {
    if (DLikely(current_date.month == start_date.month)) {
      if (DLikely(current_date.day > start_date.day)) {
        return 1;
      } else if (current_date.day == start_date.day) {
        if (DLikely(current_date.hour > start_date.hour)) {
          return 1;
        } else if (current_date.hour == start_date.hour) {
          if (DLikely(current_date.minute > start_date.minute)) {
            return 1;
          } else if (current_date.minute == start_date.minute) {
            if (DLikely(current_date.seconds > start_date.seconds)) {
              return 1;
            } else if (current_date.seconds == start_date.seconds) {
              return 2;
            }
          }
        }
      }
    } else if (DLikely(current_date.month > start_date.month)) {
      return 1;
    }
  } else if (DLikely(current_date.year > start_date.year)) {
    return 1;
  }
  return 0;
}

TS32 dayOfMonthes(TU8 cur_month, TU8 start_month, TU32 year)
{
  TS32 days = 0;
  TU8 i = 0;
  if (DLikely(isLeapYear(year))) {
    if (DLikely(cur_month > start_month)) {
      for (i = start_month; i < cur_month; i ++) {
        days += getDayOfMonthInLeapYear(i);
      }
    } else if (cur_month == start_month) {
      return 0;
    } else {
      LOG_FATAL("negative\n");
      return 0;
    }
  } else {
    if (DLikely(cur_month > start_month)) {
      for (i = start_month; i < cur_month; i ++) {
        days += getDayOfMonthInNormalYear(i);
      }
    } else if (cur_month == start_month) {
      return 0;
    } else {
      LOG_FATAL("negative\n");
      return 0;
    }
  }
  return days;
}

TS32 dayOfMonthesInLeapYear(TU8 cur_month, TU8 start_month)
{
  if (DLikely(cur_month > start_month)) {
    TS32 days = 0;
    TU8 i = 0;
    for (i = start_month; i < cur_month; i ++) {
      days += getDayOfMonthInLeapYear(i);
    }
    return days;
  } else if (cur_month == start_month) {
  } else {
    LOG_FATAL("negative\n");
  }
  return 0;
}

TS32 dayOfMonthesInNormalYear(TU8 cur_month, TU8 start_month)
{
  if (DLikely(cur_month > start_month)) {
    TS32 days = 0;
    TU8 i = 0;
    for (i = start_month; i < cur_month; i ++) {
      days += getDayOfMonthInNormalYear(i);
    }
    return days;
  } else if (cur_month == start_month) {
  } else {
    LOG_FATAL("negative\n");
  }
  return 0;
}

TS32 dayOfYears(TU32 cur_year, TU32 start_year)
{
  TS32 days = 0;
  TU32 i = 0;
  if (cur_year > start_year) {
    for (i = start_year; i < cur_year; i ++) {
      days += getDayOfYear(i);
    }
  } else if (cur_year == start_year) {
    return 0;
  } else {
    LOG_FATAL("negative\n");
    return 0;
  }
  return days;
}

TU64 secondsToEndOfDay(SDateTime &current_date)
{
  TU64 v = (TU64)(24 - current_date.hour) * (TU64)3600;
  v -= (TU64)current_date.minute * 60;
  v -= (TU64)current_date.seconds;
  return v;
}

TU64 secondsToEndOfMonth(SDateTime &current_date)
{
  TU64 v = getDayOfMonth(current_date.month, current_date.year);
  v -= current_date.day;
  v *= 3600 * 24;
  v += secondsToEndOfDay(current_date);
  return v;
}

TU64 secondsToEndOfYear(SDateTime &current_date)
{
  TU64 v = dayOfMonthes(12, current_date.month, current_date.year);
  v -= current_date.day;
  v *= 3600 * 24;
  v += secondsToEndOfDay(current_date);
  return v;
}

void gfDateCalculateGap(SDateTime &current_date, SDateTime &start_date, SDateTimeGap &gap)
{
  TU64 vd = 0, vs = 0;
  if (DLikely(current_date.year == start_date.year)) {
    vd = dayOfMonthes(current_date.month, start_date.month, current_date.year);
    vd += current_date.day;
    vd -= start_date.day;
  } else {
    vd = dayOfYears(current_date.year, start_date.year);
    vd += dayOfMonthes(current_date.month, 0, current_date.year);
    vd += current_date.day;
    vd -= dayOfMonthes(start_date.month, 0, start_date.year);
    vd -= start_date.day;
  }
  vs = ((TU64)current_date.hour * 3600) + ((TU64)current_date.minute * 60) + current_date.seconds;
  vs += (3600 * 24) * vd;
  vs -= ((TU64)start_date.hour * 3600) + ((TU64)start_date.minute * 60) + start_date.seconds;
  gap.days = vd;
  gap.overall_seconds = vs;
  gap.total_hours = vs / 3600;
  vs -= gap.total_hours * 3600;
  gap.total_minutes = vs / 60;
  gap.total_seconds = vs - (gap.total_minutes * 60);
}

void gfDateCalculateNextTime(SDateTime &next_date, SDateTime &start_date, TU64 seconds)
{
  TU64 v = secondsToEndOfDay(start_date);
  if (DLikely(seconds < v)) {
    next_date = start_date;
    if (seconds < ((TU64)60 - start_date.seconds)) {
      start_date.seconds += seconds;
      return;
    } else {
      next_date.minute ++;
      seconds -= ((TU64)60 - next_date.seconds);
      next_date.seconds = 0;
    }
    TU32 v0 = seconds / 60;
    TU32 v1 = next_date.minute + v0;
    TU32 v2 = v1 / 60;
    next_date.hour += v2;
    next_date.minute = v1 - (v2 * 60);
    next_date.seconds = seconds - (v0 * 60);
  } else {
    LOG_FATAL("gfDateCalculateNextTime: TO DO\n");
    return;
  }
}

void gfEncodingBase16(TChar *out, const TU8 *in, TInt in_size)
{
  const TChar b16[32] =
    "0123456789ABCDEF";
  while (in_size > 0) {
    out[0] = b16[(in[0] >> 4) & 0x0f];
    out[1] = b16[in[0] & 0x0f];
    out += 2;
    in += 1;
    in_size --;
  }
}

void gfDecodingBase16(TU8 *out, const TU8 *in, TInt in_size)
{
  TChar hex[8] = {0};
  TU32 value = 0;
  while (in_size > 1) {
    hex[0] = in[0];
    hex[1] = in[1];
    sscanf(hex, "%x", &value);
    out[0] = value;
    out ++;
    in += 2;
    in_size -= 2;
  }
}

/* ---------------- private code */
static const TU8 _gBase64Map2[256] = {
  0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff,

  0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
  0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
  0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x00, 0x01,
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
  0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
  0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
  0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,

  0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

#define DBASE64_DEC_STEP(i) do { \
    bits = _gBase64Map2[in[i]]; \
    if (bits & 0x80) \
      goto out ## i; \
    v = i ? (v << 6) + bits : bits; \
  } while(0)

TInt gfDecodingBase64(TU8 *out, const TU8 *in_str, TInt out_size)
{
  TU8 *dst = out;
  TU8 *end = out + out_size;
  // no sign extension
  const TU8 *in = in_str;
  unsigned bits = 0xff;
  unsigned v;
  while (end - dst > 3) {
    DBASE64_DEC_STEP(0);
    DBASE64_DEC_STEP(1);
    DBASE64_DEC_STEP(2);
    DBASE64_DEC_STEP(3);
    // Using AV_WB32 directly confuses compiler
    //v = DBSWAP32C(v << 8);
    dst[2] = (v & 0xff);
    dst[1] = ((v >> 8) & 0xff);
    dst[0] = ((v >> 16) & 0xff);
    dst += 3;
    in += 4;
  }
  if (end - dst) {
    DBASE64_DEC_STEP(0);
    DBASE64_DEC_STEP(1);
    DBASE64_DEC_STEP(2);
    DBASE64_DEC_STEP(3);
    *dst++ = v >> 16;
    if (end - dst)
    { *dst++ = v >> 8; }
    if (end - dst)
    { *dst++ = v; }
    in += 4;
  }
  while (1) {
    DBASE64_DEC_STEP(0);
    in++;
    DBASE64_DEC_STEP(0);
    in++;
    DBASE64_DEC_STEP(0);
    in++;
    DBASE64_DEC_STEP(0);
    in++;
  }
out3:
  *dst++ = v >> 10;
  v <<= 2;
out2:
  *dst++ = v >> 4;
out1:
out0:
  return bits & 1 ? -1 : dst - out;
}

TChar *gfEncodingBase64(TChar *out, TInt out_size, const TU8 *in, TInt in_size)
{
  static const TChar b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  TChar *ret, *dst;
  unsigned i_bits = 0;
  TInt i_shift = 0;
  TInt bytes_remaining = in_size;
  ret = dst = out;
  while (bytes_remaining > 3) {
    i_bits = ((in[0] << 16) | (in[1] << 8) | in[2]);
    in += 3; bytes_remaining -= 3;
    *dst++ = b64[ i_bits >> 18        ];
    *dst++ = b64[(i_bits >> 12) & 0x3F];
    *dst++ = b64[(i_bits >> 6) & 0x3F];
    *dst++ = b64[(i_bits) & 0x3F];
  }
  i_bits = 0;
  while (bytes_remaining) {
    i_bits = (i_bits << 8) + *in++;
    bytes_remaining--;
    i_shift += 8;
  }
  while (i_shift > 0) {
    *dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
    i_shift -= 6;
  }
  while ((dst - ret) & 3) {
    *dst++ = '=';
  }
  *dst = '\0';
  return ret;
}

#define DRANDOM_SEED_1 17
#define DRANDOM_SEED_2 39
#define DRANDOM_SEED_3 89
#define DRANDOM_SEED_4 71

TU32 gfRandom32(void)
{
  static TU32 random_count = 0;
  SDateTime datetime;
  gfGetCurrentDateTime(&datetime);
  TU32 value = datetime.seconds;
  value += (DRANDOM_SEED_1 * random_count);
  value &= (((DRANDOM_SEED_2 * random_count) & 0xff) << 8);
  value |= (((DRANDOM_SEED_3 * random_count) & 0xff) << 16);
  value += DRANDOM_SEED_4 + random_count;
  random_count ++;
  return value;
}

TU8 *gfGenerateAudioExtraData(StreamFormat format, TInt samplerate, TInt channel_number, TUint &size)
{
  SSimpleAudioSpecificConfig *p_simple_header = NULL;
  size = 0;
  switch (format) {
    case StreamFormat_AAC:
      size = 2;
      p_simple_header = (SSimpleAudioSpecificConfig *) DDBG_MALLOC((size + 3) & (~3), "GAE0");
      p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
      switch (samplerate) {
        case 44100:
          samplerate = eSamplingFrequencyIndex_44100;
          break;
        case 48000:
          samplerate = eSamplingFrequencyIndex_48000;
          break;
        case 24000:
          samplerate = eSamplingFrequencyIndex_24000;
          break;
        case 16000:
          samplerate = eSamplingFrequencyIndex_16000;
          break;
        case 8000:
          samplerate = eSamplingFrequencyIndex_8000;
          break;
        case 12000:
          samplerate = eSamplingFrequencyIndex_12000;
          break;
        case 32000:
          samplerate = eSamplingFrequencyIndex_32000;
          break;
        case 22050:
          samplerate = eSamplingFrequencyIndex_22050;
          break;
        case 11025:
          samplerate = eSamplingFrequencyIndex_11025;
          break;
        default:
          LOG_ERROR("NOT support sample rate (%d) here.\n", samplerate);
          break;
      }
      p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
      p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
      p_simple_header->channelConfiguration = channel_number;
      p_simple_header->bitLeft = 0;
      break;
    default:
      LOG_ERROR("NOT support audio codec (%d) here.\n", format);
      break;
  }
  return (TU8 *)p_simple_header;
}

const TChar *gfGetServerManagerStateString(EServerManagerState state)
{
  switch (state) {
    case EServerManagerState_idle:
      return "(EServerManagerState_idle)";
      break;
    case EServerManagerState_noserver_alive:
      return "(EServerManagerState_noserver_alive)";
      break;
    case EServerManagerState_running:
      return "(EServerManagerState_running)";
      break;
    case EServerManagerState_halt:
      return "(EServerManagerState_halt)";
      break;
    case EServerManagerState_error:
      return "(EServerManagerState_error)";
      break;
    default:
      LOG_ERROR("Unknown module state %d\n", state);
      break;
  }
  return "(Unknown module state)";
}

void gfPrintCodecInfoes(SStreamCodecInfos *pInfo)
{
  TUint i = 0;
  SStreamCodecInfo *info = NULL;
  DASSERT(pInfo);
  if (pInfo) {
    info = pInfo->info;
    for (i = 0; i < pInfo->total_stream_number; i ++, info++) {
      LOG_ALWAYS("codec_id %d, stream_index %d, stream_presence %d, stream_enabled %d, inited %d\n", info->codec_id, info->stream_index, info->stream_presence, info->stream_enabled, info->inited);
      if (StreamType_Video == info->stream_type) {
        LOG_ALWAYS("video: format %d, parameters as follow\n", info->stream_format);
        LOG_ALWAYS("    pic_width %d, pic_height %d, pic_offset_x %d, pic_offset_y %d\n", info->spec.video.pic_width, info->spec.video.pic_height, info->spec.video.pic_offset_x, info->spec.video.pic_offset_y);
        LOG_ALWAYS("    framerate_num %d, framerate_den %d, framerate %f bitrate %d\n", info->spec.video.framerate_num, info->spec.video.framerate_den, info->spec.video.framerate, info->spec.video.bitrate);
        LOG_ALWAYS("    M %d, N %d, IDRInterval %d\n", info->spec.video.M, info->spec.video.N, info->spec.video.IDRInterval);
        LOG_ALWAYS("    sample_aspect_ratio_num %d, sample_aspect_ratio_den %d, lowdelay %d, entropy_type %d\n", info->spec.video.sample_aspect_ratio_num, info->spec.video.sample_aspect_ratio_den, info->spec.video.lowdelay, info->spec.video.entropy_type);
      } else if (StreamType_Audio == info->stream_type) {
        LOG_ALWAYS("audio: format %d, parameters as follow\n", info->stream_format);
        LOG_ALWAYS("    sample_rate %d, sample_format %d, channel_number %d\n", info->spec.audio.sample_rate, info->spec.audio.sample_format, info->spec.audio.channel_number);
        LOG_ALWAYS("    channel_layout %d, frame_size %d, bitrate %d\n", info->spec.audio.channel_layout, info->spec.audio.frame_size, info->spec.audio.bitrate);
        LOG_ALWAYS("    need_skip_adts_header %d, pts_unit_num %d, pts_unit_den %d\n", info->spec.audio.need_skip_adts_header, info->spec.audio.pts_unit_num, info->spec.audio.pts_unit_den);
        LOG_ALWAYS("    is_channel_interlave %d, is_big_endian %d, codec_format %d, customized_codec_type %d\n", info->spec.audio.is_channel_interlave, info->spec.audio.is_big_endian, info->spec.audio.codec_format, info->spec.audio.customized_codec_type);
      } else if (StreamType_Subtitle == info->stream_type) {
        LOG_FATAL("TO DO\n");
      } else if (StreamType_PrivateData == info->stream_type) {
        LOG_FATAL("TO DO\n");
      } else {
        LOG_FATAL("BAD info->stream_type %d\n", info->stream_type);
      }
    }
  }
}

TU8 *__validStartPoint(TU8 *start, TUint &size)
{
  TU8 start_code_prefix[4] = {0, 0, 0, 1};
  TU8 start_code_ex_prefix[3] = {0, 0, 1};
  while (size > 4) {
    if (memcmp(start, start_code_prefix, sizeof(start_code_prefix)) == 0) {
      start += 4;
      size -= 4;
      continue;
    } else if (memcmp(start, start_code_ex_prefix, sizeof(start_code_ex_prefix)) == 0) {
      start += 3;
      size -= 3;
      continue;
    } else {
      return start;
    }
  }
  return start;
}

bool gfGetSpsPps(TU8 *pBuffer, TUint size, TU8 *&p_spspps, TUint &spspps_size, TU8 *&p_IDR, TU8 *&p_ret_data)
{
  TUint checking_size = 0;
  TU32 start_code;
  bool find_sps = false;
  bool find_pps = false;
  bool find_IDR = false;
  bool find_SEI = false;
  TU8 type;
  DASSERT(pBuffer);
  if (!pBuffer) {
    LOG_FATAL("NULL pointer in _get_sps_pps.\n");
    return false;
  }
  LOG_NOTICE("[debug info], input %p, size %d, data [%02x %02x %02x %02x %02x]\n", pBuffer, size, pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);
  p_spspps = pBuffer;
  p_IDR = pBuffer;
  p_ret_data = NULL;
  spspps_size = 0;
  start_code = 0xffffffff;
  while ((checking_size + 1) < size) {
    start_code <<= 8;
    start_code |= pBuffer[checking_size];
    if (start_code == 0x00000001) {
      type = (0x1f & pBuffer[checking_size + 1]);
      if (0x05 == type) {
        DASSERT(find_sps);
        DASSERT(find_pps);
        DASSERT(!find_IDR);
        p_IDR = pBuffer + checking_size - 3;
        find_IDR = true;
        LOG_WARN("[debug info], find IDR, checking_size - 3 %d\n", checking_size - 3);
        break;
      } else if (0x06 == type) {
        //to do
        find_SEI = true;
        LOG_WARN("[debug info], find SEI, checking_size - 3 %d\n", checking_size - 3);
      } else if (0x07 == type) {
        DASSERT(!find_sps);
        DASSERT(!find_pps);
        DASSERT(!find_IDR);
        p_spspps = pBuffer + checking_size - 3;
        find_sps = true;
        LOG_WARN("[debug info], find sps, checking_size - 3 %d\n", checking_size - 3);
      } else if (0x08 == type) {
        DASSERT(find_sps);
        DASSERT(!find_pps);
        DASSERT(!find_IDR);
        find_pps = true;
        LOG_WARN("[debug info], find pps, checking_size - 3 %d\n", checking_size - 3);
      } else if (0x09 == type) {
        p_ret_data = pBuffer + checking_size - 3;
        LOG_WARN("[debug info], find delimiter, checking_size - 3 %d\n", checking_size - 3);
      } else {
        LOG_WARN("unknown type %d.\n", type);
      }
    }
    checking_size ++;
  }
  if (find_sps && find_IDR) {
    DASSERT(find_pps);
    DASSERT(((TMemSize)p_IDR) > ((TMemSize)p_spspps));
    if (((TMemSize)p_IDR) > ((TMemSize)p_spspps)) {
      spspps_size = ((TMemSize)p_IDR) - ((TMemSize)p_spspps);
    } else {
      LOG_FATAL("internal error!!\n");
      return false;
    }
    LOG_WARN("[debug info], input %p, find IDR %p, sps %p, seq size %d, checking_size %d\n", pBuffer, p_IDR, p_spspps, spspps_size, checking_size);
    return true;
  } else if (find_SEI) {
    //todo
  }
  LOG_WARN("cannot find sps(%d), pps(%d), IDR header(%d), please check code!!\n", find_sps, find_pps, find_IDR);
  return false;
}

TU32 gfSkipDelimter(TU8 *p)
{
  if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (ENalType_AUD == (p[4] & 0x1f))) {
    return 6;
  }
  return 0;
}

TU32 gfSkipSEI(TU8 *p, TU32 len)
{
  TU8 *po = p;
  TUint state = 0;
  while (len) {
    switch (state) {
      case 0:
        if (!(*p)) {
          state = 1;
        }
        break;
      case 1: //0
        if (!(*p)) {
          state = 2;
        } else {
          state = 0;
        }
        break;
      case 2: //0 0
        if (!(*p)) {
          state = 3;
        } else if (1 == (*p)) {
          if (ENalType_SEI > (p[1] & 0x1f)) {
            return (TU32)((p - 2) - po);
          }
        } else {
          state = 0;
        }
        break;
      case 3: //0 0 0
        if (!(*p)) {
          state = 3;
        } else if (1 == (*p)) {
          if (ENalType_SEI > (p[1] & 0x1f)) {
            return (TU32)((p - 3) - po);
          }
        } else {
          state = 0;
        }
        break;
      default:
        LOG_FATAL("impossible to comes here\n");
        break;
    }
    p ++;
    len --;
  }
  return 0;
}

TU32 gfSkipDelimterHEVC(TU8 *p)
{
  if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (EHEVCNalType_AUD == ((p[4] >> 1) & 0x3f))) {
    return 7;
  }
  return 0;
}

TU32 gfSkipSEIHEVC(TU8 *p, TU32 len)
{
  TU8 *po = p;
  TUint state = 0;
  TU8 nal_type = 0;
  while (len) {
    switch (state) {
      case 0:
        if (!(*p)) {
          state = 1;
        }
        break;
      case 1: //0
        if (!(*p)) {
          state = 2;
        } else {
          state = 0;
        }
        break;
      case 2: //0 0
        if (!(*p)) {
          state = 3;
        } else if (1 == (*p)) {
          nal_type = ((p[1] >> 1) & 0x3f);
          if ((EHEVCNalType_SUFFIX_SEI != nal_type) && (EHEVCNalType_PREFIX_SEI != nal_type)) {
            return (TU32)((p - 2) - po);
          }
        } else {
          state = 0;
        }
        break;
      case 3: //0 0 0
        if (!(*p)) {
          state = 3;
        } else if (1 == (*p)) {
          nal_type = ((p[1] >> 1) & 0x3f);
          if ((EHEVCNalType_SUFFIX_SEI != nal_type) && (EHEVCNalType_PREFIX_SEI != nal_type)) {
            return (TU32)((p - 3) - po);
          }
        } else {
          state = 0;
        }
        break;
      default:
        LOG_FATAL("impossible to comes here\n");
        break;
    }
    p ++;
    len --;
  }
  return 0;
}

TU8 *gfAppendTSPriData(TU8 *dest, void *src, ETSPrivateDataType data_type, TU16 sub_type, TU16 data_len, TU16 max_data_len, TU16 &consumed_data_len)
{
  TU8 *p_cur_dest = NULL;
  TU16 packing_data_len = 0;
  if (!dest || !src || !data_len) {
    LOG_ERROR("gfAppendTSPriData, invalid paras, dest=%p, src=%p, data_len=%hu\n", dest, src, data_len);
    return p_cur_dest;
  }
  switch (data_type) {
    case ETSPrivateDataType_StartTime: {
        if (sizeof(SDateTime) != data_len) {
          LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_StartTime, data_len=%hu invalid(should be %lu)\n", data_len, (TULong)sizeof(SDateTime));
          return p_cur_dest;
        }
        p_cur_dest = dest;
        packing_data_len = sizeof(STSPrivateDataStartTime);
        STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
        p_header->data_type_4cc_31to24 = 0x54;  //T
        p_header->data_type_4cc_23to16 = 0x53;   //S
        p_header->data_type_4cc_15to8 = 0x53;    //S
        p_header->data_type_4cc_7to0 = 0x54;     //T
        p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
        p_header->data_sub_type_7to0 = sub_type & 0xff;
        p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
        p_header->data_length_7to0 = packing_data_len & 0xff;
        p_cur_dest += sizeof(STSPrivateDataHeader);
        STSPrivateDataStartTime *p_start_time_dest = (STSPrivateDataStartTime *)p_cur_dest;
        SDateTime *p_start_time_src = (SDateTime *)src;
        p_start_time_dest->year_15to8 = (p_start_time_src->year >> 8) & 0xff;
        p_start_time_dest->year_7to0 = p_start_time_src->year & 0xff;
        p_start_time_dest->month = p_start_time_src->month;
        p_start_time_dest->day = p_start_time_src->day;
        p_start_time_dest->hour = p_start_time_src->hour;
        p_start_time_dest->minute = p_start_time_src->minute;
        p_start_time_dest->seconds = p_start_time_src->seconds;
        p_cur_dest += packing_data_len;
        consumed_data_len = data_len;
      }
      break;
    case ETSPrivateDataType_Duration: {
        if (sizeof(TU16) != data_len) {
          LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_Duration, data_len=%hu invalid(should be %lu)\n", data_len, (TULong)sizeof(TU16));
          return p_cur_dest;
        }
        p_cur_dest = dest;
        packing_data_len = sizeof(STSPrivateDataDuration);
        STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
        p_header->data_type_4cc_31to24 = 0x54;  //T
        p_header->data_type_4cc_23to16 = 0x53;   //S
        p_header->data_type_4cc_15to8 = 0x44;    //D
        p_header->data_type_4cc_7to0 = 0x55;     //U
        p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
        p_header->data_sub_type_7to0 = sub_type & 0xff;
        p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
        p_header->data_length_7to0 = packing_data_len & 0xff;
        p_cur_dest += sizeof(STSPrivateDataHeader);
        STSPrivateDataDuration *p_duration_dest = (STSPrivateDataDuration *)p_cur_dest;
        TU16 *p_duration_src = (TU16 *)src;
        p_duration_dest->file_duration_15to8 = ((*p_duration_src) >> 8) & 0xff;
        p_duration_dest->file_duration_7to0 = (*p_duration_src) & 0xff;
        p_cur_dest += packing_data_len;
        consumed_data_len = data_len;
      }
      break;
    case ETSPrivateDataType_ChannelName: {
        if (data_len > max_data_len) {
          LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_ChannelName, data_len=%hu invalid(should <= %hu)\n", data_len, max_data_len);
          return p_cur_dest;
        }
        p_cur_dest = dest;
        packing_data_len = data_len;
        STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
        p_header->data_type_4cc_31to24 = 0x54;  //T
        p_header->data_type_4cc_23to16 = 0x53;   //S
        p_header->data_type_4cc_15to8 = 0x43;    //C
        p_header->data_type_4cc_7to0 = 0x4E;     //N
        p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
        p_header->data_sub_type_7to0 = sub_type & 0xff;
        p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
        p_header->data_length_7to0 = packing_data_len & 0xff;
        p_cur_dest += sizeof(STSPrivateDataHeader);
        TU8 *p_chaname_src = (TU8 *)src;
        memcpy(p_cur_dest, p_chaname_src, packing_data_len);
        p_cur_dest += packing_data_len;
        consumed_data_len = data_len;
      }
      break;
    case ETSPrivateDataType_TSPakIdx4GopStart: {
        //TU16 max_data_len = MPEG_TS_TP_PACKET_SIZE-MPEG_TS_TP_PACKET_HEADER_SIZE-DPESPakStartCodeSize-DPESPakStreamIDSize-DPESPakLengthSize-sizeof(STSPrivateDataHeader);
        if (0 != data_len % sizeof(TU32)) {
          LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_TSPakIdx4GopStart, data_len=%hu invalid(should multiple of %lu bytes)\n", data_len, (TULong)sizeof(TU32));
          return p_cur_dest;
        }
        p_cur_dest = dest;
        consumed_data_len = (data_len > max_data_len) ? max_data_len : data_len;
        packing_data_len = consumed_data_len;
        STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
        p_header->data_type_4cc_31to24 = 0x54;  //T
        p_header->data_type_4cc_23to16 = 0x53;   //S
        p_header->data_type_4cc_15to8 = 0x50;    //P
        p_header->data_type_4cc_7to0 = 0x49;     //I
        p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
        p_header->data_sub_type_7to0 = sub_type & 0xff;
        p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
        p_header->data_length_7to0 = packing_data_len & 0xff;
        p_cur_dest += sizeof(STSPrivateDataHeader);
        TU32 *p_tspakidx_src = (TU32 *)src;
        for (TU32 i = 0; i < consumed_data_len / sizeof(TU32); i++) {
          STSPrivateDataTSPakIdx4GopStart *p_tspakidx_dest = (STSPrivateDataTSPakIdx4GopStart *)p_cur_dest;
          p_tspakidx_dest->idx_31to24 = ((*p_tspakidx_src) >> 24) & 0xff;
          p_tspakidx_dest->idx_23to16 = ((*p_tspakidx_src) >> 16) & 0xff;
          p_tspakidx_dest->idx_15to8 = ((*p_tspakidx_src) >> 8) & 0xff;
          p_tspakidx_dest->idx_7to0 = (*p_tspakidx_src) & 0xff;
          p_cur_dest += sizeof(STSPrivateDataTSPakIdx4GopStart);
          p_tspakidx_src++;
        }
      }
      break;
    case ETSPrivateDataType_MotionEvent: {//0x54534D45;//TSME
        consumed_data_len = 0;
        LOG_WARN("gfAppendTSPriData, ETSPrivateDataType_MotionEvent NoImplement\n");
      }
      break;
    default: {
        consumed_data_len = 0;
        LOG_ERROR("gfAppendTSPriData, invalid paras, data_type=%d\n", data_type);
      }
      break;
  }
  return p_cur_dest;
}

EECode gfRetrieveTSPriData(void *dest, TU16 dest_buf_len, TU8 *src, TU16 src_len, ETSPrivateDataType data_type, TU16 sub_type)
{
  if (!dest || !dest_buf_len || !src || !src_len || data_type >= ETSPrivateDataType_Num) {
    LOG_ERROR("gfRetrieveTSPriData, invalid paras, dest=%p, dest_buf_len=%hu, src=%p, src_len=%hu, data_type=%d\n", dest, dest_buf_len, src, src_len, data_type);
    return EECode_BadParam;
  }
  //TU16 max_check_len = MPEG_TS_TP_PACKET_SIZE-MPEG_TS_TP_PACKET_HEADER_SIZE-DPESPakStartCodeSize-DPESPakStreamIDSize-DPESPakLengthSize;
  TU8 *p_cur_header = src;
  while ((p_cur_header - src) < src_len) {
    STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_header;
    TU32 data_type_4cc = (p_header->data_type_4cc_31to24 << 24) | (p_header->data_type_4cc_23to16 << 16) | (p_header->data_type_4cc_15to8 << 8) | p_header->data_type_4cc_7to0;
    TU32 data_type_wanted_4cc = 0;
    TU16 data_sub_type = (p_header->data_sub_type_15to8 << 8) | p_header->data_sub_type_7to0;
    TU16 data_length = (p_header->data_length_15to8 << 8) | p_header->data_length_7to0;
    switch (data_type) {
      case ETSPrivateDataType_StartTime:
        data_type_wanted_4cc = 0x54535354;//TSST
        break;
      case ETSPrivateDataType_Duration:
        data_type_wanted_4cc = 0x54534455;//TSDU
        break;
      case ETSPrivateDataType_ChannelName:
        data_type_wanted_4cc = 0x5453434E;//TSCN
        break;
      case ETSPrivateDataType_TSPakIdx4GopStart:
        data_type_wanted_4cc = 0x54535049;//TSPI
        break;
      case ETSPrivateDataType_MotionEvent:
        data_type_wanted_4cc = 0x54534D45;//TSME
        break;
      default:
        LOG_ERROR("gfRetrieveTSPriData, invalid data_type=%d\n", data_type);
        return EECode_BadParam;
    }
    if ((data_type_wanted_4cc == data_type_4cc) && (sub_type == data_sub_type)) {
      if (data_length > dest_buf_len) {
        LOG_ERROR("gfRetrieveTSPriData, dest buffer is not large enough to contain data_type=%d\n", data_type);
        return EECode_InternalLogicalBug;
      }
      p_cur_header += sizeof(STSPrivateDataHeader);//now p_cur_header point to the beginning of data
      switch (data_type) {
        case ETSPrivateDataType_StartTime: {
            SDateTime *p_start_time_dest = (SDateTime *)dest;
            STSPrivateDataStartTime *p_start_time_src = (STSPrivateDataStartTime *)p_cur_header;
            p_start_time_dest->year = (p_start_time_src->year_15to8 << 8) | p_start_time_src->year_7to0;
            p_start_time_dest->month = p_start_time_src->month;
            p_start_time_dest->day = p_start_time_src->day;
            p_start_time_dest->hour = p_start_time_src->hour;
            p_start_time_dest->minute = p_start_time_src->minute;
            p_start_time_dest->seconds = p_start_time_src->seconds;
          }
          break;
        case ETSPrivateDataType_Duration: {
            TU16 *p_duration_dest = (TU16 *)dest;
            STSPrivateDataDuration *p_duration_src = (STSPrivateDataDuration *)p_cur_header;
            *p_duration_dest = (p_duration_src->file_duration_15to8 << 8) | p_duration_src->file_duration_7to0;
          }
          break;
        case ETSPrivateDataType_ChannelName: {
            TU8 *p_chaname_dest = (TU8 *)dest;
            memcpy(p_chaname_dest, p_cur_header, data_length);
          }
          break;
        case ETSPrivateDataType_TSPakIdx4GopStart: {
            TU32 *p_tspakidx_dest = (TU32 *)dest;
            STSPrivateDataTSPakIdx4GopStart *p_tspakidx_src = (STSPrivateDataTSPakIdx4GopStart *)p_cur_header;
            for (TU32 i = 0; i < data_length / sizeof(STSPrivateDataTSPakIdx4GopStart); i++) {
              *p_tspakidx_dest = (p_tspakidx_src->idx_31to24 << 24) | (p_tspakidx_src->idx_23to16 << 16) | (p_tspakidx_src->idx_15to8 << 8) | p_tspakidx_src->idx_7to0;
              p_tspakidx_dest++;
              p_tspakidx_src++;
            }
          }
          break;
        case ETSPrivateDataType_MotionEvent:
          LOG_WARN("gfRetrieveTSPriData, ETSPrivateDataType_MotionEvent NoImplement\n");
        default:
          LOG_ERROR("gfRetrieveTSPriData, invalid data_type=%d\n", data_type);
          return EECode_BadParam;
      }
      return EECode_OK;
    } else {
      p_cur_header += sizeof(STSPrivateDataHeader) + data_length;
    }
  }
  LOG_WARN("gfRetrieveTSPriData, can not found data_type=%d sub_type=%hu in current source.\n", data_type, sub_type);
  return EECode_BadParam;
}

void gfHorizontalDownResampleUVU8By2(TU8 *psrc_u, TU8 *psrc_v, TU32 src_linesize, TU32 width, TU32 height)
{
  TU8 *p_1 = psrc_u;
  TU8 *p_2 = psrc_u;
  TU32 i = 0, j = 0;
  DASSERT(src_linesize >= width);
  src_linesize -= width;
  width = width >> 1;
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      p_1[0] = p_2[0];
      p_1 ++;
      p_2 += 2;
    }
    p_2 += src_linesize;
  }
  p_1 = psrc_v;
  p_2 = psrc_v;
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      p_1[0] = p_2[0];
      p_1 ++;
      p_2 += 2;
    }
    p_2 += src_linesize;
  }
}

void gfVerticalDownResampleUVU8By2(TU8 *psrc_u, TU8 *psrc_v, TU32 src_linesize, TU32 width, TU32 height)
{
  TU8 *p_1 = psrc_u + src_linesize;
  TU8 *p_2 = psrc_u + (src_linesize << 1);
  TU32 j = 0;
  for (j = 1; j < height; j ++) {
    memcpy(p_1, p_2, width);
    p_1 += src_linesize;
    p_2 += (src_linesize << 1);
  }
  p_1 = psrc_v + src_linesize;
  p_2 = psrc_v + (src_linesize << 1);
  for (j = 1; j < height; j ++) {
    memcpy(p_1, p_2, width);
    p_1 += src_linesize;
    p_2 += (src_linesize << 1);
  }
}

const TChar *gfGetRTPRecieverStateString(TU32 state)
{
  switch (state) {
    case DATA_THREAD_STATE_READ_FIRST_RTP_PACKET:
      return "(DATA_THREAD_STATE_READ_FIRST_RTP_PACKET)";
      break;
    case DATA_THREAD_STATE_READ_REMANING_RTP_PACKET:
      return "(DATA_THREAD_STATE_READ_REMANING_RTP_PACKET)";
      break;
    case DATA_THREAD_STATE_WAIT_OUTBUFFER:
      return "(DATA_THREAD_STATE_WAIT_OUTBUFFER)";
      break;
    case DATA_THREAD_STATE_SKIP_DATA:
      return "(DATA_THREAD_STATE_SKIP_DATA)";
      break;
    case DATA_THREAD_STATE_READ_FRAME_HEADER:
      return "(DATA_THREAD_STATE_READ_FRAME_HEADER)";
      break;
    case DATA_THREAD_STATE_READ_RTP_HEADER:
      return "(DATA_THREAD_STATE_READ_RTP_HEADER)";
      break;
    case DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER:
      return "(DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER)";
      break;
    case DATA_THREAD_STATE_READ_RTP_AUDIO_HEADER:
      return "(DATA_THREAD_STATE_READ_RTP_AUDIO_HEADER)";
      break;
    case DATA_THREAD_STATE_READ_RTP_VIDEO_DATA:
      return "(DATA_THREAD_STATE_READ_RTP_VIDEO_DATA)";
      break;
    case DATA_THREAD_STATE_READ_RTP_AAC_HEADER:
      return "(DATA_THREAD_STATE_READ_RTP_AAC_HEADER)";
      break;
    case DATA_THREAD_STATE_READ_RTP_AUDIO_DATA:
      return "(DATA_THREAD_STATE_READ_RTP_AUDIO_DATA)";
      break;
    case DATA_THREAD_STATE_READ_RTCP:
      return "(DATA_THREAD_STATE_READ_RTCP)";
      break;
    case DATA_THREAD_STATE_COMPLETE:
      return "(DATA_THREAD_STATE_COMPLETE)";
      break;
    case DATA_THREAD_STATE_ERROR:
      return "(DATA_THREAD_STATE_ERROR)";
      break;
    default:
      LOG_ERROR("Unknown rtprcv state %d\n", state);
      break;
  }
  return "(Unknown rtprcv state)";
}

EECode gfAllocateNV12FrameBuffer(CIBuffer *buffer, TU32 width, TU32 height, TU32 alignment)
{
  TU32 size = 0;
  TU32 aligned_width = 0, aligned_height = 0;
  TU8 *p = NULL, *palign = NULL;
  if (DUnlikely(!buffer)) {
    LOG_FATAL("NULL buffer pointer\n");
    return EECode_BadParam;
  }
  if (DUnlikely(!width || !height)) {
    LOG_ERROR("zero width %d or height %d\n", width, height);
    return EECode_BadParam;
  }
  if (DUnlikely(buffer->mbAllocated)) {
    LOG_ERROR("already allocated\n");
    return EECode_BadState;
  }
  buffer->mVideoWidth = width;
  buffer->mVideoHeight = height;
  aligned_width = (width + (alignment - 1)) & (~(alignment - 1));
  aligned_height = (height + 1) & (~(1));
  size = (aligned_width * aligned_height * 3 / 2) + (alignment * 2);
  p = (TU8 *) DDBG_MALLOC(size, "NVFB");
  if (DUnlikely(!p)) {
    LOG_ERROR("not enough memory %u, width height: %d %d\n", size, buffer->mVideoWidth, buffer->mVideoHeight);
    return EECode_NoMemory;
  }
  buffer->SetDataPtrBase(p, 0);
  buffer->SetDataMemorySize(size, 0);
  size = aligned_width * aligned_height;
  palign = (TU8 *)(((TULong)p + (alignment - 1)) & (~(alignment - 1)));
  buffer->SetDataPtr(palign, 0);
  palign = (TU8 *)(((TULong)palign + (aligned_width * aligned_height) + (alignment - 1)) & (~(alignment - 1)));
  buffer->SetDataPtr(palign, 1);
  buffer->SetDataPtr(NULL, 2);
  buffer->SetDataLineSize(aligned_width, 0);
  buffer->SetDataLineSize(aligned_width, 1);
  buffer->SetDataLineSize(0, 2);
  buffer->mbAllocated = 1;
  return EECode_OK;
}

TU32 gfIOSMSkipBox(TU8 *p)
{
  TU32 size = 0;
  TChar name[8] = {0};
  DBER32(size, p);
  memcpy(name, p + 4, 4);
  LOG_PRINTF("skip box %d, %s\n", size, name);
  return size;
}

EECode gfSDPProcessVideoExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size)
{
    TInt in_size = 0, out_size = 0;
    TChar *p_out_buffer = NULL;
    TU8 *p_in_buffer = NULL, *p_next = NULL;

    if (DUnlikely((!subsession) || (!p_extra_data) || (!size))) {
        LOG_FATAL("gfSDPProcessVideoExtraData: NULL %p, %p or zero %d\n", subsession, p_extra_data, size);
        return EECode_BadParam;
    }

    if (StreamFormat_H264 == subsession->format) {
        //LOG_NOTICE("%p, size %d\n", p_extra_data, size);
        //gfPrintMemory(p_extra_data, size);

        if ((0x0 == p_extra_data[0]) && (0x0 == p_extra_data[1]) && (0x0 == p_extra_data[2]) && (0x01 == p_extra_data[3]) && (0x09 == p_extra_data[4])) {
            p_extra_data += 6;
            size -= 6;
            while (0x0 != p_extra_data[0]) {
                if (size > 20) {
                    p_extra_data ++;
                    size --;
                } else {
                    LOG_FATAL("BAD case\n");
                    break;
                }
            }
        }

        //LOG_NOTICE("%p, size %d\n", p_extra_data, size);
        //gfPrintMemory(p_extra_data, size);

        p_in_buffer = p_extra_data + 5;
        p_out_buffer = &subsession->profile_level_id_base16[0];
        in_size = 3;
        out_size = 6;
        gfEncodingBase16(p_out_buffer, p_in_buffer, in_size);
        p_out_buffer[out_size] = 0x0;

        p_next = gfNALUFindNextStartCode(p_extra_data + 4, size - 4);
        if (DUnlikely(!p_next)) {
            LOG_FATAL("do not find pps?\n");
            return EECode_InternalLogicalBug;
        }

        //LOG_NOTICE("%p, size %d, p_next %p, gap %d\n", p_extra_data, size, p_next, (TU32)(p_next - p_extra_data));
        //gfPrintMemory(p_extra_data, size);
        if (0x0 == p_extra_data[2]) {
            DASSERT(0x0 == p_extra_data[0]);
            DASSERT(0x0 == p_extra_data[1]);
            DASSERT(0x1 == p_extra_data[3]);
            p_in_buffer = p_extra_data + 4;
        } else {
            DASSERT(0x0 == p_extra_data[0]);
            DASSERT(0x0 == p_extra_data[1]);
            DASSERT(0x1 == p_extra_data[2]);
            p_in_buffer = p_extra_data + 3;
        }

        DASSERT(ENalType_SPS == (p_in_buffer[0] & 0x1f));
        p_out_buffer = &subsession->sps_base64[0];
        in_size = (TULong)p_next - 4 - (TULong)p_in_buffer;
        DASSERT(in_size > 20);
        DASSERT(in_size < 128);
        out_size = 4 * ((in_size + 2) / 3);
        gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
        p_out_buffer[out_size] = 0x0;

        //LOG_NOTICE("%p, size %d, p_next %p, gap %d\n", p_extra_data, size, p_next, (TU32)(p_next - p_extra_data));
        //gfPrintMemory(p_extra_data, size);

        p_in_buffer = p_next;
        DASSERT(ENalType_PPS == (p_in_buffer[0] & 0x1f));
        p_out_buffer = &subsession->pps_base64[0];
        in_size = ((TULong)(p_extra_data) + size) - (TULong)p_in_buffer;

        if (DUnlikely(in_size > 20)) {
            LOG_FATAL("extra data corruption, in_size %d\n", in_size);
            return EECode_DataCorruption;
        }

        DASSERT(in_size < 20);
        DASSERT(in_size > 3);
        out_size = 4 * ((in_size + 2) / 3);
        gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
        p_out_buffer[out_size] = 0x0;

        LOG_INFO("[h264 streaming]: profile level id %s\n", &subsession->profile_level_id_base16[0]);
        LOG_INFO("[h264 streaming]: sps %s\n", &subsession->sps_base64[0]);
        LOG_INFO("[h264 streaming]: pps %s\n", &subsession->pps_base64[0]);

        subsession->data_comes = 1;
        subsession->parameters_setup = 1;
    } else if (StreamFormat_H265 == subsession->format) {
        //LOG_NOTICE("%p, size %d\n", p_extra_data, size);
        //gfPrintMemory(p_extra_data, size);
        TU8 has_vps = 0, has_sps = 0, has_pps = 0, has_idr = 0;
        TU8 *p_vps = NULL, *p_sps = NULL, *p_pps = NULL, *p_pps_end = NULL, *p_idr = NULL;
        gfFindH265VpsSpsPpsIdr(p_extra_data, size, has_vps, has_sps, has_pps, has_idr, p_vps, p_sps, p_pps, p_pps_end, p_idr);

        if (has_vps && has_sps && has_pps) {
            if (p_vps && p_sps && p_pps && p_pps_end) {
                if (0x0 == p_vps[2]) {
                    DASSERT(0x0 == p_vps[0]);
                    DASSERT(0x0 == p_vps[1]);
                    DASSERT(0x1 == p_vps[3]);
                    p_in_buffer = p_vps + 4;
                    in_size = (TInt) (p_sps - p_vps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 4;
                } else {
                    DASSERT(0x0 == p_vps[0]);
                    DASSERT(0x0 == p_vps[1]);
                    DASSERT(0x1 == p_vps[2]);
                    p_in_buffer = p_vps + 3;
                    in_size = (TInt) (p_sps - p_vps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 3;
                }

                p_out_buffer = &subsession->hevc_vps_base64[0];
                out_size = 4 * ((in_size + 2) / 3);
                gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
                p_out_buffer[out_size] = 0x0;

                if (0x0 == p_sps[2]) {
                    DASSERT(0x0 == p_sps[0]);
                    DASSERT(0x0 == p_sps[1]);
                    DASSERT(0x1 == p_sps[3]);
                    p_in_buffer = p_sps + 4;
                    in_size = (TInt) (p_pps - p_sps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 4;
                } else {
                    DASSERT(0x0 == p_sps[0]);
                    DASSERT(0x0 == p_sps[1]);
                    DASSERT(0x1 == p_sps[2]);
                    p_in_buffer = p_sps + 3;
                    in_size = (TInt) (p_pps - p_sps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 3;
                }

                p_out_buffer = &subsession->hevc_sps_base64[0];
                out_size = 4 * ((in_size + 2) / 3);
                gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
                p_out_buffer[out_size] = 0x0;

                SHEVCDecoderConfigurationRecord record;
                TU32 width, height;
                gfParseHEVCSPS(&record, p_in_buffer, (TU32) in_size, width, height);
                subsession->video_width = width;
                subsession->video_height = height;
                subsession->hevc_profile_space = record.general_profile_space;
                subsession->hevc_profile_id = record.general_profile_idc;
                subsession->hevc_tier_flag = record.general_tier_flag;
                subsession->hevc_level_id = record.general_level_idc;
                subsession->hevc_interop_constraints = record.general_constraint_indicator_flags;

                if (0x0 == p_pps[2]) {
                    DASSERT(0x0 == p_pps[0]);
                    DASSERT(0x0 == p_pps[1]);
                    DASSERT(0x1 == p_pps[3]);
                    p_in_buffer = p_pps + 4;
                    in_size = (TInt) (p_pps_end - p_pps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 4;
                } else {
                    DASSERT(0x0 == p_pps[0]);
                    DASSERT(0x0 == p_pps[1]);
                    DASSERT(0x1 == p_pps[2]);
                    p_in_buffer = p_pps + 3;
                    in_size = (TInt) (p_pps_end - p_pps);
                    DASSERT(in_size < 128);
                    DASSERT(in_size > 6);
                    in_size -= 3;
                }

                p_out_buffer = &subsession->hevc_pps_base64[0];
                out_size = 4 * ((in_size + 2) / 3);
                gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
                p_out_buffer[out_size] = 0x0;

                LOG_INFO("[h265 streaming]: width %d, height %d, profile space %d, profile id %d, tier flag %d, level id %d, hevc_interop_constraints %" DPrint64x "\n", \
                    subsession->video_width, subsession->video_height, subsession->hevc_profile_space, \
                    subsession->hevc_profile_id, subsession->hevc_tier_flag, subsession->hevc_level_id, \
                    subsession->hevc_interop_constraints);
                LOG_INFO("[h265 streaming]: vps %s\n", &subsession->hevc_vps_base64[0]);
                LOG_INFO("[h265 streaming]: sps %s\n", &subsession->hevc_sps_base64[0]);
                LOG_INFO("[h265 streaming]: pps %s\n", &subsession->hevc_pps_base64[0]);

            } else {
                LOG_FATAL("not complete h265 seq header, %p, %p, %p, %p\n", p_vps, p_sps, p_pps, p_pps_end);
                gfPrintMemory(p_extra_data, size);
                return EECode_InternalLogicalBug;
            }
        }
    } else {
        LOG_FATAL("BAD video format %x\n", subsession->format);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode gfSDPProcessAudioExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size)
{
    if (DUnlikely((!subsession) || (!p_extra_data) || (!size))) {
        LOG_FATAL("gfSDPProcessAudioExtraData: NULL %p, %p or zero %d\n", subsession, p_extra_data, size);
        return EECode_BadParam;
    }

    if (p_extra_data && size) {
        DASSERT(size < 32);
        if (DLikely(32 > size)) {
            gfEncodingBase16(subsession->aac_config_base16, p_extra_data, size);
            subsession->aac_config_base16[31] = 0x0;
        }
    }

    subsession->data_comes = 1;
    subsession->parameters_setup = 1;

    return EECode_OK;
}

