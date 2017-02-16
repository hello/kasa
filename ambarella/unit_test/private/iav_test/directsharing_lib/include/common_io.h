/**
 * common_io.h
 *
 * History:
 *  2013/09/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_IO_H__
#define __COMMON_IO_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    EIOPosition_NotSpecified = 0,
    EIOPosition_Begin,
    EIOPosition_End,
    EIOPosition_Current,
} EIOPosition;

typedef enum {
    EIOType_File = 0,
    EIOType_HTTP,
} EIOType;

enum {
    EIOFlagBit_Read = (1 << 0),
    EIOFlagBit_Write = (1 << 1),
    EIOFlagBit_Append = (1 << 2),
    EIOFlagBit_Text = (1 << 3),
    EIOFlagBit_Binary = (1 << 4),
    EIOFlagBit_Exclusive = (1 << 5),
};

class IIO
{
public:
    virtual void Delete() = 0;

public:
    virtual EECode Open(TChar* name, TU32 flags) = 0;
    virtual EECode Close() = 0;

    virtual EECode SetProperty(TIOSize write_block_size, TIOSize read_block_size) = 0;
    virtual EECode GetProperty(TIOSize& write_block_size, TIOSize& read_block_size) const = 0;

    virtual EECode Write(TU8* pdata, TU32 size, TIOSize count, TU8 sync = 0) = 0;
    virtual EECode Read(TU8* pdata, TU32 size, TIOSize& count) = 0;

    virtual EECode Seek(TIOSize offset, EIOPosition posision) = 0;
    virtual void Sync() = 0;

    virtual EECode Query(TIOSize& total_size, TIOSize& current_posotion) const = 0;
};

extern IIO* gfCreateIO(EIOType type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

