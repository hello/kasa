/*******************************************************************************
 * directsharing_video_live_stream_dispatcher.h
 *
 * History:
 *    2015/03/10 - [Zhi He] create file
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

#ifndef __DIRECTSHARING_VIDEO_LIVE_STREAM_DISPATCHER_H__
#define __DIRECTSHARING_VIDEO_LIVE_STREAM_DISPATCHER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingVideoLiveStreamDispatcher: public IDirectSharingDispatcher
{
protected:
    CIDirectSharingVideoLiveStreamDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingVideoLiveStreamDispatcher();
    EECode Construct();

public:
    static CIDirectSharingVideoLiveStreamDispatcher *Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

public:
    virtual void Destroy();

public:
    virtual EECode SetResource(SSharedResource *resource);
    virtual EECode QueryResource(SSharedResource *&resource) const;

public:
    virtual EECode AddSender(IDirectSharingSender *sender);
    virtual EECode RemoveSender(IDirectSharingSender *sender);

public:
    virtual EECode SendData(TU8 *pdata, TU32 datasize, TU8 data_flag);

public:
    virtual void SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback);

private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    const CIClockReference *mpSystemClockReference;
    IMsgSink *mpMsgSink;

    IMutex *mpMutex;
    SSharedResource mSharedResourceInformation;
    CIDoubleLinkedList *mpSenderList;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

