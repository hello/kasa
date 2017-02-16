/*
 * cryptochip_library_if.h
 *
 * History:
 *	2015/09/21 - [Zhi He] create file for cryptochip library
 *
 * Copyright (C) 2015 -2025, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __CRYPTOCHIP_LIBRARY_IF_H__
#define __CRYPTOCHIP_LIBRARY_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*TFCCCreateHandle)();
typedef void (*TFCCDestroyHandle)(void *handle);

typedef int (*TFCCQueryChipStatus)(void *handle, unsigned int *is_config_zone_locked, unsigned int *is_otp_data_zone_locked);
typedef int (*TFCCQueryConfigZone)(void *handle, unsigned char *p_otp_data, unsigned int *data_size);
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

