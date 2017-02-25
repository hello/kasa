/*******************************************************************************
 * am_define.cpp
 *
 * History:
 *   2013-2-19 - [ypchang] created file
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

uint32_t random_number()
{
  struct timeval current = {0};
  gettimeofday(&current, nullptr);
  srand(current.tv_usec);

  return (rand() % ((uint32_t)-1));
}

#ifdef ENABLE_ORYX_PERFORMANCE_PROFILE
void print_current_time(const char *info)
{
    struct timeval current;
    gettimeofday(&current, NULL);
    printf("\033[1;31m##### %s:\t%lu.%lu\033[0m\n", info, current.tv_sec, current.tv_usec);
}
#endif
