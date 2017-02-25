/*******************************************************************************
 * simple_storage.cpp
 *
 * History:
 *    2014/02/11 - [Zhi He] create file
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

#define _FILE_OFFSET_BITS 64

#include "common_config.h"

#ifdef BUILD_OS_WINDOWS
#include <io.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <dirent.h>

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"
#include "common_private_data.h"

#include "storage_lib_if.h"

#include "simple_storage.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStorageManagement *gfCreateSimpleStorageManagement()
{
    CSimpleStorageManagement *thiz = CSimpleStorageManagement::Create();
    return thiz;
}

CSimpleStorageManagement *CSimpleStorageManagement::Create()
{
    CSimpleStorageManagement *result = new CSimpleStorageManagement();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CSimpleStorageManagement::CSimpleStorageManagement()
    : mpMutex(NULL)
    , mpSimpleStorage(NULL)
    , mRootDir(NULL)
    , mRootDirNameLength(0)
    , mConfigApi(NULL)
{
}

EECode CSimpleStorageManagement::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("CSimpleStorageManagement, gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }
    mpSimpleStorage = (SSimpleStorage *)DDBG_MALLOC(sizeof(SSimpleStorage), "STCX");
    if (DLikely(mpSimpleStorage)) {
        memset(mpSimpleStorage, 0x0, sizeof(SSimpleStorage));
    } else {
        LOG_FATAL("CSimpleStorageManagement, No Memory\n");
        return EECode_NoMemory;
    }
    mConfigApi = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 1280);
    if (DUnlikely(!mConfigApi)) {
        LOG_ERROR("CSimpleStorageManagement, gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

CSimpleStorageManagement::~CSimpleStorageManagement()
{
    if (mpSimpleStorage->current_channel_number) {
        ClearDataBase(mRootDir, mpSimpleStorage->current_channel_number);
    }
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
    if (mpSimpleStorage) {
        DDBG_FREE(mpSimpleStorage,  "STCX");
        mpSimpleStorage = NULL;
    }
    if (mRootDir) {
        DDBG_FREE(mRootDir, "STRT");
        mRootDir = NULL;
        mRootDirNameLength = 0;
    }
    if (mConfigApi) {
        mConfigApi->Destroy();
        mConfigApi = NULL;
    }
}

void CSimpleStorageManagement::Delete()
{
    delete this;
}

EECode CSimpleStorageManagement::SaveDataBase(TU32 &channel_number)
{
    AUTO_LOCK(mpMutex);

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TChar read_string2[256] = {0};
    TUint valid_channel_number = 0;

    //save channel data
    sprintf(read_string, "%s/%s", mRootDir, DDataBaseMainFileDefaultName);
    LOG_NOTICE("SaveDataBase to [%s]\n", read_string);
    EECode err = mConfigApi->OpenConfigFile(read_string, 1, 0, 1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("OpenConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
        return EECode_InternalLogicalBug;
    }

    channel_number = mpSimpleStorage->current_channel_number;
    sprintf(read_string, "%d", mpSimpleStorage->current_channel_number);
    err = mConfigApi->SetContentValue("channelnumber", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("SetContentValue(channelnumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        err = mConfigApi->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
            return EECode_InternalLogicalBug;
        }
        return EECode_InternalLogicalBug;
    }
    for (TUint index = 0; index < DSYSTEM_MAX_CHANNEL_NUM && valid_channel_number < mpSimpleStorage->current_channel_number; index++) {
        if (!mpSimpleStorage->simple_storage_channel[index].enabled) {
            continue;
        }
        sprintf(read_string, "channel_%d_name", valid_channel_number);
        err = mConfigApi->SetContentValue((const TChar *)read_string, mpSimpleStorage->simple_storage_channel[index].channel_name);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }

        sprintf(read_string, "channel_%d_enabled", valid_channel_number);
        sprintf(read_string1, "%d", mpSimpleStorage->simple_storage_channel[index].enabled);
        err = mConfigApi->SetContentValue((const TChar *)read_string, read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }

        sprintf(read_string, "channel_%d_max_file_number", valid_channel_number);
        sprintf(read_string1, "%d", mpSimpleStorage->simple_storage_channel[index].max_storage_file_number);
        err = mConfigApi->SetContentValue((const TChar *)read_string, read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }

        sprintf(read_string, "channel_%d_redudant_file_number", valid_channel_number);
        sprintf(read_string1, "%d", mpSimpleStorage->simple_storage_channel[index].redudant_storage_file_number);
        err = mConfigApi->SetContentValue((const TChar *)read_string, read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }

        sprintf(read_string, "channel_%d_single_file_duration_minutes", valid_channel_number);
        sprintf(read_string1, "%d", mpSimpleStorage->simple_storage_channel[index].single_file_duration_minutes);
        err = mConfigApi->SetContentValue((const TChar *)read_string, read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }
        valid_channel_number++;
    }
    err = mConfigApi->SaveConfigFile(NULL);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("SaveConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
        err = mConfigApi->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
            return EECode_InternalLogicalBug;
        }
        return EECode_InternalLogicalBug;
    }
    err = mConfigApi->CloseConfigFile();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", DDataBaseMainFileDefaultName, err, gfGetErrorCodeString(err));
        return EECode_InternalLogicalBug;
    }

    //save file data
    valid_channel_number = 0;
    for (TUint index = 0; index < DSYSTEM_MAX_CHANNEL_NUM && valid_channel_number < mpSimpleStorage->current_channel_number; index++) {
        if (!mpSimpleStorage->simple_storage_channel[index].enabled) {
            continue;
        }
        sprintf(read_string, "%s/%s/data.ini", mRootDir, mpSimpleStorage->simple_storage_channel[index].channel_name);
        EECode err = mConfigApi->OpenConfigFile((const TChar *)read_string, 1, 0, 1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("OpenConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            return EECode_InternalLogicalBug;
        }

        SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
        if (DUnlikely(!p_channel->filelist)) {
            //LOG_ERROR("cannot save channel data, filelist=%p\n", p_channel->filelist);
            err = mConfigApi->SetContentValue("filenumber", (TChar *)"0");
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetContentValue(filenumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = mConfigApi->CloseConfigFile();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                    return EECode_InternalLogicalBug;
                }
                return EECode_InternalLogicalBug;
            }
            err = mConfigApi->SaveConfigFile(NULL);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SaveConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                err = mConfigApi->CloseConfigFile();
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                    return EECode_InternalLogicalBug;
                }
                return EECode_InternalLogicalBug;
            }
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            continue;//if 0 files in channel, just set filenumber and continue to check next channel
        }
        sprintf(read_string1, "%d", p_channel->filelist->NumberOfNodes());
        err = mConfigApi->SetContentValue("filenumber", (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(filenumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }

        CIDoubleLinkedList::SNode *pnode = p_channel->filelist->FirstNode();
        TUint index1 = 0;
        while (pnode) {
            SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
            if (pfile) {
                sprintf(read_string1, "file_%d_starttime", index1);
                sprintf(read_string2, "%04d-%02d-%02d-%02d-%02d-%02d", pfile->start_time.year, pfile->start_time.month, pfile->start_time.day, pfile->start_time.hour, pfile->start_time.minute, pfile->start_time.seconds);
                err = mConfigApi->SetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }

                sprintf(read_string1, "file_%d_name", index1);
                err = mConfigApi->SetContentValue((const TChar *)read_string1, pfile->file_name);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }

                sprintf(read_string1, "file_%d_reference_cnt", index1);
                sprintf(read_string2, "%d", pfile->reference_count);
                err = mConfigApi->SetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }

                sprintf(read_string1, "file_%d_duration_minutes", index1);
                sprintf(read_string2, "%d", pfile->file_duration_minutes);
                err = mConfigApi->SetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }

                sprintf(read_string1, "file_%d_write_flag", index1);
                sprintf(read_string2, "%d", pfile->write_flag);
                err = mConfigApi->SetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }
            }
            pnode = p_channel->filelist->NextNode(pnode);
            index1++;
        }

        err = mConfigApi->SaveConfigFile(NULL);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SaveConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        }
        err = mConfigApi->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            return EECode_InternalLogicalBug;
        }
        valid_channel_number++;
    }

    return EECode_OK;
}

void CSimpleStorageManagement::inner_clear_channel_storage(SSimpleStorageChannel *p_channel)
{
    if (p_channel->filelist) {
        while (p_channel->filelist->LastNode()) {
            if ((SSimpleStorageFile *)p_channel->filelist->LastNode()->p_context) {
                free((SSimpleStorageFile *)p_channel->filelist->LastNode()->p_context);
            }
            p_channel->filelist->RemoveContent(p_channel->filelist->LastNode()->p_context);
        }
        DASSERT(0 == p_channel->filelist->NumberOfNodes());
    }
}
/*
EECode CSimpleStorageManagement::LoadDataBaseFromFile(TChar* config_file, TU32& channel_number)
{
    AUTO_LOCK(mpMutex);
    EECode err = inner_load_database_from_file(config_file, channel_number);
    return err;
}
*/
EECode CSimpleStorageManagement::inner_load_database_from_file(TChar *config_file, TU32 &channel_number)
{
    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TChar read_string2[256] = {0};
    TU32 val = 0;
    TInt tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0, tmp5 = 0, tmp6 = 0;

    //load channel data
    EECode err = mConfigApi->OpenConfigFile((const TChar *)config_file, 0, 0, 1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("OpenConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
        return EECode_InternalLogicalBug;
    }

    err = mConfigApi->GetContentValue("channelnumber", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(channelnumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        err = mConfigApi->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
            return EECode_InternalLogicalBug;
        }
        return EECode_InternalLogicalBug;
    } else {
        sscanf(read_string, "%d", &val);
        if (DSYSTEM_MAX_CHANNEL_NUM < val) {
            LOG_ERROR("inner_load_database_from_file, invalid paras, val=%u(max channel number %d)\n", val, DSYSTEM_MAX_CHANNEL_NUM);
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_BadParam;
        }
        channel_number = mpSimpleStorage->current_channel_number = val;
        LOG_INFO("channelnumber:%s, %d\n", read_string, mpSimpleStorage->current_channel_number);
    }

    for (TUint index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        sprintf(read_string, "channel_%d_name", index);
        err = mConfigApi->GetContentValue((const TChar *)read_string, mpSimpleStorage->simple_storage_channel[index].channel_name);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        } else {
            LOG_INFO("%s: %s\n", read_string, mpSimpleStorage->simple_storage_channel[index].channel_name);
        }

        sprintf(read_string, "channel_%d_enabled", index);
        err = mConfigApi->GetContentValue((const TChar *)read_string, (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        } else {
            sscanf(read_string1, "%d", &val);
            mpSimpleStorage->simple_storage_channel[index].enabled = val;
            LOG_INFO("%s: %d\n", read_string, mpSimpleStorage->simple_storage_channel[index].enabled);
        }

        sprintf(read_string, "channel_%d_max_file_number", index);
        err = mConfigApi->GetContentValue((const TChar *)read_string, (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        } else {
            sscanf(read_string1, "%d", &val);
            mpSimpleStorage->simple_storage_channel[index].max_storage_file_number = val;
            LOG_INFO("%s: %d\n", read_string, mpSimpleStorage->simple_storage_channel[index].max_storage_file_number);
        }

        sprintf(read_string, "channel_%d_redudant_file_number", index);
        err = mConfigApi->GetContentValue((const TChar *)read_string, (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        } else {
            sscanf(read_string1, "%d", &val);
            mpSimpleStorage->simple_storage_channel[index].redudant_storage_file_number = val;
            LOG_INFO("%s: %d\n", read_string, mpSimpleStorage->simple_storage_channel[index].redudant_storage_file_number);
        }

        sprintf(read_string, "channel_%d_single_file_duration_minutes", index);
        err = mConfigApi->GetContentValue((const TChar *)read_string, (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
                return EECode_InternalLogicalBug;
            }
            return EECode_InternalLogicalBug;
        } else {
            sscanf(read_string1, "%d", &val);
            mpSimpleStorage->simple_storage_channel[index].single_file_duration_minutes = (TU8)val;
            LOG_INFO("%s: %d\n", read_string, mpSimpleStorage->simple_storage_channel[index].single_file_duration_minutes);
        }
    }

    err = mConfigApi->CloseConfigFile();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", config_file, err, gfGetErrorCodeString(err));
        return EECode_InternalLogicalBug;
    }

    //load file data
    //for every channel, check whether file list exist, if not, creat doublelist first; if yes, clear doublelist first, then insert node from loaded data
    for (TUint index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
        if (DUnlikely(!p_channel->filelist)) {
            p_channel->filelist = new CIDoubleLinkedList();
            if (DUnlikely(!p_channel->filelist)) {
                LOG_FATAL("inner_load_database_from_file, creat CIDoubleLinkedList fail\n");
                return EECode_NoMemory;
            }
        } else {
            inner_clear_channel_storage(p_channel);
        }

        sprintf(read_string, "%s/%s/data.ini", mRootDir, mpSimpleStorage->simple_storage_channel[index].channel_name);
        EECode err = mConfigApi->OpenConfigFile((const TChar *)read_string, 0, 0, 1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("inner_load_database_from_file, OpenConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            continue;//return EECode_InternalLogicalBug;
        }

        TInt filenumber = 0;
        err = mConfigApi->GetContentValue("filenumber", (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("inner_load_database_from_file, GetContentValue(filenumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            err = mConfigApi->CloseConfigFile();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                continue;//return EECode_InternalLogicalBug;
            }
            continue;//return EECode_InternalLogicalBug;
        } else {
            sscanf(read_string1, "%d", &filenumber);
            LOG_INFO("channel %s filenumber: %d\n", mpSimpleStorage->simple_storage_channel[index].channel_name, filenumber);
        }

        for (TInt index1 = filenumber - 1; index1 >= 0; index1--) {

            if (p_channel->filelist->NumberOfNodes() < p_channel->max_storage_file_number + p_channel->redudant_storage_file_number) {
                if (p_channel->filelist->NumberOfNodes() >= p_channel->max_storage_file_number) {
                    LOG_WARN("inner_load_database_from_file, in channel %s reach max_storage_file_number=%d, creat new unit in redudant storage now.\n", mpSimpleStorage->simple_storage_channel[index].channel_name, p_channel->max_storage_file_number);
                }

                SSimpleStorageFile *p_new_file = (SSimpleStorageFile *)DDBG_MALLOC(sizeof(SSimpleStorageFile), "STFL");
                if (DUnlikely(!p_new_file)) {
                    LOG_FATAL("inner_load_database_from_file, new file %d for channel %s fail\n", index1, mpSimpleStorage->simple_storage_channel[index].channel_name);
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_NoMemory;
                }
                memset(p_new_file, 0x0, sizeof(SSimpleStorageFile));

                sprintf(read_string1, "file_%d_starttime", index1);
                err = mConfigApi->GetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("inner_load_database_from_file, GetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        continue;//return EECode_InternalLogicalBug;
                    }
                    continue;//return EECode_InternalLogicalBug;
                } else {
                    sscanf(read_string2, "%d-%d-%d-%d-%d-%d", &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6);
                    p_new_file->start_time.year = (TU16)tmp1;
                    p_new_file->start_time.month = (TU8)tmp2;
                    p_new_file->start_time.day = (TU8)tmp3;
                    p_new_file->start_time.hour = (TU8)tmp4;
                    p_new_file->start_time.minute = (TU8)tmp5;
                    p_new_file->start_time.seconds = (TU8)tmp6;
                    LOG_INFO("%s: %hu-%hu-%hu-%hu-%hu-%hu\n", read_string1, p_new_file->start_time.year, p_new_file->start_time.month, p_new_file->start_time.day, p_new_file->start_time.hour, p_new_file->start_time.minute, p_new_file->start_time.seconds);
                }

                sprintf(read_string1, "file_%d_name", index1);
                err = mConfigApi->GetContentValue((const TChar *)read_string1, p_new_file->file_name);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("inner_load_database_from_file, GetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        continue;//return EECode_InternalLogicalBug;
                    }
                    continue;//return EECode_InternalLogicalBug;
                } else {
                    LOG_INFO("%s: %s\n", read_string1, p_new_file->file_name);
                }

                sprintf(read_string1, "file_%d_reference_cnt", index1);
                err = mConfigApi->GetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("inner_load_database_from_file, GetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        continue;//return EECode_InternalLogicalBug;
                    }
                    continue;//return EECode_InternalLogicalBug;
                } else {
                    sscanf(read_string2, "%d", &tmp1);
                    p_new_file->reference_count = (TU16)tmp1;
                    LOG_INFO("%s: %hu\n", read_string1, p_new_file->reference_count);
                }

                sprintf(read_string1, "file_%d_duration_minutes", index1);
                err = mConfigApi->GetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("inner_load_database_from_file, GetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        continue;//return EECode_InternalLogicalBug;
                    }
                    continue;//return EECode_InternalLogicalBug;
                } else {
                    sscanf(read_string2, "%d", &tmp1);
                    p_new_file->file_duration_minutes = (TU16)tmp1;
                    LOG_INFO("%s: %hu\n", read_string1, p_new_file->file_duration_minutes);
                }

                sprintf(read_string1, "file_%d_write_flag", index1);
                err = mConfigApi->GetContentValue((const TChar *)read_string1, read_string2);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("inner_load_database_from_file, GetContentValue(%s) fail, ret %d, %s.\n", read_string1, err, gfGetErrorCodeString(err));
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        continue;//return EECode_InternalLogicalBug;
                    }
                    continue;//return EECode_InternalLogicalBug;
                } else {
                    sscanf(read_string2, "%d", &tmp1);
                    p_new_file->write_flag = (TU8)tmp1;
                    LOG_INFO("%s: %hu\n", read_string1, p_new_file->write_flag);
                }

                CIDoubleLinkedList::SNode *pnewnode  = p_channel->filelist->InsertContent(NULL, (void *)p_new_file);
                if (DUnlikely(!pnewnode)) {
                    LOG_FATAL("inner_load_database_from_file, add new file to filelist for channel %s fail\n", mpSimpleStorage->simple_storage_channel[index].channel_name);
                    err = mConfigApi->CloseConfigFile();
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                        return EECode_InternalLogicalBug;
                    }
                    return EECode_InternalLogicalBug;
                }

            } else {
                LOG_WARN("inner_load_database_from_file, in channel %s reach max_storage_file_number+redudant_storage_file_number=%d, cannot load anymore.\n", mpSimpleStorage->simple_storage_channel[index].channel_name, p_channel->max_storage_file_number + p_channel->redudant_storage_file_number);
                break;
            }
        }

        err = mConfigApi->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            continue;//return EECode_InternalLogicalBug;
        }

    }

    return EECode_OK;
}

