/*******************************************************************************
 * common_io.h
 *
 * History:
 *  2013/09/09 - [Zhi He] create file
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

#ifndef __COMMON_IO_H__
#define __COMMON_IO_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    EIOPosition_NotSpecified = 0,
    EIOPosition_Begin,
    EIOPosition_End,
    EIOPosition_Current,
} EIOPosition;

typedef enum {
    EIOType_File = 0,
    EIOType_HTTP,
} EIOType;

enum {
    EIOFlagBit_Read = (1 << 0),
    EIOFlagBit_Write = (1 << 1),
    EIOFlagBit_Append = (1 << 2),
    EIOFlagBit_Text = (1 << 3),
    EIOFlagBit_Binary = (1 << 4),
    EIOFlagBit_Exclusive = (1 << 5),
};

class IIO
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IIO() {}

public:
    virtual EECode Open(TChar *name, TU32 flags) = 0;
    virtual EECode Close() = 0;

    virtual EECode SetProperty(TIOSize write_block_size, TIOSize read_block_size) = 0;
    virtual EECode GetProperty(TIOSize &write_block_size, TIOSize &read_block_size) const = 0;

    virtual EECode Write(TU8 *pdata, TU32 size, TIOSize count, TU8 sync = 0) = 0;
    virtual EECode Read(TU8 *pdata, TU32 size, TIOSize &count) = 0;

    virtual EECode Seek(TIOSize offset, EIOPosition posision) = 0;
    virtual void Sync() = 0;

    virtual EECode Query(TIOSize &total_size, TIOSize &current_posotion) const = 0;
};

extern IIO *gfCreateIO(EIOType type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

