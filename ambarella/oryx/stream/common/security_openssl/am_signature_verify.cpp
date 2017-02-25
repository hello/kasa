/*******************************************************************************
 * am_signature_verify.cpp
 *
 * History:
 *   2016-9-23 - [ccjing] created file
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
#include "am_signature_verify.h"

AMISignatureVerify* AMISignatureVerify::create(const char *key_file)
{
  return (AMISignatureVerify*)(AMSignatureVerify::create(key_file));
}

AMSignatureVerify* AMSignatureVerify::create(const char *key_file)
{
  AMSignatureVerify* result = new AMSignatureVerify();
  if (result && (!result->init(key_file))) {
    ERROR("Failed to init AMSignatureVerify");
    delete result;
    result = nullptr;
  }
  return result;
}

AMSignatureVerify::AMSignatureVerify()
{
}

AMSignatureVerify::~AMSignatureVerify()
{
  EVP_MD_CTX_cleanup(&m_md_ctx);
  EVP_PKEY_free(m_evp_key);
}

bool AMSignatureVerify::init(const char *key_file)
{
  bool ret = true;
  BIO *bio = nullptr;
  do {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    char err_buf[512] = {0};
    if (!key_file) {
      ERROR("The key_file is invalid.");
      ret = false;
      break;
    }
    INFO("Public key file is : %s", key_file);
    bio = BIO_new(BIO_s_file());
    if (!bio) {
      ERROR("Bio new fail");
      ret = false;
      break;
    }
    if (BIO_read_filename(bio, key_file) < 0) {
      ERROR("bio read filename error.");
      ret = false;
      break;
    }
    if (m_evp_key) {
      WARN("The evp key is exist, destroy it first.");
      EVP_PKEY_free(m_evp_key);
      m_evp_key = nullptr;
    }
    m_evp_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    if (!m_evp_key) {
      ERR_error_string(ERR_get_error(), err_buf);
      ERROR("PEM_read_bio_PUBKEY: %s", err_buf);
      ret = false;
      break;
    }
  } while(0);
  BIO_free(bio);
  return ret;
}

bool AMSignatureVerify::init_sig_verify()
{
  bool ret = true;
  do {
    EVP_MD_CTX_init(&m_md_ctx);
    if (EVP_VerifyInit(&m_md_ctx, EVP_sha256()) != 1) {
      ERROR("EVP_VerifyInit error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMSignatureVerify::verify_update(const uint8_t *data, uint32_t data_len)
{
  bool ret = true;
  do {
    if (EVP_VerifyUpdate(&m_md_ctx, data, data_len) != 1) {
      ERROR("EVP_VerifyUpdate error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMSignatureVerify::verify_final(const uint8_t *sig_value, uint32_t sig_len)
{
  bool ret = true;
  do {
    if (EVP_VerifyFinal(&m_md_ctx, sig_value, sig_len, m_evp_key) != 1) {
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void AMSignatureVerify::clean_md_ctx()
{
  EVP_MD_CTX_cleanup(&m_md_ctx);
}

void AMSignatureVerify::destroy()
{
  delete this;
}

