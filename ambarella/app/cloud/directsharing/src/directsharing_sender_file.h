
/**
 * directsharing_sender_file.h
 *
 * History:
 *    2015/03/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DIRECTSHARING_SENDER_FILE_H__
#define __DIRECTSHARING_SENDER_FILE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingSenderFile: public IDirectSharingSender
{
protected:
    CIDirectSharingSenderFile(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingSenderFile();
    EECode Construct();

public:
    static CIDirectSharingSenderFile *Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

public:
    virtual void Destroy();

public:
    virtual EDirectSharingStatusCode SendData(TU8 *pdata, TU32 datasize, TU8 data_flag);
    virtual EDirectSharingStatusCode StartSendFile(TChar *filename, TFileTransferFinishCallBack callback, void *callback_context);

public:
    virtual void SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback);

public:
    virtual void MainLoop();

private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    const CIClockReference *mpSystemClockReference;
    IMsgSink *mpMsgSink;

    TSocketHandler mSocket;

private:
    IThread *mpThread;
    FILE *mpFile;

    TU8 *mpReadBuffer;
    TInt mnReadBufferSize;

    void *mpCallbackContext;
    TFileTransferFinishCallBack mCallback;
    TInt mbRunning;

private:
    void *mpProgressCallbackContext;
    TTransferUpdateProgressCallBack mfProgressCallback;

    TU64 mnTotalBytes;
    TU64 mnSentBytes;
    TInt mnProgress;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

