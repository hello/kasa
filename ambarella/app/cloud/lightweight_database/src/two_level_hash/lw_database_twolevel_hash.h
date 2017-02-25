/*******************************************************************************
 * lw_database_twolevel_hash.h
 *
 * History:
 *  2014/06/09 - [Zhi He] create file
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

#ifndef __LW_DATABASE_TWOLEVEL_HASH_H__
#define __LW_DATABASE_TWOLEVEL_HASH_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef struct {
    TUniqueID id;
    void *p_item;
} SDSItem;

typedef struct {
    CIDoubleLinkedList *child_list;
} SLevel2Node;

typedef struct {
    SLevel2Node *p_node2;
} SLevel1Node;

//-----------------------------------------------------------------------
//
//  CLWDataBaseTwoLevelHash
//
//-----------------------------------------------------------------------

class CLWDataBaseTwoLevelHash: public CObject, public ILightWeightDataBase
{
    typedef CObject inherited;

public:
    static ILightWeightDataBase *Create(const TChar *pName, const volatile SPersistCommonConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);

public:
    CLWDataBaseTwoLevelHash(const TChar *pName, const volatile SPersistCommonConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);

    EECode Construct();
    virtual ~CLWDataBaseTwoLevelHash();

public:
    virtual void Delete();
    virtual void Destroy();

public:
    virtual EECode ConfigureHashParams(TU8 param1, TU8 param2, TU8 param3, TU8 param4, TU8 param5, TU8 param6);
    virtual EECode ConfigureItemParams(void *owner, TLightWeightDataBaseReadExtCallBack read_callback, TLightWeightDataBaseWriteExtCallBack write_callback, TLightWeightDataBaseInitializeCallBack initialize_callback, TDataBaseItemSize item_size);

public:
    virtual EECode LoadDataBase(TChar *filename, TChar *ext_filename);
    virtual EECode SaveDataBase(TChar *filename, TChar *ext_filename);

public:
    virtual EECode AddItem(void *item, TDataBaseItemSize item_mem_size, TU32 remalloc = 1);
    virtual EECode RemoveItem(TUniqueID item_id);
    virtual EECode QueryItem(TUniqueID item_id, void *&item, TDataBaseItemSize &item_size);
    virtual EECode QueryAvaiableIndex(TUniqueID initial_id, TUniqueID &avaiable_index);
    virtual EECode QueryHeadNodeFromMainID(void *&p_header, TUniqueID id);
    virtual EECode QueryNextNodeFromHeadNode(void *p_header, void *&p_node);
    virtual TU64 GetItemNumber() const;

private:
    EECode insertItem(void *data, TDataBaseItemSize size, TU32 remalloc = 1);
    EECode deleteItem(TUniqueID id);

private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;

private:
    void *mpExtCallBackContext;
    TLightWeightDataBaseReadExtCallBack mfReadExtCallBack;
    TLightWeightDataBaseWriteExtCallBack mfWriteExtCallBack;
    TLightWeightDataBaseInitializeCallBack mfInitializeCallBack;

private:
    TUniqueID mHostMask;
    TUniqueID mLevel1Mask;
    TUniqueID mLevel2Mask;

    TU8 mHostShift;
    TU8 mLevel1Shift;
    TU8 mLevel2Shift;
    TU8 mReserved;

private:
    TU32 mnLevel1NodeNumber;
    TU32 mnLevel2NodeNumber;

    SLevel1Node *mpLevel1NodeList;

private:
    TU64 mTotalItemNumber;
    TU16 mItemLength;
    TU16 mReserved0;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

