/*******************************************************************************
 * am_define.cpp
 *
 * History:
 *   2013-2-19 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_define.h"
#include <sys/time.h>
#include <stdio.h>

char* amstrdup(const char* str)
{
  char* newstr = NULL;
  if (AM_LIKELY(str)) {
    newstr = new char[strlen(str) + 1];
    if (AM_LIKELY(newstr)) {
      memset(newstr, 0, strlen(str) + 1);
      strcpy(newstr, str);
    }
  }

  return newstr;
}

unsigned long long get_gcd(unsigned long long a, unsigned long long b)
{
  while(((a > b) ? (a %= b) : (b %= a)));
  return (a + b);
}

unsigned long long get_lcm(unsigned long long a, unsigned long long b)
{
  return (a * b) / get_gcd(a, b);
}

std::string base64_encode(const uint8_t *data, uint32_t data_len)
{
  std::string str;

  if (AM_LIKELY(data && (data_len > 0))) {
    const char base64Char[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint32_t numOrig24BitValues = data_len / 3;
    bool havePadding  = (data_len > (numOrig24BitValues * 3));
    bool havePadding2 = (data_len == (numOrig24BitValues * 3 + 2));
    const uint32_t numResultBytes =
        4 * (numOrig24BitValues + (havePadding ? 1 : 0));
    char base64_str[numResultBytes + 1];
    uint32_t i = 0;

    /* Map each full group of 3 input bytes into 4 output base-64 characters: */
    for (i = 0; i < numOrig24BitValues; ++ i) {
      base64_str[4 * i + 0] = base64Char[(data[3 * i] >> 2) & 0x3F];
      base64_str[4 * i + 1] = base64Char[(((data[3 * i] & 0x3) << 4) |
          (data[3 * i + 1] >> 4)) & 0x3F];
      base64_str[4 * i + 2] = base64Char[((data[3 * i + 1] << 2) |
          (data[3 * i + 2] >> 6)) & 0x3F];
      base64_str[4 * i + 3] = base64Char[data[3 * i + 2] & 0x3F];
    }

    /* Now, take padding into account.  (Note: i == numOrig24BitValues) */
    if (havePadding) {
      base64_str[4 * i + 0] = base64Char[(data[ 3 * i] >> 2) & 0x3F];
      if (havePadding2) {
        base64_str[4 * i + 1] = base64Char[(((data[3 * i] & 0x3) << 4) |
                                           (data[3 * i + 1] >> 4)) & 0x3F];
        base64_str[4 * i + 2] = base64Char[(data[3 * i + 1] << 2) & 0x3F];
      } else {
        base64_str[4 * i + 1] = base64Char[((data[3 * i] & 0x3) << 4) & 0x3F];
        base64_str[4 * i + 2] = '=';
      }
      base64_str[4 * i + 3] = '=';
    }

    base64_str[numResultBytes] = '\0';
    str = std::string(base64_str);
  }

  return str;
}

#ifdef ENABLE_ORYX_PERFORMANCE_PROFILE
void print_current_time(const char *info)
{
    struct timeval current;
    gettimeofday(&current, NULL);
    printf("\033[1;31m##### %s:\t%lu.%lu\033[0m\n", info, current.tv_sec, current.tv_usec);
}
#endif
