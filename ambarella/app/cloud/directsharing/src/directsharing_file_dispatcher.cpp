
/**
 * directsharing_file_dispatcher.cpp
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

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "directsharing_if.h"

#include "directsharing_file_dispatcher.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static void __file_transfer_finished_callback(void *owner, void *p_sender, EECode ret_code)
{
    CIDirectSharingFileDispatcher *thiz = (CIDirectSharingFileDispatcher *) owner;
    thiz->RemoveSender((IDirectSharingSender *) p_sender);
}

IDirectSharingDispatcher *gfCreateDirectSharingFileDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingDispatcher *)CIDirectSharingFileDispatcher::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingFileDispatcher::CIDirectSharingFileDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : mpPersistCommonConfig(pconfig)
    , mpSystemClockReference(p_system_clock_reference)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mpSenderList(0)
{
}

CIDirectSharingFileDispatcher::~CIDirectSharingFileDispatcher()
{
}

CIDirectSharingFileDispatcher *CIDirectSharingFileDispatcher::Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingFileDispatcher *result = new CIDirectSharingFileDispatcher(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIDirectSharingFileDispatcher::Construct()
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

void CIDirectSharingFileDispatcher::Destroy()
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

EECode CIDirectSharingFileDispatcher::SetResource(SSharedResource *resource)
{
    if (DLikely(resource)) {
        memcpy(&mSharedResourceInformation, resource, sizeof(SSharedResource));
    } else {
        LOG_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIDirectSharingFileDispatcher::QueryResource(SSharedResource *&resource) const
{
    resource = (SSharedResource *) &mSharedResourceInformation;
    return EECode_OK;
}

EECode CIDirectSharingFileDispatcher::AddSender(IDirectSharingSender *sender)
{
    AUTO_LOCK(mpMutex);

    if (mpSenderList->NumberOfNodes()) {
        LOG_ERROR("already have one\n");
        sender->Destroy();
        return EECode_Busy;
    }

    if (DLikely(sender)) {
        mpSenderList->InsertContent(NULL, (void *) sender, 0);
        sender->SetProgressCallBack(mpProgressCallbackContext, mfProgressCallback);
        sender->StartSendFile(mSharedResourceInformation.property.file.filename, __file_transfer_finished_callback, (void *) this);
    } else {
        LOG_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIDirectSharingFileDispatcher::RemoveSender(IDirectSharingSender *sender)
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

EECode CIDirectSharingFileDispatcher::SendData(TU8 *pdata, TU32 datasize, TU8 data_flag)
{
    LOG_FATAL("not support callback\n");
    return EECode_NotSupported;
}

void CIDirectSharingFileDispatcher::SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback)
{
    mpProgressCallbackContext = owner;
    mfProgressCallback = progress_callback;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


