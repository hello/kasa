
/**
 * directsharing_video_live_stream_dispatcher.cpp
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

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "directsharing_if.h"

#include "directsharing_video_live_stream_dispatcher.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IDirectSharingDispatcher *gfCreateDirectSharingVideoLiveStreamDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingDispatcher *)CIDirectSharingVideoLiveStreamDispatcher::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingVideoLiveStreamDispatcher::CIDirectSharingVideoLiveStreamDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : mpPersistCommonConfig(pconfig)
    , mpSystemClockReference(p_system_clock_reference)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mpSenderList(NULL)
{
}

CIDirectSharingVideoLiveStreamDispatcher::~CIDirectSharingVideoLiveStreamDispatcher()
{
}

CIDirectSharingVideoLiveStreamDispatcher *CIDirectSharingVideoLiveStreamDispatcher::Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingVideoLiveStreamDispatcher *result = new CIDirectSharingVideoLiveStreamDispatcher(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_ERROR("gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    mpSenderList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSenderList)) {
        LOG_ERROR("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    memset(&mSharedResourceInformation, 0x0, sizeof(SSharedResource));

    return EECode_OK;
}

void CIDirectSharingVideoLiveStreamDispatcher::Destroy()
{
    if (mpSenderList) {
        CIDoubleLinkedList::SNode *pnode;
        IDirectSharingSender *p_sender;
        pnode = mpSenderList->FirstNode();
        while (pnode) {
            p_sender = (IDirectSharingSender *)(pnode->p_context);
            pnode = mpSenderList->NextNode(pnode);
            if (p_sender) {
                p_sender->Destroy();
            } else {
                LOG_FATAL("NULL p_sender.\n");
            }
        }

        delete mpSenderList;
        mpSenderList = NULL;
    }

    mpMutex->Delete();
    mpMutex = NULL;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::SetResource(SSharedResource *resource)
{
    if (DLikely(resource)) {
        memcpy(&mSharedResourceInformation, resource, sizeof(SSharedResource));
    } else {
        LOG_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::QueryResource(SSharedResource *&resource) const
{
    resource = (SSharedResource *) &mSharedResourceInformation;
    return EECode_OK;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::AddSender(IDirectSharingSender *sender)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(sender)) {
        mpSenderList->InsertContent(NULL, (void *) sender, 0);
    } else {
        LOG_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::RemoveSender(IDirectSharingSender *sender)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(sender)) {
        mpSenderList->RemoveContent((void *) sender);
    } else {
        LOG_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIDirectSharingVideoLiveStreamDispatcher::SendData(TU8 *pdata, TU32 datasize, TU8 data_flag)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpSenderList)) {
        CIDoubleLinkedList::SNode *pnode = NULL;
        CIDoubleLinkedList::SNode *cur_node = NULL;
        IDirectSharingSender *p_sender;
        pnode = mpSenderList->FirstNode();
        while (pnode) {
            p_sender = (IDirectSharingSender *)(pnode->p_context);
            cur_node = pnode;
            pnode = mpSenderList->NextNode(pnode);
            if (p_sender) {
                EDirectSharingStatusCode err = p_sender->SendData(pdata, datasize, data_flag);
                if (DUnlikely(EDirectSharingStatusCode_OK != err)) {
                    mpSenderList->FastRemoveContent(cur_node, (void *) p_sender);
                }
            } else {
                LOG_FATAL("NULL p_sender\n");
            }
        }
    } else {
        LOG_FATAL("NULL mpSenderList\n");
    }

    return EECode_OK;
}

void CIDirectSharingVideoLiveStreamDispatcher::SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback)
{
    LOG_FATAL("not support\n");
    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


