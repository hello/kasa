
/**
 * directsharing_sender.h
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

#ifndef __DIRECTSHARING_SENDER_H__
#define __DIRECTSHARING_SENDER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingSender: public IDirectSharingSender
{
protected:
    CIDirectSharingSender(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingSender();
    EECode Construct();

public:
    static CIDirectSharingSender *Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

public:
    virtual void Destroy();

public:
    virtual EDirectSharingStatusCode SendData(TU8 *pdata, TU32 datasize, TU8 data_flag);
    virtual EDirectSharingStatusCode StartSendFile(TChar *filename, TFileTransferFinishCallBack callback, void *callback_context);

public:
    virtual void SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback);

private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    const CIClockReference *mpSystemClockReference;
    IMsgSink *mpMsgSink;

    TSocketHandler mSocket;

private:
    TU8 mbWaitFirstKeyFrame;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