TU8 *CSimpleStorageManagement::find_pridata_pesheader(TU8 *p_data, TInt size)
{
    TU8 *ptr = p_data;
    if (DUnlikely(NULL == ptr)) {
        return NULL;
    }

    while (ptr < p_data + size - 4) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x01) {
                    if (*(ptr + 3) == 0xBF) {
                        return ptr;
                    }
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

SSimpleStorageFile *CSimpleStorageManagement::try_load_single_file(TChar *filename)
{
    DASSERT(filename);

    TU32 filename_len_plus_1 = strlen(filename) + 1;
    if (DUnlikely(DMaxFileNameLength < filename_len_plus_1)) {
        LOG_ERROR("filename too long %d\n", filename_len_plus_1 - 1);
        return NULL;
    }

    FILE *pf = NULL;
    TU8 *p_data = NULL;
    SSimpleStorageFile *p_new_file = NULL;

    do {
        TU8 *p_cur = NULL;
        TU16 src_len = 0, read_len = 0, ts_pak_len = 188, check_pak_num = 1;
        EECode err = EECode_OK;
        TInt ret = 0;

        pf = fopen(filename, "rb");
        if (DUnlikely(!pf)) {
            LOG_WARN("can not open file %s\n", filename);
            break;
        }

        //seek to 188*2, retrieve data after PMT
        ret = fseek(pf, 2 * ts_pak_len, SEEK_SET);
        if (DUnlikely(0 != ret)) {
            LOG_WARN("fseek to 2*188 from head failed.\n");
            break;
        }
        p_data = (TU8 *)DDBG_MALLOC(ts_pak_len * check_pak_num, "STTP");
        if (DUnlikely(!p_data)) {
            LOG_WARN("cannot DDBG_MALLOC 1*188 mem to retrieve data.\n");
            break;
        }
        read_len = fread(p_data, 1, ts_pak_len * check_pak_num, pf);
        if (DUnlikely(read_len != ts_pak_len * check_pak_num)) {
            LOG_WARN("cannot read data correctly, read_len=%u(should==%u)\n", read_len, ts_pak_len * check_pak_num);
            break;
        }
        if (DUnlikely(!(p_cur = find_pridata_pesheader(p_data, ts_pak_len * check_pak_num)))) {
            LOG_WARN("load_database_from_filesystem, can not find pridata pesheader.\n");
            break;
        }
        //now p_cur pointer to the beginning of pridata PES Header, 0x00 00 01 BF
        p_cur += 4;
        src_len = *((TU16 *)p_cur);
        if (DUnlikely(src_len < sizeof(STSPrivateDataHeader))) {
            LOG_WARN("load_database_from_filesystem, data src_len=%u(should>=%lu), please check the binary.\n",
                     src_len, (TULong)sizeof(STSPrivateDataHeader));
            break;
        }
        p_cur += 2;

        p_new_file = (SSimpleStorageFile *)DDBG_MALLOC(sizeof(SSimpleStorageFile), "STNF");
        if (DUnlikely(!p_new_file)) {
            LOG_FATAL("no memory\n");
            break;
        }
        memset(p_new_file, 0x0, sizeof(SSimpleStorageFile));

        //init start time
        p_new_file->start_time.year = (TU16)0;
        p_new_file->start_time.month = (TU8)0;
        p_new_file->start_time.day = (TU8)0;
        p_new_file->start_time.hour = (TU8)0;
        p_new_file->start_time.minute = (TU8)0;
        //parse ts file to get start time

        strcpy(p_new_file->file_name, filename);
        p_new_file->reference_count = 0;
        p_new_file->file_duration_minutes = 3;
        p_new_file->write_flag = 0;

        //now p_cur pointer to the beginning of STSPrivateDataHeader
        err = gfRetrieveTSPriData((void *) & (p_new_file->start_time), sizeof(p_new_file->start_time), p_cur, src_len, ETSPrivateDataType_StartTime, 0);
        if (DUnlikely(EECode_OK != err)) {
            LOG_WARN("gfRetrieveTSPriData, err = %d, %s\n", err, gfGetErrorCodeString(err));
            break;
        }

    } while (0);

    if (p_data) {
        free(p_data);
        p_data = NULL;
    }
    if (pf) {
        fclose(pf);
        pf = NULL;
    }

    return p_new_file;
}

EECode CSimpleStorageManagement::load_database_from_filesystem(TChar *path, TInt depth_limited, TInt depth)
{
    DIR *d;
    struct dirent *file;
    TChar dirPath[FILENAME_MAX];
    TChar filePath[FILENAME_MAX];

    if (DUnlikely(!(d = opendir(path)))) {
        LOG_ERROR("error opendir %s!!!\n", path);
        return EECode_IOError;
    }

    while ((file = readdir(d)) != NULL) {

        if ((strncmp(file->d_name, ".", 1) == 0) || (strncmp(file->d_name, "..", 2) == 0)) {
            continue;
        }

#ifdef BUILD_OS_WINDOWS
        LOG_FATAL("no d_type\n");
        if (0) {
#else
        if (4 == file->d_type) {// file->d_type: 4-directory
#endif
            if (0 == depth) {
                TU32 index = mpSimpleStorage->current_channel_number;
                strcpy(mpSimpleStorage->simple_storage_channel[index].channel_name, file->d_name);
                mpSimpleStorage->simple_storage_channel[index].enabled = 1;
                mpSimpleStorage->simple_storage_channel[index].max_storage_file_number = 20;
                mpSimpleStorage->simple_storage_channel[index].redudant_storage_file_number = 10;
                mpSimpleStorage->simple_storage_channel[index].single_file_duration_minutes = 3;
                SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
                if (DLikely(!p_channel->filelist)) {
                    p_channel->filelist = new CIDoubleLinkedList();
                    if (DUnlikely(!p_channel->filelist)) {
                        LOG_FATAL("load_database_from_filesystem, creat CIDoubleLinkedList fail\n");
                        return EECode_NoMemory;
                    }
                } else {
                    inner_clear_channel_storage(p_channel);
                }
                /*for (TU32 i=0; i<=mpSimpleStorage->current_channel_number; i++){
                    printf("chidx=%u, chnname=%s, maxfilenum=%hu, redunatfuilenum=%hu, singlefiledurationminutes=%hu, enabled=%hu, filelist=%p\n", i,mpSimpleStorage->simple_storage_channel[i].channel_name,
                        mpSimpleStorage->simple_storage_channel[i].max_storage_file_number,
                        mpSimpleStorage->simple_storage_channel[i].redudant_storage_file_number,
                        mpSimpleStorage->simple_storage_channel[i].single_file_duration_minutes,
                        mpSimpleStorage->simple_storage_channel[i].enabled,
                        mpSimpleStorage->simple_storage_channel[index].filelist);
                }*/
            }
            if ((depth + 1) >= depth_limited) {
                //LOG_WARN("out of depth range, depth=%d, depth_limited=%d\n", depth, depth_limited);
                continue;
            }
            sprintf(dirPath, "%s/%s", path, file->d_name);
            load_database_from_filesystem(dirPath, depth_limited, depth + 1);
            if (0 == depth) {
                mpSimpleStorage->current_channel_number++;
            }
#ifdef BUILD_OS_WINDOWS
            LOG_FATAL("no d_type\n");
        } else if (1) {
#else
        } else if (8 == file->d_type) {// file->d_type: 8-file
#endif
            sprintf(filePath, "%s/%s", path, file->d_name);
            if (2 == depth) {
                TU32 index = mpSimpleStorage->current_channel_number;
                SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
                if (p_channel->filelist->NumberOfNodes() < p_channel->max_storage_file_number + p_channel->redudant_storage_file_number) {
                    if (p_channel->filelist->NumberOfNodes() >= p_channel->max_storage_file_number) {
                        LOG_WARN("load_database_from_filesystem, in channel %s reach max_storage_file_number=%d, creat new unit in redudant storage now.\n", mpSimpleStorage->simple_storage_channel[index].channel_name, p_channel->max_storage_file_number);
                    }
                    SSimpleStorageFile *p_new_file = try_load_single_file(filePath);
                    if (p_new_file) {
                        CIDoubleLinkedList::SNode *pnewnode  = p_channel->filelist->InsertContent(NULL, (void *)p_new_file);
                        if (DUnlikely(!pnewnode)) {
                            LOG_FATAL("load_database_from_filesystem, add new file to filelist for channel %s fail\n", mpSimpleStorage->simple_storage_channel[index].channel_name);
                            return EECode_InternalLogicalBug;
                        }
                        LOG_PRINTF("load [%s] into data base\n", filePath);
                    } else {
                        LOG_WARN("skip file %s\n", filePath);
                    }
                } else {
                    LOG_WARN("load_database_from_filesystem, in channel %s reach max_storage_file_number+redudant_storage_file_number=%d, cannot load anymore.\n", mpSimpleStorage->simple_storage_channel[index].channel_name, p_channel->max_storage_file_number + p_channel->redudant_storage_file_number);
                    break;
                }
            }
        } else {
#ifdef BUILD_OS_WINDOWS
            LOG_WARN("not supported, skip file->d_name=%s.\n", file->d_name);
#else
            LOG_WARN("not supported file->d_type=%d, skip file->d_name=%s.\n", file->d_type, file->d_name);
#endif
            continue;
        }
    }
    closedir(d);
    return EECode_OK;
}

EECode CSimpleStorageManagement::LoadDataBase(TChar *root_dir, TU32 &channel_number)
{
    TChar config_string[256] = {0};
    EECode err = EECode_OK;

    if (DUnlikely(NULL == root_dir)) {
        LOG_ERROR("LoadDataBase, invalid paras, root_dir=%p\n", root_dir);
        return EECode_BadParam;
    }

    AUTO_LOCK(mpMutex);

    if (DUnlikely(mRootDir)) {
        DDBG_FREE(mRootDir, "STRT");
        mRootDir = NULL;
        mRootDirNameLength = 0;
    }

    mRootDirNameLength = strlen(root_dir);
    mRootDir = (TChar *)DDBG_MALLOC(mRootDirNameLength + 4, "STRT");
    if (DLikely(mRootDir)) {
        strcpy(mRootDir, root_dir);
    } else {
        LOG_FATAL("LoadDataBase, No Memory\n");
        mRootDirNameLength = 0;
        return EECode_NoMemory;
    }

    sprintf(config_string, "%s/%s", mRootDir, DDataBaseMainFileDefaultName);
    LOG_NOTICE("mRootDir [%s], config_string [%s]\n", mRootDir, config_string);

    err = inner_load_database_from_file(config_string, channel_number);
    if (DUnlikely(EECode_OK != err)) {
        LOG_WARN("load from config file fail, ret %d, %s, start scan whole root dir [%s]\n", err, gfGetErrorCodeString(err), mRootDir);
        //parse file system and load to database
        TInt depth = 0, depth_limited = 3;//default directory depth of storage data base
        load_database_from_filesystem(mRootDir, depth_limited, depth);
        channel_number = mpSimpleStorage->current_channel_number;
    }

    return EECode_OK;
}

void CSimpleStorageManagement::inner_remove_channel(SSimpleStorageChannel *p_channel)
{
    if (p_channel->filelist) {
        inner_clear_channel_storage(p_channel);
        delete p_channel->filelist;
        p_channel->filelist = NULL;
    }
    memset(p_channel, 0x0, sizeof(SSimpleStorageChannel));
}

EECode CSimpleStorageManagement::ClearDataBase(TChar *root_dir, TU32 channel_number)
{
    AUTO_LOCK(mpMutex);
    if (strcmp(mRootDir, root_dir) || mpSimpleStorage->current_channel_number != channel_number) {
        LOG_ERROR("ClearDataBase() fail, root_dir=%s, channel_number=%u invalid.\n", root_dir, channel_number);
        return EECode_InternalLogicalBug;
    }
    /*if (mRootDir) {
        free(mRootDir);
        mRootDir = NULL;
        mRootDirNameLength = 0;
    }*/
    for (TU32 index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        inner_remove_channel(&mpSimpleStorage->simple_storage_channel[index]);
    }
    mpSimpleStorage->current_channel_number = 0;
    return EECode_OK;
}

EECode CSimpleStorageManagement::AddChannel(TChar *channel_name, TU32 max_storage_minutes, TU32 redudant_storage_minutes, TU32 single_file_duration_minutes)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    TChar path[256] = "";
    if (DUnlikely(0 == single_file_duration_minutes)) {
        LOG_FATAL("AddChannel single_file_duration_minutes=0 invalid\n");
        return EECode_InternalLogicalBug;
    }
    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            LOG_ERROR("AddChannel %s already exists.\n", channel_name);
            return EECode_AlreadyExist;
        }
    }
    for (index = 0; index < DSYSTEM_MAX_CHANNEL_NUM; index++) {
        if (!mpSimpleStorage->simple_storage_channel[index].enabled) {
            break;
        }
    }
    if (index >= DSYSTEM_MAX_CHANNEL_NUM) {
        LOG_ERROR("AddChannel out of range, max channnel number=%d\n", DSYSTEM_MAX_CHANNEL_NUM);
        return EECode_InternalLogicalBug;
    }

    sprintf(path, "%s/%s", mRootDir, channel_name);

