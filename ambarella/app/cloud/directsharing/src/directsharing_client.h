
/**
 * directsharing_client.h
 *
 * History:
 *    2015/03/11 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DIRECTSHARING_CLIENT_H__
#define __DIRECTSHARING_CLIENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingClient: public IDirectSharingClient
{
protected:
    CIDirectSharingClient(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingClient();
    EECode Construct();

public:
    static CIDirectSharingClient *Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

public:
    virtual EECode ConnectServer(TChar *server_url, TSocketPort server_port, TChar *username, TChar *password);
    virtual EDirectSharingStatusCode QuerySharedResource(SSharedResource *&p_resource_list, TU32 &resource_number);
    virtual EDirectSharingStatusCode RequestResource(TU8 type, TU16 index);
    virtual EDirectSharingStatusCode ShareResource(SSharedResource *resource);

    virtual TSocketHandler RetrieveSocketHandler();

public:
    virtual void Destroy();

protected:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;

private:
    TSocketHandler mSocket;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

