/*******************************************************************************
 * uploading_receiver.h
 *
 * History:
 *    2013/12/02 - [Zhi He] create file
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

#ifndef __UPLOADING_RECEIVER_H__
#define __UPLOADING_RECEIVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CUploadingReceiver
//
//-----------------------------------------------------------------------
class CUploadingReceiver
    : virtual public CObject
    , virtual public IDemuxer
{
    typedef CObject inherited;

protected:
    CUploadingReceiver(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual ~CUploadingReceiver();
    EECode Construct();

public:
    static IDemuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual CObject *GetObject0() const;
    void Delete();

public:
    virtual EECode SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[],  IMemPool *p_mempools[], IMsgSink *p_msg_sink);

    virtual EECode SetupContext(TChar *url, void *p_agent = NULL, TU8 priority = 0, TU32 request_receive_buffer_size = 0, TU32 request_send_buffer_size = 0);
    virtual EECode DestroyContext();
    virtual EECode ReconnectServer();

    virtual EECode Seek(TTime &target_time, ENavigationPosition position = ENavigationPosition_Invalid);
    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Suspend();
    virtual EECode Pause();
    virtual EECode Resume();
    virtual EECode Flush();
    virtual EECode ResumeFlush();
    virtual EECode Purge();
    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);
    virtual EECode SetPbLoopMode(TU32 *p_loop_mode);

    virtual EECode EnableVideo(TU32 enable);
    virtual EECode EnableAudio(TU32 enable);

public:
    void PrintStatus();

public:
    virtual EECode QueryContentInfo(const SStreamCodecInfos *&pinfos) const;
    EECode GetInfo(SStreamCodecInfos *&pinfos);

    virtual EECode UpdateContext(SContextInfo *pContext) {return EECode_NoImplement;}
    virtual EECode GetExtraData(SStreamingSessionContent *pContent) {return EECode_NoImplement;}

public:
    static EECode CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
    static EECode DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    EECode ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4);
    EECode ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

    SStreamCodecInfos mStreamCodecInfos;

private:
    TU8 mbAddedToScheduler;
    TU8 mbRunning;
    TU8 mbUpdateParams;
    TU8 mReserved1;

    TU8 mbPacketStarted[EDemuxerOutputPinNumber];
    TU8 *mpDataPacketStart[EDemuxerOutputPinNumber];
    TMemSize mPacketMaxSize[EDemuxerOutputPinNumber];
    TMemSize mPacketRemainningSize[EDemuxerOutputPinNumber];
    TMemSize mPacketSize[EDemuxerOutputPinNumber];

private:
    IScheduler *mpScheduler;
    ICloudServerAgent *mpAgent;

private:
    COutputPin *mpOutputpins[EDemuxerOutputPinNumber];
    CBufferPool *mpBufferpools[EDemuxerOutputPinNumber];
    IMemPool *mpMemorypools[EDemuxerOutputPinNumber];

private:
    TU32 mVideoWidth;
    TU32 mVideoHeight;
    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;
    TU32 mVideoBitrate;

private:
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;
    TMemSize mVideoExtraBufferSize;

    TU8 *mpAudioExtraData;
    TMemSize mAudioExtraDataSize;
    TMemSize mAudioExtraBufferSize;

    //private:
    //    FILE* mpDumpFile;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

