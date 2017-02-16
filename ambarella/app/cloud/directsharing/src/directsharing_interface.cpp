/**
 * directsharing_interface.cpp
 *
 * History:
 *  2015/03/11 - [Zhi He] create file
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern IDirectSharingSender *gfCreateDirectSharingSender(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingSender *gfCreateDirectSharingSenderFile(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingReceiver *gfCreateDirectSharingReceiver(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingDispatcher *gfCreateDirectSharingVideoLiveStreamDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingDispatcher *gfCreateDirectSharingFileDispatcher(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingClient *gfCreateDirectSharingClient(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
extern IDirectSharingServer *gfCreateDirectSharingServer(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

IDirectSharingSender *gfCreateDSSender(TSocketHandler socket, TU8 resource_type)
{
    IDirectSharingSender *thiz = NULL;

    if (ESharedResourceType_File != resource_type) {
        thiz = gfCreateDirectSharingSender(socket, NULL, NULL, NULL);
    } else {
        thiz = gfCreateDirectSharingSenderFile(socket, NULL, NULL, NULL);
    }

    return thiz;
}

IDirectSharingReceiver *gfCreateDSReceiver(TSocketHandler socket)
{
    IDirectSharingReceiver *thiz = gfCreateDirectSharingReceiver(socket, NULL, NULL, NULL);

    return thiz;
}

IDirectSharingDispatcher *gfCreateDSDispatcher(ESharedResourceType type)
{
    IDirectSharingDispatcher *thiz = NULL;

    switch (type) {

        case ESharedResourceType_LiveStreamVideo:
            thiz = gfCreateDirectSharingVideoLiveStreamDispatcher(NULL, NULL, NULL);
            break;

        case ESharedResourceType_File:
            thiz = gfCreateDirectSharingFileDispatcher(NULL, NULL, NULL);
            break;

        default:
            LOG_FATAL("not support %d\n", type);
            break;
    }


    return thiz;
}

IDirectSharingClient *gfCreateDSClient()
{
    IDirectSharingClient *thiz = gfCreateDirectSharingClient(NULL, NULL, NULL);

    return thiz;
}

IDirectSharingServer *gfCreateDSServer(TSocketPort port)
{
    IDirectSharingServer *thiz = gfCreateDirectSharingServer(port, NULL, NULL, NULL);

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

