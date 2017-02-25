/*******************************************************************************
 * test_am_jpeg_exif.cpp
 *
 * History:
 *   2016-07-27 - [smdong] created file
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
#include "am_log.h"

#include "am_jpeg_exif_if.h"

AMIJpegExifPtr g_jpeg_exif = nullptr;

const char *jpeg_file_name = nullptr;
const char *exif_file_name = nullptr;
const char *output_file_name = nullptr;
uint32_t thumbnail_jpeg_qual = 0;

static void usage(int argc, char *argv[])
{
  PRINTF("\n Usage: %s [options]\n", argv[0]);
  PRINTF("  -j  input jpeg file name\n"
         "  -e  input exif file name\n"
         "  -o  output jpeg file name\n"
         "  -t  thumbnail jpeg quality(1 = worst, 100 = best)\n"
         "      if 0, then will not generate thumbnail.\n");
}

static int init_param(int argc, char *argv[])
{
  int ret = 0;
  int opt = 0;

  while ((opt = getopt(argc, argv, "j:e:o:t:")) != -1) {
    switch(opt) {
      case 'j': {
        jpeg_file_name = amstrdup(optarg);
      }break;
      case 'e': {
        exif_file_name = amstrdup(optarg);
      }break;
      case 'o': {
        output_file_name = amstrdup(optarg);
      }break;
      case 't': {
        if ((atoi(optarg) >= 0) && (atoi(optarg) <= 100)) {
          thumbnail_jpeg_qual = atoi(optarg);
        } else {
          ERROR("Invalid thumbnail option, please input [0-100].");
          ret = -1;
        }
      }break;

    }
  }

  return ret;
}

int main(int argc, char *argv[])
{
  int ret = 0;

  do {
    if (argc < 2) {
      usage(argc, argv);
      ret = -1;
      break;
    }

    if (0 != init_param(argc, argv)) {
      ERROR("Init param error");
      usage(argc, argv);
      ret = -1;
      break;
    }
    g_jpeg_exif = AMIJpegExif::create();
    if (AM_LIKELY(g_jpeg_exif != nullptr)) {
      g_jpeg_exif->combine_jpeg_exif(jpeg_file_name, exif_file_name,
                                     output_file_name, thumbnail_jpeg_qual);
    }

  } while(0);

  delete[] jpeg_file_name;
  delete[] exif_file_name;
  delete[] output_file_name;

  return ret;
}

