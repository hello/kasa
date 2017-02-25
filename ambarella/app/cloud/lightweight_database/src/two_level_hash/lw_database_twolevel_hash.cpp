/*******************************************************************************
 * lw_database_twolevel_hash.cpp
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_io.h"

#include "lw_database_if.h"
#include "lw_database_twolevel_hash.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DLW_DB_READ_ITEM_COUNT 1024

ILightWeightDataBase *gfCreateLightWeightTwoLevelHashDatabase(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    return CLWDataBaseTwoLevelHash::Create(pName, pPersistCommonConfig, pMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CLWDataBaseTwoLevelHash
//
//-----------------------------------------------------------------------

ILightWeightDataBase *CLWDataBaseTwoLevelHash::Create(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    CLWDataBaseTwoLevelHash *result = new CLWDataBaseTwoLevelHash(pName, pPersistCommonConfig, pMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CLWDataBaseTwoLevelHash::CLWDataBaseTwoLevelHash(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pName, index)
    , mpPersistCommonConfig(pPersistCommonConfig)
    , mpMsgSink(pMsgSink)
    , mpExtCallBackContext(NULL)
    , mfReadExtCallBack(NULL)
    , mfWriteExtCallBack(NULL)
    , mnLevel1NodeNumber(0)
    , mnLevel2NodeNumber(0)
    , mpLevel1NodeList(NULL)
    , mTotalItemNumber(0)
    , mItemLength(0)
{
    if (pPersistCommonConfig) {
        DASSERT(pPersistCommonConfig->database_config.host_tag_higher_bit < 64);
        DASSERT(pPersistCommonConfig->database_config.host_tag_lower_bit < pPersistCommonConfig->database_config.host_tag_higher_bit);

        TUniqueID initial_tag = 0x0fffffffffffffffLL;

        mHostMask = (TU8)(((initial_tag >> pPersistCommonConfig->database_config.host_tag_lower_bit) << (pPersistCommonConfig->database_config.host_tag_lower_bit + (64 - pPersistCommonConfig->database_config.host_tag_higher_bit))) >> (64 - pPersistCommonConfig->database_config.host_tag_higher_bit));
        mHostShift = pPersistCommonConfig->database_config.host_tag_lower_bit;

        mLevel1Mask = ((initial_tag >> pPersistCommonConfig->database_config.level1_lower_bit) << (pPersistCommonConfig->database_config.level1_lower_bit + (64 - pPersistCommonConfig->database_config.level1_higher_bit))) >> (64 - pPersistCommonConfig->database_config.level1_higher_bit);
        mLevel1Shift = pPersistCommonConfig->database_config.level1_lower_bit;

        mLevel2Mask = ((initial_tag >> pPersistCommonConfig->database_config.level2_lower_bit) << (pPersistCommonConfig->database_config.level2_lower_bit + (64 - pPersistCommonConfig->database_config.level2_higher_bit))) >> (64 - pPersistCommonConfig->database_config.level2_higher_bit);
        mLevel2Shift = pPersistCommonConfig->database_config.level2_lower_bit;

        mnLevel1NodeNumber =  0x1 << (pPersistCommonConfig->database_config.level1_higher_bit - pPersistCommonConfig->database_config.level1_lower_bit);
        mnLevel2NodeNumber =  0x1 << (pPersistCommonConfig->database_config.level2_higher_bit - pPersistCommonConfig->database_config.level2_lower_bit);
    } else {
        //LOGM_FATAL("NULL pPersistCommonConfig\n");
    }
}

EECode CLWDataBaseTwoLevelHash::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleLightWeightDataBase);

    DASSERT(mnLevel2NodeNumber);
    if (DLikely(mnLevel1NodeNumber)) {
        mpLevel1NodeList = (SLevel1Node *) malloc(sizeof(SLevel1Node) * mnLevel1NodeNumber);
        if (DUnlikely(!mpLevel1NodeList)) {
            LOGM_FATAL("malloc fail, mnLevel1NodeNumber %d, request size %d\n", mnLevel1NodeNumber, (TU32)(sizeof(SLevel1Node) * mnLevel1NodeNumber));
            return EECode_NoMemory;
        }
    } else {
        LOGM_ERROR("not configed mnLevel1NodeNumber %d\n", mnLevel1NodeNumber);
        return EECode_Error;
    }

    memset(mpLevel1NodeList, 0x0, sizeof(SLevel1Node) * mnLevel1NodeNumber);

    return EECode_OK;
}

CLWDataBaseTwoLevelHash::~CLWDataBaseTwoLevelHash()
{
    LOGM_RESOURCE("CLWDataBaseTwoLevelHash::~CLWDataBaseTwoLevelHash(), end.\n");
}

void CLWDataBaseTwoLevelHash::Delete()
{
    LOGM_RESOURCE("CLWDataBaseTwoLevelHash::Delete()\n");


    LOGM_RESOURCE("CLWDataBaseTwoLevelHash::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

void CLWDataBaseTwoLevelHash::Destroy()
{
    Delete();
}


EECode CLWDataBaseTwoLevelHash::ConfigureHashParams(TU8 param1, TU8 param2, TU8 param3, TU8 param4, TU8 param5, TU8 param6)
{
    TUniqueID initial_tag = 0x0fffffffffffffffLL;

    mHostMask = ((initial_tag >> param2) << (param1 + param2)) >> param1;
    mHostShift = param2;

    mLevel1Mask = ((initial_tag >> param4) << (param3 + param4)) >> param3;
    mLevel1Shift = param4;

    mLevel2Mask = ((initial_tag >> param6) << (param5 + param6)) >> param5;
    mLevel2Shift = param6;

    mnLevel1NodeNumber =  0x1 << (param3 - param4);
    mnLevel2NodeNumber =  0x1 << (param5 - param6);

    return EECode_OK;
}

EECode CLWDataBaseTwoLevelHash::ConfigureItemParams(void *owner, TLightWeightDataBaseReadExtCallBack read_callback, TLightWeightDataBaseWriteExtCallBack write_callback, TLightWeightDataBaseInitializeCallBack initialize_callback, TDataBaseItemSize item_size)
{
    mItemLength = item_size;

    if (DLikely(owner)) {
        mpExtCallBackContext = owner;
    }

    if (DLikely(read_callback)) {
        mfReadExtCallBack = read_callback;
    }

    if (DLikely(write_callback)) {
        mfWriteExtCallBack = write_callback;
    }

    if (DLikely(initialize_callback)) {
        mfInitializeCallBack = initialize_callback;
    }

    //LOGM_NOTICE("ConfigureItemParams %d\n", mItemLength);
    return EECode_OK;
}

EECode CLWDataBaseTwoLevelHash::LoadDataBase(TChar *filename, TChar *ext_filename)
{
    TU32 read_ext = 0;
    IIO *pio = gfCreateIO(EIOType_File);
    IIO *pio_ext = NULL;
    EECode err = EECode_OK;
    SMemVariableHeader *p_mem_header = NULL;

    TS64 tobe_added_item_number = 0;
    TS64 remain_number = 0;
    TU32 item_length = 0;

    if (!ext_filename || !mfReadExtCallBack || !mpExtCallBackContext) {
        read_ext = 0;
    } else {
        pio_ext = gfCreateIO(EIOType_File);
        if (DUnlikely(!pio_ext)) {
            LOGM_ERROR("gfCreateIO() fail\n");
            return EECode_Error;
        }
        read_ext = 1;
    }

    if (DLikely(pio)) {
        err = pio->Open(filename, EIOFlagBit_Read | EIOFlagBit_Binary);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("open file(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
            return err;
        }

        if (read_ext) {
            err = pio_ext->Open(ext_filename, EIOFlagBit_Read | EIOFlagBit_Binary);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("open file(%s) fail, ret %d %s\n", ext_filename, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        SStorageHeader header;
        TIOSize count = 1;
        TU8 *p_read_buf = NULL;
        TU8 *p_cur = NULL;

        err = pio->Read((TU8 *) &header, sizeof(SStorageHeader), count);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("read file header(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
            return err;
        }

        DASSERT(header.flag1 & DDBFlag1LittleEndian);
        DASSERT(header.flag1 & DDBFlag1UseGlobalItemSize);

        item_length = (header.global_item_size_high << 8) | header.global_item_size_low;
        tobe_added_item_number = ((TU64)header.item_total_count_1) | ((TU64)header.item_total_count_2 << 8) | ((TU64)header.item_total_count_3 << 16) | ((TU64)header.item_total_count_4 << 24) | ((TU64)header.item_total_count_5 << 32) | ((TU64)header.item_total_count_6 << 40);
        //LOGM_NOTICE("read item number %llu, item_length %d\n", tobe_added_item_number, item_length);

        p_read_buf = (TU8 *) malloc(DLW_DB_READ_ITEM_COUNT * item_length);
        if (DUnlikely(NULL == p_read_buf)) {
            LOGM_ERROR("malloc(%d) fail\n", (TU32)(DLW_DB_READ_ITEM_COUNT * item_length));
            return EECode_NoMemory;
        }

        remain_number = tobe_added_item_number;
        while (remain_number >= DLW_DB_READ_ITEM_COUNT) {
            count = DLW_DB_READ_ITEM_COUNT;
            err = pio->Read(p_read_buf, item_length, count);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("read file data(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
                free(p_read_buf);
                return err;
            }

            if (DUnlikely(!count)) {
                break;
            }

            remain_number -= count;

            p_cur = p_read_buf;
            while (count) {
                if (DLikely(read_ext)) {
                    p_mem_header = (SMemVariableHeader *) p_cur;
                    mfReadExtCallBack(mpExtCallBackContext, pio_ext, p_mem_header->total_memory_length, (void *)p_cur);
                }
                if (DLikely(mfInitializeCallBack)) {
                    mfInitializeCallBack(mpExtCallBackContext, (void *)p_cur);
                }
                insertItem(p_cur, item_length);
                p_cur += item_length;
                count --;
            }

        }

        if (remain_number) {
            count = remain_number;
            err = pio->Read(p_read_buf, item_length, count);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("read file data(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
                free(p_read_buf);
                return err;
            }

            p_cur = p_read_buf;
            while (count) {
                if (read_ext) {
                    p_mem_header = (SMemVariableHeader *) p_cur;
                    mfReadExtCallBack(mpExtCallBackContext, pio_ext, p_mem_header->total_memory_length, (void *)p_cur);
                }
                if (DLikely(mfInitializeCallBack)) {
                    mfInitializeCallBack(mpExtCallBackContext, (void *)p_cur);
                }
                insertItem(p_cur, item_length);
                p_cur += item_length;
                count --;
            }
        }

        free(p_read_buf);

        mTotalItemNumber += tobe_added_item_number;
        if (mItemLength != item_length) {
            LOGM_ERROR("something wrong with DataBase, please check. mItemLength=%hu, item_length=%u\n", mItemLength, item_length);
            return EECode_DataCorruption;
        }

    } else {
        LOGM_ERROR("gfCreateIO() fail\n");
        return EECode_Error;
    }

    if (pio_ext) {
        pio_ext->Sync();
        pio_ext->Close();
        pio_ext->Delete();
    }

    pio->Sync();
    pio->Close();
    pio->Delete();

    return EECode_OK;
}

EECode CLWDataBaseTwoLevelHash::SaveDataBase(TChar *filename, TChar *ext_filename)
{
    TU32 write_ext = 0;
    TU32 level1_tag = 0, level2_tag = 0;
    SLevel1Node *node1 = NULL;
    CIDoubleLinkedList *p_list = NULL;
    CIDoubleLinkedList::SNode *p_node = NULL;

    EECode err = EECode_OK;
    IIO *pio = gfCreateIO(EIOType_File);
    IIO *pio_ext = NULL;

    if (!ext_filename || !mfWriteExtCallBack || !mpExtCallBackContext) {
        write_ext = 0;
    } else {
        pio_ext = gfCreateIO(EIOType_File);
        if (DUnlikely(!pio_ext)) {
            LOGM_ERROR("gfCreateIO() fail\n");
            return EECode_Error;
        }
        write_ext = 1;
    }

    if (DLikely(pio)) {
        err = pio->Open(filename, EIOFlagBit_Write | EIOFlagBit_Binary);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("open file(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
            return err;
        }

        if (write_ext) {
            err = pio_ext->Open(ext_filename, EIOFlagBit_Write | EIOFlagBit_Binary);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("open file(%s) fail, ret %d %s\n", ext_filename, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        SStorageHeader header;
        memset(&header, 0x0, sizeof(SStorageHeader));
        header.flag1 = DDBFlag1LittleEndian | DDBFlag1UseGlobalItemSize;
        header.global_item_size_high = (mItemLength & 0xff00) >> 8;
        header.global_item_size_low = mItemLength & 0xff;

        LOGM_NOTICE("mItemLength %d, header.global_item_size_high %d, header.global_item_size_low %d\n", mItemLength, header.global_item_size_high, header.global_item_size_low);

        header.item_total_count_1 = (mTotalItemNumber) & 0xff;
        header.item_total_count_2 = (mTotalItemNumber >> 8) & 0xff;
        header.item_total_count_3 = (mTotalItemNumber >> 16) & 0xff;
        header.item_total_count_4 = (mTotalItemNumber >> 24) & 0xff;
        header.item_total_count_5 = (mTotalItemNumber >> 32) & 0xff;
        header.item_total_count_6 = (mTotalItemNumber >> 40) & 0xff;

        err = pio->Write((TU8 *) &header, sizeof(SStorageHeader), 1, 0);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("write file header(%s) fail, ret %d %s\n", filename, err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("gfCreateIO fail\n");
        return EECode_NoMemory;
    }

    for (level1_tag = 0; level1_tag < mnLevel1NodeNumber; level1_tag ++) {

        node1 = mpLevel1NodeList + level1_tag;
        if (!node1->p_node2) {
            continue;
        }

        for (level2_tag = 0; level2_tag < mnLevel2NodeNumber; level2_tag ++) {
            p_list = node1->p_node2[level2_tag].child_list;
            if (!p_list) {
                continue;
            } else {

                p_node = p_list->FirstNode();
                CIDoubleLinkedList *p_list1 = NULL;

                while (p_node) {
                    p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
                    if (DUnlikely(!p_list1->NumberOfNodes())) {
                        p_node = p_list->NextNode(p_node);
                        continue;
                    }
                    CIDoubleLinkedList::SNode *p_node1 = p_list1->FirstNode();

                    while (p_node1) {
                        if (write_ext) {
                            mfWriteExtCallBack(mpExtCallBackContext, pio_ext, p_node1->p_context);
                        }
                        err = pio->Write((TU8 *)p_node1->p_context, (TUint)mItemLength, 1, 0);
                        p_node1 = p_list1->NextNode(p_node1);
                    }

                    p_node = p_list->NextNode(p_node);
                }

            }
        }
    }

    if (pio_ext) {
        pio_ext->Sync();
        pio_ext->Close();
        pio_ext->Delete();
    }

    pio->Sync();
    pio->Close();
    pio->Delete();

    return EECode_OK;
}

EECode CLWDataBaseTwoLevelHash::AddItem(void *item, TDataBaseItemSize item_mem_size, TU32 re_malloc)
{
    if (DUnlikely(!item)) {
        LOGM_FATAL("NULL item\n");
        return EECode_BadParam;
    }

    if (DUnlikely(item_mem_size != mItemLength)) {
        LOGM_ERROR("item_mem_size %d, not as expected %d\n", item_mem_size, mItemLength);
        return EECode_BadParam;
    }

    return insertItem(item, item_mem_size, re_malloc);
}

EECode CLWDataBaseTwoLevelHash::RemoveItem(TUniqueID item_id)
{
    return deleteItem(item_id);
}

EECode CLWDataBaseTwoLevelHash::QueryItem(TUniqueID id, void *&item, TDataBaseItemSize &item_size)
{
    TUniqueID level1_tag = (id & mLevel1Mask) >> mLevel1Shift;
    TUniqueID level2_tag = (id & mLevel2Mask) >> mLevel2Shift;
    SMemVariableHeader *p_mem_header = NULL;

    //debug assert
    DASSERT(level1_tag < mnLevel1NodeNumber);
    DASSERT(level2_tag < mnLevel2NodeNumber);
    DASSERT(mpLevel1NodeList);

    item = NULL;
    item_size = 0;

    SLevel1Node *node1 = mpLevel1NodeList + level1_tag;
    if (DUnlikely(!node1->p_node2)) {
        LOGM_WARN("not valid level1 tag\n");
        return EECode_NotExist;
    }

    CIDoubleLinkedList *p_list = node1->p_node2[level2_tag].child_list;
    if (!p_list) {
        LOGM_WARN("not valid level2 tag\n");
        return EECode_NotExist;
    } else {
        CIDoubleLinkedList::SNode *p_node = p_list->FirstNode();
        CIDoubleLinkedList::SNode *p_node1 = NULL;
        CIDoubleLinkedList *p_list1 = NULL;
        TUniqueID cur_main_id = 0;
        TUniqueID main_id = id & DMainIDMask;

        while (p_node) {
            p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
            DASSERT(p_list1);
            if (DUnlikely(!p_list1->NumberOfNodes())) {
                p_node = p_list->NextNode(p_node);
                continue;
            }

            p_node1 = p_list1->FirstNode();
            p_mem_header = (SMemVariableHeader *) p_node1->p_context;
            cur_main_id = (p_mem_header->id) & DMainIDMask;
            if (DUnlikely(cur_main_id == main_id)) {
                while (p_node1) {
                    p_mem_header = (SMemVariableHeader *) p_node1->p_context;
                    if (p_mem_header->id == id) {
                        item = p_node1->p_context;
                        item_size = mItemLength;
                        return EECode_OK;
                    }
                    p_node1 = p_list1->NextNode(p_node1);
                }
                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            }
#if 0
            if (DUnlikely(p_mem_header->id > id)) {
                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            }
#endif
            p_node = p_list->NextNode(p_node);
        }

    }

    LOGM_WARN("no such item\n");
    return EECode_NotExist;
}

EECode CLWDataBaseTwoLevelHash::QueryAvaiableIndex(TUniqueID initial_id, TUniqueID &avaiable_index)
{
    SMemVariableHeader *p_mem_header = NULL;
    TUniqueID level1_tag = (initial_id & mLevel1Mask) >> mLevel1Shift;
    TUniqueID level2_tag = (initial_id & mLevel2Mask) >> mLevel2Shift;

    //debug assert
    DASSERT(level1_tag < mnLevel1NodeNumber);
    DASSERT(level2_tag < mnLevel2NodeNumber);
    DASSERT(mpLevel1NodeList);

    SLevel1Node *node1 = mpLevel1NodeList + level1_tag;
    if (DUnlikely(!node1->p_node2)) {
        avaiable_index = 0;
        return EECode_OK;
    }

    CIDoubleLinkedList *p_list = node1->p_node2[level2_tag].child_list;
    if (!p_list) {
        avaiable_index = 0;
        return EECode_OK;
    } else {
        CIDoubleLinkedList::SNode *p_node = p_list->FirstNode();
        CIDoubleLinkedList::SNode *p_node1 = NULL;
        CIDoubleLinkedList *p_list1 = NULL;
        TUniqueID cur_main_id = 0;
        TUniqueID main_id = initial_id & DMainIDMask;

        while (p_node) {
            p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
            DASSERT(p_list1);
            if (DUnlikely(!p_list1->NumberOfNodes())) {
                p_node = p_list->NextNode(p_node);
                continue;
            }

            p_node1 = p_list1->FirstNode();
            p_mem_header = (SMemVariableHeader *) p_node1->p_context;
            cur_main_id = (p_mem_header->id) & DMainIDMask;
            if (DUnlikely(cur_main_id == main_id)) {
                avaiable_index = 0;
                while (p_node1) {
                    p_mem_header = (SMemVariableHeader *) p_node1->p_context;
                    if ((p_mem_header->id & DExtIDIndexMask) == avaiable_index) {
                        avaiable_index ++;
                    } else {
                        return EECode_OK;
                    }
                    p_node1 = p_list1->NextNode(p_node1);
                }
                return EECode_OK;
            }

            if (DUnlikely(p_mem_header->id > initial_id)) {
                avaiable_index = 0;
                return EECode_OK;
            }

            p_node = p_list->NextNode(p_node);
        }

        avaiable_index = 0;
        return EECode_OK;
    }

    LOGM_FATAL("why comes here\n");
    return EECode_InternalLogicalBug;
}

EECode CLWDataBaseTwoLevelHash::QueryHeadNodeFromMainID(void *&p_header, TUniqueID id)
{
    SMemVariableHeader *p_mem_header = NULL;
    TUniqueID level1_tag = (id & mLevel1Mask) >> mLevel1Shift;
    TUniqueID level2_tag = (id & mLevel2Mask) >> mLevel2Shift;

    //debug assert
    DASSERT(level1_tag < mnLevel1NodeNumber);
    DASSERT(level2_tag < mnLevel2NodeNumber);
    DASSERT(mpLevel1NodeList);

    p_header = NULL;

    SLevel1Node *node1 = mpLevel1NodeList + level1_tag;
    if (DUnlikely(!node1->p_node2)) {
        LOGM_INFO("not valid level1 tag\n");
        return EECode_NotExist;
    }

    CIDoubleLinkedList *p_list = node1->p_node2[level2_tag].child_list;
    if (!p_list) {
        LOGM_INFO("not valid level2 tag\n");
        return EECode_NotExist;
    } else {
        CIDoubleLinkedList::SNode *p_node = p_list->FirstNode();
        CIDoubleLinkedList::SNode *p_node1 = NULL;
        CIDoubleLinkedList *p_list1 = NULL;
        TUniqueID cur_main_id = 0;
        TUniqueID main_id = id & DMainIDMask;

        while (p_node) {
            p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
            DASSERT(p_list1);
            if (DUnlikely(!p_list1->NumberOfNodes())) {
                p_node = p_list->NextNode(p_node);
                continue;
            }
            p_node1 = p_list1->FirstNode();
            p_mem_header = (SMemVariableHeader *) p_node1->p_context;
            cur_main_id = (p_mem_header->id) & DMainIDMask;
            if (DUnlikely(cur_main_id == main_id)) {
                p_header = (void *) p_list1;
                return EECode_OK;
            }

            if (DUnlikely(cur_main_id > main_id)) {
                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            }

            p_node = p_list->NextNode(p_node);
        }

    }

    LOGM_WARN("no such item\n");
    return EECode_NotExist;
}

EECode CLWDataBaseTwoLevelHash::QueryNextNodeFromHeadNode(void *p_header, void *&p_node)
{
    if (DUnlikely(!p_header)) {
        LOGM_WARN("NULL header\n");
        return EECode_NotExist;
    }

    CIDoubleLinkedList *p_list = (CIDoubleLinkedList *) p_header;
    if (DUnlikely(p_node == NULL)) {
        p_node = (void *)p_list->FirstNode();
    } else {
        p_node = (void *)p_list->NextNode((CIDoubleLinkedList::SNode *) p_node);
    }

    if (p_node) {
        return EECode_OK;
    }

    return EECode_NotExist;
}

TU64 CLWDataBaseTwoLevelHash::GetItemNumber() const
{
    return mTotalItemNumber;
}

EECode CLWDataBaseTwoLevelHash::insertItem(void *data, TDataBaseItemSize size, TU32 re_malloc)
{
    SMemVariableHeader *p_mem_header = (SMemVariableHeader *) data;
    TUniqueID id = p_mem_header->id;

    void *mem = data;

    if (re_malloc) {
        mem = malloc(size);
        if (DLikely(mem)) {
            memcpy(mem, data, size);
        } else {
            LOGM_FATAL("malloc memory fail\n");
            return EECode_NoMemory;
        }
    }

    TUniqueID level1_tag = (id & mLevel1Mask) >> mLevel1Shift;
    TUniqueID level2_tag = (id & mLevel2Mask) >> mLevel2Shift;

    //debug assert
    DASSERT(level1_tag < mnLevel1NodeNumber);
    DASSERT(level2_tag < mnLevel2NodeNumber);
    DASSERT(mpLevel1NodeList);

    SLevel1Node *node1 = mpLevel1NodeList + level1_tag;
    if (DUnlikely(!node1->p_node2)) {
        node1->p_node2 = (SLevel2Node *) malloc(mnLevel2NodeNumber * sizeof(SLevel2Node));
        if (DUnlikely(!node1->p_node2)) {
            LOGM_FATAL("malloc memory fail\n");
            return EECode_NoMemory;
        }

        memset(node1->p_node2, 0x0, mnLevel2NodeNumber * sizeof(SLevel2Node));
    }

    CIDoubleLinkedList *p_list = node1->p_node2[level2_tag].child_list;
    if (!p_list) {
        p_list = new CIDoubleLinkedList();
        if (DUnlikely(!p_list)) {
            LOGM_FATAL("malloc memory fail\n");
            return EECode_NoMemory;
        }

        node1->p_node2[level2_tag].child_list = p_list;

        p_list = new CIDoubleLinkedList();
        if (DUnlikely(!p_list)) {
            LOGM_FATAL("malloc memory fail\n");
            return EECode_NoMemory;
        }

        p_list->InsertContent(NULL, (void *)mem, 0);
        node1->p_node2[level2_tag].child_list->InsertContent(NULL, (void *) p_list, 0);
    } else {
        CIDoubleLinkedList::SNode *p_node = p_list->FirstNode();
        TUint has_insert = 0;
        TUniqueID cur_main_id = 0;
        TUniqueID main_id = id & DMainIDMask;
        CIDoubleLinkedList *p_list1 = NULL;

        while (p_node) {
            p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
            if (DUnlikely(!p_list1->NumberOfNodes())) {
                p_node = p_list->NextNode(p_node);
                continue;
            }
            CIDoubleLinkedList::SNode *p_node1 = p_list1->FirstNode();
            p_mem_header = (SMemVariableHeader *) p_node1->p_context;
            cur_main_id = (p_mem_header->id) & DMainIDMask;
            if (DUnlikely(cur_main_id == main_id)) {
                while (p_node1) {
                    p_mem_header = (SMemVariableHeader *) p_node1->p_context;
                    if (p_mem_header->id == id) {
                        LOGM_WARN("duplicated id 0x%llx\n", p_mem_header->id);
                        return EECode_BadParam;
                    }
                    if (p_mem_header->id > id) {
                        p_list1->InsertContent(p_node1, (void *) mem, 0);
                        has_insert = 1;
                        break;
                    }
                    p_node1 = p_list1->NextNode(p_node1);
                }

                if (!has_insert) {
                    p_list1->InsertContent(NULL, (void *) mem, 0);
                    has_insert = 1;
                }
                return EECode_OK;
            }

            if (DUnlikely(p_mem_header->id > id)) {
                p_list1 = new CIDoubleLinkedList();
                if (DUnlikely(!p_list1)) {
                    LOGM_FATAL("malloc memory fail\n");
                    return EECode_NoMemory;
                }
                p_list->InsertContent(NULL, (void *) p_list1, 0);
                has_insert = 1;
                p_list1->InsertContent(NULL, (void *) mem, 0);
                return EECode_OK;
            }

            p_node = p_list->NextNode(p_node);
        }

        if (!has_insert) {
            p_list1 = new CIDoubleLinkedList();
            if (DUnlikely(!p_list1)) {
                LOGM_FATAL("malloc memory fail\n");
                return EECode_NoMemory;
            }
            p_list->InsertContent(NULL, (void *) p_list1, 0);
            p_list1->InsertContent(NULL, (void *) mem, 0);
        }
    }

    mTotalItemNumber ++;
    return EECode_OK;
}

EECode CLWDataBaseTwoLevelHash::deleteItem(TUniqueID id)
{
    SMemVariableHeader *p_mem_header = NULL;
    TUniqueID level1_tag = (id & mLevel1Mask) >> mLevel1Shift;
    TUniqueID level2_tag = (id & mLevel2Mask) >> mLevel2Shift;

    //debug assert
    DASSERT(level1_tag < mnLevel1NodeNumber);
    DASSERT(level2_tag < mnLevel2NodeNumber);
    DASSERT(mpLevel1NodeList);

    SLevel1Node *node1 = mpLevel1NodeList + level1_tag;
    if (DUnlikely(!node1->p_node2)) {
        LOGM_WARN("not valid level1 tag\n");
        return EECode_NotExist;
    }

    CIDoubleLinkedList *p_list = node1->p_node2[level2_tag].child_list;
    if (!p_list) {
        LOGM_WARN("not valid level2 tag\n");
        return EECode_NotExist;
    } else {
        CIDoubleLinkedList::SNode *p_node = p_list->FirstNode();
        CIDoubleLinkedList::SNode *p_node1 = NULL;
        CIDoubleLinkedList *p_list1 = NULL;
        TUniqueID cur_main_id = 0;
        TUniqueID main_id = id & DMainIDMask;

        while (p_node) {
            p_list1 = (CIDoubleLinkedList *)(p_node->p_context);
            if (DUnlikely(!p_list1->NumberOfNodes())) {
                p_node = p_list->NextNode(p_node);
                continue;
            }

            p_node1 = p_list1->FirstNode();
            p_mem_header = (SMemVariableHeader *) p_node1->p_context;
            cur_main_id = (p_mem_header->id) & DMainIDMask;
            if (DUnlikely(cur_main_id == main_id)) {
                while (p_node1) {
                    p_mem_header = (SMemVariableHeader *) p_node1->p_context;
                    if (p_mem_header->id == id) {
                        free(p_node1->p_context);
                        p_list1->FastRemoveContent(p_node1, p_node1->p_context);
                        return EECode_OK;
                    }
                    p_node1 = p_list1->NextNode(p_node1);
                }
                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            }

            if (DUnlikely(p_mem_header->id > id)) {
                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            }

            p_node = p_list->NextNode(p_node);
        }
    }

    LOGM_WARN("no such item\n");
    return EECode_NotExist;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

