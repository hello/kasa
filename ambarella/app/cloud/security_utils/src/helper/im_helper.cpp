/*******************************************************************************
 * im_helper.cpp
 *
 * History:
 *  2014/07/29 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "security_utils_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

EECode gfParseProductionCode(TChar *p_production_code, TU32 length, TUniqueID &id)
{
    if (DUnlikely(!p_production_code)) {
        LOG_FATAL("NULL p_production_code\n");
        return EECode_BadParam;
    }

    if (DUnlikely((length > 512) || (length < 8))) {
        LOG_FATAL("(%d) too long or too short production code\n", length);
        return EECode_BadParam;
    }

    TU8 *p_buf = (TU8 *) DDBG_MALLOC(length, "PPC0");
    if (DUnlikely(!p_buf)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_dec_buf = (TU8 *) p_buf + (length / 2);

    gfDecodingBase16(p_buf, (const TU8 *) p_production_code, (TInt) length);
    length = length / 2;

    TMemSize size = length;
    EECode err = gfCustomizedDecryptionV0(p_buf, (TMemSize)length, p_dec_buf, size);

    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("not valid production code\n");
        DDBG_FREE(p_buf, "PPC0");
        return EECode_NoMemory;
    }

    id = ((TUniqueID)p_dec_buf[0] << 56) | ((TUniqueID)p_dec_buf[1] << 48) | ((TUniqueID)p_dec_buf[2] << 40) | ((TUniqueID)p_dec_buf[3] << 32) | ((TUniqueID)p_dec_buf[4] << 24) | ((TUniqueID)p_dec_buf[5] << 16) | ((TUniqueID)p_dec_buf[6] << 8) | ((TUniqueID)p_dec_buf[7]);
    DDBG_FREE(p_buf, "PPC0");

    return EECode_OK;
}

//TChar* gfGenerateProductionCode(TU32& production_code_length, TUniqueID id, TChar* desc, TU32 desc_len, TU8 random_seed0, TU8 random_seed1, TU8 random_seed2, TU8 random_seed3)
TChar *gfGenerateProductionCode(TU32 &production_code_length, TUniqueID id, TChar *desc, TU32 desc_len)
{
    TU32 len = sizeof(TUniqueID) + desc_len + 32;
    TChar *p_buf = (TChar *) DDBG_MALLOC(len * 2, "PPC1");
    if (DUnlikely(!p_buf)) {
        LOG_FATAL("no memory\n");
        return NULL;
    }
    TU8 *p_buf_enc = (TU8 *) DDBG_MALLOC(len, "PPC2");
    if (DUnlikely(!p_buf_enc)) {
        LOG_FATAL("no memory\n");
        DDBG_FREE(p_buf, "GPC0");
        return NULL;
    }

    //p_buf[0] = (((random_seed0 & random_seed1) ^ (random_seed2)) + 23) % 257;
    //p_buf[1] = (((random_seed1 + random_seed2) | (random_seed3)) + 89) % 263;
    //p_buf[2] = (((random_seed2 ^ random_seed3) + (random_seed0)) + 19) % 257;
    //p_buf[3] = (((random_seed3 | random_seed0) ^ (random_seed1)) + 47) % 263;

    p_buf[0] = (id >> 56) & 0xff;
    p_buf[1] = (id >> 48) & 0xff;
    p_buf[2] = (id >> 40) & 0xff;
    p_buf[3] = (id >> 32) & 0xff;
    p_buf[4] = (id >> 24) & 0xff;
    p_buf[5] = (id >> 16) & 0xff;
    p_buf[6] = (id >> 8) & 0xff;
    p_buf[7] = (id) & 0xff;

    if (desc && desc_len) {
        memcpy(p_buf + sizeof(TUniqueID), desc, desc_len);
    }

    TMemSize enc_size = len * 2 ;
    EECode err = gfCustomizedEncryptionV0(p_buf_enc, enc_size, (TU8 *)p_buf, (TMemSize)(sizeof(TUniqueID) + desc_len));
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("enc fail\n");
        DDBG_FREE(p_buf, "PPC1");
        DDBG_FREE(p_buf_enc, "PPC2");
        return NULL;
    }

    gfEncodingBase16(p_buf, (const TU8 *) p_buf_enc, (TInt) enc_size);
    production_code_length = enc_size * 2;
    DDBG_FREE(p_buf_enc, "PPC2");

    return p_buf;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

