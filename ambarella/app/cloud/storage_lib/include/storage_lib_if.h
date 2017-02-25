/*******************************************************************************
 * storage_lib_if.h
 *
 * History:
 *  2014/01/12 - [Zhi He] create file
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

#ifndef __STORAGE_LIB_IF_H__
#define __STORAGE_LIB_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef struct {
#ifdef BIG_ENDIAN
    TU8 duty_cycle_bit : 1;
    TU8 length : 7;
#else
    TU8 length : 7;
    TU8 duty_cycle_bit : 1;
#endif
} STimeLineDutyCycleUint;

typedef struct {
    TU32 total_unit_number;

    TU8 unit_length; // how many minutes
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    STimeLineDutyCycleUint *p_units;
} STimeLineDutyCycle;

typedef struct {
    TU32 total_unit_number;

    TU8 unit_length; // how many minutes
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TU8 *p_units;
} STimeLineRawMask;

typedef enum {
    EQueryStorageType_dutyCycle,
    EQueryStorageType_rawMask,
} EQueryStorageType;

typedef enum {
    EStorageMnagementType_Invalid = 0,
    EStorageMnagementType_Simple = 1,
    EStorageMnagementType_Sqlite = 2,
} EStorageMnagementType;

class IStorageManagement
{
public:
    virtual EECode LoadDataBase(TChar *root_dir, TU32 &channel_number) = 0;//if config files exist, load data base from file; if config files NOT exist, load data base from file system
    virtual EECode ClearDataBase(TChar *root_dir, TU32 channel_number) = 0;
    virtual EECode SaveDataBase(TU32 &channel_number) = 0;

public:
    virtual EECode AddChannel(TChar *channel_name, TU32 max_storage_minutes, TU32 redudant_storage_minutes, TU32 single_file_duration_minutes) = 0;
    virtual EECode RemoveChannel(TChar *channel_name) = 0;

public:
    virtual EECode ClearChannelStorage(TChar *channel_name) = 0;
    virtual EECode QueryChannelStorage(TChar *channel_name, void *&p_out, EQueryStorageType type = EQueryStorageType_dutyCycle) = 0;

public:
    virtual EECode RequestExistUint(TChar *channel_name, SDateTime *time, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;
    virtual EECode RequestExistUintSequentially(TChar *channel_name, TChar *old_file_name, TU8 direction, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;//direction: 0-newer file, 1-older file
    virtual EECode ReleaseExistUint(TChar *channel_name, TChar *file_name) = 0;

    virtual EECode AcquireNewUint(TChar *channel_name, TChar *&file_name) = 0;
    virtual EECode FinalizeNewUint(TChar *channel_name, TChar *file_name) = 0;

public:
    virtual void Delete() = 0;

protected:
    virtual ~IStorageManagement() {}
};

IStorageManagement *gfCreateStorageManagement(EStorageMnagementType type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

