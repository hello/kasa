/*******************************************************************************
 * customized_v1_encryptor.cpp
 *
 * History:
 *  2014/03/08 - [Zhi He] create file
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

#include "licence_lib_if.h"
#include "licence_encryption_if.h"

#include "customized_v1_encryptor.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static TU8 gCustomizedV1EncryptionPatten[256] = {0};
static TU8 gCustomizedV1RandomSeed[4] = {11, 43, 7, 73};
static TU8 gCustomizedV1RandomSeedSource1[8] = {37, 71, 7, 19, 101, 89, 23, 43};
static TU8 gCustomizedV1RandomSeedSource2[8] = {3, 47, 59, 83, 29, 31, 107, 71};

typedef struct {
    TU8 sync_byte0;
    TU8 sync_byte1;
    TU8 sync_byte2;
    TU8 sync_byte3;

    TU8 offset1;
    TU8 offset2;
    TU8 param1;
    TU8 param2;

} SCustomizedV1EncryptionHeader;

ILicenceEncryptor *gfCreateCustomizedV1Encryptor(TU32 seed1, TU32 seed2)
{
    CCustomizedV1Encryptor *thiz = CCustomizedV1Encryptor::Create(seed1, seed2);
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CCustomizedV1Encryptor
//
//-----------------------------------------------------------------------

CCustomizedV1Encryptor *CCustomizedV1Encryptor::Create(TU32 seed1, TU32 seed2)
{
    CCustomizedV1Encryptor *result = new CCustomizedV1Encryptor();
    if ((result) && (EECode_OK != result->Construct(seed1, seed2))) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CCustomizedV1Encryptor::Construct(TU32 seed1, TU32 seed2)
{
    if (!gCustomizedV1EncryptionPatten[0] && !gCustomizedV1EncryptionPatten[1]) {
        //TU32 dseed1 = gCustomizedV1RandomSeedSource1[seed1 & 0x7], dseed2 = gCustomizedV1RandomSeedSource2[seed2 & 0x7];
        TU32 index = 0;
        seed1 += 1;
        seed2 += 1;

        gCustomizedV1EncryptionPatten[0] = gCustomizedV1RandomSeedSource1[0];
        //gCustomizedV1EncryptionPatten[0] = dseed2;
        for (index = 0; index < 255; index ++) {
            gCustomizedV1EncryptionPatten[index + 1] = gCustomizedV1EncryptionPatten[index] + gCustomizedV1RandomSeedSource2[0];
        }

    }
    return EECode_OK;
}

CCustomizedV1Encryptor::CCustomizedV1Encryptor()
{

}

CCustomizedV1Encryptor::~CCustomizedV1Encryptor()
{

}

EECode CCustomizedV1Encryptor::Encryption(TU8 *p_encryption, TMemSize &encryption_size, TU8 *p_decryption, TMemSize decryption_size)
{
    TMemSize encrypted_size = ((decryption_size + 8 + 15) & (~15)) + 16;
    //LOG_DEBUG("encrypted_size %ld, encryption_size %ld, (decryption_size + 8 + 15) %ld, ((decryption_size + 8 + 15) & (~15)) %ld\n", encrypted_size, encryption_size, (decryption_size + 8 + 15), ((decryption_size + 8 + 15) & (~15)));

    if (DUnlikely(encrypted_size > encryption_size)) {
        LOG_FATAL("output size(%ld) should greater than (decryption_size + 24) %ld\n", encryption_size, encrypted_size);
        return EECode_BadParam;
    }

    if ((!p_encryption) || (!p_decryption) || (!encryption_size) || (!decryption_size)) {
        LOG_FATAL("NULL params\n");
        return EECode_BadParam;
    } else {
        TU8 *p = p_encryption;
        TU8 *p_src = p_decryption;
        TU8 v1 = 0, v2 = 0, v3 = 0;
        TU32 i1 = 0, i2 = 0, c1 = 0, c2 = 0;
        TU32 *p1 = NULL;
        TMemSize index = 0;

        //gCustomizedV1RandomSeed[0] += gCustomizedV1RandomSeed[1];
        //gCustomizedV1RandomSeed[2] += gCustomizedV1RandomSeed[3];
        p[0] = gCustomizedV1RandomSeed[0];
        p[1] = gCustomizedV1RandomSeed[2];

        p[2] = p[0] + p[1];
        p[3] = p[0] ^ p[1];

        p[4] = (decryption_size >> 8) & 0xff;
        p[5] = decryption_size & 0xff;

        v1 = p[6] = gCustomizedV1RandomSeed[0] + gCustomizedV1RandomSeed[3];
        v2 = p[7] = gCustomizedV1RandomSeed[1] + gCustomizedV1RandomSeed[2];
        v2 = (v2 & 0x1c) >> 2;

        p += 8;

        for (index = 0; index < decryption_size; index ++) {
            v3 = (p_src[index] ^ gCustomizedV1EncryptionPatten[v1]);
            p[index] = (v3 >> v2) | (v3 << (8 - v2));
            v1 = p_src[index];
        }

        for (; index < (encrypted_size - 24); index ++) {
            v3 += gCustomizedV1RandomSeed[2];
            p[index] = (v3 >> v2) | (v3 << (8 - v2));
        }

        i2 = (encrypted_size - 8 - 16) >> 2;
        p1 = (TU32 *)(p - 8);
        c2 = p1[0] + p1[1];
        //LOG_NOTICE("i2 %d, encrypted_size %ld, p1[0] %08x, %08x, %08x, %08x\n", i2, encrypted_size, p1[0], p1[1], p1[2], p1[3]);
        p1 += 2;
        for (i1 = 0; i1 < i2; i1 ++, p1 ++) {
            c1 = p1[0] ^ c1;
            c2 += p1[0];
        }
        c2 = c1 + c2;
        p1[0] = c1;
        p1[1] = c2;
        p1[2] = (c1 ^ c2) + 0xacbd8afe;
        p1[3] = c1 + c2 + 0x1324febd;
    }

    //LOG_NOTICE("encryption_size %ld, encrypted_size %ld\n", encryption_size, encrypted_size);
    encryption_size = encrypted_size;
    //LOG_DEBUG("encrypted_size %ld, encryption_size %ld\n", encrypted_size, encryption_size);

    return EECode_OK;
}

EECode CCustomizedV1Encryptor::Decryption(TU8 *p_encryption, TMemSize encryption_size, TU8 *p_decryption, TMemSize &decryption_size)
{
    if (DUnlikely(encryption_size & (15))) {
        LOG_ERROR("encryption_size %08x not 16 aligned\n", (TU32)encryption_size);
    }

    if (DUnlikely(encryption_size > (decryption_size + 24))) {
        LOG_FATAL("output size(%ld) should greater than (encryption_size - 24) %ld\n", encryption_size, decryption_size);
        return EECode_BadParam;
    }

    if ((!p_encryption) || (!p_decryption) || (!encryption_size) || (!decryption_size)) {
        LOG_FATAL("NULL params\n");
        return EECode_BadParam;
    } else {
        TU8 *p = p_encryption;
        TU8 *p_dst = p_decryption;
        TU8 v1 = 0, v2 = 0, v3 = 0;
        TU32 i1 = 0, i2 = 0, c1 = 0, c2 = 0;
        TU32 *p1 = NULL;
        TMemSize index = 0;

        v1 = p[0] + p[1];
        v2 = p[0] ^ p[1];
        if ((p[2] != v1) || (p[3] != v2)) {
            LOG_ERROR("check fail, p[0] %02x p[1] %02x, p[2] %02x,p[3] %02x, v1 %02x v2 %02x\n", p[0], p[1], p[2], p[3], v1, v2);
            return EECode_BadParam;
        }

        TMemSize l1 = (p[4] << 8) | p[5];
        v1 = p[6];
        v2 = p[7];
        v2 = (v2 & 0x1c) >> 2;

        p += 8;

        for (index = 0; index < l1; index ++) {
            v3 = (p[index] << v2) | (p[index] >> (8 - v2));
            p_dst[index] = v3 ^ gCustomizedV1EncryptionPatten[v1];
            v1 = p_dst[index];
        }

        i2 = (encryption_size - 8 - 16) >> 2;
        p1 = (TU32 *)(p - 8);
        c2 = p1[0] + p1[1];
        //LOG_NOTICE("encryption_size %ld, i2 %d, p1[0] %08x, %08x, %08x, %08x\n", encryption_size, i2, p1[0], p1[1], p1[2], p1[3]);
        p1 += 2;
        for (i1 = 0; i1 < i2; i1 ++, p1 ++) {
            c1 = p1[0] ^ c1;
            c2 = p1[0] + c2;
        }
        c2 = c1 + c2;
        if (p1[0] != c1) {
            LOG_ERROR("check fail, p1[0] %08x, %08x\n", p1[0], c1);
            return EECode_BadParam;
        }

        if (p1[1] != c2) {
            LOG_ERROR("check fail, p1[1] %08x, %08x\n", p1[1], c2);
            return EECode_BadParam;
        }

        if (p1[2] != ((c1 ^ c2) + 0xacbd8afe)) {
            LOG_ERROR("check fail, p1[2] %08x, %08x\n", p1[2], ((c1 ^ c2) + 0xacbd8afe));
            return EECode_BadParam;
        }

        if (p1[3] != (c1 + c2 + 0x1324febd)) {
            LOG_ERROR("check fail, p1[3] %08x, %08x\n", p1[3], ((c1 ^ c2) + 0xacbd8afe));
            return EECode_BadParam;
        }

        decryption_size = l1;

    }

    return EECode_OK;
}

void CCustomizedV1Encryptor::Destroy()
{
    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

