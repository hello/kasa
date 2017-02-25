/*******************************************************************************
 * runtime_config_xml.cpp
 *
 * History:
 *  2014/03/23 - [Zhi He] create file
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

#include "runtime_config_simple_ini.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IRunTimeConfigAPI *gfCreateSimpleINIRunTimeConfigAPI(TU32 max)
{
    IRunTimeConfigAPI *api = NULL;

    CSimpleINIRunTimeConfigAPI *thiz = new CSimpleINIRunTimeConfigAPI(max);
    if (thiz) {
        api = (IRunTimeConfigAPI *) thiz;
        return api;
    }

    LOG_FATAL("new CSimpleINIRunTimeConfigAPI() fail\n");
    return NULL;
}

CSimpleINIRunTimeConfigAPI::CSimpleINIRunTimeConfigAPI(TU32 max)
    : CObject("CSimpleINIRunTimeConfigAPI")
    , mMaxLine(max)
    , mCurrentCount(0)
    , mpContentList(NULL)
    , mpFile(NULL)
{
}

CSimpleINIRunTimeConfigAPI::~CSimpleINIRunTimeConfigAPI()
{

}

void CSimpleINIRunTimeConfigAPI::Destroy()
{
    Delete();
}

void CSimpleINIRunTimeConfigAPI::Delete()
{
    if (mpFile) {
        fclose(mpFile);
        mpFile = NULL;
    }

    if (mpContentList) {
        free(mpContentList);
        mpContentList = NULL;
    }
}

EECode CSimpleINIRunTimeConfigAPI::OpenConfigFile(const TChar *config_file_name, TU8 read_or_write, TU8 read_once, TU8 number)
{
    if (!mpContentList) {
        DASSERT(mMaxLine);
        mpContentList = (SSimpleINIContent *) DDBG_MALLOC(mMaxLine * sizeof(SSimpleINIContent), "C0SC");
        DASSERT(mpContentList);
        if (DUnlikely(!mpContentList)) {
            LOGM_FATAL("not enough memory, request (%ux%lu = %lu)\n", mMaxLine, (TULong)sizeof(SSimpleINIContent), mMaxLine * (TULong)sizeof(SSimpleINIContent));
            return EECode_NoMemory;
        }
        memset(mpContentList, 0x0, mMaxLine * sizeof(SSimpleINIContent));
    } else {
        memset(mpContentList, 0x0, mMaxLine * sizeof(SSimpleINIContent));
    }

    mCurrentCount = 0;

    if (mpFile) {
        LOG_ERROR("File already opened.\n");
        return EECode_Error;
    }
    if (!read_or_write) {//read
        mpFile = fopen(config_file_name, "rt");
        if (NULL == mpFile) {
            LOG_WARN("file %s can not be opended.\n", config_file_name);
            return EECode_Error;
        }

        TU32 current = 0;
        TChar read_string[2 * DMaxConfStringLength + 32] = {0};
        SSimpleINIContent *p_content = mpContentList;
        TChar *p = NULL;

        LOGM_NOTICE("read config file %s, max %d, once %d, number %d\n", config_file_name, mMaxLine, read_once, number);
        if (2 == number) {
            for (current = 0; current < mMaxLine; current ++) {
                memset(&read_string[0], 0x0, sizeof(read_string));
                p = fgets(&read_string[0], 2 * DMaxConfStringLength + 32, mpFile);
                if (!p) {
                    LOGM_ERROR("BAD file content, %s, current %d\n", config_file_name, current);
                    break;
                }
                p = strchr(read_string, '\n');
                if (p) {
                    *p = 0x0;
                }
                p = strchr(read_string, '\r');
                if (p) {
                    *p = 0x0;
                }

                LOGM_NOTICE("read_string %s\n", read_string);
                p = strchr(read_string, '=');
                if (p) {
                    *p = 0x0;
                } else {
                    LOGM_WARN("file end? %s, current %d\n", config_file_name, current);
                    break;
                }

                TChar *p1 = strchr(p + 1, ',');
                //LOGM_NOTICE("p1 %s\n", p1);
                if (p1) {
                    *p1 = 0x0;
                } else {
                    LOGM_ERROR("BAD file content, %s, current %d\n", config_file_name, current);
                    break;
                }

                strcpy(&p_content->content[0], read_string);
                strcpy(&p_content->value[0], p + 1);
                strcpy(&p_content->value1[0], p1 + 1);

                if (DUnlikely(feof(mpFile))) {
                    LOGM_CONFIGURATION("file eof, %s\n", config_file_name);
                    break;
                }
                p_content ++;
                mCurrentCount ++;
            }
        } else if (1 == number) {
            for (current = 0; current < mMaxLine; current ++) {
                memset(&read_string[0], 0x0, sizeof(read_string));
                p = fgets(&read_string[0], 2 * DMaxConfStringLength + 32, mpFile);
                if (!p) {
                    LOGM_NOTICE("config file(%s) end, read %d lines, max lines %d\n", config_file_name, current, mMaxLine);
                    break;
                }
                p = strchr(read_string, '\n');
                if (p) {
                    *p = 0x0;
                }
                p = strchr(read_string, '\r');
                if (p) {
                    *p = 0x0;
                }

                TChar *p = strchr(read_string, '=');
                if (p) {
                    *p = 0x0;
                } else {
                    LOGM_WARN("file end? %s, current %d\n", config_file_name, current);
                    break;
                }
                strcpy(&p_content->content[0], read_string);
                strcpy(&p_content->value[0], p + 1);

                if (DUnlikely(feof(mpFile))) {
                    LOGM_CONFIGURATION("file eof, %s\n", config_file_name);
                    break;
                }
                p_content ++;
            }
        } else {
            LOGM_ERROR("BAD number %d, %s\n", number, config_file_name);
        }
        LOGM_CONFIGURATION("read config file %s, %d\n", config_file_name, current);

    } else {//write
        mpFile = fopen(config_file_name, "wt");
        if (NULL == mpFile) {
            LOG_FATAL("file %s can not be opended.\n", config_file_name);
            return EECode_Error;
        }
    }

    mbReadOnce = read_once;
    mNumber = number;
    return  EECode_OK;
}

EECode CSimpleINIRunTimeConfigAPI::GetContentValue(const TChar *content_path, TChar *content_value, TChar *content_value1)
{
    if (DUnlikely((NULL == content_path) || (NULL == content_value))) {
        LOGM_ERROR("NULL content_path %p or NULL content_value %p\n", content_path, content_value);
        return EECode_BadParam;
    }

    TU32 current = 0;
    SSimpleINIContent *p_content = mpContentList;

    if (DUnlikely(NULL == p_content)) {
        LOGM_ERROR("NULL p_content\n");
        return EECode_BadState;
    }

    if (content_value1) {
        DASSERT(2 == mNumber);
        //DASSERT(mbReadOnce);
        for (current = 0; current < mMaxLine; current ++, p_content ++) {
            if (p_content->read && mbReadOnce) {
                continue;
            }
            if (!strcmp(content_path, p_content->content)) {
                strcpy(content_value, p_content->value);
                strcpy(content_value1, p_content->value1);
                p_content->read = 1;
                return EECode_OK;
            }
        }
    } else {
        DASSERT(1 == mNumber);
        //DASSERT(!mbReadOnce);
        for (current = 0; current < mMaxLine; current ++, p_content ++) {
            if (p_content->read && mbReadOnce) {
                continue;
            }
            if (!strcmp(content_path, p_content->content)) {
                strcpy(content_value, p_content->value);
                p_content->read = 1;
                return EECode_OK;
            }
        }
    }

    //LOGM_WARN("not find %s, end of file?\n", content_path);
    return EECode_NotExist;
}

EECode CSimpleINIRunTimeConfigAPI::SetContentValue(const TChar *content_path, TChar *content_value)
{
    if (DUnlikely((NULL == content_path) || (NULL == content_value))) {
        LOGM_ERROR("NULL content_path %p or NULL content_value %p\n", content_path, content_value);
        return EECode_BadParam;
    }

    TU32 current = 0;
    SSimpleINIContent *p_content = mpContentList;

    if (DUnlikely(NULL == p_content)) {
        LOGM_ERROR("NULL p_content\n");
        return EECode_BadState;
    }

    //if set the value of existed content, just modify it
    for (current = 0; current < mCurrentCount; current ++, p_content ++) {
        if (!strcmp(content_path, p_content->content)) {
            //strcpy(p_content->value,content_value);
            snprintf(p_content->value, DMaxConfStringLength, "%s", content_value);
            return EECode_OK;
        }
    }
    //if set one new content
    if (mCurrentCount >= mMaxLine) {
        LOGM_WARN("set new content %s out of range of file limitation %u\n", content_path, mMaxLine);
        return EECode_TooMany;
    }
    snprintf(p_content->content, DMaxConfStringLength, "%s", content_path);
    snprintf(p_content->value, DMaxConfStringLength, "%s", content_value);
    mCurrentCount ++;
    return EECode_OK;
}

EECode CSimpleINIRunTimeConfigAPI::SaveConfigFile(const TChar *config_file_name)
{
    FILE *pfile = NULL;
    if (config_file_name) {
        pfile = fopen(config_file_name, "wt");
        if (NULL == pfile) {
            LOG_FATAL("file %s can not be opended.\n", config_file_name);
            return EECode_Error;
        }
    } else if (mpFile) {
        pfile = mpFile;
    }
    SSimpleINIContent *p_content = mpContentList;
    if (DUnlikely(NULL == p_content)) {
        LOGM_ERROR("NULL p_content\n");
        return EECode_BadState;
    }
    TChar write_string[2 * DMaxConfStringLength + 32] = {0};
    TInt write_strlen = 0;
    for (TU32 i = 0; i < mCurrentCount; i++, p_content++) {
        if (mCurrentCount - 1 == i) { //last one, no add \n
            write_strlen = sprintf(write_string, "%s=%s", p_content->content, p_content->value);
        } else {
            write_strlen = sprintf(write_string, "%s=%s\n", p_content->content, p_content->value);
        }
        fwrite(write_string, write_strlen, 1, pfile);
    }

    if (config_file_name) {
        fclose(pfile);
    }
    return EECode_OK;
}

EECode CSimpleINIRunTimeConfigAPI::CloseConfigFile()
{
    if (mpFile) {
        fclose(mpFile);
        mpFile = NULL;
    }

    return EECode_OK;
}

ERunTimeConfigType CSimpleINIRunTimeConfigAPI::QueryConfigType() const
{
    return ERunTimeConfigType_SimpleINI;
}

EECode CSimpleINIRunTimeConfigAPI::NewFile(const TChar *root_name, void *&root_node)
{
    return EECode_OK;
}

void *CSimpleINIRunTimeConfigAPI::AddContent(void *p_parent, const TChar *content_name)
{
    return NULL;
}

void *CSimpleINIRunTimeConfigAPI::AddContentChild(void *p_parent, const TChar *child_name, const TChar *value)
{
    return NULL;
}

EECode CSimpleINIRunTimeConfigAPI::FinalizeFile(const TChar *file_name)
{
    if (mpFile) {
        fclose(mpFile);
        mpFile = NULL;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