#ifdef BUILD_OS_WINDOWS
    //_mkdir(path);
    LOG_FATAL("to do, no _mkdir\n");
#else
    if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH) < 0) {
        if (EEXIST != errno) {
            LOG_FATAL("AddChannel creat dir %s fail\n", path);
            return EECode_OSError;
        }
    }
#endif

    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    strcpy(p_channel->channel_name, channel_name);
    p_channel->enabled = 1;
    p_channel->max_storage_file_number = max_storage_minutes / single_file_duration_minutes + 1;
    p_channel->redudant_storage_file_number = redudant_storage_minutes / single_file_duration_minutes + 1;
    p_channel->single_file_duration_minutes = (TU8)single_file_duration_minutes;
    p_channel->filelist = new CIDoubleLinkedList();
    if (DUnlikely(!p_channel->filelist)) {
        LOG_FATAL("AddChannel creat CIDoubleLinkedList fail\n");
        return EECode_NoMemory;
    }
    mpSimpleStorage->current_channel_number++;

    return EECode_OK;
}

EECode CSimpleStorageManagement::RemoveChannel(TChar *channel_name)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("RemoveChannel cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    inner_remove_channel(&mpSimpleStorage->simple_storage_channel[index]);
    if (mpSimpleStorage->current_channel_number) {
        mpSimpleStorage->current_channel_number--;
    }

    return EECode_OK;
}

