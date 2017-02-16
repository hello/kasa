
/**
 * cloud_server_manager.h
 *
 * History:
 *    2013/12/02 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __CLOUD_SERVER_MANAGER_H__
#define __CLOUD_SERVER_MANAGER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CCloudServerManager
    : public CObject
    , public IActiveObject
    , public ICloudServerManager
{
    typedef CObject inherited;

protected:
    CCloudServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CCloudServerManager();

    EECode Construct();

public:
    static ICloudServerManager *Create(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual CObject *GetObject() const;

public:
    virtual void Destroy();
    virtual void Delete();
    virtual void PrintStatus0();
    virtual void OnRun();

    //IStreammingServer
    virtual EECode RunServerManager();
    virtual EECode ExitServerManager();
    virtual EECode Start();
    virtual EECode Stop();

    virtual ICloudServer *AddServer(CloudServerType type, TU16 server_port);
    virtual EECode RemoveServer(ICloudServer *server);
    virtual EECode AddCloudContent(ICloudServer *server_info, SCloudContent *content);
    virtual EECode RemoveCloudClient(ICloudServer *server_info, void *filter_owner);

    virtual void PrintStatus();
    EECode CloseServer(ICloudServer *server);

private:
    bool processCmd(SCMD &cmd);

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;

private:
    TU8 mbRun;
    EServerManagerState msState;

private:
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;

    CIDoubleLinkedList *mpServerList;
    TUint mnServerNumber;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

