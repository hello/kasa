/*******************************************************************************
 * am_data_signature.cpp
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
#include <arpa/inet.h>
#include "am_data_signature.h"

AMIDataSignature* AMIDataSignature::create(const char* key_file)
{
  return (AMIDataSignature*)(AMDataSignature::create(key_file));
}

AMDataSignature* AMDataSignature::create(const char* key_file)
{
  AMDataSignature* result = new AMDataSignature();
  if (result && (!result->init(key_file))) {
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMDataSignature::init(const char* key_file)
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
    INFO("Key file is : %s", key_file);
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
    m_evp_key = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    if (!m_evp_key) {
      ERR_error_string(ERR_get_error(), err_buf);
      ERROR("PEM_read_bio_PrivateKey: %s", err_buf);
      ret = false;
      break;
    }
  } while(0);
  BIO_free(bio);
  return ret;
}

AMDataSignature::AMDataSignature()
{
}

AMDataSignature::~AMDataSignature()
{
  if (m_is_ctx_init) {
    EVP_MD_CTX_cleanup(&m_md_ctx);
  }
  EVP_PKEY_free(m_evp_key);
}

bool AMDataSignature::init_signature()
{
  bool ret = true;
  do {
    EVP_MD_CTX_init(&m_md_ctx);
    m_is_ctx_init = true;
    if (EVP_SignInit(&m_md_ctx, EVP_sha256()) != 1) {
      ERROR("EVP_SignInit_ex error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMDataSignature::signature_update_data(const uint8_t *data, uint32_t data_size)
{
  bool ret = true;
  do {
    if (EVP_SignUpdate(&m_md_ctx, data, data_size) != 1) {
      ERROR("EVP_SignUpdate error.");
      ret = false;
      break;
    }
    m_update_data_len += data_size;
  } while(0);
  return ret;
}

bool AMDataSignature::signature_update_u32(uint32_t value)
{
  bool ret = true;
  do {
    uint32_t re_value = htonl(value);
    if (EVP_SignUpdate(&m_md_ctx, (uint8_t*)(&re_value), sizeof(uint32_t)) != 1) {
      ERROR("EVP_SignUpdate error.");
      ret = false;
      break;
    }
    m_update_data_len += sizeof(uint32_t);
  } while(0);
  return ret;
}

bool AMDataSignature::signature_final()
{
  bool ret = true;
  do {
    if (EVP_SignFinal(&m_md_ctx, m_sig_value, &m_sig_length, m_evp_key) != 1) {
      ERROR("EVP_SignFinal error.");
      ret = false;
      break;
    }
    if (m_sig_length > SIG_BUF_LENGTH) {
      ERROR("The length of signature value length is bigger than "
          "SIG_BUF_LENGTH : %d", SIG_BUF_LENGTH);
      ret = false;
      break;
    }
    INFO("The update data length is %u", m_update_data_len);
    m_update_data_len = 0;
  } while(0);
  return ret;
}

const uint8_t* AMDataSignature::get_signature(uint32_t& sig_len)
{
  sig_len = m_sig_length;
  return m_sig_value;
}

void AMDataSignature::clean_md_ctx()
{
  EVP_MD_CTX_cleanup(&m_md_ctx);
  m_is_ctx_init = false;
  memset(m_sig_value, 0, sizeof(m_sig_value));
  m_sig_length = 0;
}

void AMDataSignature::destroy()
{
  delete this;
}
