/*******************************************************************************
 * am_data_signature.h
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
#ifndef AM_DATA_SIGNATURE_H_
#define AM_DATA_SIGNATURE_H_
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "am_data_signature_if.h"
#define SIG_BUF_LENGTH 1024
class AMDataSignature : public AMIDataSignature
{
  public :
    virtual bool init_signature();
    virtual bool signature_update_data(const uint8_t *data, uint32_t data_size);
    virtual bool signature_update_u32(uint32_t value);
    virtual bool signature_final();
    virtual const uint8_t* get_signature(uint32_t& sig_len);
    virtual void clean_md_ctx();
    virtual void destroy();
  public :
    static AMDataSignature* create(const char *key_file);
  private :
    AMDataSignature();
    virtual ~AMDataSignature();
    bool init(const char *key_file);
  private :
    EVP_PKEY   *m_evp_key                   = nullptr;
    uint32_t    m_sig_length                = 0;
    uint32_t    m_update_data_len           = 0;
    bool        m_is_ctx_init               = false;
    uint8_t     m_sig_value[SIG_BUF_LENGTH] = {0};
    EVP_MD_CTX  m_md_ctx;
};



#endif /* AM_DATA_SIGNATURE_H_ */
