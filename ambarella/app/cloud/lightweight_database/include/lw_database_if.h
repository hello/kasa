/*******************************************************************************
 * lw_database_if.h
 *
 * History:
 *  2014/05/27 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

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

