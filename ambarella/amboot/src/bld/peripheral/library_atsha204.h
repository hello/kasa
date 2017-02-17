/*******************************************************************************
 * library_atsha204.h
 *
 * History:
 *  2015/06/23 - [Zhi He] create file for ATSHA204
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

#ifndef __LIBRARY_ATSHA204_H__
#define __LIBRARY_ATSHA204_H__

#define DSECURE_CUSTOMIZED_ENCRYPT_DATA_SLOT_START 0
#define DSECURE_CUSTOMIZED_ENCRYPT_DATA_SLOT_END_PLUS_1 10

#define DSECURE_UNIQUE_KEY_SLOT 10
#define DSECURE_UNIQUE_SECRET_SLOT 11

#define DSECURE_FIRMWARE_VERIFY_SECRET_SLOT 12
#define DSECURE_PUBKEY_VERIFY_SECRET_SLOT 13
#define DSECURE_SERIAL_NUMBER_VERIFY_SECRET_SLOT 14
#define DSECURE_RESERVED_SLOT2 15

#define DSECURE_SLOT_VALID_RANGE 16

int libatsha204_encrypt_write_slot(void* context, unsigned char* pdata, unsigned char slot);
int libatsha204_encrypt_read_slot(void* context, unsigned char* pdata, unsigned char slot);
int libatsha204_generate_hw_signature(void* context, unsigned char* pdata, unsigned char slot);
int libatsha204_verify_hw_signature(void* context, unsigned char* pdata, unsigned char slot);
int libatsha204_initialize_config_zone(void* context);
int libatsha204_initialize_otp_and_data_zone(void* context);

int libatsha204_is_config_zone_locked(void* context);
int libatsha204_is_otp_data_zone_locked(void* context);
void libatsha204_get_serial_number(void* context, unsigned char* p_buf);

void* libatsha204_open(unsigned char i2c_index);
void libatsha204_release(void* context);

#endif

