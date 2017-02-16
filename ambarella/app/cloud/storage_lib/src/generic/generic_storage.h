/**
 * generic_storage.h
 *
 * History:
 *  2014/01/16 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __GENERIC_STORAGE_H__
#define __GENERIC_STORAGE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CGenericStorageManagement
{
public:
    static Create();

protected:
    EECode Construct();

    CGenericStorageManagement();
    ~CGenericStorageManagement();

public:
    virtual EECode SetRootDir(TChar *root_dir);
    virtual EECode SetChannelNumber(TU32 channel_number);

public:
    virtual EECode SetupDataBase(TChar *root_dir, TU32 &channel_number);
    virtual EECode SaveDataBaseToFile(TChar *config_file);
    virtual EECode LoadDataBaseFromFile(TChar *config_file);

public:
    virtual EECode ClearChannelStorage(TU32 channel_index);
    virtual EECode ClearChannelStorage(TChar *channel_name);

    virtual EECode QueryChannelStorage(TU32 channel_index);
    virtual EECode QueryChannelStorage(TChar *channel_name);

public:
    virtual EECode RequestExistUint();
    virtual EECode ReleaseExistUint();

    virtual EECode AcquireNewUint();
    virtual EECode FinalizeNewUint();
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

