/**
 * common_hash_table.cpp
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_hash_table.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

CSimpleHashTable::CSimpleHashTable(EHashTableKeyType keytype, TU32 row_num)
    : mKeyType(keytype)
    , mpStaticEntry(NULL)
    , mpMutex(NULL)
    , mnEntryNum(0)
    , mnEntryRowNum(row_num)
{
}

EECode CSimpleHashTable::Construct()
{
    mpStaticEntry = new SHashTableEntry*[mnEntryRowNum];
    DASSERT(mpStaticEntry);
    memset((void *)mpStaticEntry, 0x0, mnEntryRowNum * sizeof(SHashTableEntry *));

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("gfCreateMutex fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CSimpleHashTable::~CSimpleHashTable()
{
    //delete all entry
    SHashTableEntry *p_entry = NULL;
    SHashTableEntry *p_next = NULL;
    if (mpStaticEntry) {
        for (TU32 i = 0; i < mnEntryRowNum; i++) {
            p_entry = mpStaticEntry[i];
            while (p_entry) {
                p_next = p_entry->p_next;
                deleteEntry(p_entry);
                p_entry = p_next;
                mnEntryNum--;
            }
        }
        DASSERT(mnEntryNum == 0);
        delete[] mpStaticEntry;
        mpStaticEntry = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
}

CSimpleHashTable *CSimpleHashTable::Create(EHashTableKeyType keytype, TU32 row_num)
{
    CSimpleHashTable *result = new CSimpleHashTable(keytype, row_num);
    if (result && EECode_OK != result->Construct()) {
        delete result;
        result = NULL;
    }

    return result;
}

EECode CSimpleHashTable::Add(const TChar *key0, const TChar *key1, TU32 value, TU8 update)
{
    if (DUnlikely(!key0)) {
        LOG_FATAL("key0 is NULL\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    SHashTableEntry *p_entry = lookupEntry(key0, key1, index);
    if (p_entry) {
        LOG_WARN("key: %s is added already, update %u\n", key0, update);
        if (update) {
            p_entry->value = value;
            return EECode_OK;
        }
        return EECode_Error;
    }
    p_entry = insertNewEntry(key0, key1, index);
    p_entry->value = value;

    return EECode_OK;
}

EECode CSimpleHashTable::Remove(const TChar *key0, const TChar *key1)
{
    if (DUnlikely(!key0)) {
        LOG_FATAL("key0 is NULL\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    SHashTableEntry *p_entry = lookupEntry(key0, key1, index);
    if (!p_entry) {
        LOG_WARN("No this key\n");
        return EECode_NotExist;
    }

    SHashTableEntry *p_next = p_entry->p_next;
    SHashTableEntry *p_start = mpStaticEntry[index];
    while (p_start) {
        if (p_start->p_next == p_entry) {
            p_start->p_next = p_next;
            break;
        }
        p_start = p_start->p_next;
    }

    deleteEntry(p_entry);
    return EECode_OK;
}

EECode CSimpleHashTable::Query(const TChar *key0, const TChar *key1, TU32 &value)
{
    if (DUnlikely(!key0)) {
        LOG_FATAL("key0 is NULL\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    SHashTableEntry *p_entry = lookupEntry(key0, key1, index);
    if (!p_entry) {
        LOG_FATAL("No this key\n");
        return EECode_NotExist;
    }

    value = p_entry->value;
    return EECode_OK;
}

EECode CSimpleHashTable::UpdateValue(const TChar *key0, const TChar *key1, TU32 value)
{
    if (DUnlikely(!key0)) {
        LOG_FATAL("key0 is NULL\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    SHashTableEntry *p_entry = lookupEntry(key0, key1, index);
    if (!p_entry) {
        LOG_FATAL("No this key\n");
        return EECode_NotExist;
    }

    p_entry->value = value;
    return EECode_OK;
}

void CSimpleHashTable::Delete()
{
    //delete all entry
    SHashTableEntry *p_entry = NULL;
    SHashTableEntry *p_next = NULL;
    if (mpStaticEntry) {
        for (TU32 i = 0; i < mnEntryRowNum; i++) {
            p_entry = mpStaticEntry[i];
            while (p_entry) {
                p_next = p_entry->p_next;
                deleteEntry(p_entry);
                p_entry = p_next;
                mnEntryNum--;
            }
        }
        DASSERT(mnEntryNum == 0);
        delete[] mpStaticEntry;
        mpStaticEntry = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    delete this;
}

CSimpleHashTable::SHashTableEntry *CSimpleHashTable::lookupEntry(const TChar *key0, const TChar *key1, TU32 &index)
{
    index = hashIndexFromKey(key0);

    SHashTableEntry *p_entry = mpStaticEntry[index];
    while (p_entry) {
        //TODO, check by type
        if ((strlen(p_entry->p_key0) == strlen(key0)) && (strlen(key1) == strlen(p_entry->p_key1))) {
            if ((strcmp(p_entry->p_key0, key0) == 0) && (strcmp(p_entry->p_key1, key1) == 0)) {
                break;
            }
        }
        p_entry = p_entry->p_next;
    }

    return p_entry;
}

CSimpleHashTable::SHashTableEntry *CSimpleHashTable::insertNewEntry(const TChar *key0, const TChar *key1, TU32 index)
{
    SHashTableEntry *p_new = new SHashTableEntry();
    p_new->p_next = NULL;

    //TODO
    if (mKeyType == EHashTableKeyType_String) {
        p_new->p_key0 = (TChar *) DDBG_MALLOC(strlen(key0) + 1, "C0HA");
        if (DUnlikely(p_new->p_key0 == NULL)) {
            LOG_FATAL("insertNewEntry alloc new entry fail\n");
            return NULL;
        }
        snprintf(p_new->p_key0, strlen(key0) + 1, "%s", key0);

        p_new->p_key1 = (TChar *) DDBG_MALLOC(strlen(key1) + 1, "C0Ha");
        if (DUnlikely(p_new->p_key1 == NULL)) {
            LOG_FATAL("insertNewEntry alloc new entry fail\n");
            return NULL;
        }
        snprintf(p_new->p_key1, strlen(key1) + 1, "%s", key1);
    }

    p_new->p_next = mpStaticEntry[index];
    mpStaticEntry[index] = p_new;
    mnEntryNum++;

    return p_new;
}

TU32 CSimpleHashTable::hashIndexFromKey(const TChar *key)
{
    TU32 ret = 0;
    //TODO
    if (mKeyType == EHashTableKeyType_String) {
        while (1) {
            TChar c = *key++;
            if (c == '\0') { break; }
            ret += (ret << 3) + (TU32)c;
        }
        ret %= mnEntryRowNum;
    }
    return ret;
}

EECode CSimpleHashTable::deleteEntry(SHashTableEntry *&p_entry)
{
    if (DUnlikely(p_entry)) {
        LOG_FATAL("deleteEntry: target entry is NULL\n");
        return EECode_BadParam;
    }

    DASSERT(p_entry->p_key0);
    DDBG_FREE(p_entry->p_key0, "C0HA");
    DDBG_FREE(p_entry->p_key1, "C0Ha");

    delete p_entry;
    p_entry = NULL;
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END