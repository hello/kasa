
/**
 * directsharing_file_dispatcher.h
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

#ifndef __DIRECTSHARING_FILE_DISPATCHER_H__
#define __DIRECTSHARING_FILE_DISPATCHER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingFileDispatcher: public IDirectSharingDispatcher
{
protected:
    CIDirectSharingFileDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingFileDispatcher();
    EECode Construct();

public:
    static CIDirectSharingFileDispatcher *Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

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

private:
    void *mpProgressCallbackContext;
    TTransferUpdateProgressCallBack mfProgressCallback;

};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

