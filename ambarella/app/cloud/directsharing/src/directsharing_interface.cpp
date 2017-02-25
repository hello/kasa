/*******************************************************************************
 * directsharing_interface.cpp
 *
 * History:
 *  2015/03/11 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

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

