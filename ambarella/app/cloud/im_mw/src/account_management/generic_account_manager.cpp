/*******************************************************************************
 * generic_account_manager.cpp
 *
 * History:
 *  2014/06/12 - [Zhi He] create file
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

#include "cloud_lib_if.h"
#include "lw_database_if.h"

#include "sacp_types.h"

#include "security_utils_if.h"
#include "im_mw_if.h"
#include "generic_account_manager.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DAddDataNodeNumberStep 16

static EECode _callbackhandledatanodmsg(void *owner, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    return EECode_NoImplement;
}

IAccountManager *gfCreateGenericAccountManager(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    return CGenericAccountManager::Create(pName, pPersistCommonConfig, pMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CGenericAccountManager
//
//-----------------------------------------------------------------------

IAccountManager *CGenericAccountManager::Create(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    CGenericAccountManager *result = new CGenericAccountManager(pName, pPersistCommonConfig, pMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CGenericAccountManager::CGenericAccountManager(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pName, index)
    , mpPersistCommonConfig(pPersistCommonConfig)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mpSourceDeviceAccount(NULL)
    , mpUserAccount(NULL)
    , mpCloudDataNodeAccount(NULL)
    , mpCloudControlNodeAccount(NULL)
    , mpDataNodes(NULL)
    , mpDataNodesID(NULL)
    , mMaxDataNodes(16)
    , mCurrentDataNodes(0)
{
    if (pPersistCommonConfig) {

    } else {
        LOGM_FATAL("NULL pPersistCommonConfig\n");
    }
}

EECode CGenericAccountManager::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAccountManager);

    mpMutex = gfCreateMutex();
    if (DUnlikely(mpMutex == NULL)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpSourceDeviceAccount = gfCreateLightWeightDataBase(ELightWeightDataBaseType_twoLevelHash_v1, mpPersistCommonConfig, mpMsgSink, mIndex);
    if (DUnlikely(!mpSourceDeviceAccount)) {
        LOGM_FATAL("gfCreateLightWeightDataBase fail\n");
        return EECode_NoMemory;
    }
    mpSourceDeviceAccount->ConfigureItemParams((void *)this, CallBackReadSourceAccount, CallBackWriteSourceAccount, CallBackInitialize, sizeof(SAccountInfoSourceDevice));

    mpUserAccount = gfCreateLightWeightDataBase(ELightWeightDataBaseType_twoLevelHash_v1, mpPersistCommonConfig, mpMsgSink, mIndex);
    if (DUnlikely(!mpUserAccount)) {
        LOGM_FATAL("gfCreateLightWeightDataBase fail\n");
        return EECode_NoMemory;
    }
    mpUserAccount->ConfigureItemParams((void *)this, CallBackReadUserAccount, CallBackWriteUserAccount, CallBackInitialize, sizeof(SAccountInfoUser));

    mpCloudDataNodeAccount = gfCreateLightWeightDataBase(ELightWeightDataBaseType_twoLevelHash_v1, mpPersistCommonConfig, mpMsgSink, mIndex);
    if (DUnlikely(!mpCloudDataNodeAccount)) {
        LOGM_FATAL("gfCreateLightWeightDataBase fail\n");
        return EECode_NoMemory;
    }
    mpCloudDataNodeAccount->ConfigureItemParams((void *)this, CallBackReadCloudDataNode, CallBackWriteCloudDataNode, CallBackInitialize, sizeof(SAccountInfoCloudDataNode));
    //mpCloudDataNodeAccount->ConfigureItemParams((void*)this, NULL, NULL, sizeof(SAccountInfoCloudDataNode));

    mpCloudControlNodeAccount = gfCreateLightWeightDataBase(ELightWeightDataBaseType_twoLevelHash_v1, mpPersistCommonConfig, mpMsgSink, mIndex);
    if (DUnlikely(!mpCloudControlNodeAccount)) {
        LOGM_FATAL("gfCreateLightWeightDataBase fail\n");
        return EECode_NoMemory;
    }
    mpCloudControlNodeAccount->ConfigureItemParams(NULL, NULL, NULL, CallBackInitialize, sizeof(SAccountInfoCloudControlNode));

    mpDataNodesID = (TUniqueID *) DDBG_MALLOC((mMaxDataNodes) * sizeof(TUniqueID), "IAMD");
    mpDataNodes = (SAccountInfoCloudDataNode **) DDBG_MALLOC((mMaxDataNodes) * sizeof(SAccountInfoCloudDataNode *), "IAMN");
    if (DUnlikely(!mpDataNodesID || !mpDataNodes)) {
        LOGM_FATAL("gfCreateLightWeightDataBase fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CGenericAccountManager::~CGenericAccountManager()
{
    LOGM_RESOURCE("CGenericAccountManager::~CGenericAccountManager(), end.\n");
}

void CGenericAccountManager::Delete()
{
    LOGM_RESOURCE("CGenericAccountManager::Delete()\n");

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpSourceDeviceAccount) {
        mpSourceDeviceAccount->Destroy();
        mpSourceDeviceAccount = NULL;
    }

    if (mpUserAccount) {
        mpUserAccount->Destroy();
        mpUserAccount = NULL;
    }

    if (mpCloudDataNodeAccount) {
        mpCloudDataNodeAccount->Destroy();
        mpCloudDataNodeAccount = NULL;
    }

    if (mpCloudControlNodeAccount) {
        mpCloudControlNodeAccount->Destroy();
        mpCloudControlNodeAccount = NULL;
    }

    LOGM_RESOURCE("CGenericAccountManager::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

void CGenericAccountManager::PrintStatus()
{

}

void CGenericAccountManager::PrintStatus0()
{
    PrintStatus();
}

#if 0
EECode CGenericAccountManager::UpdateNetworkAgent(SAccountInfoRoot *root, void *agent, void *pre_agent)
{
    AUTO_LOCK(mpMutex);
    if (!root) {
        LOGM_FATAL("NULL root\n");
        return EECode_BadParam;
    }

    if (pre_agent == root->p_agent) {
        root->p_agent = agent;
    }

    return EECode_OK;
}
#endif

EECode CGenericAccountManager::NewAccount(TChar *account, TChar *password, EAccountGroup group, TUniqueID &id, EAccountCompany company, TChar *product_code, TU32 product_code_length)
{
    EECode err = EECode_OK;
    TUniqueID index = 0;

    AUTO_LOCK(mpMutex);

    switch (group) {

        case EAccountGroup_SourceDevice:
            if (DLikely(mpSourceDeviceAccount)) {
                TU32 debug_without_data_server = 1;
                SAccountInfoSourceDevice *p_account = NULL;

                err = querySourceDeviceAccountByName(account, p_account);
                if ((EECode_OK == err) || (p_account)) {
                    LOGM_ERROR("source device %s already exist\n", account);
                    return EECode_AlreadyExist;
                }

                err = newSourceDeviceAccount(account, password, p_account);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("newSourceDeviceAccount fail\n");
                    return err;
                }
                id = gfGenerateHashMainID(account);

                err = mpSourceDeviceAccount->QueryAvaiableIndex(id, index);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryAvaiableIndex fail\n");
                    free(p_account);
                    return err;
                }

                SAccountInfoCloudDataNode *p_data_node = NULL;
                TU32 group_index = 0;
                err = getCurrentAvaiableGroupIndex(group_index, p_data_node);
                if (!debug_without_data_server) {
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("getCurrentAvaiableGroupIndex fail\n");
                        free(p_account);
                        return err;
                    }
                }
                id = id | (group << DExtIDTypeShift) | (company << DExtIDCompanyShift) | index | ((group_index << DExtIDDataNodeShift) & DExtIDDataNodeMask);

                p_account->root.header.id = id;

                TU32 dc_len = 0;
                TChar *p_dc_code = gfGenerateProductionCode(dc_len, id, NULL, 0);
                if (DUnlikely(dc_len > DMAX_PRODUCTION_CODE_LENGTH)) {
                    LOGM_ERROR("must not come here!\n");
                    free(p_account);
                    return EECode_InternalLogicalBug;
                }

                memcpy(p_account->ext.production_code, p_dc_code, dc_len);
                free(p_dc_code);
                p_dc_code = NULL;

                if (product_code && product_code_length) {
                    if (DLikely(product_code_length >= DMAX_PRODUCTION_CODE_LENGTH)) {
                        memcpy(product_code, p_account->ext.production_code, DMAX_PRODUCTION_CODE_LENGTH);
                    } else {
                        LOGM_ERROR("production buffer not enough\n");
                        free(p_account);
                        return EECode_BadParam;
                    }
                } else {
                    LOGM_ERROR("no production code?\n");
                    free(p_account);
                    return EECode_BadParam;
                }

                err = mpSourceDeviceAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoSourceDevice), 0);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("AddItem fail\n");
                    free(p_account);
                    return err;
                }

                if (!debug_without_data_server) {
                    err = dataNodeAddChannel(p_data_node, p_account);
                    DASSERT_OK(err);
                }
            } else {
                LOGM_ERROR("null mpSourceDeviceAccount\n");
                return err;
            }
            break;

        case EAccountGroup_UserAccount:
            if (DLikely(mpUserAccount)) {
                SAccountInfoUser *p_account = NULL;

                err = queryUserAccountByName(account, p_account);
                if ((EECode_OK == err) || (p_account)) {
                    LOGM_ERROR("user name %s already exist\n", account);
                    return EECode_AlreadyExist;
                }

                err = newUserAccount(account, password, p_account);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("newUserAccount fail\n");
                    free(p_account);
                    return err;
                }
                id = gfGenerateHashMainID(account);

                err = mpUserAccount->QueryAvaiableIndex(id, index);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryAvaiableIndex fail\n");
                    free(p_account);
                    return err;
                }

                id = id | (group << DExtIDTypeShift) | (company << DExtIDCompanyShift) | index;

                p_account->root.header.id = id;
                err = mpUserAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoUser), 0);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("AddItem fail\n");
                    free(p_account);
                    return err;
                }

            }
            break;

        case EAccountGroup_CloudDataNode:
            if (DLikely(mpCloudDataNodeAccount)) {
                LOGM_FATAL("should not comes here!\n");
                SAccountInfoCloudDataNode *p_account = NULL;
                err = newCloudDataNodeAccount(account, password, 0, p_account);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("newCloudDataNodeAccount fail\n");
                    free(p_account);
                    return err;
                }
                id = gfGenerateHashMainID(account);

                err = mpCloudDataNodeAccount->QueryAvaiableIndex(id, index);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryAvaiableIndex fail\n");
                    free(p_account);
                    return err;
                }

                id = id | (group << DExtIDTypeShift) | (company << DExtIDCompanyShift) | index;

                p_account->root.header.id = id;
                err = mpCloudDataNodeAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoCloudDataNode), 0);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("AddItem fail\n");
                    free(p_account);
                    return err;
                }

            }
            break;

        case EAccountGroup_CloudControlNode:
            if (DLikely(mpCloudControlNodeAccount)) {
                SAccountInfoCloudControlNode *p_account = NULL;
                err = newCloudControlNodeAccount(account, password, p_account);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("newCloudControlNodeAccount fail\n");
                    free(p_account);
                    return err;
                }
                id = gfGenerateHashMainID(account);

                err = mpCloudControlNodeAccount->QueryAvaiableIndex(id, index);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryAvaiableIndex fail\n");
                    free(p_account);
                    return err;
                }

                id = id | (group << DExtIDTypeShift) | (company << DExtIDCompanyShift) | index;

                p_account->root.header.id = id;
                err = mpCloudControlNodeAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoCloudControlNode), 0);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("AddItem fail\n");
                    free(p_account);
                    return err;
                }

            }
            break;

        default:
            LOGM_ERROR("Error group %d\n", group);
            return EECode_BadParam;
            break;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::DeleteAccount(TChar *account, EAccountGroup group)
{
    TUniqueID main_id = gfGenerateHashMainID(account);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;

    AUTO_LOCK(mpMutex);

    switch (group) {

        case EAccountGroup_SourceDevice:
            if (DLikely(mpSourceDeviceAccount)) {
                SAccountInfoSourceDevice *p_account = NULL;
                err = mpSourceDeviceAccount->QueryHeadNodeFromMainID(p_header, main_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
                    return err;
                }

                while (1) {
                    err = mpSourceDeviceAccount->QueryNextNodeFromHeadNode(p_header, p_node);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                        return err;
                    }

                    if (DUnlikely(!p_node)) {
                        break;
                    }

                    CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
                    p_account = (SAccountInfoSourceDevice *) t_node->p_context;
                    if (!strcmp(p_account->root.base.name, account)) {
                        err = mpSourceDeviceAccount->RemoveItem(p_account->root.header.id);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("RemoveItem fail\n");
                            return err;
                        }
                        return EECode_OK;
                    }
                }

                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            } else {

            }
            break;

        case EAccountGroup_UserAccount:
            if (DLikely(mpUserAccount)) {
                SAccountInfoUser *p_account = NULL;
                err = mpUserAccount->QueryHeadNodeFromMainID(p_header, main_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
                    return err;
                }

                while (1) {
                    err = mpUserAccount->QueryNextNodeFromHeadNode(p_header, p_node);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                        return err;
                    }

                    if (DUnlikely(!p_node)) {
                        break;
                    }

                    CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
                    p_account = (SAccountInfoUser *) t_node->p_context;
                    if (!strcmp(p_account->root.base.name, account)) {
                        err = mpUserAccount->RemoveItem(p_account->root.header.id);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("RemoveItem fail\n");
                            return err;
                        }
                        return EECode_OK;
                    }
                }

                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            } else {

            }
            break;

        case EAccountGroup_CloudDataNode:
            if (DLikely(mpCloudDataNodeAccount)) {
                SAccountInfoCloudDataNode *p_account = NULL;
                err = mpCloudDataNodeAccount->QueryHeadNodeFromMainID(p_header, main_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
                    return err;
                }

                while (1) {
                    err = mpCloudDataNodeAccount->QueryNextNodeFromHeadNode(p_header, p_node);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                        return err;
                    }

                    if (DUnlikely(!p_node)) {
                        break;
                    }

                    CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
                    p_account = (SAccountInfoCloudDataNode *) t_node->p_context;
                    if (!strcmp(p_account->root.base.name, account)) {
                        err = mpCloudDataNodeAccount->RemoveItem(p_account->root.header.id);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("RemoveItem fail\n");
                            return err;
                        }
                        return EECode_OK;
                    }
                }

                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            } else {

            }
            break;

        case EAccountGroup_CloudControlNode:
            if (DLikely(mpCloudControlNodeAccount)) {
                SAccountInfoCloudControlNode *p_account = NULL;
                err = mpCloudControlNodeAccount->QueryHeadNodeFromMainID(p_header, main_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
                    return err;
                }

                while (1) {
                    err = mpCloudControlNodeAccount->QueryNextNodeFromHeadNode(p_header, p_node);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                        return err;
                    }

                    if (DUnlikely(!p_node)) {
                        break;
                    }

                    CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
                    p_account = (SAccountInfoCloudControlNode *) t_node->p_context;
                    if (!strcmp(p_account->root.base.name, account)) {
                        err = mpCloudControlNodeAccount->RemoveItem(p_account->root.header.id);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("RemoveItem fail\n");
                            return err;
                        }
                        return EECode_OK;
                    }
                }

                LOGM_WARN("not exist\n");
                return EECode_NotExist;
            } else {

            }
            break;

        default:
            LOGM_ERROR("Error group %d\n", group);
            return EECode_BadParam;
            break;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::DeleteAccount(TUniqueID id)
{
    EAccountGroup group = (EAccountGroup)((id & DExtIDTypeMask) >> DExtIDTypeShift);
    EECode err = EECode_OK;

    AUTO_LOCK(mpMutex);

    switch (group) {

        case EAccountGroup_SourceDevice:
            if (DLikely(mpSourceDeviceAccount)) {
                err = mpSourceDeviceAccount->RemoveItem(id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("RemoveItem fail\n");
                    return err;
                }
            } else {

            }
            break;

        case EAccountGroup_UserAccount:
            if (DLikely(mpUserAccount)) {
                err = mpUserAccount->RemoveItem(id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("RemoveItem fail\n");
                    return err;
                }
            } else {

            }
            break;

        case EAccountGroup_CloudDataNode:
            if (DLikely(mpCloudDataNodeAccount)) {
                err = mpCloudDataNodeAccount->RemoveItem(id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("RemoveItem fail\n");
                    return err;
                }
            } else {

            }
            break;

        case EAccountGroup_CloudControlNode:
            if (DLikely(mpCloudControlNodeAccount)) {
                err = mpCloudControlNodeAccount->RemoveItem(id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("RemoveItem fail\n");
                    return err;
                }
            } else {

            }
            break;

        default:
            LOGM_ERROR("Error group %d\n", group);
            return EECode_BadParam;
            break;
    }
    return EECode_OK;
}

EECode CGenericAccountManager::QueryAccount(TUniqueID id, SAccountInfoRoot *&info)
{
    AUTO_LOCK(mpMutex);
    return queryAccount(id, info);
}

EECode CGenericAccountManager::QuerySourceDeviceAccountByName(TChar *name, SAccountInfoSourceDevice *&info)
{
    TUniqueID main_id = gfGenerateHashMainID(name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;
    info = NULL;

    AUTO_LOCK(mpMutex);

    if (DLikely(mpSourceDeviceAccount)) {
        err = mpSourceDeviceAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpSourceDeviceAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            info = (SAccountInfoSourceDevice *) t_node->p_context;
            if (!strcmp(info->root.base.name, name)) {
                return EECode_OK;
            }
        }

        LOGM_WARN("not exist\n");
        info = NULL;
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpSourceDeviceAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::QueryUserAccountByName(TChar *name, SAccountInfoUser *&info)
{
    AUTO_LOCK(mpMutex);
    return queryUserAccountByName(name, info);
}

EECode CGenericAccountManager::QueryDataNodeByName(TChar *name, SAccountInfoCloudDataNode *&info)
{
    TUniqueID main_id = gfGenerateHashMainID(name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;
    info = NULL;

    AUTO_LOCK(mpMutex);

    if (DLikely(mpCloudDataNodeAccount)) {
        err = mpCloudDataNodeAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpCloudDataNodeAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            info = (SAccountInfoCloudDataNode *) t_node->p_context;
            if (!strcmp(info->root.base.name, name)) {
                return EECode_OK;
            }
        }

        LOGM_WARN("not exist\n");
        info = NULL;
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpCloudDataNodeAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::NewDataNode(TChar *url, TChar *password, TUniqueID &id, TSocketPort admin_port, TSocketPort cloud_port, TSocketPort rtsp_streaming_port, TU32 max_channel_number, TU32 cur_used_channel_number)
{
    EECode err = EECode_OK;
    TUniqueID index = 0;

    AUTO_LOCK(mpMutex);

    if (DLikely(mpCloudDataNodeAccount)) {
        SAccountInfoCloudDataNode *p_account = NULL;

        TChar data_node_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
        TChar *ptmp = strchr(url, ':');
        if (ptmp) {
            snprintf(data_node_name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", url);
        } else {
            snprintf(data_node_name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s:%d", url, admin_port);
        }

        err = queryDataNodeAccountByName(data_node_name, p_account);
        if ((EECode_OK == err) || (p_account)) {
            LOGM_ERROR("data node url %s already exist\n", data_node_name);
            return EECode_AlreadyExist;
        }

        err = newCloudDataNodeAccount(url, password, admin_port, p_account);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("newCloudDataNodeAccount fail\n");
            free(p_account);
            return err;
        }
        id = gfGenerateHashMainID(p_account->root.base.name);

        err = mpCloudDataNodeAccount->QueryAvaiableIndex(id, index);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("QueryAvaiableIndex fail\n");
            free(p_account);
            return err;
        }

        id = id | (EAccountGroup_CloudDataNode << DExtIDTypeShift) | (EAccountCompany_Ambarella << DExtIDCompanyShift) | index;

        p_account->root.header.id = id;
        p_account->ext.node_index = mCurrentDataNodes;

        p_account->ext.cloud_port = cloud_port;
        p_account->ext.rtsp_streaming_port = rtsp_streaming_port;
        p_account->max_channel_number = max_channel_number;
        DASSERT(!cur_used_channel_number);
        p_account->current_channel_number = cur_used_channel_number;

        LOGM_NOTICE("add data node, %p, id %llx\n", p_account, id);
        err = mpCloudDataNodeAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoCloudDataNode), 0);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("AddItem fail\n");
            free(p_account);
            return err;
        }

        if (DUnlikely(mCurrentDataNodes >= mMaxDataNodes)) {
            TUniqueID *ptmp = (TUniqueID *) DDBG_MALLOC((mMaxDataNodes + DAddDataNodeNumberStep) * sizeof(TUniqueID), "IAMD");
            SAccountInfoCloudDataNode **ptmp_nodes = (SAccountInfoCloudDataNode **) DDBG_MALLOC((mMaxDataNodes + DAddDataNodeNumberStep) * sizeof(SAccountInfoCloudDataNode *), "IAMN");
            if (DUnlikely((!ptmp) || (!ptmp_nodes))) {
                LOGM_ERROR("no memory\n");
                return EECode_NoMemory;
            }
            mMaxDataNodes += DAddDataNodeNumberStep;
            memcpy(ptmp, mpDataNodesID, mCurrentDataNodes * sizeof(TUniqueID));
            DDBG_FREE(mpDataNodesID, "IAMD");
            mpDataNodesID = ptmp;
            memcpy(ptmp_nodes, mpDataNodes, mCurrentDataNodes * sizeof(SAccountInfoCloudDataNode *));
            DDBG_FREE(mpDataNodes, "IAMN");
            mpDataNodes = ptmp_nodes;
        }
        mpDataNodesID[mCurrentDataNodes] = id;
        mpDataNodes[mCurrentDataNodes] = p_account;
        LOGM_NOTICE("data node index %d, %llx, %p\n", mCurrentDataNodes, mpDataNodesID[mCurrentDataNodes], mpDataNodes[mCurrentDataNodes]);
        mCurrentDataNodes ++;

        //TODO, conncet data node after new data node
        err = connectDataNode(p_account);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_NOTICE("after new data node, connectDataNode fail\n");
        }

        //free(p_account);
    }

    return EECode_OK;
}

EECode CGenericAccountManager::DeleteDataNode(TChar *url, TSocketPort admin_port)
{
    if (DUnlikely(!url)) {
        LOGM_FATAL("NULL url\n");
        return EECode_BadParam;
    }

    TChar *ptmp = strchr(url, ':');
    TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};

    if (!ptmp) {
        snprintf(account_name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s:%d", url, admin_port);
    } else {
        snprintf(account_name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", url);
    }

    TUniqueID main_id = gfGenerateHashMainID(account_name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;

    AUTO_LOCK(mpMutex);

    if (DLikely(mpCloudDataNodeAccount)) {
        SAccountInfoCloudDataNode *p_account = NULL;
        err = mpCloudDataNodeAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpCloudDataNodeAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            p_account = (SAccountInfoCloudDataNode *) t_node->p_context;
            if (!strcmp(p_account->root.base.name, account_name)) {
                err = mpCloudDataNodeAccount->RemoveItem(p_account->root.header.id);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("RemoveItem fail\n");
                    return err;
                }
                return EECode_OK;
            }
        }

        LOGM_WARN("not exist\n");
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpCloudDataNodeAccount\n");
    }

    return EECode_Error;
}

EECode CGenericAccountManager::QueryAllDataNodeID(TUniqueID *&p_id, TU32 &data_node_number)
{
    p_id = mpDataNodesID;
    data_node_number = mCurrentDataNodes;
    return EECode_OK;
}

EECode CGenericAccountManager::QueryAllDataNodeInfo(SAccountInfoCloudDataNode ** &p_info, TU32 &data_node_number)
{
    p_info = mpDataNodes;
    data_node_number = mCurrentDataNodes;
    return EECode_OK;
}

EECode CGenericAccountManager::QueryDataNodeInfoByIndex(TU32 index, SAccountInfoCloudDataNode *&p_info)
{
    AUTO_LOCK(mpMutex);
    if (mCurrentDataNodes <= index) {
        LOG_FATAL("%d exceed max value %d\n", index, mCurrentDataNodes);
        return EECode_BadParam;
    }
    p_info = mpDataNodes[index];
    DASSERT(mpDataNodes[index]);

    return EECode_OK;
}

EECode CGenericAccountManager::NewUserAccount(TChar *account, TChar *password, TUniqueID &id, EAccountCompany company, SAccountInfoUser *&p_user)
{
    EECode err = EECode_OK;
    TUniqueID index = 0;
    AUTO_LOCK(mpMutex);

    if (DLikely(mpUserAccount)) {
        SAccountInfoUser *p_account = NULL;

        err = queryUserAccountByName(account, p_account);
        if ((EECode_OK == err) || (p_account)) {
            LOGM_ERROR("user name %s already exist\n", account);
            return EECode_AlreadyExist;
        }

        err = newUserAccount(account, password, p_account);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("newUserAccount fail\n");
            free(p_account);
            return err;
        }
        id = gfGenerateHashMainID(account);

        err = mpUserAccount->QueryAvaiableIndex(id, index);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("QueryAvaiableIndex fail\n");
            free(p_account);
            return err;
        }

        id = id | (EAccountGroup_UserAccount << DExtIDTypeShift) | (company << DExtIDCompanyShift) | index;

        p_account->root.header.id = id;
        err = mpUserAccount->AddItem((void *) p_account, (TDataBaseItemSize)sizeof(SAccountInfoUser), 0);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("AddItem fail\n");
            free(p_account);
            return err;
        }

        p_user = p_account;
    } else {
        LOGM_FATAL("NULL mpUserAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}


EECode CGenericAccountManager::ConnectDataNode(TUniqueID id)
{
    AUTO_LOCK(mpMutex);

    SAccountInfoCloudDataNode *p_datanode_info = NULL;
    EECode err = queryDataNodeAccount(id, p_datanode_info);

    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("bad id %llx\n", id);
        return EECode_BadParam;
    }

    return connectDataNode(p_datanode_info);
}

EECode CGenericAccountManager::ConnectAllDataNode()
{
    EECode err = EECode_OK;
    AUTO_LOCK(mpMutex);

    for (TU32 i = 0; i < mCurrentDataNodes; i++) {
        err = connectDataNode(mpDataNodes[i]);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("ConnectDataNode fail, id: %llx, err %d, %s\n", mpDataNodesID[i], err, gfGetErrorCodeString(err));
            return err;
        }
    }
    return EECode_OK;
}

EECode CGenericAccountManager::DisconnectAllDataNode()
{
    AUTO_LOCK(mpMutex);

    for (TU32 i = 0; i < mCurrentDataNodes; i++) {
        CICommunicationClientPort *p_clientport = (CICommunicationClientPort *)mpDataNodes[i]->root.p_agent;
        if (DUnlikely(!p_clientport)) {
            LOGM_FATAL("Data node (%u) p_agent is NULL, check me!\n", i);
            continue;
        }
        p_clientport->Disconnect();
        p_clientport->Destroy();
        mpDataNodes[i]->root.p_agent = NULL;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::SendDataToDataNode(TU32 index, TU8 *p_data, TU32 size)
{
    DASSERT(p_data);
    AUTO_LOCK(mpMutex);

    SAccountInfoCloudDataNode *p_datanode_info = NULL;
    EECode err = queryDataNodeAccount(mpDataNodesID[index], p_datanode_info);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("QueryDataNodeInfoByIndex fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    CICommunicationClientPort *p_port = (CICommunicationClientPort *)p_datanode_info->root.p_agent;
    if (DUnlikely(!p_port)) {
        LOGM_FATAL("CICommunicationClientPort point is NULL\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(p_port->GetConnectionStatus() != 1)) {
        LOGM_FATAL("Requested data node is not connected!\n");
        return EECode_InternalLogicalBug;
    }

    TInt ret = p_port->Write(p_data, size);
    if (DUnlikely(ret < 0)) {
        LOGM_FATAL("SendDataToDataNode fail\n");
        return EECode_IOError;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateUserDynamicCode(TU32 index, TChar *user_name, TChar *device_name, TU32 dynamic_code)
{
    DASSERT(user_name);
    DASSERT(device_name);
    AUTO_LOCK(mpMutex);
    SAccountInfoCloudDataNode *p_datanode_info = NULL;
    EECode err = queryDataNodeAccount(mpDataNodesID[index], p_datanode_info);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("QueryDataNodeInfoByIndex fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    CICommunicationClientPort *p_port = (CICommunicationClientPort *)p_datanode_info->root.p_agent;
    if (DUnlikely(!p_port)) {
        LOGM_FATAL("CICommunicationClientPort point is NULL\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(p_port->GetConnectionStatus() != 1)) {
        LOGM_FATAL("Requested data node is not connected!\n");
        return EECode_InternalLogicalBug;
    }

    err = p_port->UpdateUserDynamicCode(user_name, device_name, dynamic_code);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("SendUserAuthenticationInfoToDataNode fail\n");
        return err;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateDeviceDynamicCode(TU32 index, TChar *name, TU32 dynamic_code)
{
    DASSERT(name);
    AUTO_LOCK(mpMutex);
    SAccountInfoCloudDataNode *p_datanode_info = NULL;
    EECode err = queryDataNodeAccount(mpDataNodesID[index], p_datanode_info);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("QueryDataNodeInfoByIndex fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    CICommunicationClientPort *p_port = (CICommunicationClientPort *)p_datanode_info->root.p_agent;
    if (DUnlikely(!p_port)) {
        LOGM_FATAL("CICommunicationClientPort point is NULL\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(p_port->GetConnectionStatus() != 1)) {
        LOGM_FATAL("Requested data node is not connected!\n");
        return EECode_InternalLogicalBug;
    }

    err = p_port->UpdateDeviceDynamicCode(name, dynamic_code);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("SendDeviceAuthenticationInfoToDataNode fail\n");
        return err;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateDeviceDynamicCode(const SAccountInfoCloudDataNode *datanode, TChar *name, TU32 dynamic_code)
{
    DASSERT(name);
    DASSERT(datanode);
    AUTO_LOCK(mpMutex);
    EECode err = EECode_OK;

    CICommunicationClientPort *p_port = (CICommunicationClientPort *)datanode->root.p_agent;
    if (DUnlikely(!p_port)) {
        LOGM_FATAL("CICommunicationClientPort point is NULL\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(p_port->GetConnectionStatus() != 1)) {
        LOGM_FATAL("Requested data node is not connected!\n");
        return EECode_InternalLogicalBug;
    }

    err = p_port->UpdateDeviceDynamicCode(name, dynamic_code);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("SendUserAuthenticationInfoToDataNode fail\n");
        return err;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::OwnSourceDevice(TUniqueID userid, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    AUTO_LOCK(mpMutex);

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device id %llu\n", deviceid);
        return err;
    }

    if (DUnlikely(DInvalidUniqueID != p_device->ext.ownerid)) {
        LOGM_FATAL("the target device has been owned by user: 0x%llx!\n", p_device->ext.ownerid);
        return EECode_NoPermission;
    }

    err = queryUserAccount(userid, p_user);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("query user account fail. user id 0x%llx\n", userid);
        return err;
    }

    err = userAddOwnedDevice(p_user, deviceid);

    p_device->ext.ownerid = userid;
    return err;
}

EECode CGenericAccountManager::OwnSourceDeviceByUserContext(SAccountInfoUser *p_user, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoSourceDevice *p_device = NULL;

    AUTO_LOCK(mpMutex);

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device id %llu\n", deviceid);
        return err;
    }

    if (DUnlikely(DInvalidUniqueID != p_device->ext.ownerid)) {
        LOGM_FATAL("the target device has been owned by user: 0x%llx!\n", p_device->ext.ownerid);
        return EECode_NoPermission;
    }

    err = userAddOwnedDevice(p_user, deviceid);

    p_device->ext.ownerid = p_user->root.header.id;
    return err;
}

EECode CGenericAccountManager::SourceSetOwner(SAccountInfoSourceDevice *p_device, SAccountInfoUser *p_user)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely((!p_device) || (!p_user))) {
        LOGM_FATAL("NULL p_device %p or NULL p_user %p\n", p_device, p_user);
        return EECode_BadParam;
    }

    TUniqueID dev_id = p_device->root.header.id, usr_id = p_user->root.header.id;
    LOG_NOTICE("dev_id %llx, usr_id %llx\n", dev_id, usr_id);
    if (DUnlikely(EAccountGroup_SourceDevice != ((dev_id & DExtIDTypeMask) >> DExtIDTypeShift))) {
        LOGM_FATAL("BAD dev id(%llx) type, name %s\n", dev_id, p_device->root.base.name);
        return EECode_BadParam;
    }

    if (DUnlikely(EAccountGroup_UserAccount != ((usr_id & DExtIDTypeMask) >> DExtIDTypeShift))) {
        LOGM_FATAL("BAD usr id(%llx) type, name %s\n", usr_id, p_user->root.base.name);
        return EECode_BadParam;
    }

    if (DUnlikely(DInvalidUniqueID != p_device->ext.ownerid)) {
        LOGM_FATAL("the target device has been owned by user: 0x%llx!\n", p_device->ext.ownerid);
        return EECode_NoPermission;
    }

    userAddOwnedDevice(p_user, dev_id);
    p_device->ext.ownerid = usr_id;
    return EECode_OK;
}

EECode CGenericAccountManager::InviteFriend(SAccountInfoUser *p_user, TChar *name, TUniqueID &friendid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_friend_user = NULL;
    friendid = DInvalidUniqueID;

    if (DUnlikely((!p_user) || (!name))) {
        LOGM_FATAL("NULL p_user %p or NULL name %p\n", p_user, name);
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccountByName(name, p_friend_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query friend account fail. user id %llx\n", p_friend_user->root.header.id);
        return err;
    }

    err = userAddFriend(p_user, p_friend_user->root.header.id);
    friendid = p_friend_user->root.header.id;
    //todo, notify added friend

    return err;
}

EECode CGenericAccountManager::InviteFriendByID(SAccountInfoUser *p_user, TUniqueID friendid, TChar *name)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_friend_user = NULL;

    if (DUnlikely((!p_user) || (DInvalidUniqueID == friendid))) {
        LOGM_FATAL("NULL p_user %p or invalid id %llx\n", p_user, friendid);
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(friendid, p_friend_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query friend account fail. friend id %llx\n", friendid);
        return err;
    }

    err = userAddFriend(p_user, friendid);
    if (err != EECode_OK) {
        LOGM_FATAL("add friend fail. friend id %llx\n", friendid);
        return err;
    }

    memcpy(name, p_friend_user->root.base.name, DMAX_ACCOUNT_NAME_LENGTH);

    return err;
}

EECode CGenericAccountManager::AcceptFriend(SAccountInfoUser *p_user, TUniqueID friendid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_friend_user = NULL;

    if (DUnlikely((!p_user) || (DInvalidUniqueID == friendid))) {
        LOGM_FATAL("NULL p_user %p, or not valid friend id %llx\n", p_user, friendid);
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(friendid, p_friend_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", friendid);
        return err;
    }

    if (!isInUserFriendList(p_friend_user, p_user->root.header.id)) {
        LOGM_WARN("not in taeget's friend list\n");
        return EECode_NoPermission;
    }

    return userAddFriend(p_user, friendid);
}

EECode CGenericAccountManager::RemoveFriend(SAccountInfoUser *p_user, TUniqueID friendid)
{
    if (DUnlikely((!p_user) || (DInvalidUniqueID == friendid))) {
        LOGM_FATAL("NULL p_user %p, or not valid friend id %llx\n", p_user, friendid);
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    userRemoveFriend(p_user, friendid);

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateUserPrivilege(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TU32 number_of_devices, TUniqueID *p_list_of_devices)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoUser *p_target_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    if (DUnlikely(!number_of_devices || !p_list_of_devices)) {
        LOGM_FATAL("NULL params\n");
        return EECode_BadParam;
    }

    if (DUnlikely(userid == target_userid)) {
        LOGM_FATAL("id is same\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(userid, p_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", userid);
        return err;
    }

    err = queryUserAccount(target_userid, p_target_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", target_userid);
        return err;
    }

    TU32 i = 0;
    for (i = 0; i < number_of_devices; i ++) {
        err = querySourceDeviceAccount(p_list_of_devices[i], p_device);
        if (DUnlikely(err != EECode_OK)) {
            LOGM_FATAL("query device account fail. device_id id %llu\n", p_list_of_devices[i]);
            return err;
        }

        if (p_device->ext.ownerid != userid) {
            LOGM_FATAL("ownership check fail\n");
            return EECode_NoPermission;
        }

        deviceAddOrUpdateSharedUser(p_device, target_userid, privilege_mask);
    }

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateUserPrivilegeSingleDevice(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoUser *p_target_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    if (DUnlikely(userid == target_userid)) {
        LOGM_FATAL("id is same\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(userid, p_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", userid);
        return err;
    }

    err = queryUserAccount(target_userid, p_target_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", target_userid);
        return err;
    }

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    if (p_device->ext.ownerid != userid) {
        LOGM_FATAL("ownership check fail\n");
        return EECode_NoPermission;
    }

    deviceAddOrUpdateSharedUser(p_device, target_userid, privilege_mask);
    userAddSharedDevice(p_target_user, deviceid);

    return EECode_OK;
}

EECode CGenericAccountManager::UpdateUserPrivilegeSingleDeviceByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid)
{
    EECode err = EECode_OK;

    SAccountInfoUser *p_target_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    DASSERT(p_user);
    AUTO_LOCK(mpMutex);

    err = queryUserAccount(target_userid, p_target_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", target_userid);
        return err;
    }

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    if (DUnlikely(p_device->ext.ownerid != p_user->root.header.id)) {
        LOGM_FATAL("ownership check fail\n");
        return EECode_NoPermission;
    }

    deviceAddOrUpdateSharedUser(p_device, target_userid, privilege_mask);
    userAddSharedDevice(p_target_user, deviceid);

    return EECode_OK;
}

EECode CGenericAccountManager::DonateSourceDeviceOwnship(TUniqueID userid, TUniqueID target_userid, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoUser *p_target_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    if (DUnlikely(userid == target_userid)) {
        LOGM_FATAL("id is same\n");
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(userid, p_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", userid);
        return err;
    }

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    if (p_device->ext.ownerid != userid) {
        LOGM_FATAL("ownership check fail\n");
        return EECode_NoPermission;
    }

    userRemoveOwnedDevice(p_user, deviceid);

    if (target_userid != DInvalidUniqueID) {
        err = queryUserAccount(target_userid, p_target_user);
        if (err != EECode_OK) {
            LOGM_FATAL("query user account fail. user id %llu\n", target_userid);
            return err;
        }

        //to do, send notification
    }

    p_device->ext.ownerid = target_userid;
    return EECode_OK;
}

EECode CGenericAccountManager::DonateSourceDeviceOwnshipByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_target_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    DASSERT(p_user);

    AUTO_LOCK(mpMutex);

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    if (p_device->ext.ownerid != p_user->root.header.id) {
        LOGM_FATAL("ownership check fail\n");
        return EECode_NoPermission;
    }

    userRemoveOwnedDevice(p_user, deviceid);

    if (target_userid != DInvalidUniqueID) {
        err = queryUserAccount(target_userid, p_target_user);
        if (err != EECode_OK) {
            LOGM_FATAL("query user account fail. user id %llu\n", target_userid);
            return err;
        }

        //to do, send notification
    }

    p_device->ext.ownerid = target_userid;
    return EECode_OK;
}

EECode CGenericAccountManager::AcceptSharing(TUniqueID userid, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(userid, p_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", userid);
        return err;
    }

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    //check valid
    if (!isInDeviceSharedlist(p_device, userid)) {
        LOGM_WARN("not in device's share list\n");
        return EECode_NoPermission;
    }

    userAddSharedDevice(p_user, deviceid);

    return EECode_OK;
}

EECode CGenericAccountManager::AcceptDonation(TUniqueID userid, TUniqueID deviceid)
{
    EECode err = EECode_OK;
    SAccountInfoUser *p_user = NULL;
    SAccountInfoSourceDevice *p_device = NULL;

    AUTO_LOCK(mpMutex);

    err = queryUserAccount(userid, p_user);
    if (err != EECode_OK) {
        LOGM_FATAL("query user account fail. user id %llu\n", userid);
        return err;
    }

    err = querySourceDeviceAccount(deviceid, p_device);
    if (err != EECode_OK) {
        LOGM_FATAL("query device account fail. device_id id %llu\n", deviceid);
        return err;
    }

    //check valid
    if (p_device->ext.ownerid == userid) {
        userAddOwnedDevice(p_user, deviceid);
    } else {
        LOGM_ERROR("bad request\n");
        return EECode_NoPermission;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::RetrieveUserOwnedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list)
{
    AUTO_LOCK(mpMutex);

    SAccountInfoUser *p_user = NULL;
    EECode err = queryUserAccount(userid, p_user);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("query user account fail. user id %llx\n", userid);
        return err;
    }

    count = p_user->admindevicenum;
    id_list = p_user->p_admindeviceid;

    return EECode_OK;
}

EECode CGenericAccountManager::RetrieveUserFriendList(TUniqueID userid, TU32 &count, TUniqueID *&id_list)
{
    AUTO_LOCK(mpMutex);

    SAccountInfoUser *p_user = NULL;
    EECode err = queryUserAccount(userid, p_user);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("query user account fail. user id %llx\n", userid);
        return err;
    }

    count = p_user->friendsnum;
    id_list = p_user->p_friendsid;

    return EECode_OK;
}

EECode CGenericAccountManager::RetrieveUserSharedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list)
{
    AUTO_LOCK(mpMutex);

    SAccountInfoUser *p_user = NULL;
    EECode err = queryUserAccount(userid, p_user);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("query user account fail. user id %llx\n", userid);
        return err;
    }

    count = p_user->shareddevicenum;
    id_list = p_user->p_shareddeviceid;

    return EECode_OK;
}

EECode CGenericAccountManager::LoadFromDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id)
{
    EECode err = EECode_OK;

    err = mpSourceDeviceAccount->LoadDataBase(source_device_file_name, source_device_ext_file_name);
    DASSERT_OK(err);

    err = mpUserAccount->LoadDataBase(user_account_filename, user_account_ext_filename);
    DASSERT_OK(err);

    err = mpCloudDataNodeAccount->LoadDataBase(data_node_filename, data_node_ext_filename);
    DASSERT_OK(err);

    if (DLikely(data_node_list_filename)) {
        IIO *p_io = gfCreateIO(EIOType_File);
        if (DLikely(p_io)) {
            EECode err = p_io->Open(data_node_list_filename, EIOFlagBit_Read | EIOFlagBit_Binary);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("open %s fail\n", data_node_list_filename);
                return EECode_IOError;
            }

            SStorageHeader header;
            TIOSize read_count = 1;
            memset(&header, 0x0, sizeof(SStorageHeader));
            err = p_io->Read((TU8 *) &header, sizeof(SStorageHeader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(header.flag1 == (DDBFlag1LittleEndian | DDBFlag1UseGlobalItemSize));
            mCurrentDataNodes = (TU32)((header.item_total_count_1) | ((TU64)header.item_total_count_2 << 8) | ((TU64)header.item_total_count_3 << 16) | ((TU64)header.item_total_count_4 << 24) | ((TU64)header.item_total_count_5 << 32) | ((TU64)header.item_total_count_6 << 40));
            mMaxDataNodes = mCurrentDataNodes + 8;
            mpDataNodesID = (TUniqueID *) DDBG_MALLOC(mMaxDataNodes * sizeof(TUniqueID), "IAMD");
            if (DUnlikely(!mpDataNodesID)) {
                LOGM_FATAL("no memory, request size %ld\n", (TULong)(mMaxDataNodes * sizeof(TUniqueID)));
                return EECode_NoMemory;
            }
            read_count = mCurrentDataNodes;
            err = p_io->Read((TU8 *) mpDataNodesID, sizeof(TUniqueID), read_count);
            DASSERT_OK(err);

            //LOGM_NOTICE("mCurrentDataNodes %d, mpDataNodesID[0] %llx \n", mCurrentDataNodes, mpDataNodesID[0]);

            p_io->Sync();
            p_io->Close();
            p_io->Delete();
            p_io = NULL;

            mpDataNodes = (SAccountInfoCloudDataNode **) DDBG_MALLOC(mMaxDataNodes * sizeof(SAccountInfoCloudDataNode *), "IAMN");
            if (DUnlikely(!mpDataNodes)) {
                LOGM_FATAL("no memory, request size %ld\n", (TULong)(mMaxDataNodes * sizeof(SAccountInfoCloudDataNode *)));
                return EECode_NoMemory;
            }

            for (TU32 i = 0; i < mCurrentDataNodes; i ++) {
                void *p_tmp = NULL;
                TDataBaseItemSize tmp_size = 0;
                mpCloudDataNodeAccount->QueryItem(mpDataNodesID[i], p_tmp, tmp_size);
                mpDataNodes[i] = (SAccountInfoCloudDataNode *) p_tmp;
                //LOGM_NOTICE("mpDataNodes[i]'s url %s\n", mpDataNodes[i]->ext.url);
            }
        }
    } else {
        LOGM_FATAL("NULL data_node_ext_filename\n");
    }

    err = mpCloudControlNodeAccount->LoadDataBase(control_node_filename, NULL);
    DASSERT_OK(err);

    return EECode_OK;
}

EECode CGenericAccountManager::StoreToDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id)
{
    EECode err = EECode_OK;

    err = mpSourceDeviceAccount->SaveDataBase(source_device_file_name, source_device_ext_file_name);
    DASSERT_OK(err);

    err = mpUserAccount->SaveDataBase(user_account_filename, user_account_ext_filename);
    DASSERT_OK(err);

    err = mpCloudDataNodeAccount->SaveDataBase(data_node_filename, data_node_ext_filename);
    DASSERT_OK(err);

    if (mpDataNodesID && mCurrentDataNodes) {
        if (DLikely(data_node_list_filename)) {
            IIO *p_io = gfCreateIO(EIOType_File);
            if (DLikely(p_io)) {
                EECode err = p_io->Open(data_node_list_filename, EIOFlagBit_Write | EIOFlagBit_Binary);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("open %s fail\n", data_node_list_filename);
                    return EECode_IOError;
                }

                SStorageHeader header;
                memset(&header, 0x0, sizeof(SStorageHeader));
                header.flag1 = DDBFlag1LittleEndian | DDBFlag1UseGlobalItemSize;
                header.global_item_size_high = ((sizeof(TUniqueID)) & 0xff00) >> 8;
                header.global_item_size_low = (sizeof(TUniqueID)) & 0xff;
                header.item_total_count_1 = (mCurrentDataNodes) & 0xff;
                header.item_total_count_2 = (mCurrentDataNodes >> 8) & 0xff;
                header.item_total_count_3 = (mCurrentDataNodes >> 16) & 0xff;
                header.item_total_count_4 = (mCurrentDataNodes >> 24) & 0xff;
                //header.item_total_count_5 = (mCurrentDataNodes >> 32) & 0xff;
                //header.item_total_count_6 = (mCurrentDataNodes >> 40) & 0xff;
                err = p_io->Write((TU8 *) &header, sizeof(SStorageHeader), 1);
                DASSERT_OK(err);

                LOGM_NOTICE("mCurrentDataNodes %d, mpDataNodesID[0] %llx \n", mCurrentDataNodes, mpDataNodesID[0]);

                err = p_io->Write((TU8 *) mpDataNodesID, sizeof(TUniqueID), mCurrentDataNodes);
                DASSERT_OK(err);

                p_io->Sync();
                p_io->Close();
                p_io->Delete();
                p_io = NULL;
            }
        } else {
            LOGM_FATAL("NULL data_node_ext_filename\n");
        }
    } else {
        LOGM_WARN("no data node?\n");
    }

    err = mpCloudControlNodeAccount->SaveDataBase(control_node_filename, NULL);
    DASSERT_OK(err);

    return EECode_OK;
}

EECode CGenericAccountManager::PrivilegeQueryDevice(TUniqueID user_id, TUniqueID device_id, SAccountInfoSourceDevice *&p_device)
{
    void *p_dev = NULL;
    TDataBaseItemSize size = 0;

    EECode err = mpSourceDeviceAccount->QueryItem(device_id, p_dev, size);
    if (DUnlikely((EECode_OK != err) || (!p_dev))) {
        LOGM_ERROR("bad device id %llx\n", device_id);
        return EECode_NotExist;
    }

    p_device = (SAccountInfoSourceDevice *)p_dev;
    if (p_device->ext.ownerid == user_id) {
        return EECode_OK;
    }

    if ((!p_device->friendsnum) || (!p_device->p_sharedfriendsid)) {
        p_device = NULL;
        LOGM_ERROR("user %llx do not have query device id %llx's privilege\n", user_id, device_id);
        return EECode_NoPermission;
    }

    TU32 i = 0;
    for (i = 0; i < p_device->friendsnum; i ++) {
        if (user_id == p_device->p_sharedfriendsid[i]) {
            return EECode_OK;
        }
    }

    p_device = NULL;
    LOGM_ERROR("user %llx do not have query device id %llx's privilege\n", user_id, device_id);
    return EECode_NoPermission;
}

EECode CGenericAccountManager::PrivilegeQueryUser(SAccountInfoUser *p_user, TUniqueID friend_id, SAccountInfoUser *&p_friend)
{
    SAccountInfoUser *tmp_friend = NULL;
    if (EECode_OK != queryUserAccount(friend_id, tmp_friend)) {
        LOGM_ERROR("bad id %llx\n", friend_id);
        return EECode_NotExist;
    }

    if (!isInUserFriendList(tmp_friend, p_user->root.header.id)) {
        LOGM_WARN("not in taeget's friend list\n");
        return EECode_NoPermission;
    }

    p_friend = tmp_friend;
    return EECode_OK;
}

void CGenericAccountManager::Destroy()
{
    Delete();
    return;
}

EECode CGenericAccountManager::CallBackReadSourceAccount(void *context, IIO *p_io, TU32 datasize, void *p_node)
{
    TU32 read_size = 0;
    EECode err = EECode_OK;

    SAccountInfoSourceDevice *p_info = (SAccountInfoSourceDevice *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params, %p, %p\n", p_io, p_node);
        return EECode_InternalLogicalBug;
    }

    if (!datasize) {
        return EECode_OK;
    }

    STLV16Header tlvheader;
    TIOSize read_count = 0;

    read_size = (sizeof(TUniqueID) + sizeof(TU32)) * p_info->friendsnum;
    if (DLikely(read_size)) {
        p_info->p_sharedfriendsid = (TUniqueID *)DDBG_MALLOC(read_size, "IAMd");
        if (DLikely(p_info->p_sharedfriendsid)) {
            read_count = 1;
            err = p_io->Read((TU8 *)&tlvheader, sizeof(tlvheader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(ETLV16Type_SourceDeviceSharePrivilegeComboList == tlvheader.type);
            DASSERT(read_size == tlvheader.length);

            read_count = p_info->friendsnum;
            err = p_io->Read((TU8 *)p_info->p_sharedfriendsid, sizeof(TU32) + sizeof(TUniqueID), read_count);
            DASSERT_OK(err);
            DASSERT(read_count == p_info->friendsnum);
            p_info->friendsnum_max = p_info->friendsnum;

            p_info->p_privilege_mask = (TU32 *)(p_info->p_sharedfriendsid + p_info->friendsnum);
        }
    }

    DASSERT(datasize == read_size);

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackReadUserAccount(void *context, IIO *p_io, TU32 datasize, void *p_node)
{
    TU32 read_size = 0, tot_size = 0;
    EECode err = EECode_OK;

    SAccountInfoUser *p_info = (SAccountInfoUser *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params, %p, %p\n", p_io, p_node);
        return EECode_InternalLogicalBug;
    }

    if (!datasize) {
        return EECode_OK;
    }

    STLV16Header tlvheader;
    TIOSize read_count = 0;

    read_size = sizeof(TUniqueID) * p_info->admindevicenum;
    if (DLikely(read_size)) {
        tot_size += read_size;
        p_info->p_admindeviceid = (TUniqueID *)DDBG_MALLOC(read_size, "IAMd");
        if (DLikely(p_info->p_admindeviceid)) {
            read_count = 1;
            err = p_io->Read((TU8 *)&tlvheader, sizeof(tlvheader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(ETLV16Type_UserOwnedDeviceList == tlvheader.type);
            DASSERT(read_size == tlvheader.length);

            read_count = p_info->admindevicenum;
            err = p_io->Read((TU8 *)p_info->p_admindeviceid, sizeof(TUniqueID), read_count);
            DASSERT_OK(err);
            DASSERT(read_count == p_info->admindevicenum);
            p_info->admindevicenum_max = p_info->admindevicenum;
        }
    }

    read_size = sizeof(TUniqueID) * p_info->shareddevicenum;
    if (DLikely(read_size)) {
        tot_size += read_size;
        p_info->p_shareddeviceid = (TUniqueID *)DDBG_MALLOC(read_size, "IAMd");
        if (DLikely(p_info->p_shareddeviceid)) {
            read_count = 1;
            err = p_io->Read((TU8 *)&tlvheader, sizeof(tlvheader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(ETLV16Type_UserSharedDeviceList == tlvheader.type);
            DASSERT(read_size == tlvheader.length);

            read_count = p_info->shareddevicenum;
            err = p_io->Read((TU8 *)p_info->p_shareddeviceid, sizeof(TUniqueID), read_count);
            DASSERT_OK(err);
            DASSERT(read_count == p_info->shareddevicenum);
            p_info->shareddevicenum_max = p_info->shareddevicenum;
        }
    }

    read_size = sizeof(TUniqueID) * p_info->friendsnum;
    if (DLikely(read_size)) {
        tot_size += read_size;
        p_info->p_friendsid = (TUniqueID *)DDBG_MALLOC(read_size, "IAMd");
        if (DLikely(p_info->p_friendsid)) {
            read_count = 1;
            err = p_io->Read((TU8 *)&tlvheader, sizeof(tlvheader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(ETLV16Type_UserFriendList == tlvheader.type);
            DASSERT(read_size == tlvheader.length);

            read_count = p_info->friendsnum;
            err = p_io->Read((TU8 *)p_info->p_friendsid, sizeof(TUniqueID), read_count);
            DASSERT_OK(err);
            DASSERT(read_count == p_info->friendsnum);
            p_info->friendsnum_max = p_info->friendsnum;
        }
    }

    DASSERT(datasize == tot_size);

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackReadCloudDataNode(void *context, IIO *p_io, TU32 datasize, void *p_node)
{
    TU32 read_size = 0;
    EECode err = EECode_OK;

    SAccountInfoCloudDataNode *p_info = (SAccountInfoCloudDataNode *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params, %p, %p\n", p_io, p_node);
        return EECode_InternalLogicalBug;
    }

    if (!datasize) {
        return EECode_OK;
    }

    STLV16Header tlvheader;
    TIOSize read_count = 0;

    read_size = (sizeof(TUniqueID)) * p_info->current_channel_number;
    if (DLikely(read_size)) {
        p_info->p_channel_id = (TUniqueID *)DDBG_MALLOC((sizeof(TUniqueID)) * p_info->max_channel_number, "IAMd");
        if (DLikely(p_info->p_channel_id)) {
            read_count = 1;
            err = p_io->Read((TU8 *)&tlvheader, sizeof(tlvheader), read_count);
            DASSERT_OK(err);
            DASSERT(1 == read_count);
            DASSERT(ETLV16Type_DataNodeChannelIDList == tlvheader.type);
            DASSERT(read_size == tlvheader.length);

            read_count = p_info->current_channel_number;
            err = p_io->Read((TU8 *)p_info->p_channel_id, sizeof(TUniqueID), read_count);
            DASSERT_OK(err);
            DASSERT(read_count == p_info->current_channel_number);
        }
    }

    DASSERT(datasize == read_size);

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackWriteSourceAccount(void *context, IIO *p_io, void *p_node)
{
    TU32 write_size = 0;
    EECode err = EECode_OK;

    SAccountInfoSourceDevice *p_info = (SAccountInfoSourceDevice *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params\n");
        return EECode_InternalLogicalBug;
    }

    STLV16Header tlvheader;
    TIOSize write_count = 0;

    write_size = (sizeof(TU32) + sizeof(TUniqueID)) * p_info->friendsnum;
    if (DLikely(write_size)) {
        tlvheader.type = ETLV16Type_SourceDeviceSharePrivilegeComboList;
        tlvheader.length = (p_info->friendsnum * (sizeof(TU32) + sizeof(TUniqueID)));
        write_count = 1;
        err = p_io->Write((TU8 *)&tlvheader, sizeof(tlvheader), write_count);
        DASSERT_OK(err);
        DASSERT(1 == write_count);

        DASSERT(p_info->p_sharedfriendsid);
        write_count = p_info->friendsnum;
        err = p_io->Write((TU8 *)p_info->p_sharedfriendsid, sizeof(TUniqueID), write_count);
        DASSERT_OK(err);
        DASSERT(write_count == p_info->friendsnum);

        DASSERT(p_info->p_privilege_mask);
        write_count = p_info->friendsnum;
        err = p_io->Write((TU8 *)p_info->p_privilege_mask, sizeof(TU32), write_count);
        DASSERT_OK(err);
        DASSERT(write_count == p_info->friendsnum);
    }

    p_info->root.header.total_memory_length = write_size;

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackWriteUserAccount(void *context, IIO *p_io, void *p_node)
{
    TU32 write_size = 0, tot_size = 0;
    EECode err = EECode_OK;

    SAccountInfoUser *p_info = (SAccountInfoUser *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params\n");
        return EECode_InternalLogicalBug;
    }

    STLV16Header tlvheader;
    TIOSize write_count = 0;

    write_size = sizeof(TUniqueID) * p_info->admindevicenum;
    if (DLikely(write_size && p_info->p_admindeviceid)) {
        tot_size += write_size;

        tlvheader.type = ETLV16Type_UserOwnedDeviceList;
        tlvheader.length = (p_info->admindevicenum * sizeof(TUniqueID));
        write_count = 1;
        err = p_io->Write((TU8 *)&tlvheader, sizeof(tlvheader), write_count);
        DASSERT_OK(err);

        write_count = p_info->admindevicenum;
        err = p_io->Write((TU8 *)p_info->p_admindeviceid, sizeof(TUniqueID), write_count);
        DASSERT_OK(err);
        p_info->admindevicenum_max = p_info->admindevicenum;
    }

    write_size = sizeof(TUniqueID) * p_info->shareddevicenum;
    if (DLikely(write_size && p_info->p_shareddeviceid)) {
        tot_size += write_size;

        tlvheader.type = ETLV16Type_UserSharedDeviceList;
        tlvheader.length = (p_info->shareddevicenum * sizeof(TUniqueID));
        write_count = 1;
        err = p_io->Write((TU8 *)&tlvheader, sizeof(tlvheader), write_count);
        DASSERT_OK(err);

        write_count = p_info->shareddevicenum;
        err = p_io->Write((TU8 *)p_info->p_shareddeviceid, sizeof(TUniqueID), write_count);
        DASSERT_OK(err);
        p_info->shareddevicenum_max = p_info->shareddevicenum;
    }

    write_size = sizeof(TUniqueID) * p_info->friendsnum;
    if (DLikely(write_size && p_info->p_friendsid)) {
        tot_size += write_size;

        tlvheader.type = ETLV16Type_UserFriendList;
        tlvheader.length = (p_info->friendsnum * sizeof(TUniqueID));
        write_count = 1;
        err = p_io->Write((TU8 *)&tlvheader, sizeof(tlvheader), write_count);
        DASSERT_OK(err);

        write_count = p_info->friendsnum;
        err = p_io->Write((TU8 *)p_info->p_friendsid, sizeof(TUniqueID), write_count);
        DASSERT_OK(err);
        p_info->friendsnum_max = p_info->friendsnum;
    }

    p_info->root.header.total_memory_length = tot_size;

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackWriteCloudDataNode(void *context, IIO *p_io, void *p_node)
{
    TU32 write_size = 0;
    EECode err = EECode_OK;

    SAccountInfoCloudDataNode *p_info = (SAccountInfoCloudDataNode *) p_node;
    if (DUnlikely(!p_io || !p_node)) {
        LOG_FATAL("NULL params\n");
        return EECode_InternalLogicalBug;
    }

    STLV16Header tlvheader;
    TIOSize write_count = 0;

    write_size = (sizeof(TUniqueID)) * p_info->current_channel_number;
    if (DLikely(write_size)) {
        tlvheader.type = ETLV16Type_DataNodeChannelIDList;
        tlvheader.length = p_info->current_channel_number * sizeof(TUniqueID);
        write_count = 1;
        err = p_io->Write((TU8 *)&tlvheader, sizeof(tlvheader), write_count);
        DASSERT_OK(err);
        DASSERT(1 == write_count);

        DASSERT(p_info->p_channel_id);
        write_count = p_info->current_channel_number;
        err = p_io->Write((TU8 *)p_info->p_channel_id, sizeof(TUniqueID), write_count);
        DASSERT_OK(err);
        DASSERT(write_count == p_info->current_channel_number);
    }

    p_info->root.header.total_memory_length = write_size;

    return EECode_OK;
}

EECode CGenericAccountManager::CallBackInitialize(void *context, void *p_node)
{
    SAccountInfoRoot *p_info = (SAccountInfoRoot *) p_node;
    if (DUnlikely(!p_node)) {
        LOG_FATAL("NULL params\n");
        return EECode_InternalLogicalBug;
    }

    p_info->p_agent = NULL;

    return EECode_OK;
}

EECode CGenericAccountManager::newSourceDeviceAccount(TChar *account, TChar *password, SAccountInfoSourceDevice *&ret)
{
    ret = NULL;

    if (DUnlikely((!account) || (!password))) {
        LOGM_FATAL("NULL account name or password\n");
        return EECode_BadParam;
    }

    TInt account_name_len = strlen(account);
    TInt password_len = strlen(password);

    if (DUnlikely((account_name_len >= DMAX_ACCOUNT_NAME_LENGTH) || (password_len >= DIdentificationStringLength))) {
        LOGM_WARN("too long account name(%d) or password(%d)\n", account_name_len, password_len);
        return EECode_BadParam;
    }

    SAccountInfoSourceDevice *p_account = (SAccountInfoSourceDevice *) DDBG_MALLOC(sizeof(SAccountInfoSourceDevice), "IAMS");
    if (DLikely(p_account)) {
        memset(p_account, 0x0, sizeof(SAccountInfoSourceDevice));
        snprintf(p_account->root.base.name, DMAX_ACCOUNT_NAME_LENGTH, "%s", account);
        snprintf(p_account->root.base.password, DIdentificationStringLength, "%s", password);
    } else {
        LOGM_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    ret = p_account;

    return EECode_OK;
}

EECode CGenericAccountManager::newUserAccount(TChar *account, TChar *password, SAccountInfoUser *&ret)
{
    ret = NULL;

    if (DUnlikely((!account) || (!password))) {
        LOGM_FATAL("NULL account name or password\n");
        return EECode_BadParam;
    }

    TInt account_name_len = strlen(account);
    TInt password_len = strlen(password);

    if (DUnlikely((account_name_len >= DMAX_ACCOUNT_NAME_LENGTH) || (password_len >= DIdentificationStringLength))) {
        LOGM_WARN("too long account name(%d) or password(%d)\n", account_name_len, password_len);
        return EECode_BadParam;
    }

    SAccountInfoUser *p_account = (SAccountInfoUser *) DDBG_MALLOC(sizeof(SAccountInfoUser), "IAMU");
    if (DLikely(p_account)) {
        memset(p_account, 0x0, sizeof(SAccountInfoUser));
        snprintf(p_account->root.base.name, DMAX_ACCOUNT_NAME_LENGTH, "%s", account);
        snprintf(p_account->root.base.password, DIdentificationStringLength, "%s", password);
    } else {
        LOGM_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    ret = p_account;

    return EECode_OK;
}

EECode CGenericAccountManager::newCloudDataNodeAccount(TChar *account, TChar *password, TSocketPort admin_port, SAccountInfoCloudDataNode *&ret)
{
    ret = NULL;

    if (DUnlikely((!account) || (!password))) {
        LOGM_FATAL("NULL account name or password\n");
        return EECode_BadParam;
    }

    TInt account_name_len = strlen(account);
    TInt password_len = strlen(password);

    if (DUnlikely((account_name_len >= DMAX_ACCOUNT_NAME_LENGTH) || (password_len >= DIdentificationStringLength))) {
        LOGM_WARN("too long account name(%d) or password(%d)\n", account_name_len, password_len);
        return EECode_BadParam;
    }

    SAccountInfoCloudDataNode *p_account = (SAccountInfoCloudDataNode *) DDBG_MALLOC(sizeof(SAccountInfoCloudDataNode), "IAMC");
    if (DLikely(p_account)) {
        memset(p_account, 0x0, sizeof(SAccountInfoCloudDataNode));
        TChar *ptmp = strchr(account, ':');
        if (ptmp) {
            snprintf(p_account->root.base.name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", account);
            memcpy(p_account->ext.url, account, (TU32)(ptmp - account));
        } else {
            snprintf(p_account->root.base.name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s:%d", account, admin_port);
            memcpy(p_account->ext.url, account, strlen(account));
        }
        snprintf(p_account->root.base.password, DIdentificationStringLength, "%s", password);
    } else {
        LOGM_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    p_account->ext.admin_port = admin_port;

    ret = p_account;

    return EECode_OK;
}

EECode CGenericAccountManager::newCloudControlNodeAccount(TChar *account, TChar *password, SAccountInfoCloudControlNode *&ret)
{
    ret = NULL;

    if (DUnlikely((!account) || (!password))) {
        LOGM_FATAL("NULL account name or password\n");
        return EECode_BadParam;
    }

    TInt account_name_len = strlen(account);
    TInt password_len = strlen(password);

    if (DUnlikely((account_name_len >= DMAX_ACCOUNT_NAME_LENGTH) || (password_len >= DIdentificationStringLength))) {
        LOGM_WARN("too long account name(%d) or password(%d)\n", account_name_len, password_len);
        return EECode_BadParam;
    }

    SAccountInfoCloudControlNode *p_account = (SAccountInfoCloudControlNode *) DDBG_MALLOC(sizeof(SAccountInfoCloudControlNode), "IAMc");
    if (DLikely(p_account)) {
        memset(p_account, 0x0, sizeof(SAccountInfoCloudControlNode));
        snprintf(p_account->root.base.name, DMAX_ACCOUNT_NAME_LENGTH, "%s", account);
        snprintf(p_account->root.base.password, DIdentificationStringLength, "%s", password);
    } else {
        LOGM_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    ret = p_account;

    return EECode_OK;
}

EECode CGenericAccountManager::queryAccount(TUniqueID id, SAccountInfoRoot *&p_info)
{
    EAccountGroup group = (EAccountGroup)((id & DExtIDTypeMask) >> DExtIDTypeShift);
    void *p_tmp = NULL;
    TDataBaseItemSize data_size = 0;
    EECode err = EECode_OK;

    p_info = NULL;

    switch (group) {

        case EAccountGroup_SourceDevice:
            if (DLikely(mpSourceDeviceAccount)) {
                err = mpSourceDeviceAccount->QueryItem(id, p_tmp, data_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryItem fail\n");
                    return err;
                }
                DASSERT(p_tmp);
                DASSERT(data_size == sizeof(SAccountInfoSourceDevice));
                p_info = (SAccountInfoRoot *) p_tmp;
            } else {

            }
            break;

        case EAccountGroup_UserAccount:
            if (DLikely(mpUserAccount)) {
                err = mpUserAccount->QueryItem(id, p_tmp, data_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryItem fail\n");
                    return err;
                }
                DASSERT(p_tmp);
                DASSERT(data_size == sizeof(SAccountInfoUser));
                p_info = (SAccountInfoRoot *) p_tmp;
            } else {

            }
            break;

        case EAccountGroup_CloudDataNode:
            if (DLikely(mpCloudDataNodeAccount)) {
                err = mpCloudDataNodeAccount->QueryItem(id, p_tmp, data_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryItem fail\n");
                    return err;
                }
                DASSERT(p_tmp);
                DASSERT(data_size == sizeof(SAccountInfoUser));
                p_info = (SAccountInfoRoot *) p_tmp;
            } else {

            }
            break;

        case EAccountGroup_CloudControlNode:
            if (DLikely(mpCloudControlNodeAccount)) {
                err = mpCloudControlNodeAccount->QueryItem(id, p_tmp, data_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("QueryItem fail\n");
                    return err;
                }
                DASSERT(p_tmp);
                DASSERT(data_size == sizeof(SAccountInfoUser));
                p_info = (SAccountInfoRoot *) p_tmp;
            } else {

            }
            break;

        default:
            LOGM_ERROR("Error group %d, id %llx\n", group, id);
            return EECode_BadParam;
            break;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::querySourceDeviceAccount(TUniqueID id, SAccountInfoSourceDevice *&p_info)
{
    void *p_tmp = NULL;
    TDataBaseItemSize data_size = 0;
    EECode err = EECode_OK;

    if (DLikely(mpSourceDeviceAccount)) {
        err = mpSourceDeviceAccount->QueryItem(id, p_tmp, data_size);
        if (DUnlikely(EECode_OK != err)) {
            p_info = NULL;
            LOGM_ERROR("RemoveItem fail\n");
            return err;
        }
        DASSERT(p_tmp);
        DASSERT(data_size == sizeof(SAccountInfoSourceDevice));
        p_info = (SAccountInfoSourceDevice *) p_tmp;
    } else {
        LOGM_FATAL("NULL mpSourceDeviceAccount\n");
        p_info = NULL;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::querySourceDeviceAccountByName(TChar *name, SAccountInfoSourceDevice *&info)
{
    TUniqueID main_id = gfGenerateHashMainID(name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;
    SAccountInfoSourceDevice *p_info = NULL;
    info = NULL;

    if (DLikely(mpSourceDeviceAccount)) {
        err = mpSourceDeviceAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            //LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpSourceDeviceAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            p_info = (SAccountInfoSourceDevice *)(t_node->p_context);
            if (!strcmp(p_info->root.base.name, name)) {
                info = p_info;
                return EECode_OK;
            }
        }

        //LOGM_WARN("not exist\n");
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpSourceDeviceAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_NotExist;
}
EECode CGenericAccountManager::queryUserAccount(TUniqueID id, SAccountInfoUser *&p_info)
{
    void *p_tmp = NULL;
    TDataBaseItemSize data_size = 0;
    EECode err = EECode_OK;

    if (DLikely(mpUserAccount)) {
        err = mpUserAccount->QueryItem(id, p_tmp, data_size);
        if (DUnlikely(EECode_OK != err)) {
            p_info = NULL;
            LOGM_ERROR("RemoveItem fail\n");
            return err;
        }
        DASSERT(p_tmp);
        DASSERT(data_size == sizeof(SAccountInfoUser));
        p_info = (SAccountInfoUser *) p_tmp;
    } else {
        LOGM_FATAL("NULL mpUserAccount\n");
        p_info = NULL;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::queryUserAccountByName(TChar *name, SAccountInfoUser *&info)
{
    TUniqueID main_id = gfGenerateHashMainID(name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;
    info = NULL;

    if (DLikely(mpUserAccount)) {
        err = mpUserAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            //LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpUserAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail\n");
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            info = (SAccountInfoUser *)(t_node->p_context);
            if (!strcmp(info->root.base.name, name)) {
                return EECode_OK;
            }
        }

        //LOGM_WARN("not exist\n");
        info = NULL;
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpUserAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::queryDataNodeAccount(TUniqueID id, SAccountInfoCloudDataNode *&p_info)
{
    void *p_tmp = NULL;
    TDataBaseItemSize data_size = 0;
    EECode err = EECode_OK;

    if (DLikely(mpCloudDataNodeAccount)) {
        err = mpCloudDataNodeAccount->QueryItem(id, p_tmp, data_size);
        if (DUnlikely(EECode_OK != err)) {
            p_info = NULL;
            LOGM_ERROR("RemoveItem fail\n");
            return err;
        }
        DASSERT(p_tmp);
        DASSERT(data_size == sizeof(SAccountInfoCloudDataNode));
        p_info = (SAccountInfoCloudDataNode *) p_tmp;
    } else {
        LOGM_FATAL("NULL mpCloudDataNodeAccount\n");
        p_info = NULL;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::queryDataNodeAccountByName(TChar *name, SAccountInfoCloudDataNode *&info)
{
    TUniqueID main_id = gfGenerateHashMainID(name);

    EECode err = EECode_OK;
    void *p_header = NULL;
    void *p_node = NULL;
    info = NULL;

    if (DLikely(mpCloudDataNodeAccount)) {
        err = mpCloudDataNodeAccount->QueryHeadNodeFromMainID(p_header, main_id);
        if (DUnlikely(EECode_OK != err)) {
            //LOGM_ERROR("QueryHeadNodeFromMainID fail\n");
            return err;
        }

        while (1) {
            err = mpCloudDataNodeAccount->QueryNextNodeFromHeadNode(p_header, p_node);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("QueryNextNodeFromHeadNode fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (DUnlikely(!p_node)) {
                break;
            }

            CIDoubleLinkedList::SNode *t_node = (CIDoubleLinkedList::SNode *)p_node;
            info = (SAccountInfoCloudDataNode *)(t_node->p_context);
            if (!strcmp(info->root.base.name, name)) {
                return EECode_OK;
            }
        }

        //LOGM_WARN("not exist\n");
        info = NULL;
        return EECode_NotExist;
    } else {
        LOGM_FATAL("NULL mpCloudDataNodeAccount\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::queryControlNodeAccount(TUniqueID id, SAccountInfoCloudControlNode *&p_info)
{
    void *p_tmp = NULL;
    TDataBaseItemSize data_size = 0;
    EECode err = EECode_OK;

    if (DLikely(mpCloudControlNodeAccount)) {
        err = mpCloudControlNodeAccount->QueryItem(id, p_tmp, data_size);
        if (DUnlikely(EECode_OK != err)) {
            p_info = NULL;
            LOGM_ERROR("RemoveItem fail\n");
            return err;
        }
        DASSERT(p_tmp);
        DASSERT(data_size == sizeof(SAccountInfoCloudControlNode));
        p_info = (SAccountInfoCloudControlNode *) p_tmp;
    } else {
        LOGM_FATAL("NULL mpCloudControlNodeAccount\n");
        p_info = NULL;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CGenericAccountManager::userAddFriend(SAccountInfoUser *p_user, TUniqueID friendid)
{
    if (!p_user->p_friendsid) {
        p_user->p_friendsid = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * DInitialMaxFriendsNumber, "IAMd");
        if (DLikely(p_user->p_friendsid)) {
            p_user->p_friendsid[0] = friendid;
            p_user->friendsnum = 1;
            p_user->friendsnum_max = DInitialMaxDeviceNumber;
        } else {
            p_user->friendsnum = p_user->friendsnum_max = 0;
            LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * DInitialMaxFriendsNumber);
            return EECode_NoMemory;
        }
    } else {
        for (TU32 i = 0; i < p_user->friendsnum; i++) {
            if (p_user->p_friendsid[i] == friendid) {
                return EECode_AlreadyExist;
            }
        }

        if (DLikely(p_user->friendsnum < p_user->friendsnum_max)) {
            p_user->p_friendsid[p_user->friendsnum] = friendid;
            p_user->friendsnum++;
        } else {
            DASSERT(p_user->friendsnum == p_user->friendsnum_max);

            TUniqueID *ptmp = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->friendsnum_max), "IAMd");
            if (DLikely(ptmp)) {
                memcpy(ptmp, p_user->p_friendsid, sizeof(TUniqueID) * p_user->friendsnum);
                free(p_user->p_friendsid);
                p_user->p_friendsid = ptmp;
                p_user->friendsnum_max += DInitialMaxDeviceNumber;
                DASSERT(4096 > p_user->friendsnum_max);
                p_user->p_friendsid[p_user->friendsnum] = friendid;
                p_user->friendsnum++;
            } else {
                LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->friendsnum_max));
                return EECode_NoMemory;
            }
        }
    }

    return EECode_OK;
}

void CGenericAccountManager::userRemoveFriend(SAccountInfoUser *p_user, TUniqueID friendid)
{
    if (!p_user->p_friendsid) {
        LOGM_WARN("has no friendlist\n");
        return;
    } else {
        for (TU32 i = 0; i < p_user->friendsnum; i++) {
            if (p_user->p_friendsid[i] == friendid) {
                p_user->p_friendsid[i] = p_user->p_friendsid[p_user->friendsnum - 1];
                p_user->friendsnum --;
                return;
            }
        }
    }
    LOGM_WARN("not in friendlist\n");
}

EECode CGenericAccountManager::userAddOwnedDevice(SAccountInfoUser *p_user, TUniqueID deviceid)
{
    if (!p_user->p_admindeviceid) {
        p_user->p_admindeviceid = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * DInitialMaxFriendsNumber, "IAMd");
        if (DLikely(p_user->p_admindeviceid)) {
            p_user->p_admindeviceid[0] = deviceid;
            p_user->admindevicenum = 1;
            p_user->admindevicenum_max = DInitialMaxDeviceNumber;
        } else {
            p_user->admindevicenum = p_user->admindevicenum_max = 0;
            LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * DInitialMaxFriendsNumber);
            return EECode_NoMemory;
        }
    } else {
        for (TU32 i = 0; i < p_user->admindevicenum; i++) {
            if (p_user->p_admindeviceid[i] == deviceid) {
                LOGM_WARN("already exist\n");
                return EECode_AlreadyExist;
            }
        }

        if (DLikely(p_user->admindevicenum < p_user->admindevicenum_max)) {
            p_user->p_admindeviceid[p_user->admindevicenum] = deviceid;
            LOG_NOTICE("%d, deviceid %llx\n", p_user->admindevicenum, deviceid);
            p_user->admindevicenum++;
        } else {
            DASSERT(p_user->friendsnum == p_user->friendsnum_max);

            TUniqueID *ptmp = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->admindevicenum), "IAMd");
            if (DLikely(ptmp)) {
                memcpy(ptmp, p_user->p_admindeviceid, sizeof(TUniqueID) * p_user->admindevicenum);
                free(p_user->p_admindeviceid);
                p_user->p_admindeviceid = ptmp;
                p_user->admindevicenum_max += DInitialMaxDeviceNumber;
                DASSERT(4096 > p_user->admindevicenum_max);
                p_user->p_admindeviceid[p_user->admindevicenum] = deviceid;
                LOGM_ERROR("p_user->admindevicenum %d, p_user->p_admindeviceid[p_user->admindevicenum] %llx\n", p_user->admindevicenum, p_user->p_admindeviceid[p_user->admindevicenum]);

                p_user->admindevicenum++;
            } else {
                LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->admindevicenum));
                return EECode_NoMemory;
            }
        }
    }

    return EECode_OK;
}

void CGenericAccountManager::userRemoveOwnedDevice(SAccountInfoUser *p_user, TUniqueID deviceid)
{
    if (!p_user->p_admindeviceid) {
        LOGM_FATAL("do not have owned list yet\n");
        return;
    } else {
        for (TU32 i = 0; i < p_user->admindevicenum; i++) {
            if (p_user->p_admindeviceid[i] == deviceid) {
                p_user->p_admindeviceid[i] = p_user->p_admindeviceid[p_user->admindevicenum - 1];
                p_user->admindevicenum --;
                return;
            }
        }
    }

}

EECode CGenericAccountManager::userAddSharedDevice(SAccountInfoUser *p_user, TUniqueID deviceid)
{
    if (!p_user->p_shareddeviceid) {
        p_user->p_shareddeviceid = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * DInitialMaxFriendsNumber, "IAMd");
        if (DLikely(p_user->p_shareddeviceid)) {
            p_user->p_shareddeviceid[0] = deviceid;
            p_user->shareddevicenum = 1;
            p_user->shareddevicenum_max = DInitialMaxDeviceNumber;
        } else {
            p_user->shareddevicenum = p_user->shareddevicenum_max = 0;
            LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * DInitialMaxFriendsNumber);
            return EECode_NoMemory;
        }
    } else {
        for (TU32 i = 0; i < p_user->shareddevicenum; i++) {
            if (p_user->p_shareddeviceid[i] == deviceid) {
                LOGM_WARN("already exist\n");
                return EECode_AlreadyExist;
            }
        }

        if (DLikely(p_user->shareddevicenum < p_user->shareddevicenum_max)) {
            p_user->p_shareddeviceid[p_user->shareddevicenum] = deviceid;
            p_user->shareddevicenum++;
        } else {
            DASSERT(p_user->friendsnum == p_user->friendsnum_max);

            TUniqueID *ptmp = (TUniqueID *) DDBG_MALLOC(sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->shareddevicenum), "IAMd");
            if (DLikely(ptmp)) {
                memcpy(ptmp, p_user->p_shareddeviceid, sizeof(TUniqueID) * p_user->shareddevicenum);
                free(p_user->p_shareddeviceid);
                p_user->p_shareddeviceid = ptmp;
                p_user->shareddevicenum_max += DInitialMaxDeviceNumber;
                DASSERT(4096 > p_user->shareddevicenum_max);
                p_user->p_shareddeviceid[p_user->shareddevicenum] = deviceid;
                p_user->shareddevicenum++;
            } else {
                LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * (DInitialMaxDeviceNumber + p_user->shareddevicenum));
                return EECode_NoMemory;
            }
        }
    }

    return EECode_OK;
}

void CGenericAccountManager::userRemoveSharedDevice(SAccountInfoUser *p_user, TUniqueID deviceid)
{
    if (!p_user->p_shareddeviceid) {
        LOGM_WARN("has no shared device list\n");
        return;
    } else {
        for (TU32 i = 0; i < p_user->shareddevicenum; i++) {
            if (p_user->p_shareddeviceid[i] == deviceid) {
                p_user->p_shareddeviceid[i] = p_user->p_shareddeviceid[p_user->shareddevicenum - 1];
                p_user->shareddevicenum --;
                return;
            }
        }
    }
    LOGM_WARN("not in shared device list\n");
}

EECode CGenericAccountManager::deviceAddOrUpdateSharedUser(SAccountInfoSourceDevice *p_device, TUniqueID friendid, TU32 privilege_mask)
{
    //add into device's share list
    if (p_device->p_sharedfriendsid) {
        //if exist
        for (TU32 i = 0; i < p_device->friendsnum; i++) {
            if (p_device->p_sharedfriendsid[i] == friendid) {
                LOG_WARN("already in device's share list\n");
                p_device->p_privilege_mask[i] = privilege_mask;
                return EECode_AlreadyExist;
            }
        }

        if (p_device->friendsnum < p_device->friendsnum_max) {
            p_device->p_sharedfriendsid[p_device->friendsnum] = friendid;
            p_device->friendsnum ++;
        } else {
            DASSERT(p_device->friendsnum == p_device->friendsnum_max);
            TUniqueID *ptmp = (TUniqueID *) DDBG_MALLOC((sizeof(TU32) + sizeof(TUniqueID)) * (DInitialMaxFriendsNumber + p_device->friendsnum_max), "IAMd");
            if (DLikely(ptmp)) {
                TU32 *ptmp_mask = (TU32 *)(ptmp + p_device->friendsnum_max);
                memcpy(ptmp, p_device->p_sharedfriendsid, sizeof(TUniqueID) * p_device->friendsnum);
                memcpy(ptmp_mask, p_device->p_privilege_mask, sizeof(TU32) * p_device->friendsnum);
                DDBG_FREE(p_device->p_sharedfriendsid, "IAMd");
                p_device->p_sharedfriendsid = ptmp;
                p_device->p_privilege_mask = ptmp_mask;
                p_device->friendsnum_max += DInitialMaxFriendsNumber;
                DASSERT(4096 > p_device->friendsnum_max);
                p_device->p_sharedfriendsid[p_device->friendsnum] = friendid;
                p_device->p_privilege_mask[p_device->friendsnum] = privilege_mask;
                p_device->friendsnum++;
            } else {
                LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * (DInitialMaxFriendsNumber + p_device->friendsnum_max));
                return EECode_NoMemory;
            }
        }
    } else {
        p_device->p_sharedfriendsid = (TUniqueID *) DDBG_MALLOC((sizeof(TU32) + sizeof(TUniqueID)) * DInitialMaxFriendsNumber, "IAMd");
        if (DLikely(p_device->p_sharedfriendsid)) {
            p_device->p_privilege_mask = (TU32 *)(p_device->p_sharedfriendsid + p_device->friendsnum_max);
            p_device->p_sharedfriendsid[0] = friendid;
            p_device->p_privilege_mask[0] = privilege_mask;
            p_device->friendsnum = 1;
            p_device->friendsnum_max = DInitialMaxFriendsNumber;
        } else {
            p_device->friendsnum = p_device->friendsnum_max = 0;
            LOGM_FATAL("no memory, request size %zu\n", sizeof(TUniqueID) * DInitialMaxFriendsNumber);
            return EECode_NoMemory;
        }
    }
    return EECode_OK;
}

void CGenericAccountManager::deviceRemoveSharedUser(SAccountInfoSourceDevice *p_device, TUniqueID friendid)
{
    if (!p_device->p_sharedfriendsid) {
        LOGM_WARN("has no friendlist\n");
        return;
    }

    for (TU32 i = 0; i < p_device->friendsnum; i++) {
        if (p_device->p_sharedfriendsid[i] == friendid) {
            p_device->p_sharedfriendsid[i] = p_device->p_sharedfriendsid[p_device->friendsnum - 1];
            p_device->friendsnum --;
            return;
        }
    }
}

TU32 CGenericAccountManager::isInDeviceSharedlist(SAccountInfoSourceDevice *p_device, TUniqueID friendid)
{
    if (!p_device->p_sharedfriendsid) {
        LOGM_WARN("has no friendlist\n");
        return 0;
    }

    for (TU32 i = 0; i < p_device->friendsnum; i++) {
        if (p_device->p_sharedfriendsid[i] == friendid) {
            return 1;
        }
    }

    return 0;
}

TU32 CGenericAccountManager::isInUserFriendList(SAccountInfoUser *p_user, TUniqueID friendid)
{
    if (!p_user->p_friendsid) {
        LOGM_WARN("has no friendlist\n");
        return 0;
    } else {
        for (TU32 i = 0; i < p_user->friendsnum; i++) {
            if (p_user->p_friendsid[i] == friendid) {
                return 1;
            }
        }
    }
    return 0;
}

EECode CGenericAccountManager::getCurrentAvaiableGroupIndex(TU32 &ret_index, SAccountInfoCloudDataNode *&p_data_node)
{
    ret_index = 0;
    p_data_node = NULL;
    if (DUnlikely(!mpDataNodes || !mpDataNodesID)) {
        LOGM_ERROR("no data node??\n");
        return EECode_InternalLogicalBug;
    }

    TU32 index = 0;

    for (index = 0; index < mCurrentDataNodes; index ++) {
        if (mpDataNodes[index]->max_channel_number > mpDataNodes[index]->current_channel_number) {
            if (0) {
                //to do, if data node is not alive or online
                continue;
            }
            //mpDataNodes[index]->current_channel_number ++;
            p_data_node = mpDataNodes[index];
            //to do, data node index to group index
            ret_index = index;
            return EECode_OK;
        }
    }

    LOGM_ERROR("current alive data node is not enough\n");
    return EECode_TooMany;
}

EECode CGenericAccountManager::dataNodeAddChannel(SAccountInfoCloudDataNode *p_data_node, SAccountInfoSourceDevice *p_device)
{
    if (DUnlikely(!p_data_node || !p_device)) {
        LOGM_ERROR("NULL params\n");
        return EECode_InternalLogicalBug;
    }

    DASSERT(p_data_node->current_channel_number < p_data_node->max_channel_number);

    p_data_node->p_channel_id[p_data_node->current_channel_number] = p_device->root.header.id;
    p_data_node->current_channel_number ++;

    CICommunicationClientPort *p_clientport = (CICommunicationClientPort *)p_data_node->root.p_agent;
    if (DLikely(p_clientport)) {
        EECode err = p_clientport->AddDeviceChannel(p_device->root.base.name);
        if (DUnlikely(err != EECode_OK)) {
            LOGM_FATAL("AddDeviceChannel fail\n");
            return err;
        }
    } else {
        LOGM_WARN("Requested data node is not on line!\n");
    }

    return EECode_OK;
}

EECode CGenericAccountManager::connectDataNode(SAccountInfoCloudDataNode *p_datanode_info)
{
    EECode err = EECode_OK;

    if (DUnlikely(p_datanode_info->root.p_agent)) {
        LOGM_WARN("ConnectDataNode, p_agent is not NULL\n");
    }

    CICommunicationClientPort *p_clientport = CICommunicationClientPort::Create((void *)this, _callbackhandledatanodmsg);
    if (DUnlikely(!p_clientport)) {
        LOGM_FATAL("CICommunicationClientPort::Create fail\n");
        return EECode_Error;
    }

    err = p_clientport->PrepareConnection(p_datanode_info->ext.url, p_datanode_info->ext.admin_port, (TChar *)"name", (TChar *)"password");
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("PrepareConnection fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    //todo
    TChar id_string[] = "123456";
    err = p_clientport->SetIdentifyString(id_string);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("SetIdentifyString fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = p_clientport->LoopConnectServer(100000);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("LoopConnectServer fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    p_datanode_info->root.p_agent = p_clientport;

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

