
/**
 * im_server_manager.h
 *
 * History:
 *    2014/06/17 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __IM_SERVER_MANAGER_H__
#define __IM_SERVER_MANAGER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIMServerManager
    : public CObject
    , public IActiveObject
    , public IIMServerManager
{
    typedef CObject inherited;

protected:
    CIMServerManager(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIMServerManager();
    EECode Construct();

public:
    static IIMServerManager *Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual CObject *GetObject() const;

public:
    virtual void Destroy();
    virtual void Delete();
    virtual void PrintStatus0();
    virtual void OnRun();

    virtual EECode StartServerManager();
    virtual EECode StopServerManager();

    virtual EECode AddServer(IIMServer *server);
    virtual EECode RemoveServer(IIMServer *server);

    virtual void PrintStatus();

private:
    void processCmd(SCMD &cmd);

protected:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;

private:
    TU8 mbRun;
    TU8 msState;
    TU16 mnServerNumber;

private:
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;

    CIDoubleLinkedList *mpServerList;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

