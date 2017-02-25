/*******************************************************************************
 * cryptochip_library_if.h
 *
 * History:
 *	2015/09/21 - [Zhi He] create file for cryptochip library
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

#ifndef __CRYPTOCHIP_LIBRARY_IF_H__
#define __CRYPTOCHIP_LIBRARY_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*TFCCCreateHandle)();
typedef void (*TFCCDestroyHandle)(void *handle);

typedef int (*TFCCQueryChipStatus)(void *handle, unsigned int *is_config_zone_locked, unsigned int *is_otp_data_zone_locked);
typedef int (*TFCCQueryConfigZone)(void *handle, unsigned char *p_config, unsigned int *data_size);
typedef int (*TFCCSetOTPZone)(void *handle, unsigned char *p_otp_data, unsigned int data_size);
typedef int (*TFCCQueryOTPZone)(void *handle, unsigned char *p_otp_data, unsigned int data_size);
typedef unsigned char * (*TFCCQuerySerialNumber)(void *handle, unsigned int *serial_number_length);

typedef int (*TFCCInitializeConfigZone)(void *handle);
typedef int (*TFCCInitializeOTPDataZone)(void *handle);

typedef int (*TFCCEncryptReadSlot)(void *handle, unsigned char *pdata, unsigned char slot);
typedef int (*TFCCEncryptWriteSlot)(void *handle, unsigned char *pdata, unsigned char slot);

typedef int (*TFCCGenerateHWSignature)(void *handle, unsigned char *pdata, unsigned char slot);
typedef int (*TFCCVerifyHWSignature)(void *handle, unsigned char *pdata, unsigned char slot);

typedef struct {
    TFCCCreateHandle f_create_handle;
    TFCCDestroyHandle f_destroy_handle;

    //query chip status, if config zone is locked, if otp and data zone is locked
    TFCCQueryChipStatus f_query_chip_status;
    TFCCQueryConfigZone f_query_config_zone;
    TFCCQuerySerialNumber f_query_serial_number;

    //set otp data zone's content
    TFCCSetOTPZone f_set_otp_zone;
    //query otp zone
    TFCCQueryOTPZone f_query_otp_zone;

    //initialize chip, if chip are initialized (locked), there's no way to undo (unlock) it
    TFCCInitializeConfigZone f_initialize_config_zone;
    TFCCInitializeOTPDataZone f_initialize_otp_data_zone;

    //generate/verify hw signature
    TFCCGenerateHWSignature f_gen_hwsign;
    TFCCVerifyHWSignature f_verify_hwsign;

    //encrypt read/write
    TFCCEncryptReadSlot f_encrypt_read;
    TFCCEncryptWriteSlot f_encrypt_write;
} sf_cryptochip_interface;

typedef enum {
    ECRYPTOCHIP_TYPE_ATMEL_ATSHA204 = 0x0,
} ECRYPTOCHIP_TYPE;

int get_cryptochip_library_interface(sf_cryptochip_interface *context, ECRYPTOCHIP_TYPE cryptochip_type);

#ifdef __cplusplus
}
#endif

#endif

