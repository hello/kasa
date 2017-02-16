/**
 * common_hash_table.h
 *
 * History:
 *    2014/08/14 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_HASH_TABLE_H__
#define __COMMON_HASH_TABLE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CSimpleHashTable
{
public:
    typedef enum {
        EHashTableKeyType_String = 0x0,
        EHashTableKeyType_Uxx = 0x1
    } EHashTableKeyType;

public:
    static CSimpleHashTable *Create(EHashTableKeyType keytype, TU32 row_num);
    CSimpleHashTable(EHashTableKeyType keytype, TU32 row_num);
    EECode Construct();
    virtual ~CSimpleHashTable();

public:
    EECode Add(const TChar *key0, const TChar *key1, TU32 value, TU8 update = 0);
    EECode Remove(const TChar *key0, const TChar *key1);
    EECode Query(const TChar *key0, const TChar *key1, TU32 &value);
    EECode UpdateValue(const TChar *key0, const TChar *key1, TU32 value);

public:
    void Delete();

private:
    struct SHashTableEntry {
        SHashTableEntry *p_next;
        TChar *p_key0;
        TChar *p_key1;
        TU32 value;
    };

private:
    SHashTableEntry *lookupEntry(const TChar *key0, const TChar *key1, TU32 &index);
    SHashTableEntry *insertNewEntry(const TChar *key0, const TChar *key1, TU32 index);
    TU32 hashIndexFromKey(const TChar *key);
    EECode deleteEntry(SHashTableEntry *&p_entry);

private:
    EHashTableKeyType mKeyType;
    SHashTableEntry **mpStaticEntry;
    IMutex *mpMutex;

    TU32 mnEntryNum;
    TU32 mnEntryRowNum;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif
