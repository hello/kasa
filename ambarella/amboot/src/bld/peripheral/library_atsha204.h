/*
 * library_atsha204.h
 *
 * History:
 *	2015/06/23 - [Zhi He] create file for ATSHA204
 *
 * Copyright (C) 2015 -2025, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

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

void* libatsha204_open();
void libatsha204_release(void* context);

#endif

