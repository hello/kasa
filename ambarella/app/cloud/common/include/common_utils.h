/*******************************************************************************
 * common_utils.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DMAP_UNIQUEID_TO_MEM(mem, id) \
    mem[0] = (id >> 56) & 0xff;  \
    mem[1] = (id >> 48) & 0xff;  \
    mem[2] = (id >> 40) & 0xff;  \
    mem[3] = (id >> 32) & 0xff;  \
    mem[4] = (id >> 24) & 0xff;  \
    mem[5] = (id >> 16) & 0xff;  \
    mem[6] = (id >> 8) & 0xff;  \
    mem[7] = id & 0xff

#define DMAP_MEM_TO_UNIQUEID(mem, id) \
    id = ((TUniqueID)mem[0] << 56) | ((TUniqueID)mem[1] << 48) \
         | ((TUniqueID)mem[2] << 40) | ((TUniqueID)mem[3] << 32) \
         | ((TUniqueID)mem[4] << 24) | ((TUniqueID)mem[5] << 16) \
         | ((TUniqueID)mem[6] << 8) | ((TUniqueID)mem[7])

#define DMAP_TU64_TO_MEM   DMAP_UNIQUEID_TO_MEM
#define DMAP_MEM_TO_TU64   DMAP_MEM_TO_UNIQUEID

//-----------------------------------------------------------------------
//
//  CIQueue
//
//-----------------------------------------------------------------------

class CIQueue
{
public:
    struct WaitResult {
        CIQueue *pDataQ;
        void *pOwner;
        TUint blockSize;
    };

    enum QType {
        Q_MSG,
        Q_DATA,
        Q_NONE,
    };

public:
    static CIQueue *Create(CIQueue *pMainQ, void *pOwner, TUint blockSize, TUint nReservedSlots);
    void Delete();

private:
    CIQueue(CIQueue *pMainQ, void *pOwner);
    EECode Construct(TUint blockSize, TUint nReservedSlots);
    ~CIQueue();

public:
    EECode PostMsg(const void *pMsg, TUint msgSize);
    EECode SendMsg(const void *pMsg, TUint msgSize);

    void GetMsg(void *pMsg, TUint msgSize);
    bool PeekMsg(void *pMsg, TUint msgSize);
    void Reply(EECode result);

    bool GetMsgEx(void *pMsg, TUint msgSize);
    void Enable(bool bEnabled = true);

    EECode PutData(const void *pBuffer, TUint size);

    QType WaitDataMsg(void *pMsg, TUint msgSize, WaitResult *pResult);
    QType WaitDataMsgCircularly(void *pMsg, TUint msgSize, WaitResult *pResult);
    QType WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue);
    void WaitMsg(void *pMsg, TUint msgSize);
    bool PeekData(void *pBuffer, TUint size);
    TUint GetDataCnt() const {AUTO_LOCK(mpMutex); return mnData;}

private:
    EECode swicthToNextDataQueue(CIQueue *pCurrent);

public:
    bool IsMain() { return mpMainQ == NULL; }
    bool IsSub() { return mpMainQ != NULL; }

private:
    struct List {
        List *pNext;
        bool bAllocated;
        void Delete();
    };

private:
    void *mpOwner;
    bool mbDisabled;

    CIQueue *mpMainQ;
    CIQueue *mpPrevQ;
    CIQueue *mpNextQ;

    IMutex *mpMutex;
    ICondition *mpCondReply;
    ICondition *mpCondGet;
    ICondition *mpCondSendMsg;

    TUint mnGet;
    TUint mnSendMsg;

    TUint mBlockSize;
    TUint mnData;

    List *mpTail;
    List *mpFreeList;

    List mHead;

    List *mpSendBuffer;
    TU8 *mpReservedMemory;

    EECode *mpMsgResult;

private:
    CIQueue *mpCurrentCircularlyQueue;

private:
    static void Copy(void *to, const void *from, TUint bytes) {
        if (bytes == sizeof(void *)) {
            *(void **)to = *(void **)from;
        } else {
            memcpy(to, from, bytes);
        }
    }

    List *AllocNode();

    void WriteData(List *pNode, const void *pBuffer, TUint size);
    void ReadData(void *pBuffer, TUint size);

};

//-----------------------------------------------------------------------
//
//  ISimpleQueue
//
//-----------------------------------------------------------------------

class ISimpleQueue
{
public:
    virtual void Destroy() = 0;

protected:
    virtual ~ISimpleQueue() {}

public:
    virtual TU32 GetCnt() = 0;
    virtual void Lock() = 0;
    virtual void UnLock() = 0;

    virtual void Enqueue(TULong ctx) = 0;
    virtual TULong Dequeue() = 0;
    virtual TU32 TryDequeue(TULong &ret_ctx) = 0;
};

extern ISimpleQueue *gfCreateSimpleQueue(TU32 num);

//-----------------------------------------------------------------------
//
//  CIConditionSlot
//
//-----------------------------------------------------------------------

class CIConditionSlot
{
public:
    static CIConditionSlot *Create(IMutex *p_mutex);
    void Delete();

private:
    CIConditionSlot(IMutex *p_mutex);
    EECode Construct();
    ~CIConditionSlot();

public:
    void Reset();
    void SetReplyCode(EECode ret_code);
    void GetReplyCode(EECode &ret_code) const;
    void SetReplyContext(TPointer reply_context, TU32 reply_type, TU32 reply_count, TUniqueID reply_id);
    void GetReplyContext(TPointer &reply_context, TU32 &reply_type, TU32 &reply_count, TUniqueID &reply_id) const;

public:
    EECode Wait();
    void Signal();
    //void SignalAll();

    void SetSessionNumber(TU32 session_number);
    TU32 GetSessionNumber() const;

    void SetCheckField(TU32 check_field);
    TU32 GetCheckField() const;

    void SetTime(SDateTime *p_time);
    void GetTime(SDateTime *&p_time) const;

private:
    TU32 mSessionNumber;

    //check field
    TU32 mCheckField;
    SDateTime mTime;

    TU32 mnWaiters;
    IMutex *mpMutex;
    ICondition *mpCondition;

private:
    TPointer mpReplyContext;
    TU32 mReplyType;
    TU32 mnReplyCount;
    TUniqueID mReplyID;
    EECode mReplyCode;
};

enum {
    eAudioObjectType_AAC_MAIN = 1,
    eAudioObjectType_AAC_LC = 2,
    eAudioObjectType_AAC_SSR = 3,
    eAudioObjectType_AAC_LTP = 4,
    eAudioObjectType_AAC_scalable = 6,
    //add others, todo

    eSamplingFrequencyIndex_96000 = 0,
    eSamplingFrequencyIndex_88200 = 1,
    eSamplingFrequencyIndex_64000 = 2,
    eSamplingFrequencyIndex_48000 = 3,
    eSamplingFrequencyIndex_44100 = 4,
    eSamplingFrequencyIndex_32000 = 5,
    eSamplingFrequencyIndex_24000 = 6,
    eSamplingFrequencyIndex_22050 = 7,
    eSamplingFrequencyIndex_16000 = 8,
    eSamplingFrequencyIndex_12000 = 9,
    eSamplingFrequencyIndex_11025 = 0xa,
    eSamplingFrequencyIndex_8000 = 0xb,
    eSamplingFrequencyIndex_7350 = 0xc,
    eSamplingFrequencyIndex_escape = 0xf,//should not be this value
};

//refer to iso14496-3
#ifdef BUILD_OS_WINDOWS
#pragma pack(push,1)
typedef struct {
    TU8 samplingFrequencyIndex_high : 3;
    TU8 audioObjectType : 5;
    TU8 bitLeft : 3;
    TU8 channelConfiguration : 4;
    TU8 samplingFrequencyIndex_low : 1;
} SSimpleAudioSpecificConfig;
#pragma pack(pop)
#else
typedef struct {
    TU8 samplingFrequencyIndex_high : 3;
    TU8 audioObjectType : 5;
    TU8 bitLeft : 3;
    TU8 channelConfiguration : 4;
    TU8 samplingFrequencyIndex_low : 1;
} __attribute__((packed))SSimpleAudioSpecificConfig;
#endif

extern TU8 *gfGenerateAACExtraData(TU32 samplerate, TU32 channel_number, TU32 &size);
extern EECode gfGetH264Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size);
extern EECode gfGetH264SPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size);
extern EECode gfGetH265Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size);
extern EECode gfGetH265VPSSPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_vps, TU32 &vps_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size);

extern TU8 *gfNALUFindFirstAVCSliceHeader(TU8 *p, TU32 len);
extern TU8 *gfNALUFindFirstAVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type);

extern void gfEncodingBase16(TChar *out, const TU8 *in, TInt in_size);
extern void gfDecodingBase16(TU8 *out, const TU8 *in, TInt in_size);
//extern EECode gfEncodingBase64(TU8* p_src, TU8* p_dest, TMemSize src_size, TMemSize& output_size);
//extern EECode gfDecodingBase64(TU8* p_src, TU8* p_dest, TMemSize src_size, TMemSize& output_size);
extern TInt gfDecodingBase64(TU8 *out, const TU8 *in_str, TInt out_size);
extern TChar *gfEncodingBase64(TChar *out, TInt out_size, const TU8 *in, TInt in_size);

extern TU16 gfEndianNativeToNetworkShort(TU16 input);

extern void gfFillTLV16Struct(TU8 *p_payload, const ETLV16Type type[], const TU16 length[], void *value[], TInt count = 1);
extern void gfParseTLV16Struct(TU8 *p_payload, ETLV16Type type[], TU16 length[], void *value[], TInt count = 1);

extern TU8 *gfGetNextTLV8(TU8 &type, TU8 &length, TU8 *&p_next_unit, TU32 &remain_size);


extern void gfRGBA8888_2_YUV420p_BT709FullRange(TU8 *rgba, TU32 width, TU32 height, TU32 linesize, TU8 *y, TU8 *u, TU8 *v, TU32 linesize_y, TU32 linesize_u, TU32 linesize_v);
extern void gfBGRA8888_2_YUV420p_BT709FullRange(TU8 *rgba, TU32 width, TU32 height, TU32 linesize, TU8 *y, TU8 *u, TU8 *v, TU32 linesize_y, TU32 linesize_u, TU32 linesize_v);

extern TInt gfStrcasecmp(const TChar *s1, const TChar *s2);
extern TInt gfStrncasecmp(const TChar *s1, const TChar *s2, TInt len);

extern void gfGetCommonVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day);

extern TU32 gfMethNextP2(TU32 x);
extern TU32 gfSimpleHash(TU32 val);

typedef struct {
    TInt xmin, xmax;
    TInt ymin, ymax;
} SRectI;

typedef struct {
    float xmin, xmax;
    float ymin, ymax;
} SRectF;

extern bool gfRctfInsideRctf(SRectF *rct_a, const SRectF *rct_b);
extern bool gfRctiInsideRcti(SRectI *rct_a, const SRectI *rct_b);
extern void gfRctfTranslate(SRectF *rect, float x, float y);
extern void gfRctiTranslate(SRectI *rect, TInt x, TInt y);

void gfJoinDirFile(TChar *dst, const TMemSize maxlen, const TChar *dir, const TChar *file);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

