/*******************************************************************************
 * digital_signature_if.h
 *
 * History:
 *	2015/04/03 - [Zhi He] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __DIGITAL_SIGNATURE_IF_H__
#define __DIGITAL_SIGNATURE_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
  SU_ECODE_OK = 0x00,
  SU_ECODE_INVALID_ARGUMENT = 0x01,
  SU_ECODE_INVALID_INPUT = 0x02,
  SU_ECODE_INVALID_OUTPUT = 0x03,
  SU_ECODE_INVALID_KEY = 0x04,
  SU_ECODE_SIGNATURE_VERIFY_FAIL = 0x05,

  SU_ECODE_ERROR_FROM_LIB = 0x10,
};

enum {
  KEY_FORMAT_PEM = 0x0,
};

enum {
  SHA_TYPE_SHA1 = 0x1,
  SHA_TYPE_SHA256 = 0x2,
  SHA_TYPE_SHA384 = 0x3,
  SHA_TYPE_SHA512 = 0x4,
};

enum {
  CYPHER_TYPE_AES128 = 0x1,
  CYPHER_TYPE_AES256 = 0x2,
  CYPHER_TYPE_AES384 = 0x3,
  CYPHER_TYPE_AES512 = 0x4,
  CYPHER_TYPE_RC4 = 0x5,
  CYPHER_TYPE_CHACHA20 = 0x6,
};

enum {
  BLOCK_CYPHER_MODE_NONE = 0x0, //for stream cypher
  BLOCK_CYPHER_MODE_ECB = 0x1,
  BLOCK_CYPHER_MODE_CBC = 0x2,
  BLOCK_CYPHER_MODE_CTR = 0x3,
  BLOCK_CYPHER_MODE_CFB = 0x4,
  BLOCK_CYPHER_MODE_OFB = 0x5,
};

int generate_rsa_key(char* output_file, int bits, int pubexp, int key_format);
int get_public_rsa_key(char* output_file, char* input_file);

int signature_file(char* file, char* digital_signature, char* keyfile, int sha_type);
int verify_signature(char* file, char* digital_signature, char* keyfile, int sha_type);

#ifdef __cplusplus
}
#endif

#endif

