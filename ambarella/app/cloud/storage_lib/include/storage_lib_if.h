/**
 * storage_lib_if.h
 *
 * History:
 *  2014/01/12 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __STORAGE_LIB_IF_H__
#define __STORAGE_LIB_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef struct {
#ifdef BIG_ENDIAN
    TU8 duty_cycle_bit : 1;
    TU8 length : 7;
#else
    TU8 length : 7;
    TU8 duty_cycle_bit : 1;
#endif
} STimeLineDutyCycleUint;

typedef struct {
    TU32 total_unit_number;

    TU8 unit_length; // how many minutes
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    STimeLineDutyCycleUint *p_units;
} STimeLineDutyCycle;

typedef struct {
    TU32 total_unit_number;

    TU8 unit_length; // how many minutes
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TU8 *p_units;
} STimeLineRawMask;

typedef enum {
    EQueryStorageType_dutyCycle,
    EQueryStorageType_rawMask,
} EQueryStorageType;

typedef enum {
    EStorageMnagementType_Invalid = 0,
    EStorageMnagementType_Simple = 1,
    EStorageMnagementType_Sqlite = 2,
} EStorageMnagementType;

class IStorageManagement
{
public:
    virtual EECode LoadDataBase(TChar *root_dir, TU32 &channel_number) = 0;//if config files exist, load data base from file; if config files NOT exist, load data base from file system
    virtual EECode ClearDataBase(TChar *root_dir, TU32 channel_number) = 0;
    virtual EECode SaveDataBase(TU32 &channel_number) = 0;

public:
    virtual EECode AddChannel(TChar *channel_name, TU32 max_storage_minutes, TU32 redudant_storage_minutes, TU32 single_file_duration_minutes) = 0;
    virtual EECode RemoveChannel(TChar *channel_name) = 0;

public:
    virtual EECode ClearChannelStorage(TChar *channel_name) = 0;
    virtual EECode QueryChannelStorage(TChar *channel_name, void *&p_out, EQueryStorageType type = EQueryStorageType_dutyCycle) = 0;

public:
    virtual EECode RequestExistUint(TChar *channel_name, SDateTime *time, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;
    virtual EECode RequestExistUintSequentially(TChar *channel_name, TChar *old_file_name, TU8 direction, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;//direction: 0-newer file, 1-older file
    virtual EECode ReleaseExistUint(TChar *channel_name, TChar *file_name) = 0;

    virtual EECode AcquireNewUint(TChar *channel_name, TChar *&file_name) = 0;
    virtual EECode FinalizeNewUint(TChar *channel_name, TChar *file_name) = 0;

public:
    virtual void Delete() = 0;

protected:
    virtual ~IStorageManagement() {}
};

IStorageManagement *gfCreateStorageManagement(EStorageMnagementType type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