EECode CSimpleStorageManagement::ClearChannelStorage(TChar *channel_name)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("ClearChannelStorage cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    inner_clear_channel_storage(&mpSimpleStorage->simple_storage_channel[index]);
    return EECode_OK;
}

EECode CSimpleStorageManagement::QueryChannelStorage(TChar *channel_name, void *&p_out, EQueryStorageType type)
{
    return EECode_NoImplement;
}

EECode CSimpleStorageManagement::RequestExistUint(TChar *channel_name, SDateTime *time, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    file_name = NULL;
    file_duration_minutes = 0;

    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("RequestExistUint cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    if (!p_channel->enabled) {
        LOG_ERROR("RequestExistUint failed, channel %s disabled.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    if (!p_channel->filelist) {
        LOG_ERROR("RequestExistUint failed, channel %s database empty.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    LOG_NOTICE("RequestExistUint, duration %u, request time: %u, %u, %u, %u, %u\n", p_channel->single_file_duration_minutes, time->year, time->month, time->day, time->hour, time->minute);

    TTime request_time = time->minute * 60 + time->seconds;
    TTime file_time = 0;
    TTime duration = p_channel->single_file_duration_minutes * 60;
    if (request_time <= duration) { request_time += time->hour * 3600; }

    CIDoubleLinkedList::SNode *pnode = p_channel->filelist->LastNode();
    while (pnode) {
        SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
        if (pfile) {
            //TODO, only to search the files in same day
            if (pfile->start_time.year == time->year &&
                    pfile->start_time.month == time->month &&
                    pfile->start_time.day == time->day &&
                    !pfile->write_flag) {

                if (time->minute > p_channel->single_file_duration_minutes) {
                    if (pfile->start_time.hour == time->hour) {
                        file_time = pfile->start_time.minute * 60 + pfile->start_time.seconds;
                        if ((request_time - file_time) < duration) {
                            file_name  = pfile->file_name;
                            *file_start_time = pfile->start_time;
                            file_duration_minutes = pfile->file_duration_minutes;
                            pfile->reference_count++;
                            return EECode_OK;
                        }
                    }
                } else {
                    if (DUnlikely(pfile->start_time.hour > time->hour)) {
                        LOG_NOTICE("RequestExistUint, hour level diff!!\n");
                        file_name  = pfile->file_name;
                        *file_start_time = pfile->start_time;
                        file_duration_minutes = pfile->file_duration_minutes;
                        pfile->reference_count++;
                        return EECode_OK;
                    } else if ((pfile->start_time.hour + 1) >= time->hour) {
                        file_time = pfile->start_time.hour * 3600 + pfile->start_time.minute * 60 + pfile->start_time.seconds;
                        if ((request_time - file_time) < duration) {
                            file_name  = pfile->file_name;
                            *file_start_time = pfile->start_time;
                            file_duration_minutes = pfile->file_duration_minutes;
                            pfile->reference_count++;
                            return EECode_OK;
                        }
                    }
                }
            }
        }
        pnode = p_channel->filelist->PreNode(pnode);
    }
    LOG_ERROR("RequestExistUint failed.\n");
    return EECode_BadParam;
}

EECode CSimpleStorageManagement::RequestExistUintSequentially(TChar *channel_name, TChar *old_file_name, TU8 direction, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    file_name = NULL;
    file_duration_minutes = 0;

    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("RequestExistUintSequentially cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    if (!p_channel->enabled) {
        LOG_ERROR("RequestExistUintSequentially failed, channel %s disabled.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    if (!p_channel->filelist) {
        LOG_ERROR("RequestExistUintSequentially failed, channel %s database empty.\n", channel_name);
        return EECode_InternalLogicalBug;
    }

    CIDoubleLinkedList::SNode *pnode = p_channel->filelist->FirstNode();
    CIDoubleLinkedList::SNode *pnode_tmp = NULL;
    if (DUnlikely(!pnode)) {
        LOG_ERROR("channel do not have file yet\n");
        return EECode_NotExist;
    }

    while (pnode) {
        SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
        if (DLikely(pfile)) {
            if (NULL == old_file_name) {//if not specify the old file name, return the latest file which not in write status
                if (!pfile->write_flag) {
                    file_name  = pfile->file_name;
                    *file_start_time = pfile->start_time;
                    file_duration_minutes = pfile->file_duration_minutes;
                    pfile->reference_count++;
                    return EECode_OK;
                }
            } else if (!strcmp(old_file_name, pfile->file_name)) {
                pnode_tmp = direction ? p_channel->filelist->NextNode(pnode) : p_channel->filelist->PreNode(pnode);
                if (pnode_tmp) {
                    SSimpleStorageFile *pfile_tmp = (SSimpleStorageFile *)pnode_tmp->p_context;
                    if (pfile_tmp) {
                        if (!pfile_tmp->write_flag) {
                            file_name  = pfile_tmp->file_name;
                            *file_start_time = pfile_tmp->start_time;
                            file_duration_minutes = pfile_tmp->file_duration_minutes;
                            pfile_tmp->reference_count++;
                            return EECode_OK;
                        }
                    }
                }
            }
        } else {
            LOG_WARN("NULL file %p\n", pfile);
        }
        pnode = p_channel->filelist->NextNode(pnode);
    }

    LOG_ERROR("RequestExistUintSequentially, do not find matched filename, input param (old_file_name %p).\n", old_file_name);
    return EECode_BadParam;
}

EECode CSimpleStorageManagement::ReleaseExistUint(TChar *channel_name, TChar *file_name)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("ReleaseExistUint cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    if (!p_channel->enabled) {
        LOG_ERROR("ReleaseExistUint failed, channel %s disabled.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    if (!p_channel->filelist) {
        LOG_ERROR("ReleaseExistUint failed, channel %s database empty.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    CIDoubleLinkedList::SNode *pnode = p_channel->filelist->FirstNode();
    while (pnode) {
        SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
        if (pfile) {
            if (!strcmp(pfile->file_name, file_name)) {
                if (0 == pfile->reference_count) {
                    LOG_ERROR("ReleaseExistUint logic bug, %s already no reference.\n", file_name);
                    return EECode_InternalLogicalBug;
                }
                pfile->reference_count--;
                return EECode_OK;
            }
        }
        pnode = p_channel->filelist->NextNode(pnode);
    }

    LOG_ERROR("ReleaseExistUint cannot find %s.\n", file_name);
    return EECode_BadParam;
}

EECode CSimpleStorageManagement::inner_acquire_new_uint(SSimpleStorageChannel *p_channel, TChar *channel_name, TChar *&file_name, CIDoubleLinkedList::SNode *p_replaced_node)
{
    TChar path[256] = "";
    SSimpleStorageFile *p_new_file = (SSimpleStorageFile *)DDBG_MALLOC(sizeof(SSimpleStorageFile), "STNF");
    if (DUnlikely(!p_new_file)) {
        LOG_FATAL("new file for channel %s fail\n", channel_name);
        return EECode_NoMemory;
    }
    memset(p_new_file, 0x0, sizeof(SSimpleStorageFile));

    SDateTime time = {0};
    EECode err = gfGetCurrentDateTime(&time);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetCurrentDateTime return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    TU8 new_dir = 0;
    if (p_channel->cur_dir_year != time.year || p_channel->cur_dir_month != time.month || p_channel->cur_dir_day != time.day) {
        p_channel->cur_dir_year = time.year;
        p_channel->cur_dir_month = time.month;
        p_channel->cur_dir_day = time.day;
        //sprintf(cur_dir, "%04d%02d%02d", p_channel->cur_dir_year, p_channel->cur_dir_month, p_channel->cur_dir_day);
        new_dir = 1;
    }
    p_new_file->start_time = time;
    p_new_file->write_flag = 1;
    p_new_file->file_duration_minutes = p_channel->single_file_duration_minutes;
    sprintf(path, "%s/%s/%04d%02d%02d", mRootDir, channel_name, p_channel->cur_dir_year, p_channel->cur_dir_month, p_channel->cur_dir_day);
    if (new_dir && (-1 == access(path, 0))) {
        //sprintf(path, "%s/%s/%04d%02d%02d", mRootDir, channel_name, p_channel->cur_dir_year, p_channel->cur_dir_month, p_channel->cur_dir_day);

#ifdef BUILD_OS_WINDOWS
        //_mkdir(path);
        LOG_FATAL("to do, no _mkdir\n");
#else
        if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH) < 0) {
            if (EEXIST != errno) {
                LOG_FATAL("inner_acquire_new_uint creat dir %s fail\n", path);
                return EECode_OSError;
            }
        }
#endif

    }

    if (p_replaced_node) {
        SSimpleStorageFile *pfile = (SSimpleStorageFile *)p_replaced_node->p_context;
        /*sprintf(cmd, "rm -r %s", pfile->file_name);
        LOG_INFO("%s\n", cmd);
        system(cmd);*/
        if (remove((const TChar *)pfile->file_name) < 0) {
            LOG_FATAL("inner_acquire_new_uint remove file %s fail\n", pfile->file_name);
            //return EECode_OSError;
        }
        p_channel->filelist->RemoveContent(p_replaced_node->p_context);
        free(p_replaced_node->p_context);
    }
    sprintf(p_new_file->file_name, "%s/%s/%04d%02d%02d/%02d-%02d.ts", mRootDir, channel_name, p_channel->cur_dir_year, p_channel->cur_dir_month, p_channel->cur_dir_day, time.hour, time.minute);
    CIDoubleLinkedList::SNode *pnewnode  = p_channel->filelist->InsertContent(NULL, (void *)p_new_file);
    if (DUnlikely(!pnewnode)) {
        LOG_FATAL("add new file to filelist for channel %s fail\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    file_name = p_new_file->file_name;
    return EECode_OK;
}

EECode CSimpleStorageManagement::AcquireNewUint(TChar *channel_name, TChar *&file_name)
{
    EECode ret = EECode_OK;
    TU32 index = 0;
    AUTO_LOCK(mpMutex);

    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("AcquireNewUint cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    if (!p_channel->enabled) {
        LOG_ERROR("AcquireNewUint failed, channel %s disabled.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    if (!p_channel->filelist) {
        LOG_ERROR("AcquireNewUint failed, channel %s database empty.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    //filelist node number [0, max_storage_file_number+redudant_storage_file_number), new unit
    if (p_channel->filelist->NumberOfNodes() < p_channel->max_storage_file_number + p_channel->redudant_storage_file_number) {
        if (p_channel->filelist->NumberOfNodes() >= p_channel->max_storage_file_number) {
            LOG_WARN("AcquireNewUint reach max_storage_file_number=%d, creat new unit in redudant storage now.\n", p_channel->max_storage_file_number);
        }
        if (EECode_OK != (ret = inner_acquire_new_uint(p_channel, channel_name, file_name,  NULL))) {
            return ret;
        }
        return EECode_OK;
    } else {//else look for unit in redudant storgae files to replace
        TUint redudant_file_cnt = 0;
        CIDoubleLinkedList::SNode *pnode = p_channel->filelist->LastNode();
        while ((redudant_file_cnt < p_channel->redudant_storage_file_number) && pnode) {
            SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
            if (pfile) {
                if (0 == pfile->reference_count) { //first choice: redudant storgae file whose reference count ==0
                    if (EECode_OK != (ret = inner_acquire_new_uint(p_channel, channel_name, file_name,  pnode))) {
                        return ret;
                    }
                    return EECode_OK;
                }
            }
            pnode = p_channel->filelist->PreNode(pnode);
            redudant_file_cnt++;
        }
        //second choice: all redudant storage files referenced, then replace the last node in filelist(oldest file)
        LOG_WARN("AcquireNewUint, no redudant file is free, we have to replace the oldest one.\n");
        if (EECode_OK != (ret = inner_acquire_new_uint(p_channel, channel_name, file_name,  p_channel->filelist->LastNode()))) {
            return ret;
        }
        return EECode_OK;
    }
}

EECode CSimpleStorageManagement::FinalizeNewUint(TChar *channel_name, TChar *file_name)
{
    AUTO_LOCK(mpMutex);
    TU32 index = 0;
    for (index = 0; index < mpSimpleStorage->current_channel_number; index++) {
        if (!strcmp(mpSimpleStorage->simple_storage_channel[index].channel_name, channel_name)) {
            break;
        }
    }
    if (index >= mpSimpleStorage->current_channel_number) {
        LOG_ERROR("FinalizeNewUint cannot find %s\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    SSimpleStorageChannel *p_channel = &mpSimpleStorage->simple_storage_channel[index];
    if (!p_channel->enabled) {
        LOG_ERROR("FinalizeNewUint failed, channel %s disabled.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    if (!p_channel->filelist) {
        LOG_ERROR("FinalizeNewUint failed, channel %s database empty.\n", channel_name);
        return EECode_InternalLogicalBug;
    }
    CIDoubleLinkedList::SNode *pnode = p_channel->filelist->FirstNode();
    while (pnode) {
        SSimpleStorageFile *pfile = (SSimpleStorageFile *)pnode->p_context;
        if (pfile) {
            if (!strcmp(pfile->file_name, file_name) && pfile->write_flag) {
                pfile->write_flag = 0;
                return EECode_OK;
            }
        }
        pnode = p_channel->filelist->NextNode(pnode);
    }
    LOG_ERROR("FinalizeNewUint failed.\n");
    return EECode_InternalLogicalBug;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

