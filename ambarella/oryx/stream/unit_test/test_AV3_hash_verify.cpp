/*******************************************************************************
 * test_AV3_hash_verify.cpp
 *
 * History:
 *   2016-9-27 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_file_sink_if.h"
#include "am_signature_verify_if.h"
#include <string>

int main(int argc, char* argv[])
{
  uint8_t read_data[1024] = {0};
  uint32_t read_size = 0;
  uint8_t sig_value[256] = {0};
  uint32_t sig_len = 256;
  AMIFileReader *file_reader = nullptr;
  AMISignatureVerify *verify = nullptr;
  int ret = 0;
  do {
    if (argc < 3) {
      PRINTF("Please use :test_AV3_hash_verify AV3_file key_file");
      break;
    }
    std::string AV3_file = argv[1];
    std::string key_file = argv[2];
    file_reader = AMIFileReader::create();
    if (!file_reader) {
      ERROR("Failed to create AMIFileReader");
      ret = -1;
      break;
    }
    if (!file_reader->open_file(AV3_file.c_str())) {
      ERROR("Failed to open file.");
      ret = -1;
      break;
    }
    if (!file_reader->seek_file(36)) {
      ERROR("Failed to seek file");
      ret = -1;
      break;
    }
    if ((sig_len = file_reader->read_file(sig_value, sig_len)) != 256) {
      ERROR("Failed to read sig value");
      ret = -1;
      break;
    }
    if (!file_reader->seek_file(36 + 512)) {
      ERROR("Failed to seek file");
      ret = -1;
      break;
    }
    verify = AMISignatureVerify::create(key_file.c_str());
    if (!verify) {
      ERROR("Failed to create AMISignatureVerify instance.");
      ret = -1;
      break;
    }
    if (!verify->init_sig_verify()) {
      ERROR("Failed to init sig verify.");
      ret = -1;
      break;
    }
    while((read_size = file_reader->read_file(read_data, 1024)) > 0) {
      if (!verify->verify_update(read_data, read_size)) {
        ERROR("Failed to verify update.");
        ret = -1;
        break;
      }
    }
    if (!verify->verify_final(sig_value, sig_len)) {
      PRINTF("Verify error.");
      ret = -1;
    } else {
      PRINTF("Verify success.");
    }
  } while(0);
  if (file_reader) {
    file_reader->close_file();
    file_reader->destroy();
  }
  if (verify) {
    verify->destroy();
  }
  return ret;
}
