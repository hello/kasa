/*******************************************************************************
 * common_hash_table.h
 *
 * History:
 *    2014/08/14 - [Zhi He] create file
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

#ifndef __COMMON_HASH_TABLE_H__
#define __COMMON_HASH_TABLE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CSimpleHashTable
{
public:
    typedef enum {
        EHashTableKeyType_String = 0x0,
        EHashTableKeyType_Uxx = 0x1
    } EHashTableKeyType;

public:
    static CSimpleHashTable *Create(EHashTableKeyType keytype, TU32 row_num);
    CSimpleHashTable(EHashTableKeyType keytype, TU32 row_num);
    EECode Construct();
    virtual ~CSimpleHashTable();

public:
    EECode Add(const TChar *key0, const TChar *key1, TU32 value, TU8 update = 0);
    EECode Remove(const TChar *key0, const TChar *key1);
    EECode Query(const TChar *key0, const TChar *key1, TU32 &value);
    EECode UpdateValue(const TChar *key0, const TChar *key1, TU32 value);

public:
    void Delete();

private:
    struct SHashTableEntry {
        SHashTableEntry *p_next;
        TChar *p_key0;
        TChar *p_key1;
        TU32 value;
    };

private:
    SHashTableEntry *lookupEntry(const TChar *key0, const TChar *key1, TU32 &index);
    SHashTableEntry *insertNewEntry(const TChar *key0, const TChar *key1, TU32 index);
    TU32 hashIndexFromKey(const TChar *key);
    EECode deleteEntry(SHashTableEntry *&p_entry);

private:
    EHashTableKeyType mKeyType;
    SHashTableEntry **mpStaticEntry;
    IMutex *mpMutex;

    TU32 mnEntryNum;
    TU32 mnEntryRowNum;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif
