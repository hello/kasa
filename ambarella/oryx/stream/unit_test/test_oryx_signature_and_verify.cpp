/*******************************************************************************
 * test_oryx_signature_and_verify.cpp
 *
 * History:
 *   2016-9-26 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <stdio.h>
#include <string.h>
#include "am_signature_verify_if.h"
#include "am_data_signature_if.h"

bool data_signature(uint8_t *sig_value_buf, uint32_t sig_buf_len,
                    uint32_t &sig_value_len, const char *key_file)
{
  bool ret = true;
  AMIDataSignature *signature = nullptr;
  std::string mess1 = "Test message";
  std::string mess2 = "Hello World";
  do {
    signature = AMIDataSignature::create(key_file);
    if (!signature) {
      ERROR("Failed to create AMIDataSignature instance.");
      ret = false;
      break;
    }
    if (!signature->init_signature()) {
      ERROR("Failed to init signature.");
      ret = false;
      break;
    }
    if (!signature->signature_update_data((const uint8_t*)mess1.c_str(),
                                     mess1.size())) {
      ERROR("Failed to signature update.");
      ret = false;
      break;
    }
    if (!signature->signature_update_data((const uint8_t*)mess2.c_str(),
                                     mess2.size())) {
      ERROR("Failed to signature update.");
      ret = false;
      break;
    }
    if (!signature->signature_final()) {
      ERROR("Signature final error.");
      ret = false;
      break;
    }
    uint32_t sig_len = 0;
    const uint8_t *sig_data = signature->get_signature(sig_len);
    if (sig_len == 0) {
      ERROR("Failed to get signature value.");
      ret = false;
      break;
    }
    if (sig_buf_len < sig_len) {
      ERROR("The signature value buf is too small, the buf len is %u, "
          "the signature value len is %u", sig_buf_len, sig_len);
      ret = false;
      break;
    }
    memcpy(sig_value_buf, sig_data, sig_len);
    ERROR("sig_len is %u", sig_len);
    sig_value_len = sig_len;
    for (uint32_t i = 0; i < sig_len; ++ i) {
      if (i % 10 == 0) {
        printf("\n");
      }
      printf("%x", sig_data[i]);
    }
    printf("\n");
  } while(0);
  if (signature) {
    signature->destroy();
  }
  return ret;
}

bool verify_sig_data(const uint8_t *sig_value, uint32_t sig_value_len,
                     const char *key_file)
{
  bool ret = true;
  AMISignatureVerify *verify = nullptr;
  std::string mess1 = "Test message";
  std::string mess2 = "Hello World";
  do {
    verify = AMISignatureVerify::create(key_file);
    if (!verify) {
      ERROR("Failed to create AMISignatureVerify instance.");
      ret = false;
      break;
    }
    if (!verify->init_sig_verify()) {
      ERROR("Failed to init sig verify.");
      ret = false;
      break;
    }
    if (!verify->verify_update((const uint8_t*)mess1.c_str(), mess1.size())) {
      ERROR("Failed to verify update.");
      ret = false;
      break;
    }
    if (!verify->verify_update((const uint8_t*)mess2.c_str(), mess2.size())) {
      ERROR("Failed to verify update.");
      ret = false;
      break;
    }
    if (!verify->verify_final(sig_value, sig_value_len)) {
      ERROR("Failed to verify final.");
      ret = false;
    }
  } while(0);
  if (verify) {
    verify->destroy();
  }
  return ret;
}

int main()
{
  int ret = 0;
  std::string private_key = "/etc/oryx/stream/muxer/private.pem";
  std::string pub_key = "/etc/oryx/stream/muxer/public.pem";
  uint8_t sig_value[1024] = {0};
  uint32_t sig_length = 0;
  do {
    if (!data_signature(sig_value, sizeof(sig_value),
                       sig_length, private_key.c_str())) {
      ERROR("Faield to data signature.");
      ret = -1;
      break;
    }
    if (!verify_sig_data(sig_value, sig_length, pub_key.c_str())) {
      PRINTF("Verify sig data error.");
    } else {
      PRINTF("Verify sig data success.");
    }
  } while(0);
  return ret;
}

