/*******************************************************************************
 * hm_video_decoder.h
 *
 * History:
 *    2014/11/28 - [Zhi He] create file
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

#ifdef BUILD_MODULE_HEVC_HM_DEC

#ifndef __HM_VIDEO_DECODER_H__
#define __HM_VIDEO_DECODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// SHMVideoDecoder
//
//-----------------------------------------------------------------------
class SHMVideoDecoder: public CObject, virtual public IVideoDecoder
{
    typedef CObject inherited;

protected:
    SHMVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual ~SHMVideoDecoder();
    EECode Construct();

public:
    static IVideoDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual CObject *GetObject0() const;

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SVideoDecoderInputParam *param);
    virtual EECode DestroyContext();

    virtual EECode SetBufferPool(IBufferPool *buffer_pool);

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer);
    virtual EECode Flush();

    virtual EECode Suspend();

    virtual EECode SetExtraData(TU8 *p, TMemSize size);

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);

    virtual EECode SetFrameRate(TUint framerate_num, TUint frameate_den);

    virtual EDecoderMode GetDecoderMode() const;

    //direct mode
    virtual EECode DecodeDirect(CIBuffer *in_buffer);
    virtual EECode SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool);

    //turbo mode
    virtual EECode QueryFreeZoom(TUint &free_zoom, TU32 &fullness);
    virtual EECode QueryLastDisplayedPTS(TTime &pts);
    virtual EECode PushData(CIBuffer *in_buffer, TInt flush);
    virtual EECode PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining);

    virtual EECode TuningPB(TU8 fw, TU16 frame_tick);

private:
    EECode decode(vector<uint8_t> *nalUnit, CIBuffer *&out_buffer);
    EECode decode_direct(vector<uint8_t> *nalUnit);
    EECode setupDecoder();
    void destroyDecoder();
    Bool isNaluWithinTargetDecLayerIdSet(InputNALUnit *nalu);
    void xWriteOutput(TComList<TComPic *> *pcListPic, UInt tId, CIBuffer *&out_buffer);
    void xWriteOutput_direct(TComList<TComPic *> *pcListPic, UInt tId);
    void xFlushOutput(TComList<TComPic *> *pcListPic, CIBuffer *&out_buffer);
    void xFlushOutput_direct(TComList<TComPic *> *pcListPic);
    void xCleanOutput(TComList<TComPic *> *pcListPic);
    void copyFrame(TComPicYuv *pic_yuv, CIBuffer *&out_buffer);
    CIBuffer *allocBuffer();
    void sendBuffer(CIBuffer *out_buffer);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IBufferPool *mpBufferPool;
    IOutputPin *mpOutputPin;

private:
    TU8 *mpExtraData;
    TMemSize mExtraDataBufferSize;
    TMemSize mExtraDataSize;

private:
    TU8 mbDecoderSetup;
    TU8 mbNeedSendSyncPointBuffer;
    TU8 mbNewSequence;
    TU8 mReserved2;

    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;

    TU32 mVideoWidth;
    TU32 mVideoHeight;

private:
    TDecTop m_cTDecTop;

    Int m_iPOCLastDisplay;              ///< last POC in display order

    Int m_iSkipFrame;                           ///< counter for frames prior to the random access point to skip

    Int m_iMaxTemporalLayer;                  ///< maximum temporal layer to be decoded
    std::vector<Int> m_targetDecLayerIdSet;             ///< set of LayerIds to be included in the sub-bitstream extraction process.

    Int poc;
    TComList<TComPic *> *pcListPic;
    Bool loopFiltered;

private:
    vector<uint8_t> *mpSendNal;
    vector<uint8_t> *mpSendDupNal;
    InputNALUnit *mpNalUnit;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

#endif

