/*******************************************************************************
 * generic_storage.h
 *
 * History:
 *  2014/01/16 - [Zhi He] create file
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

#ifndef __GENERIC_STORAGE_H__
#define __GENERIC_STORAGE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CGenericStorageManagement
{
public:
    static Create();

protected:
    EECode Construct();

    CGenericStorageManagement();
    ~CGenericStorageManagement();

public:
    virtual EECode SetRootDir(TChar *root_dir);
    virtual EECode SetChannelNumber(TU32 channel_number);

public:
    virtual EECode SetupDataBase(TChar *root_dir, TU32 &channel_number);
    virtual EECode SaveDataBaseToFile(TChar *config_file);
    virtual EECode LoadDataBaseFromFile(TChar *config_file);

public:
    virtual EECode ClearChannelStorage(TU32 channel_index);
    virtual EECode ClearChannelStorage(TChar *channel_name);

    virtual EECode QueryChannelStorage(TU32 channel_index);
    virtual EECode QueryChannelStorage(TChar *channel_name);

public:
    virtual EECode RequestExistUint();
    virtual EECode ReleaseExistUint();

    virtual EECode AcquireNewUint();
    virtual EECode FinalizeNewUint();
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

