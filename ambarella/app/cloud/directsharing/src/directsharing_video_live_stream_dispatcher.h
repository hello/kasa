
/**
 * directsharing_video_live_stream_dispatcher.h
 *
 * History:
 *    2015/03/10 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

