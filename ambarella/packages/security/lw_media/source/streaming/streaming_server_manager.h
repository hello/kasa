
/**
 * streaming_server_manager.h
 *
 * History:
 *    2012/12/21 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __STREAMING_SERVER_MANAGER_H__
#define __STREAMING_SERVER_MANAGER_H__

class CStreamingServerManager
    : public CObject
    , public IActiveObject
    , public IStreamingServerManager
{
    typedef CObject inherited;

protected:
    CStreamingServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CStreamingServerManager();

    EECode Construct();

public:
    static IStreamingServerManager *Create(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual CObject *GetObject() const;

public:
    virtual void PrintStatus0();
    virtual void Destroy();
    virtual void Delete();
    virtual void OnRun();

    //IStreammingServer
    virtual EECode RunServerManager();
    virtual EECode ExitServerManager();

    virtual EECode Start();
    virtual EECode Stop();

    virtual IStreamingServer *AddServer(StreamingServerType type, StreamingServerMode mode, TU16 server_port, TU8 enable_video, TU8 enable_audio);
    virtual EECode RemoveServer(IStreamingServer *server);
    virtual EECode AddStreamingContent(IStreamingServer *server_info, SStreamingSessionContent *content);

    virtual EECode RemoveSession(IStreamingServer *server_info, void *session);

    virtual void PrintStatus();
    EECode CloseServer(IStreamingServer *server);

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

#endif

