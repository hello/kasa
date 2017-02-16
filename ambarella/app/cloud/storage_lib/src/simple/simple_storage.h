/**
 * simple_storage.h
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

#ifndef __SIMPLE_STORAGE_H__
#define __SIMPLE_STORAGE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DDataBaseMainFileDefaultName "storage.ini"

typedef struct {
    SDateTime start_time;
    TChar file_name[DMaxFileNameLength];//absolute file path

    TU16 reference_count;
    TU16 file_duration_minutes;
    TU8 write_flag;
    TU8 reserved0, reserved1, reserved2;
} SSimpleStorageFile;

typedef struct {
    TChar channel_name[DMaxChannelNameLength];

    TU8 enabled;
    TU8 max_storage_file_number;
    TU8 redudant_storage_file_number;
    TU8 single_file_duration_minutes;

    TU16 cur_dir_year;
    TU8 cur_dir_month;
    TU8 cur_dir_day;

    CIDoubleLinkedList *filelist;
} SSimpleStorageChannel;

typedef struct {
    TU32 current_channel_number;
    SSimpleStorageChannel simple_storage_channel[DSYSTEM_MAX_CHANNEL_NUM];
} SSimpleStorage;

class CSimpleStorageManagement: public IStorageManagement
{
public:
    static CSimpleStorageManagement *Create();

protected:
    CSimpleStorageManagement();

    EECode Construct();
    virtual ~CSimpleStorageManagement();

public:
    virtual void Delete();

public:
    virtual EECode LoadDataBase(TChar *root_dir, TU32 &channel_number);
    virtual EECode ClearDataBase(TChar *root_dir, TU32 channel_number);
    virtual EECode SaveDataBase(TU32 &channel_number);

public:
    virtual EECode AddChannel(TChar *channel_name, TU32 max_storage_minutes, TU32 redudant_storage_minutes, TU32 single_file_duration_minutes);
    virtual EECode RemoveChannel(TChar *channel_name);

public:
    virtual EECode ClearChannelStorage(TChar *channel_name);
    virtual EECode QueryChannelStorage(TChar *channel_name, void *&p_out, EQueryStorageType type = EQueryStorageType_dutyCycle);

public:
    virtual EECode RequestExistUint(TChar *channel_name, SDateTime *time, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes);
    virtual EECode RequestExistUintSequentially(TChar *channel_name, TChar *old_file_name, TU8 direction, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes);
    virtual EECode ReleaseExistUint(TChar *channel_name, TChar *file_name);

    virtual EECode AcquireNewUint(TChar *channel_name, TChar *&file_name);
    virtual EECode FinalizeNewUint(TChar *channel_name, TChar *file_name);

private:
    void inner_clear_channel_storage(SSimpleStorageChannel *p_channel);
    void inner_remove_channel(SSimpleStorageChannel *p_channel);
    EECode inner_acquire_new_uint(SSimpleStorageChannel *p_channel, TChar *channel_name, TChar *&file_name, CIDoubleLinkedList::SNode *p_replaced_node);
    TU8 *find_pridata_pesheader(TU8 *p_data, TInt size);
    SSimpleStorageFile *try_load_single_file(TChar *filename);
    EECode load_database_from_filesystem(TChar *path, TInt depth_limited, TInt depth);
    EECode inner_load_database_from_file(TChar *config_file, TU32 &channel_number);

private:
    IMutex *mpMutex;
    SSimpleStorage *mpSimpleStorage;

    TChar *mRootDir;
    TULong mRootDirNameLength;
    IRunTimeConfigAPI *mConfigApi;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

