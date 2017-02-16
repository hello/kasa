/**
 * lw_database_if.h
 *
 * History:
 *  2014/05/27 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __LW_DATABASE_IF_H__
#define __LW_DATABASE_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  ILightWeightDataBase
//
//-----------------------------------------------------------------------
typedef TU32 TDataBaseItemSize;

#define DDBFlag1LittleEndian (1 << 7)
#define DDBFlag1UseGlobalItemSize (1 << 6)

typedef struct {
    TU8 flag1;
    TU8 flag2;
    TU8 flag3;
    TU8 flag4;

    TU8 global_item_size_high;
    TU8 global_item_size_low;

    TU8 item_total_count_1;
    TU8 item_total_count_2;
    TU8 item_total_count_3;
    TU8 item_total_count_4;
    TU8 item_total_count_5;
    TU8 item_total_count_6;

    TU8 reserved[52];
} SStorageHeader;

TUniqueID gfGenerateHashMainID(TChar *name);

typedef EECode(*TLightWeightDataBaseReadExtCallBack)(void *context, IIO *p_io, TU32 datasize, void *p_node);
typedef EECode(*TLightWeightDataBaseWriteExtCallBack)(void *context, IIO *p_io, void *p_node);
typedef EECode(*TLightWeightDataBaseInitializeCallBack)(void *context, void *p_node);

class ILightWeightDataBase
{
public:
    virtual EECode ConfigureHashParams(TU8 param1, TU8 param2, TU8 param3, TU8 param4, TU8 param5, TU8 param6) = 0;
    virtual EECode ConfigureItemParams(void *owner, TLightWeightDataBaseReadExtCallBack read_callback, TLightWeightDataBaseWriteExtCallBack write_callback, TLightWeightDataBaseInitializeCallBack initial_callback, TDataBaseItemSize item_size) = 0;

public:
    virtual EECode LoadDataBase(TChar *filename, TChar *ext_filename) = 0;
    virtual EECode SaveDataBase(TChar *filename, TChar *ext_filename) = 0;

public:
    virtual EECode AddItem(void *item, TDataBaseItemSize item_mem_size, TU32 remalloc = 1) = 0;
    virtual EECode RemoveItem(TUniqueID item_id) = 0;
    virtual EECode QueryItem(TUniqueID item_id, void *&item, TDataBaseItemSize &item_size) = 0;
    virtual EECode QueryAvaiableIndex(TUniqueID initial_id, TUniqueID &avaiable_index) = 0;

public:
    virtual EECode QueryHeadNodeFromMainID(void *&p_header, TUniqueID id) = 0;
    virtual EECode QueryNextNodeFromHeadNode(void *p_header, void *&p_node) = 0;
    virtual TU64 GetItemNumber() const = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ILightWeightDataBase() {}
};

typedef enum {
    ELightWeightDataBaseType_none = 0x0,

    ELightWeightDataBaseType_twoLevelHash_v1 = 0x01,

    ELightWeightDataBaseType_simpleArray = 0x02,
} ELightWeightDataBaseType;

ILightWeightDataBase *gfCreateLightWeightDataBase(ELightWeightDataBaseType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

