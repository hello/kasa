/**
 * file_io.h
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

#ifndef __FILE_IO_H__
#define __FILE_IO_H__

class CFileIO: public IIO
{
public:
    static IIO *Create();
    virtual void Delete();

protected:
    CFileIO();
    ~CFileIO();
    EECode Construct();

public:
    virtual EECode Open(TChar *name, TU32 flags);
    virtual EECode Close();

    virtual EECode SetProperty(TIOSize write_block_size, TIOSize read_block_size);
    virtual EECode GetProperty(TIOSize &write_block_size, TIOSize &read_block_size) const;

    virtual EECode Write(TU8 *pdata, TU32 size, TIOSize count, TU8 sync = 0);
    virtual EECode Read(TU8 *pdata, TU32 size, TIOSize &count);

    virtual EECode Seek(TIOSize offset, EIOPosition posision);
    virtual void Sync();

    virtual EECode Query(TIOSize &total_size, TIOSize &current_posotion) const;

private:
    FILE *mpFile;
    TChar *mpFileName;
    TUint mnFileNameSize;
    TU32 mInitFlag;
    TFileSize mTotalFileSize;

    TFileSize mReadTotalFileSize;
    TFileSize mWirteCurrentFileSize;
};

#endif

