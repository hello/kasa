/*******************************************************************************
 * security_utils_if.h
 *
 * History:
 *  2014/06/30 - [Zhi He] create file
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

#ifndef __SECURITY_UTILS_IF_H__
#define __SECURITY_UTILS_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

TU32 gfHashAlgorithmV3(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV4(TU8 *p_input, TU32 input_size);
TU64 gfHashAlgorithmV5(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV6(TU8 *p_input, TU32 input_size);
TU64 gfHashAlgorithmV7(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV8(TU8 *p_input, TU32 input_size);

EECode gfCustomizedEncryptionV0(TU8 *p_encryption, TMemSize &encryption_size, TU8 *p_decryption, TMemSize decryption_size);
EECode gfCustomizedDecryptionV0(TU8 *p_encryption, TMemSize encryption_size, TU8 *p_decryption, TMemSize &decryption_size);

EECode gfParseProductionCode(TChar *p_production_code, TU32 length, TUniqueID &id);
TChar *gfGenerateProductionCode(TU32 &production_code_length, TUniqueID id, TChar *desc, TU32 desc_len);

//standard secure hash function
void gfStandardHashMD5Oneshot(TU8 *input, TU32 size, TChar *&output);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

