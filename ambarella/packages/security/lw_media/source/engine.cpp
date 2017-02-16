/**
 * engine.cpp
 *
 * History:
 *    2015/07/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "network_al.h"

#include "internal_streaming.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"
#include "filters_interface.h"

#include "engine.h"

TUint isComponentIDValid(TGenericID id, TComponentType check_type)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely(check_type != type)) {
            LOG_ERROR("id %08x is not a %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_2(TGenericID id, TComponentType check_type, TComponentType check_type_2)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type))) {
            LOG_ERROR("id %08x is not a %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_3(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_4(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_5(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type) && (check_type_5 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(check_type_5), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_6(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5, TComponentType check_type_6)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type) && (check_type_5 != type) && (check_type_6 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(check_type_5), gfGetComponentStringFromGenericComponentType(check_type_6), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

EFilterType __filterTypeFromGenericComponentType(TU8 generic_component_type, TU8 scheduled_muxer)
{
    EFilterType type = EFilterType_Invalid;

    switch (generic_component_type) {
        case EGenericComponentType_Demuxer:
            type = EFilterType_Demuxer;
            break;

        case EGenericComponentType_VideoDecoder:
            type = EFilterType_VideoDecoder;
            break;

        case EGenericComponentType_AudioDecoder:
            type = EFilterType_AudioDecoder;
            break;

        case EGenericComponentType_VideoEncoder:
            type = EFilterType_VideoEncoder;
            break;

        case EGenericComponentType_AudioEncoder:
            type = EFilterType_AudioEncoder;
            break;

        case EGenericComponentType_VideoRenderer:
            type = EFilterType_VideoRenderer;
            break;

        case EGenericComponentType_AudioRenderer:
            type = EFilterType_AudioRenderer;
            break;

        case EGenericComponentType_Muxer:
            if (!scheduled_muxer) {
                type = EFilterType_Muxer;
            } else {
                type = EFilterType_ScheduledMuxer;
            }
            break;

        case EGenericComponentType_StreamingTransmiter:
            type = EFilterType_RTSPTransmiter;
            break;

        case EGenericComponentType_FlowController:
            type = EFilterType_FlowController;
            break;

        default:
            LOG_FATAL("(%x) not a filter type.\n", generic_component_type);
            break;
    }

    return type;
}

IGenericEngineControl *CreateLWMDEngine()
{
    return (IGenericEngineControl *)CLWMDEngine::Create("CLWMDEngine", 0);
}

//-----------------------------------------------------------------------
//
// CLWMDEngine
//
//-----------------------------------------------------------------------
CLWMDEngine *CLWMDEngine::Create(const TChar *pname, TU32 index)
{
    CLWMDEngine *result = new CLWMDEngine(pname, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CLWMDEngine::CLWMDEngine(const TChar *pname, TU32 index)
    : inherited(pname, index)
    , mnPlaybackPipelinesMaxNumber(EPipelineMaxNumber_Playback)
    , mnPlaybackPipelinesCurrentNumber(0)
    , mnRecordingPipelinesMaxNumber(EPipelineMaxNumber_Recording)
    , mnRecordingPipelinesCurrentNumber(0)
    , mnStreamingPipelinesMaxNumber(EPipelineMaxNumber_Streaming)
    , mnStreamingPipelinesCurrentNumber(0)
    , mpPlaybackPipelines(NULL)
    , mpRecordingPipelines(NULL)
    , mpStreamingPipelines(NULL)
    , mbSetMediaConfig(0)
    , mbSchedulersCreated(0)
    , mRuningCount(0)
    , mbExternalClock(0)
    , mbReconnecting(0)
    , mbLoopPlay(0)
    , mbClockManagerStarted(0)
    , mpClockManager(NULL)
    , mpSystemClockReference(NULL)
    , mpClockSource(NULL)
    , mbRun(1)
    , mbGraghBuilt(0)
    , mbProcessingRunning(0)
    , mbProcessingStarted(0)
    , mpMutex(NULL)
    , msState(EEngineState_not_alive)
{
    TU32 i = 0;

    for (i = EGenericComponentType_TAG_filter_start; i < EGenericComponentType_TAG_filter_end; i ++) {
        mnComponentFiltersNumbers[i] = 0;
    }

    for (i = 0; i < (EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start); i ++) {
        mnComponentProxysNumbers[i] = 0;
    }
    mnComponentFiltersMaxNumbers[EGenericComponentType_Demuxer] = EComponentMaxNumber_Demuxer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoDecoder] = EComponentMaxNumber_VideoDecoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioDecoder] = EComponentMaxNumber_AudioDecoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoEncoder] = EComponentMaxNumber_VideoEncoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioEncoder] = EComponentMaxNumber_AudioEncoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoRenderer] = EComponentMaxNumber_VideoRenderer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioRenderer] = EComponentMaxNumber_AudioRenderer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_Muxer] = EComponentMaxNumber_Muxer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_StreamingTransmiter] = EComponentMaxNumber_RTSPStreamingTransmiter;
    mnComponentFiltersMaxNumbers[EGenericComponentType_FlowController] = EComponentMaxNumber_FlowController;

    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingServer;
    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingContent;
    mnComponentProxysMaxNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_ConnectionPin;

    mpListFilters = NULL;
    mpListProxys = NULL;
    mpListConnections = NULL;

    mbReconnecting = 0;
    mbLoopPlay = 1;
}

EECode CLWMDEngine::Construct()
{
    EECode ret = inherited::Construct();
    if (DUnlikely(ret != EECode_OK)) {
        LOGM_FATAL("CLWMDEngine::Construct, inherited::Construct fail, ret %d\n", ret);
        return ret;
    }

    mConfigLogLevel = ELogLevel_Info;

    mpListFilters = new CIDoubleLinkedList();
    if (DUnlikely(!mpListFilters)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpListProxys = new CIDoubleLinkedList();
    if (DUnlikely(!mpListProxys)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpListConnections = new CIDoubleLinkedList();
    if (DUnlikely(!mpListConnections)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpStreamingServerList = new CIDoubleLinkedList();
    if (DUnlikely(!mpStreamingServerList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    TU32 request_memsize = sizeof(SPlaybackPipeline) * mnPlaybackPipelinesMaxNumber;
    mpPlaybackPipelines = (SPlaybackPipeline *) DDBG_MALLOC(request_memsize, "E4PP");
    if (DLikely(mpPlaybackPipelines)) {
        memset(mpPlaybackPipelines, 0x0, request_memsize);
    } else {
        mnPlaybackPipelinesMaxNumber = 0;
        LOGM_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SRecordingPipeline) * mnRecordingPipelinesMaxNumber;
    mpRecordingPipelines = (SRecordingPipeline *) DDBG_MALLOC(request_memsize, "E4RP");
    if (DLikely(mpRecordingPipelines)) {
        memset(mpRecordingPipelines, 0x0, request_memsize);
    } else {
        mnRecordingPipelinesMaxNumber = 0;
        LOGM_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SStreamingPipeline) * mnStreamingPipelinesMaxNumber;
    mpStreamingPipelines = (SStreamingPipeline *) DDBG_MALLOC(request_memsize, "E4SP");
    if (DLikely(mpStreamingPipelines)) {
        memset(mpStreamingPipelines, 0x0, request_memsize);
    } else {
        mnStreamingPipelinesMaxNumber = 0;
        LOGM_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpClockManager = CIClockManager::Create();
    if (DUnlikely(!mpClockManager)) {
        LOGM_FATAL("CIClockManager::Create() fail\n");
        return EECode_NoMemory;
    }

    mpClockSource = gfCreateClockSource(EClockSourceType_generic);
    if (DUnlikely(!mpClockSource)) {
        LOGM_FATAL("gfCreateClockSource() fail\n");
        return EECode_NoMemory;
    }

    ret = mpClockManager->SetClockSource(mpClockSource);

    ret = mpClockManager->Start();
    mbClockManagerStarted = 1;

    mpSystemClockReference = CIClockReference::Create();
    if (DUnlikely(!mpSystemClockReference)) {
        LOGM_FATAL("CIClockReference::Create() fail\n");
        return EECode_NoMemory;
    }

    ret = mpClockManager->RegisterClockReference(mpSystemClockReference);

    mPersistMediaConfig.p_clock_source = mpClockSource;
    mPersistMediaConfig.p_system_clock_manager = mpClockManager;
    mPersistMediaConfig.p_system_clock_reference = mpSystemClockReference;

    mpWorkQ->Run();

    mpStreamingContent = (SStreamingSessionContent *)DDBG_MALLOC((mnStreamingPipelinesMaxNumber) * sizeof(SStreamingSessionContent), "E4SC");
    if (DLikely(mpStreamingContent)) {
        memset(mpStreamingContent, 0x0, (mnStreamingPipelinesMaxNumber) * sizeof(SStreamingSessionContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CLWMDEngine::~CLWMDEngine()
{
    if (mbSelfLogFile) {
        gfCloseLogFile();
    }
}

void CLWMDEngine::Delete()
{
    mpWorkQ->Exit();

    if (mpSystemClockReference) {
        mpSystemClockReference->Delete();
        mpSystemClockReference = NULL;
    }

    if (!mbExternalClock) {
        if (mpClockManager) {
            if (mbClockManagerStarted) {
                mpClockManager->Stop();
                mbClockManagerStarted = 0;
            }
            mpClockManager->Delete();
            mpClockManager = NULL;
        }

        if (mpClockSource) {
            mpClockSource->GetObject0()->Delete();
            mpClockSource = NULL;
        }
    }

    clearGraph();
    destroySchedulers();

    if (0 < mPersistMediaConfig.dsp_config.device_fd) {
        gfCloseIAVHandle(mPersistMediaConfig.dsp_config.device_fd);
        mPersistMediaConfig.dsp_config.device_fd = -1;
    }

    if (mpPlaybackPipelines) {
        DDBG_FREE(mpPlaybackPipelines, "E4PP");
        mpPlaybackPipelines = NULL;
    }

    if (mpRecordingPipelines) {
        DDBG_FREE(mpRecordingPipelines, "E4RP");
        mpRecordingPipelines = NULL;
    }

    if (mpStreamingPipelines) {
        DDBG_FREE(mpStreamingPipelines, "E4SP");
        mpStreamingPipelines = NULL;
    }

    if (mpListFilters) {
        delete mpListFilters;
        mpListFilters = NULL;
    }

    if (mpListProxys) {
        delete mpListProxys;
        mpListProxys = NULL;
    }

    if (mpListConnections) {
        delete mpListConnections;
        mpListConnections = NULL;
    }

    if (mpStreamingServerList) {
        delete mpStreamingServerList;
        mpStreamingServerList = NULL;
    }

    inherited::Delete();
}

EECode CLWMDEngine::PostEngineMsg(SMSG &msg)
{
    return inherited::PostEngineMsg(msg);
}

EECode CLWMDEngine::GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const
{
    pconfig = (volatile SPersistMediaConfig *)&mPersistMediaConfig;
    return EECode_OK;
}

EECode CLWMDEngine::SetMediaConfig()
{
    mbSetMediaConfig = 1;
    return EECode_OK;
}

EECode CLWMDEngine::PrintCurrentStatus(TGenericID id, TULong print_flag)
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_PrintCurrentStatus;
    cmd.res32_1 = id;
    cmd.res64_1 = print_flag;
    err = mpWorkQ->PostMsg(cmd);

    return err;
}

EECode CLWMDEngine::BeginConfigProcessPipeline(TU32 customized_pipeline)
{
    DASSERT(EEngineState_not_alive == msState);
    DASSERT(0 == mbGraghBuilt);

    if (DUnlikely(mbGraghBuilt)) {
        LOGM_FATAL("mbGraghBuilt %d yet?\n", mbGraghBuilt);
        return EECode_BadState;
    }

    if (DUnlikely(EEngineState_not_alive != msState)) {
        LOGM_FATAL("Current engine status is %u, not EEngineState_not_alive\n", msState);
        return EECode_BadState;
    }
    return EECode_OK;
}

EECode CLWMDEngine::NewComponent(TU32 component_type, TGenericID &component_id, const TChar *prefer_string)
{
    SComponent *component = NULL;
    EECode err = EECode_OK;

    if (DLikely((EGenericComponentType_TAG_filter_start <= component_type) && (EGenericComponentType_TAG_filter_end > component_type))) {

        DASSERT(mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]);
        if (mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]) {
            component_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(component_type, mnComponentFiltersNumbers[component_type]);

            component = newComponent((TComponentType)component_type, (TComponentIndex)mnComponentFiltersNumbers[component_type], prefer_string);
            if (component) {
                mpListFilters->InsertContent(NULL, (void *)component, 0);
            } else {
                LOGM_ERROR("NO memory\n");
                return EECode_NoMemory;
            }

            mnComponentFiltersNumbers[component_type] ++;
        } else {
            LOGM_ERROR("too many components %d now, max value %d, component type %d.\n", mnComponentFiltersNumbers[component_type], mnComponentFiltersMaxNumbers[component_type], component_type);
            return EECode_OutOfCapability;
        }

    } else {

        switch (component_type) {
            case EGenericComponentType_StreamingServer: {
                    err = addStreamingServer(component_id, StreamingServerType_RTSP, StreamingServerMode_Unicast);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("addStreamingServer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type, DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id), prefer_string);
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mpListProxys->InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            default:
                LOGM_ERROR("BAD component type %d, %s\n", component_type, gfGetComponentStringFromGenericComponentType(component_type));
                return EECode_BadParam;
                break;
        }

    }

    LOG_PRINTF("[Gragh]: NewComponent(%s), id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)component_type), component_id);

    return err;
}

EECode CLWMDEngine::ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type)
{
    SConnection *connection;
    SComponent *up_component = NULL, *down_component = NULL;

    TU8 up_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(upper_component_id);
    TU8 down_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(down_component_id);
    TComponentIndex up_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(upper_component_id);
    TComponentIndex down_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(down_component_id);

    if (DUnlikely(upper_component_id == down_component_id)) {
        LOGM_ERROR("upper_component_id == down_component_id? please check code\n");
        return EECode_BadCommand;
    }

    if (DUnlikely((up_type >= EGenericComponentType_TAG_filter_end) || (down_type >= EGenericComponentType_TAG_filter_end) || (up_index >= mnComponentFiltersNumbers[up_type]) || (down_index >= mnComponentFiltersNumbers[down_type]))) {
        LOGM_ERROR("Bad parameters when connect: up id %08x, type [%s], down id %08x, type [%s], up number limit %d, down number limit %d\n",
                   upper_component_id,
                   gfGetComponentStringFromGenericComponentType((TComponentType)up_type),
                   down_component_id,
                   gfGetComponentStringFromGenericComponentType((TComponentType)up_type),
                   mnComponentFiltersNumbers[up_type],
                   mnComponentFiltersNumbers[down_type]);
        return EECode_BadParam;
    }

    up_component = findComponentFromID(upper_component_id);
    down_component = findComponentFromID(down_component_id);

    if ((!up_component) || (!down_component)) {
        LOGM_ERROR("BAD component id, cannot find component, upper %08x, down %08x, upper %p, down %p\n", upper_component_id, down_component_id, up_component, down_component);
        return EECode_BadParam;
    }

    connection = newConnetion(upper_component_id, down_component_id, up_component, down_component);
    if (connection) {
        mpListConnections->InsertContent(NULL, (void *)connection, 0);
    } else {
        LOGM_ERROR("NO memory\n");
        return EECode_NoMemory;
    }

    connection_id = connection->connection_id;
    connection->pin_type = pin_type;

    LOG_PRINTF("[Gragh]: ConnectComponent(%s[%d] to %s[%d]), pin type %d, connection id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)up_type), up_index, gfGetComponentStringFromGenericComponentType((TComponentType)down_type), down_index, pin_type, connection_id);

    return EECode_OK;
}

EECode CLWMDEngine::SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id)
{
    if (DLikely(mpPlaybackPipelines)) {
        if (DLikely((mnPlaybackPipelinesCurrentNumber + 1) < mnPlaybackPipelinesMaxNumber)) {
            SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + mnPlaybackPipelinesCurrentNumber;
            TU32 video_path_enable = 0;
            TU32 audio_path_enable = 0;

            if (video_source_id && video_decoder_id && video_renderer_id) {

                if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }

                if (DUnlikely(0 == isComponentIDValid(video_decoder_id, EGenericComponentType_VideoDecoder))) {
                    LOGM_ERROR("not valid video decoder id %08x\n", video_decoder_id);
                    return EECode_BadParam;
                }

                if (DUnlikely(0 == isComponentIDValid(video_renderer_id, EGenericComponentType_VideoRenderer))) {
                    LOGM_ERROR("not valid video renderer id %08x\n", video_renderer_id);
                    return EECode_BadParam;
                }

                video_path_enable = 1;
            }

            if (audio_source_id && audio_decoder_id && audio_renderer_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }

                if (audio_decoder_id) {
                    if (DUnlikely(0 == isComponentIDValid(audio_decoder_id, EGenericComponentType_AudioDecoder))) {
                        LOGM_ERROR("not valid audio decoder id %08x\n", audio_decoder_id);
                        return EECode_BadParam;
                    }
                } else {
                    LOG_NOTICE("no audio decoder component, PCM raw mode?\n");
                }

                if (DUnlikely(0 == isComponentIDValid(audio_renderer_id, EGenericComponentType_AudioRenderer))) {
                    LOGM_ERROR("not valid audio renderer/output id %08x\n", audio_renderer_id);
                    return EECode_BadParam;
                }

                audio_path_enable = 1;
            }

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, mnPlaybackPipelinesCurrentNumber);
                p_pipeline->index = mnPlaybackPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Playback;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;

                if (video_path_enable) {
                    p_pipeline->video_source_id = video_source_id;
                    p_pipeline->video_decoder_id = video_decoder_id;
                    p_pipeline->video_renderer_id = video_renderer_id;

                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id = audio_source_id;
                    p_pipeline->audio_decoder_id = audio_decoder_id;
                    p_pipeline->audio_renderer_id = audio_renderer_id;

                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->video_suspended = 1;
                p_pipeline->audio_suspended = 1;

                mnPlaybackPipelinesCurrentNumber ++;
                playback_pipeline_id = p_pipeline->pipeline_id;

            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many playback pipeline %d, max value %d\n", mnPlaybackPipelinesCurrentNumber, mnPlaybackPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpPlaybackPipelines %p\n", mpPlaybackPipelines);
        return EECode_InternalLogicalBug;
    }

    LOG_PRINTF("[Gragh]: SetupPlaybackPipeline() done, id 0x%08x\n", playback_pipeline_id);

    return EECode_OK;
}

EECode CLWMDEngine::SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id)
{
    if (DLikely(mpRecordingPipelines)) {
        if (DLikely((mnRecordingPipelinesCurrentNumber + 1) < mnRecordingPipelinesMaxNumber)) {
            SRecordingPipeline *p_pipeline = mpRecordingPipelines + mnRecordingPipelinesCurrentNumber;
            TU32 video_path_enable = 0;
            TU32 audio_path_enable = 0;

            if (DUnlikely(0 == isComponentIDValid(sink_id, EGenericComponentType_Muxer))) {
                LOGM_ERROR("not valid sink_id %08x\n", sink_id);
                return EECode_BadParam;
            }

            if (video_source_id) {
                if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            LOG_NOTICE("video_path_enable %u, audio_path_enable %u\n", video_path_enable, audio_path_enable);

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, mnRecordingPipelinesCurrentNumber);
                p_pipeline->index = mnRecordingPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Recording;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;

                if (video_path_enable) {
                    p_pipeline->video_source_id[0] = video_source_id;
                    p_pipeline->video_source_number = 1;
                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id[0] = audio_source_id;
                    p_pipeline->audio_source_number = 1;
                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->sink_id = sink_id;

                mnRecordingPipelinesCurrentNumber ++;
                recording_pipeline_id = p_pipeline->pipeline_id;
            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many recording pipeline %d, max value %d\n", mnRecordingPipelinesCurrentNumber, mnRecordingPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpRecordingPipelines %p\n", mpRecordingPipelines);
        return EECode_InternalLogicalBug;
    }

    LOG_PRINTF("[Gragh]: SetupRecordingPipeline() done, id 0x%08x\n", recording_pipeline_id);

    return EECode_OK;
}

EECode CLWMDEngine::SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id)
{
    if (DLikely(mpStreamingPipelines)) {
        if (DLikely((mnStreamingPipelinesCurrentNumber + 1) < mnStreamingPipelinesMaxNumber)) {
            SStreamingPipeline *p_pipeline = mpStreamingPipelines + mnStreamingPipelinesCurrentNumber;
            TU32 video_path_enable = 0;
            TU32 audio_path_enable = 0;

            if (DUnlikely(0 == isComponentIDValid(streaming_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
                LOGM_ERROR("not valid streaming_transmiter_id %08x\n", streaming_transmiter_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(streaming_server_id, EGenericComponentType_StreamingServer))) {
                LOGM_ERROR("not valid streaming_server_id %08x\n", streaming_server_id);
                return EECode_BadParam;
            }

            if (video_source_id) {
                if (DUnlikely(0 == isComponentIDValid_2(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_VideoEncoder))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid_2(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_AudioEncoder))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, mnStreamingPipelinesCurrentNumber);
                p_pipeline->index = mnStreamingPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Streaming;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;

                if (video_path_enable) {
                    p_pipeline->video_source_id[0] = video_source_id;
                    p_pipeline->video_source_number = 1;
                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id[0] = audio_source_id;
                    p_pipeline->audio_source_number = 1;
                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->data_transmiter_id = streaming_transmiter_id;
                p_pipeline->server_id = streaming_server_id;

                mnStreamingPipelinesCurrentNumber ++;
                streaming_pipeline_id = p_pipeline->pipeline_id;
            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many streaming pipeline %d, max value %d\n", mnStreamingPipelinesCurrentNumber, mnStreamingPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpStreamingPipelines %p\n", mpStreamingPipelines);
        return EECode_InternalLogicalBug;
    }

    LOG_PRINTF("[Gragh]: SetupStreamingPipeline() done, id 0x%08x\n", streaming_pipeline_id);

    return EECode_OK;
}

EECode CLWMDEngine::FinalizeConfigProcessPipeline()
{
    EECode err = EECode_OK;

    if (DLikely(0 == mbGraghBuilt)) {

        DASSERT(EEngineState_not_alive == msState);

        err = createSchedulers();
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("createSchedulers() fail, err %d\n", err);
            return err;
        }

        mPersistMediaConfig.dsp_config.device_fd = gfOpenIAVHandle();
        LOG_NOTICE("open iav fd %d\n", mPersistMediaConfig.dsp_config.device_fd);

        err = createGraph();
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("createGraph() fail, err %d\n", err);
            return err;
        }

        msState = EEngineState_idle;
    } else {
        LOGM_FATAL("mbGraphBuilt?\n");
        return EECode_BadState;
    }

    msState = EEngineState_idle;
    return EECode_OK;
}

EECode CLWMDEngine::SetSourceUrl(TGenericID source_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(source_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("SetSourceUrl: cannot find component, id %08x\n", source_component_id);
        return EECode_BadParam;
    }

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 1024)) {
            LOGM_ERROR("url too long(size %ld) in SetSourceUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E4UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        }
    } else {
        if (p_component->url) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
        }
    }

    EECode err = EECode_OK;
    if (mbProcessingRunning) {
        if (DLikely(NULL == p_component->p_filter)) {
            LOGM_ERROR("mbProcessingRunning, butp_component->p_filter is NULL, in SetSourceUrl?\n");
            return EECode_InternalLogicalBug;
        }
        if (url) {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set source url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id, url);
        } else {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set source NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id);
        }
        err = p_component->p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, (void *) url);
        DASSERT_OK(err);
    } else {
        if (url) {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set source url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id, url);
        } else {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set source NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id);
        }
    }

    return err;
}

EECode CLWMDEngine::SetSinkUrl(TGenericID sink_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(sink_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("SetSinkUrl: cannot find component, id %08x\n", sink_component_id);
        return EECode_BadParam;
    }

    if (DLikely(NULL == p_component->p_filter)) {
        LOG_NOTICE("[API flow]: pre-set sink url\n");
    }

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 1024)) {
            LOGM_ERROR("url too long(size %ld) in SetSinkUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E4UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        }
    } else {
        if (p_component->url) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
        }
    }

    EECode err = EECode_OK;
    if (mbProcessingRunning) {
        if (url) {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set sink url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id, url);
        } else {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set sink NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id);
        }
        err = p_component->p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, (void *) url);
        DASSERT_OK(err);
    } else {
        if (url) {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set sink url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id, url);
        } else {
            LOG_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set sink NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id);
        }
    }

    return err;
}

EECode CLWMDEngine::SetStreamingUrl(TGenericID pipeline_id, TChar *url)
{
    if (0 == isComponentIDValid(pipeline_id, EGenericPipelineType_Streaming)) {
        LOGM_ERROR("BAD pipeline id %08x\n", pipeline_id);
        return EECode_BadParam;
    }

    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(pipeline_id);
    if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
        LOGM_ERROR("BAD pipeline id %08x, index %d exceed max value %d\n", pipeline_id, index, mnStreamingPipelinesCurrentNumber);
        return EECode_BadParam;
    }

    if (DUnlikely(NULL == mpStreamingPipelines)) {
        LOGM_ERROR("mpStreamingPipelines is NULL?\n");
        return EECode_BadState;
    }

    SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 64)) {
            LOGM_ERROR("url too long(size %ld) in SetStreamingUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_pipeline->url && (p_pipeline->url_buffer_size > url_length)) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        } else {
            if (p_pipeline->url) {
                DDBG_FREE(p_pipeline->url, "E4UR");
            }
            p_pipeline->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
            if (DLikely(p_pipeline->url)) {
                p_pipeline->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        }
    } else {
        if (p_pipeline->url) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
        }
    }

    if (mbGraghBuilt) {
        if (DLikely(mpStreamingContent)) {
            SStreamingSessionContent *p_content = mpStreamingContent + index;
            memset(p_content->session_name, 0x0, sizeof(p_content->session_name));
            snprintf(p_content->session_name, sizeof(p_content->session_name) - 1, "%s", p_pipeline->url);
        } else {
            LOGM_FATAL("NULL mpStreamingContent here?\n");
            return EECode_BadState;
        }
    }

    return EECode_OK;
}

EECode CLWMDEngine::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
    return inherited::SetAppMsgCallback(MsgProc, context);
}

EECode CLWMDEngine::RunProcessing()
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_RunProcessing;
    err = mpWorkQ->SendCmd(cmd);

    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("CLWMDEngine::RunProcessing() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CLWMDEngine::ExitProcessing()
{
    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("CLWMDEngine::ExitProcessing: not in running state\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_ExitProcessing;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingRunning = 0;
    } else {
        LOGM_ERROR("CLWMDEngine::ExitProcessing() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CLWMDEngine::Start()
{
    if (DUnlikely(mbProcessingStarted)) {
        LOGM_ERROR("CLWMDEngine::Start(): already started\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Start;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingStarted = 1;
    } else {
        LOGM_ERROR("CLWMDEngine::Start() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CLWMDEngine::Stop()
{
    if (DUnlikely(!mbProcessingStarted)) {
        LOGM_ERROR("CLWMDEngine::Stop(): not started\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Stop;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingStarted = 0;
    } else {
        LOGM_ERROR("CLWMDEngine::Stop() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CLWMDEngine::GenericControl(EGenericEngineConfigure config_type, void *param)
{
    EECode err = EECode_OK;

    if (!param) {
        LOG_NOTICE("NULL param for type 0x%08x\n", config_type);
    }

    switch (config_type) {

        case EGenericEngineConfigure_RTSPServerPort: {
                SConfigRTSPServerPort *p_port = (SConfigRTSPServerPort *) param;
                DASSERT(p_port);
                DASSERT(EGenericEngineConfigure_RTSPServerPort == p_port->check_field);
                if (p_port) {
                    mPersistMediaConfig.rtsp_server_config.rtsp_listen_port = p_port->port;
                    LOGM_CONFIGURATION("user set rtsp server port %d\n", mPersistMediaConfig.rtsp_server_config.rtsp_listen_port);
                }
            }
            break;

        case EGenericEngineConfigure_RTSPServerNonblock:
            mPersistMediaConfig.rtsp_server_config.enable_nonblock_timeout = 1;
            break;

        case EGenericEngineConfigure_VideoStreamID: {
                SConfigVideoStreamID *p_vsid = (SConfigVideoStreamID *) param;
                DASSERT(p_vsid);
                DASSERT(EGenericEngineConfigure_VideoStreamID == p_vsid->check_field);
                if (p_vsid) {
                    mPersistMediaConfig.rtsp_server_config.video_stream_id = p_vsid->stream_id;
                    LOGM_CONFIGURATION("user set video streaming id %d\n", mPersistMediaConfig.rtsp_server_config.video_stream_id);
                }
            }
            break;

        case EGenericEngineConfigure_ConfigVout: {
                SConfigVout *config_vout = (SConfigVout *) param;
                DASSERT(config_vout);
                DASSERT(EGenericEngineConfigure_ConfigVout == config_vout->check_field);
                if (config_vout) {
                    mPersistMediaConfig.dsp_config.use_digital_vout = config_vout->b_digital_vout;
                    mPersistMediaConfig.dsp_config.use_hdmi_vout = config_vout->b_hdmi_vout;
                    mPersistMediaConfig.dsp_config.use_cvbs_vout = config_vout->b_cvbs_vout;
                }
            }
            break;

        case EGenericEngineConfigure_ConfigAudioDevice: {
                SConfigAudioDevice* config_audio_device = (SConfigAudioDevice*) param;
                DASSERT(config_audio_device);
                DASSERT(EGenericEngineConfigure_ConfigAudioDevice == config_audio_device->check_field);
                if (config_audio_device) {
                    snprintf((TChar *)mPersistMediaConfig.audio_device.audio_device_name, sizeof(mPersistMediaConfig.audio_device.audio_device_name), "%s", config_audio_device->audio_device_name);
                    LOGM_CONFIGURATION("audio device:%s\n", mPersistMediaConfig.audio_device.audio_device_name);
                }
            }
            break;

        case EGenericEngineConfigure_StepPlay: {
                SUserParamStepPlay *step_play = (SUserParamStepPlay *)param;
                DASSERT(step_play);
                DASSERT(EGenericEngineConfigure_StepPlay == step_play->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(step_play->component_id);
                if (!p_pipeline || !p_pipeline->p_video_renderer || !p_pipeline->p_video_renderer->p_filter) {
                    LOGM_ERROR("EGenericEngineConfigure_StepPlay, not valid pipeline id, %08x.\n", step_play->component_id);
                    return EECode_BadParam;
                }

                err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_step, 1, param);
                p_pipeline->video_runtime_paused = 1;

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (!p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("pause audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_pause, 1, NULL);
                        p_pipeline->audio_runtime_paused = 1;
                    }
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (!p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("disable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_disable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 1;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Pause: {
                SUserParamPause *pause = (SUserParamPause *)param;
                DASSERT(pause);
                DASSERT(EGenericEngineConfigure_Pause == pause->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(pause->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Pause, not valid pipeline id, %08x.\n", pause->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    if (!p_pipeline->video_runtime_paused) {
                        LOGM_INFO("pause video renderer\n");
                        err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_pause, 1, NULL);
                        p_pipeline->video_runtime_paused = 1;
                    }
                }

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (!p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("pause audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_pause, 1, NULL);
                        p_pipeline->audio_runtime_paused = 1;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Resume: {
                SUserParamResume *resume = (SUserParamResume *)param;
                DASSERT(resume);
                DASSERT(EGenericEngineConfigure_Resume == resume->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(resume->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Resume, not valid pipeline id, %08x.\n", resume->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    if (p_pipeline->video_runtime_paused) {
                        LOGM_INFO("resume video renderer\n");
                        err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_resume, 1, NULL);
                        p_pipeline->video_runtime_paused = 0;
                    }
                }

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("resume audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_resume, 1, NULL);
                        p_pipeline->audio_runtime_paused = 0;
                    }
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("enable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_enable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 0;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Seek: {
                SPbSeek *seek = (SPbSeek *)param;
                SPbFeedingRule speed;
                DASSERT(seek);
                DASSERT(EGenericEngineConfigure_Seek == seek->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(seek->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Seek, not valid pipeline id, %08x.\n", seek->component_id);
                    return EECode_BadParam;
                }

                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("enable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_enable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 0;
                    }
                }

                speed.direction = 0;
                speed.feeding_rule = DecoderFeedingRule_AllFrames;
                speed.speed = 0x100;
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_NOTICE("seek video source\n");
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, param);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
                    LOGM_NOTICE("seek audio source\n");
                    p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, param);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        case EGenericEngineConfigure_FastForward:
        case EGenericEngineConfigure_FastBackward: {
                SPbFeedingRule *ff = (SPbFeedingRule *)param;
                SQueryLastShownTimeStamp time;
                SPbSeek seek;
                DASSERT(ff);
                DASSERT((EGenericEngineConfigure_FastForward == ff->check_field) || (EGenericEngineConfigure_FastBackward == ff->check_field));
                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(ff->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("fast ff/bw, not valid pipeline id, %08x.\n", ff->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (!p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("disable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_disable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 1;
                    }
                }
                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_get_last_shown_timestamp, 0, &time);
                    LOGM_INFO("get time %lld\n", time.time);
                }

                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    seek.target = time.time / 90;
                    seek.position = ENavigationPosition_Begining;
                    LOGM_INFO("seek to %lld (ms)\n", seek.target);
                    err = p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, param);
                }
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, param);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        case EGenericEngineConfigure_FastForwardFromBegin: {
                SPbFeedingRule *ff = (SPbFeedingRule *)param;
                SPbSeek seek;
                DASSERT(ff);
                DASSERT(EGenericEngineConfigure_FastForwardFromBegin == ff->check_field);
                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(ff->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("fast ff/bw, not valid pipeline id, %08x.\n", ff->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (!p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("disable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_disable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 1;
                    }
                }

                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_INFO("seek to begin\n");
                    seek.target = 0;
                    seek.position = ENavigationPosition_Begining;
                    err = p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, param);
                }
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, param);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        case EGenericEngineConfigure_FastBackwardFromEnd: {
                SPbFeedingRule *ff = (SPbFeedingRule *)param;
                SPbSeek seek;
                DASSERT(ff);
                DASSERT(EGenericEngineConfigure_FastBackwardFromEnd == ff->check_field);
                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(ff->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("fast ff/bw, not valid pipeline id, %08x.\n", ff->component_id);
                    return EECode_BadParam;
                }
                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (!p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("disable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_disable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 1;
                    }
                }

                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_INFO("seek to end\n");
                    seek.target = 0;
                    seek.position = ENavigationPosition_End;
                    err = p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, param);
                }
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, param);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        case EGenericEngineConfigure_Resume1xFromCurrent: {
                SResume1xPlayback *r = (SResume1xPlayback *)param;
                SPbSeek seek;
                SPbFeedingRule speed;
                SQueryLastShownTimeStamp time;
                DASSERT(r);
                DASSERT((EGenericEngineConfigure_Resume1xFromCurrent == r->check_field));
                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(r->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("resume to 1xfw, not valid pipeline id, %08x.\n", r->component_id);
                    return EECode_BadParam;
                }
                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_get_last_shown_timestamp, 0, &time);
                    LOGM_INFO("get time %lld\n", time.time);
                }
                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);
                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("enable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_enable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 0;
                    }
                }
                seek.target = time.time / 90;
                seek.position = ENavigationPosition_Begining;
                speed.direction = 0;
                speed.feeding_rule = DecoderFeedingRule_AllFrames;
                speed.speed = 0x100;
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_NOTICE("seek video source\n");
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
                    LOGM_NOTICE("seek audio source\n");
                    p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        case EGenericEngineConfigure_Resume1xFromBegin: {
                SResume1xPlayback *r = (SResume1xPlayback *)param;
                SPbSeek seek;
                SPbFeedingRule speed;
                DASSERT(r);
                DASSERT((EGenericEngineConfigure_Resume1xFromBegin == r->check_field));
                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(r->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("resume to 1xfw, not valid pipeline id, %08x.\n", r->component_id);
                    return EECode_BadParam;
                }
                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("enable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_enable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 0;
                    }
                }
                seek.target = 0;
                seek.position = ENavigationPosition_Begining;
                speed.direction = 0;
                speed.feeding_rule = DecoderFeedingRule_AllFrames;
                speed.speed = 0x100;
                if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_NOTICE("seek video source\n");
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, &speed);
                }
                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
                    LOGM_NOTICE("seek audio source\n");
                    p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, &seek);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);
            }
            break;

        default: {
                LOGM_FATAL("Unknown config_type 0x%8x\n", (TU32)config_type);
                err = EECode_BadCommand;
            }
            break;
    }

    return err;
}

void CLWMDEngine::Destroy()
{
    Delete();
}

void CLWMDEngine::OnRun()
{
    SCMD cmd, nextCmd;
    SMSG msg;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    DASSERT(mpFilterMsgQ);
    DASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(EECode_OK);

    while (mbRun) {
        LOGM_FLOW("CLWMDEngine: msg cnt from filters %d.\n", mpFilterMsgQ->GetDataCnt());

        //wait user/app set data source
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        LOGM_DEBUG("wait data msg done\n");
        if (type == CIQueue::Q_MSG) {
            LOGM_DEBUG("processCMD\n");
            processCMD(cmd);
            LOGM_DEBUG("processCMD done\n");
        } else if (type == CIQueue::Q_DATA) {
            //msg from filters
            DASSERT(mpFilterMsgQ == result.pDataQ);
            if (result.pDataQ->PeekData(&msg, sizeof(msg))) {
                LOGM_DEBUG("processGenericMsg\n");
                if (!processGenericMsg(msg)) {
                    //need implement, process the msg
                    LOGM_ERROR("not-processed cmd %d.\n", msg.code);
                }
                LOGM_DEBUG("processGenericMsg done\n");
            }
        } else {
            LOGM_ERROR("Fatal error here.\n");
        }
    }

    LOGM_STATE("CLWMDEngine: start Clear gragh for safe.\n");
    NewSession();
    mpWorkQ->CmdAck(EECode_OK);
}

SComponent *CLWMDEngine::newComponent(TComponentType type, TComponentIndex index, const TChar *prefer_string)
{
    SComponent *p = (SComponent *) DDBG_MALLOC(sizeof(SComponent), "E4CC");

    if (p) {
        memset(p, 0x0, sizeof(SComponent));
        p->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);
        p->type = type;
        p->index = index;
        p->cur_state = EComponentState_not_alive;

        if (prefer_string) {
            strncpy(p->prefer_string, prefer_string, DMaxPreferStringLen - 1);
        }
    }

    return p;
}

void CLWMDEngine::deleteComponent(SComponent *p)
{
    if (p) {
        if (p->p_filter) {
            p->p_filter->GetObject0()->Delete();
            p->p_filter = NULL;
        }
        if (p->url) {
            DDBG_FREE(p->url, "E4UR");
            p->url = NULL;
        }
        DDBG_FREE(p, "E4CC");
    }
}

SConnection *CLWMDEngine::newConnetion(TGenericID up_id, TGenericID down_id, SComponent *up_component, SComponent *down_component)
{
    SConnection *p = (SConnection *) DDBG_MALLOC(sizeof(SConnection), "E4Cc");
    if (p) {
        memset(p, 0x0, sizeof(SConnection));
        p->connection_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_ConnectionPin, mnComponentProxysNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start]++);
        p->up_id = up_id;
        p->down_id = down_id;

        p->up = up_component;
        p->down = down_component;

        p->state = EConnectionState_not_alive;
    }

    return p;
}

void CLWMDEngine::deleteConnection(SConnection *p)
{
    if (p) {
        DDBG_FREE(p, "E4Cc");
    }
}

SComponent *CLWMDEngine::findComponentFromID(TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    pnode = mpListProxys->FirstNode();
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListProxys->NextNode(pnode);
    }

    LOGM_ERROR("findComponentFromID: cannot find component, id 0x%08x\n", id);
    return NULL;
}

SComponent *CLWMDEngine::findComponentFromTypeIndex(TU8 type, TU8 index)
{
    TGenericID id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);

    return findComponentFromID(id);
}

SComponent *CLWMDEngine::findComponentFromFilter(IFilter *p_filter)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->p_filter == p_filter) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOG_WARN("findComponentFromFilter: cannot find component, by filter %p\n", p_filter);
    return NULL;
}

SComponent *CLWMDEngine::queryFilterComponent(TGenericID filter_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->id == filter_id) {
            return p_component;
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    return NULL;
}

SComponent *CLWMDEngine::queryFilterComponent(TComponentType type, TComponentIndex index)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->type == type
                && p_component->index == index) {
            return p_component;
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    return NULL;
}

SConnection *CLWMDEngine::queryConnection(TGenericID connection_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListConnections->FirstNode();
    SConnection *p_connection = NULL;

    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection->connection_id == connection_id) {
            return p_connection;
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    return NULL;
}

SConnection *CLWMDEngine::queryConnection(TGenericID up_id, TGenericID down_id, StreamType type)
{
    CIDoubleLinkedList::SNode *pnode = NULL;
    SConnection *p_connection = NULL;

    pnode = mpListConnections->FirstNode();

    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if ((p_connection->up_id == up_id) && (p_connection->down_id == down_id)) {
            if (p_connection->pin_type == type) {
                return p_connection;
            }
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    return NULL;
}

SPlaybackPipeline *CLWMDEngine::queryPlaybackPipeline(TGenericID id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);

    if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
        SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;
        return p_pipeline;
    } else {
        LOGM_ERROR("BAD id %08x, mpPlaybackPipelines %p\n", id, mpPlaybackPipelines);
    }

    return NULL;
}

SPlaybackPipeline *CLWMDEngine::queryPlaybackPipelineFromARID(TGenericID id)
{
    TU32 i = 0;
    SPlaybackPipeline *p_pipeline = NULL;

    for (i = 0; i < mnPlaybackPipelinesCurrentNumber; i ++) {
        p_pipeline = mpPlaybackPipelines + i;
        if (p_pipeline->audio_renderer_id == id) {
            return p_pipeline;
        }
    }

    return NULL;
}

EECode CLWMDEngine::setupPlaybackPipeline(TComponentIndex index)
{
    if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
        SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;
        TGenericID video_source_id = p_pipeline->video_source_id;
        TGenericID video_decoder_id = p_pipeline->video_decoder_id;
        TGenericID video_renderer_id = p_pipeline->video_renderer_id;
        TGenericID audio_source_id = p_pipeline->audio_source_id;
        TGenericID audio_decoder_id = p_pipeline->audio_decoder_id;
        TGenericID audio_renderer_id = p_pipeline->audio_renderer_id;

        DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, index) == p_pipeline->pipeline_id);
        DASSERT(p_pipeline->index == index);
        DASSERT(EGenericPipelineType_Playback == p_pipeline->type);
        DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
        DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

        if (video_source_id) {

            if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_Demuxer))) {
                LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(video_decoder_id, EGenericComponentType_VideoDecoder))) {
                LOGM_ERROR("not valid video decoder id %08x\n", video_decoder_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(video_renderer_id, EGenericComponentType_VideoRenderer))) {
                LOGM_ERROR("not valid video renderer id %08x\n", video_renderer_id);
                return EECode_BadParam;
            }

            p_pipeline->video_enabled = 1;
        } else {
            DASSERT(!video_decoder_id);
            DASSERT(!video_renderer_id);
            p_pipeline->video_enabled = 0;
        }

        if (audio_source_id) {
            if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_Demuxer))) {
                LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                return EECode_BadParam;
            }

            if (audio_decoder_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_decoder_id, EGenericComponentType_AudioDecoder))) {
                    LOGM_ERROR("not valid audio decoder id %08x\n", audio_decoder_id);
                    return EECode_BadParam;
                }
            }

            if (DUnlikely(0 == isComponentIDValid(audio_renderer_id, EGenericComponentType_AudioRenderer))) {
                LOGM_ERROR("not valid audio renderer/output id %08x\n", audio_renderer_id);
                return EECode_BadParam;
            }

            p_pipeline->audio_enabled = 1;
        } else {
            DASSERT(!audio_decoder_id);
            DASSERT(!audio_renderer_id);
            p_pipeline->audio_enabled = 0;
        }

        if (DLikely(p_pipeline->video_enabled || p_pipeline->audio_enabled)) {
            p_pipeline->pipeline_state = EGenericPipelineState_build_gragh;
            SComponent *p_component = NULL;
            SConnection *p_connection = NULL;

            if (p_pipeline->video_enabled) {
                p_component = findComponentFromID(p_pipeline->video_source_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_source = p_component;
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_source_id %08x\n", p_pipeline->video_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_component = findComponentFromID(p_pipeline->video_decoder_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_decoder = p_component;
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_decoder_id %08x\n", p_pipeline->video_decoder_id);
                    return EECode_InternalLogicalBug;
                }

                p_component = findComponentFromID(p_pipeline->video_renderer_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_renderer = p_component;
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_renderer_id %08x\n", p_pipeline->video_renderer_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(p_pipeline->video_source_id, p_pipeline->video_decoder_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->vd_input_connection_id = p_connection->connection_id;
                    p_pipeline->p_vd_input_connection = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_source_id %08x and video_decoder_id %08x\n", p_pipeline->video_source_id, p_pipeline->video_decoder_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(p_pipeline->video_decoder_id, p_pipeline->video_renderer_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->vd_output_connection_id = p_connection->connection_id;
                    p_pipeline->p_vd_output_connection = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_decoder_id %08x and video_renderer_id %08x\n", p_pipeline->video_decoder_id, p_pipeline->video_renderer_id);
                    return EECode_InternalLogicalBug;
                }

            }

            if (p_pipeline->audio_enabled) {
                p_component = findComponentFromID(p_pipeline->audio_source_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_audio_source = p_component;
                } else {
                    LOGM_ERROR("BAD p_pipeline->audio_source_id %08x\n", p_pipeline->audio_source_id);
                    return EECode_InternalLogicalBug;
                }

                if (p_pipeline->audio_decoder_id) {
                    p_component = findComponentFromID(p_pipeline->audio_decoder_id);
                    if (DLikely(p_component)) {
                        p_component->playback_pipeline_number ++;
                        p_pipeline->p_audio_decoder = p_component;
                    } else {
                        LOGM_ERROR("BAD p_pipeline->audio_decoder_id %08x\n", p_pipeline->audio_decoder_id);
                        return EECode_InternalLogicalBug;
                    }
                }

                p_component = findComponentFromID(p_pipeline->audio_renderer_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_audio_renderer = p_component;
                } else {
                    LOGM_ERROR("BAD p_pipeline->audio_renderer_id %08x\n", p_pipeline->audio_renderer_id);
                    return EECode_InternalLogicalBug;
                }

                if (p_pipeline->audio_decoder_id) {
                    p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_decoder_id, StreamType_Audio);
                    if (DLikely(p_connection)) {
                        p_pipeline->ad_input_connection_id = p_connection->connection_id;
                        p_pipeline->p_ad_input_connection = p_connection;
                    } else {
                        LOGM_ERROR("no connection between audio_source_id %08x and audio_decoder_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_decoder_id);
                        return EECode_InternalLogicalBug;
                    }

                    p_connection = queryConnection(p_pipeline->audio_decoder_id, p_pipeline->audio_renderer_id, StreamType_Audio);
                    if (DLikely(p_connection)) {
                        p_pipeline->ad_output_connection_id = p_connection->connection_id;
                        p_pipeline->p_ad_output_connection = p_connection;
                    } else {
                        LOGM_ERROR("no connection between audio_decoder_id %08x and audio_renderer_id %08x\n", p_pipeline->audio_decoder_id, p_pipeline->audio_renderer_id);
                        return EECode_InternalLogicalBug;
                    }
                } else {
                    p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_renderer_id, StreamType_Audio);
                    if (DLikely(p_connection)) {
                        p_pipeline->as_output_connection_id = p_connection->connection_id;
                        p_pipeline->p_as_output_connection = p_connection;
                    } else {
                        LOGM_ERROR("no connection between audio_source_id %08x and audio_renderer_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_renderer_id);
                        return EECode_InternalLogicalBug;
                    }
                }
            }

            //if ((p_pipeline->video_enabled) && (p_pipeline->audio_enabled) && p_pipeline->p_video_decoder) {
            //    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_wait_audio_precache, 1, NULL);
            //}

            p_pipeline->pipeline_state = EGenericPipelineState_suspended;
            LOGM_PRINTF("playback pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);

        } else {
            LOGM_ERROR("video and audio path are both disabled?\n");
            return EECode_BadParam;
        }

    } else {
        LOGM_ERROR("BAD state, mpPlaybackPipelines %p\n", mpPlaybackPipelines);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CLWMDEngine::setupRecordingPipeline(TComponentIndex index)
{
    if (DLikely(mpRecordingPipelines && (mnRecordingPipelinesCurrentNumber > index))) {
        SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;
        TGenericID video_source_id = p_pipeline->video_source_id[0];
        TGenericID audio_source_id = p_pipeline->audio_source_id[0];
        TGenericID sink_id = p_pipeline->sink_id;

        DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, index) == p_pipeline->pipeline_id);
        DASSERT(p_pipeline->index == index);
        DASSERT(EGenericPipelineType_Recording == p_pipeline->type);
        DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
        DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

        if (DUnlikely(0 == isComponentIDValid(sink_id, EGenericComponentType_Muxer))) {
            LOGM_ERROR("not valid sink_id %08x\n", sink_id);
            return EECode_BadParam;
        }

        if (video_source_id) {
            if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_Demuxer))) {
                LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                return EECode_BadParam;
            }
            p_pipeline->video_enabled = 1;
        } else {
            p_pipeline->video_enabled = 0;
        }

        if (audio_source_id) {
            if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_Demuxer))) {
                LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                return EECode_BadParam;
            }
            p_pipeline->audio_enabled = 1;
        } else {
            p_pipeline->audio_enabled = 0;
        }

        if (DLikely(p_pipeline->video_enabled || p_pipeline->audio_enabled)) {
            p_pipeline->pipeline_state = EGenericPipelineState_build_gragh;
            SComponent *p_component = NULL;
            SConnection *p_connection = NULL;

            p_component = findComponentFromID(sink_id);
            if (DLikely(p_component)) {
                p_component->recording_pipeline_number ++;
                p_pipeline->p_sink = p_component;
            } else {
                LOGM_ERROR("BAD sink_id %08x\n", sink_id);
                return EECode_InternalLogicalBug;
            }

            if (p_pipeline->video_enabled) {
                p_component = findComponentFromID(video_source_id);
                if (DLikely(p_component)) {
                    p_component->recording_pipeline_number ++;
                    p_pipeline->p_video_source[0] = p_component;
                } else {
                    LOGM_ERROR("BAD video_source_id %08x\n", video_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(video_source_id, sink_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->video_source_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_video_source_connection[0] = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_source_id %08x and sink_id %08x\n", video_source_id, sink_id);
                    return EECode_InternalLogicalBug;
                }
            }

            if (p_pipeline->audio_enabled) {
                p_component = findComponentFromID(audio_source_id);
                if (DLikely(p_component)) {
                    p_component->recording_pipeline_number ++;
                    p_pipeline->p_audio_source[0] = p_component;
                } else {
                    LOGM_ERROR("BAD audio_source_id %08x\n", audio_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(audio_source_id, sink_id, StreamType_Audio);
                if (DLikely(p_connection)) {
                    p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_audio_source_connection[0] = p_connection;
                } else {
                    LOGM_ERROR("no connection between audio_source_id %08x and sink_id %08x\n", audio_source_id, sink_id);
                    return EECode_InternalLogicalBug;
                }

            }

            p_pipeline->pipeline_state = EGenericPipelineState_suspended;
            LOG_PRINTF("recording pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
        } else {
            LOGM_ERROR("video and audio path are both disabled?\n");
            return EECode_BadParam;
        }

    } else {
        LOGM_ERROR("BAD state, mpRecordingPipelines %p\n", mpRecordingPipelines);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CLWMDEngine::setupStreamingPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnStreamingPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((NULL == mpStreamingPipelines) || (NULL == mpStreamingContent))) {
        LOGM_FATAL("NULL mpStreamingPipelines %p or mpStreamingContent %p\n", mpStreamingPipelines, mpStreamingContent);
        return EECode_InternalLogicalBug;
    }

    SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;
    SStreamingSessionContent *p_content = mpStreamingContent + index;
    TComponentIndex track_video_input_index = 0, track_audio_input_index = 0;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_Streaming == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    //LOGM_DEBUG("streaming pipeline, %d\n", index);

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->server_id, EGenericComponentType_StreamingServer))) {
        LOGM_ERROR("not valid p_pipeline->server_id %08x\n", p_pipeline->server_id);
        return EECode_BadParam;
    }

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->data_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
        LOGM_ERROR("not valid p_pipeline->data_transmiter_id %08x\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }

    if (p_pipeline->video_source_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_2(p_pipeline->video_source_id[0], EGenericComponentType_Demuxer, EGenericComponentType_VideoEncoder))) {
            LOGM_ERROR("not valid p_pipeline->video_source_id[0] %08x\n", p_pipeline->video_source_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->video_enabled = 1;
        //LOGM_DEBUG("streaming pipeline, have video source\n");
    } else {
        p_pipeline->video_enabled = 0;
    }

    if (p_pipeline->audio_source_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_2(p_pipeline->audio_source_id[0], EGenericComponentType_Demuxer, EGenericComponentType_AudioEncoder))) {
            LOGM_ERROR("not valid p_pipeline->audio_source_id[0] %08x\n", p_pipeline->audio_source_id[0]);
            return EECode_BadParam;
        }
        //LOGM_DEBUG("streaming pipeline, have audio source, pin muxer 0x%08x\n", p_pipeline->audio_pinmuxer_id);
        p_pipeline->audio_enabled = 1;
    } else {
        p_pipeline->audio_enabled = 0;
        //LOGM_DEBUG("streaming pipeline, no audio source, and no audio pin muxer\n");
    }

    IStreamingServer *p_server = NULL;
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    p_server = findStreamingServer(p_pipeline->server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad p_pipeline->server_id 0x%08x.\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_component = findComponentFromID(p_pipeline->server_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL server component, or BAD p_pipeline->server_id 0x%08x?\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_pipeline->p_server = p_component;
    p_component->streaming_pipeline_number ++;

    p_component = findComponentFromID(p_pipeline->data_transmiter_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL transmiter component, or BAD p_pipeline->data_transmiter_id 0x%08x?\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }
    p_pipeline->p_data_transmiter = p_component;
    p_component->streaming_pipeline_number ++;

    if (p_pipeline->video_enabled) {
        p_component = findComponentFromID(p_pipeline->video_source_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL video source component, or BAD p_pipeline->video_source_id[0] 0x%08x?\n", p_pipeline->video_source_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->p_video_source[0] = p_component;
        p_component->streaming_pipeline_number ++;

        p_connection = queryConnection(p_pipeline->video_source_id[0], p_pipeline->data_transmiter_id, StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_source_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_video_source_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->video_source_id[0] %08x and p_pipeline->data_transmiter_id %08x\n", p_pipeline->video_source_id[0], p_pipeline->data_transmiter_id);
            return EECode_InternalLogicalBug;
        }

        track_video_input_index = p_connection->down_pin_index;
    }

    if (p_pipeline->audio_enabled) {

        if (p_pipeline->audio_source_id[0]) {
            p_component = findComponentFromID(p_pipeline->audio_source_id[0]);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio source component, or BAD p_pipeline->audio_source_id[0] 0x%08x?\n", p_pipeline->audio_source_id[0]);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_source[0] = p_component;
            p_component->streaming_pipeline_number ++;
        }

        if (DLikely(p_pipeline->p_audio_source[0])) {
            p_connection = queryConnection(p_pipeline->audio_source_id[0], p_pipeline->data_transmiter_id, StreamType_Audio);
            if (DLikely(p_connection)) {
                p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
                p_pipeline->p_audio_source_connection[0] = p_connection;
            } else {
                LOGM_ERROR("no connection between audio_source_id[0] %08x and transmiter %08x\n", p_pipeline->audio_source_id[0], p_pipeline->data_transmiter_id);
                return EECode_InternalLogicalBug;
            }
            track_audio_input_index = p_connection->down_pin_index;
        } else {
            LOGM_FATAL("should not comes here\n");
        }


    }

    p_pipeline->pipeline_state = EGenericPipelineState_suspended;
    LOG_PRINTF("[Gragh]: streaming pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingContent, index);
    p_content->content_index = index;

    p_content->p_server = p_server;

    snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "%s", p_pipeline->url);
    p_content->session_name[DMaxStreamContentUrlLength - 1] = 0x0;

    p_content->enabled = 1;
    p_content->sub_session_count = 0;
    p_content->content_type = 0;

    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
        EECode reterr = EECode_OK;
        SQueryInterface query;

        if (p_pipeline->video_enabled) {
            query.index = track_video_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Video;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_video] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_video]->p_transmiter));
            p_content->sub_session_count ++;
            LOG_INFO("streaming pipeline(%08x), add video sub session\n", p_pipeline->pipeline_id);
        }

        if (p_pipeline->audio_enabled) {
            query.index = track_audio_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Audio;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_audio] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_audio]->p_transmiter));
            p_content->sub_session_count ++;
            LOG_INFO("streaming pipeline(%08x), add audio sub session\n", p_pipeline->pipeline_id);
        }
    } else {
        LOGM_FATAL("NULL p_pipeline(%08x)->p_data_transmiter->p_filter\n", p_pipeline->pipeline_id);
        return EECode_InternalLogicalBug;
    }

    p_content->has_video = p_pipeline->video_enabled;
    if (p_pipeline->video_enabled) {
        p_content->sub_session_content[ESubSession_video]->enabled = 1;
    }

    p_content->has_audio = p_pipeline->audio_enabled;
    if (p_pipeline->audio_enabled) {
        p_content->sub_session_content[ESubSession_audio]->enabled = 1;
    }

    if (DLikely(mpStreamingServerManager)) {
        LOG_PRINTF("[Gragh]: pipeline %08x, add streaming content %s, has video %d, has audio %d\n", p_pipeline->pipeline_id, p_content->session_name, p_content->has_video, p_content->has_audio);
        p_content->p_streaming_transmiter_filter = p_pipeline->p_data_transmiter->p_filter;
        if (p_pipeline->p_video_source[0]) {
            p_content->p_video_source_filter = p_pipeline->p_video_source[0]->p_filter;
        }
        if (p_pipeline->p_audio_source[0]) {
            p_content->p_audio_source_filter = p_pipeline->p_audio_source[0]->p_filter;
        }
        mpStreamingServerManager->AddStreamingContent(p_server, p_content);
    }

    return EECode_OK;
}

void CLWMDEngine::setupStreamingServerManger()
{
    DASSERT(!mpStreamingServerManager);
    if (mpStreamingServerManager) {
        return;
    }

    mpStreamingServerManager = gfCreateStreamingServerManager(&mPersistMediaConfig, (IMsgSink *)this, mpSystemClockReference);
    if (!mpStreamingServerManager) {
        LOGM_ERROR("Create CStreammingServerManager fail.\n");
        return;
    }

    mpStreamingServerManager->RunServerManager();

}

EECode CLWMDEngine::addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode)
{
    IStreamingServer *p_server;
    server_id = 0;

    if (!mpStreamingServerManager) {
        setupStreamingServerManger();
        if (!mpStreamingServerManager) {
            LOGM_ERROR("setupStreamingServerManger() fail\n");
            return EECode_Error;
        }
    }

    //add new server
    mPersistMediaConfig.rtsp_streaming_video_enable = 1;
    mPersistMediaConfig.rtsp_server_config.rtsp_listen_port = 554;
    LOGM_NOTICE("streaming server port %d, video enable %d, audio enable %d\n", mPersistMediaConfig.rtsp_server_config.rtsp_listen_port, mPersistMediaConfig.rtsp_streaming_video_enable, mPersistMediaConfig.rtsp_streaming_audio_enable);
    p_server = mpStreamingServerManager->AddServer(type, mode, mPersistMediaConfig.rtsp_server_config.rtsp_listen_port, mPersistMediaConfig.rtsp_streaming_video_enable, mPersistMediaConfig.rtsp_streaming_audio_enable);

    if (DLikely(p_server)) {
        server_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingServer, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start]);
        p_server->SetServerID(server_id, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start], EGenericComponentType_StreamingServer);
        mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] ++;
        mpStreamingServerList->InsertContent(NULL, (void *)p_server, 0);
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, mode);

        SMSG msg;
        memset(&msg, 0x0, sizeof(msg));
        msg.code = EMSGType_ServerError;
        msg.sessionID = mSessionID;
        PostAppMsg(msg);
    }

    return EECode_Error;
}

EECode CLWMDEngine::removeStreamingServer(TGenericID server_id)
{
    if (DUnlikely(!mpStreamingServerManager)) {
        LOGM_ERROR("NULL mpStreamingServerManager, but someone invoke CLWMDEngine::removeStreamingServer?\n");
        return EECode_Error;
    }

    IStreamingServer *p_server = findStreamingServer(server_id);
    if (DLikely(p_server)) {
        mpStreamingServerList->RemoveContent((void *)p_server);
        mpStreamingServerManager->RemoveServer(p_server);
    } else {
        LOGM_ERROR("NO Server's id is 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

IStreamingServer *CLWMDEngine::findStreamingServer(TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode;
    IStreamingServer *p_server;
    TGenericID id1;
    TComponentIndex index;
    TComponentType type;

    pnode = mpStreamingServerList->FirstNode();
    while (pnode) {
        p_server = (IStreamingServer *) pnode->p_context;
        p_server->GetServerID(id1, index, type);
        if (id1 == id) {
            LOG_INFO("find streaming server in list(id 0x%08x).\n", id);
            return p_server;
        }
        pnode = mpStreamingServerList->NextNode(pnode);
    }

    LOG_WARN("cannot find streaming server in list(id 0x%08x).\n", id);
    return NULL;
}

void CLWMDEngine::exitServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->ExitServerManager();
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }
}

void CLWMDEngine::startServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Start();
    }
}

void CLWMDEngine::stopServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Stop();
    }
}

EECode CLWMDEngine::createGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;
    IFilter *pfilter = NULL;
    EECode err;
    TU32 uppin_index, uppin_sub_index, downpin_index;

    LOGM_INFO("createGraph start.\n");

    //create all filters
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOG_INFO("create component, type %d, %s, index %d\n", p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
            pfilter = gfFilterFactory(__filterTypeFromGenericComponentType(p_component->type, mPersistMediaConfig.number_scheduler_io_writer), (const SPersistMediaConfig *)&mPersistMediaConfig, (IMsgSink *)this, (TU32)p_component->index);
            if (DLikely(pfilter)) {
                p_component->p_filter = pfilter;
            } else {
                LOGM_FATAL("CLWMDEngine::createGraph gfFilterFactory(type %d) fail.\n", p_component->type);
                return EECode_InternalLogicalBug;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOGM_INFO("createGraph filters construct done, start connection.\n");

    pnode = mpListConnections->FirstNode();
    //implement connetions
    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection) {
            uppin_sub_index = 0;
            //debug assert
            DASSERT((p_connection->up) && (p_connection->down));
            //DASSERT(p_connection->up_id == ((TU32)DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p_connection->up->type, p_connection->up->index)));
            //DASSERT(p_connection->down_id == ((TU32)DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p_connection->down->type, p_connection->down->index)));
            LOGM_INFO("Start connect, upper: type(%s) %d, index %d, down: type(%s) %d, index %d\n", gfGetComponentStringFromGenericComponentType(p_connection->up->type), p_connection->up->type, p_connection->up->index, gfGetComponentStringFromGenericComponentType(p_connection->down->type), p_connection->down->type, p_connection->down->index);

            DASSERT((StreamType_Video == p_connection->pin_type) || (StreamType_Audio == p_connection->pin_type));
            err = p_connection->up->p_filter->AddOutputPin(uppin_index, uppin_sub_index, p_connection->pin_type);
            DASSERT(EECode_OK == err);
            err = p_connection->down->p_filter->AddInputPin(downpin_index, p_connection->pin_type);
            DASSERT(EECode_OK == err);

            if ((err = Connect(p_connection->up->p_filter, uppin_index, p_connection->down->p_filter, downpin_index, uppin_sub_index)) != EECode_OK) {
                LOGM_ERROR("Connect(up(%s) 0x%08x, down(%s) 0x%08x) failed, ret %d, %s\n", gfGetComponentStringFromGenericComponentType(p_connection->up->type), p_connection->up->id, gfGetComponentStringFromGenericComponentType(p_connection->down->type), p_connection->down->id, err, gfGetErrorCodeString(err));
                return err;
            }

            p_connection->up_pin_index = (TU8)uppin_index;
            p_connection->down_pin_index = (TU8)downpin_index;
            p_connection->up_pin_sub_index = (TU8)uppin_sub_index;
            LOGM_INFO("Connect success, up index %d, sub index %d, down index %d\n", uppin_index, uppin_sub_index, downpin_index);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    err = setupPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("setupPipelines return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOGM_INFO("createGraph done.\n");

    mbGraghBuilt = 1;

    return EECode_OK;
}

EECode CLWMDEngine::setupPipelines(void)
{
    TComponentIndex index = 0;
    EECode err = EECode_OK;

    LOG_NOTICE("setupPipelines: pb pipelines %d, rec pipelines %d, streaming pipelines %d\n",
               mnPlaybackPipelinesCurrentNumber, mnRecordingPipelinesCurrentNumber, mnStreamingPipelinesCurrentNumber);

    DASSERT(mpPlaybackPipelines);
    DASSERT(mpRecordingPipelines);
    DASSERT(mpStreamingPipelines);

    if (mpPlaybackPipelines && mnPlaybackPipelinesCurrentNumber) {
        for (index = 0; index < mnPlaybackPipelinesCurrentNumber; index ++) {
            err = setupPlaybackPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, playback pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpRecordingPipelines && mnRecordingPipelinesCurrentNumber) {
        for (index = 0; index < mnRecordingPipelinesCurrentNumber; index ++) {
            err = setupRecordingPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, recording pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpStreamingPipelines && mnStreamingPipelinesCurrentNumber) {
        for (index = 0; index < mnStreamingPipelinesCurrentNumber; index ++) {
            err = setupStreamingPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, streaming pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    return EECode_OK;
}

EECode CLWMDEngine::clearGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    LOGM_INFO("clearGraph start.\n");

    //delete all filters
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, id %08x, type %d, %s, index %d\n", p_component->id, p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    //delete all proxys
    pnode = mpListProxys->FirstNode();
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, id %08x type %d, %s, index %d\n", p_component->id, p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListProxys->NextNode(pnode);
    }

    //delete all connections
    pnode = mpListConnections->FirstNode();
    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection) {
            LOGM_INFO("destroy connection, pin type %d, id %08x\n", p_connection->pin_type, p_connection->connection_id);
            deleteConnection(p_connection);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    LOGM_INFO("clearGraph end.\n");

    return EECode_OK;
}

bool CLWMDEngine::processGenericMsg(SMSG &msg)
{
    SMSG msg0;
    SComponent *p_component = NULL;

    //LOG_INFO("[MSG flow]: code %d, msg.session id %d, session %d, before AUTO_LOCK.\n", msg.code, msg.sessionID, mSessionID);

    AUTO_LOCK(mpMutex);

    if (!IsSessionMsg(msg)) {
        LOGM_ERROR("!!!!not session msg, code %d, msg.session id %d, session %d.\n", msg.code, msg.sessionID, mSessionID);
        return true;
    }

    //check components
    if (msg.p_owner) {
        p_component = findComponentFromFilter((IFilter *)msg.p_owner);
        DASSERT(p_component);
        if (!p_component) {
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code 0x%08x\n", msg.code);
            return true;
        }
    } else if (msg.owner_id) {
        p_component = queryFilterComponent(msg.owner_id);
        DASSERT(p_component);
        if (!p_component) {
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code 0x%08x\n", msg.code);
            return true;
        }
    }

    switch (msg.code) {

        case EMSGType_Timeout:
            LOG_INFO("[MSG flow]: Demuxer timeout msg comes, type %d, index %d, GenericID 0x%08x.\n", p_component->type, p_component->index, p_component->id);

            if (!mbReconnecting) {
                DASSERT(p_component->p_filter);
                if (p_component->p_filter) {
                    if (EGenericComponentType_Demuxer == p_component->type) {
                        memset(&msg0, 0x0, sizeof(msg0));
                        msg0.code = EMSGType_RePlay;
                        msg0.sessionID = mSessionID;
                        msg0.owner_id = p_component->id;
                        PostAppMsg(msg0);
                        mbReconnecting = 1;
                    } else {
                        LOGM_ERROR("need handle here\n");
                    }
                }
            }
            break;

        case EMSGType_PlaybackStatisticsFPS:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_PlaybackStatisticsFPS;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            msg0.f0 = msg.f0;
            PostAppMsg(msg0);
            break;

        case EMSGType_VideoSize:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_VideoSize;
            msg0.sessionID = mSessionID;
            msg0.p1 = msg.p1;
            msg0.p2 = msg.p2;
            PostAppMsg(msg0);
            break;

        case EMSGType_PlaybackEOS:
            //memset(&msg0, 0x0, sizeof(msg0));
            //msg0.code = EMSGType_PlaybackEOS;
            //msg0.sessionID = mSessionID;
            //msg0.owner_id = p_component->id;
            //PostAppMsg(msg0);
            break;

        case EMSGType_MissionComplete:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_MissionComplete;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_ClientReconnectDone:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_ClientReconnectDone;
            msg0.sessionID = mSessionID;
            PostAppMsg(msg0);
            mbReconnecting = 0;
            break;

        case EMSGType_UnknownError:
        case EMSGType_NetworkError:
            if ((!mbReconnecting) && (!mbLoopPlay)) {
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_NetworkError;
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->id;
                PostAppMsg(msg0);
            } else if (!mbReconnecting) {
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_RePlay;
                LOG_NOTICE("network issue, reconnect now\n");
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->id;
                PostAppMsg(msg0);
                mbReconnecting = 1;
            }
            break;

        case EMSGType_StorageError:
            //post msg to app
            LOGM_ERROR("[MSG flow]: EMSGType_StorageError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_StorageError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            //abort recording, the muxed file maybe corrupted
            LOGM_ERROR("EMSGType_StorageError: TODO\n");
            break;

        case EMSGType_SystemError:
            LOGM_ERROR("[MSG flow]: EMSGType_SystemError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_SystemError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_IOError:
            LOGM_ERROR("[MSG flow]: EMSGType_IOError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_IOError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_DeviceError:
            LOGM_ERROR("[MSG flow]: EMSGType_DeviceError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_DeviceError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_NotifyNewFileGenerated:
            LOG_INFO("EMSGType_NotifyNewFileGenerated msg comes, IFilter %p, filter type %d, index %d, current index %ld\n", (IFilter *)msg.p_owner, p_component->type, p_component->index, msg.p2);

            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_NotifyNewFileGenerated;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_NotifyUDECUpdateResolution:
            PostAppMsg(msg);
            break;

        case EMSGType_StreamingError_TCPSocketConnectionClose:
            LOG_NOTICE("EMSGType_StreamingError_TCPSocketConnectionClose msg comes, session %08lx, transmiter filter %08lx, streaming server %08lx\n", msg.p0, msg.p1, msg.p2);
            if (mpStreamingServerManager) {
                mpStreamingServerManager->RemoveSession((IStreamingServer *) msg.p2, (void *) msg.p0);
            }
            break;

        case EMSGType_StreamingError_UDPSocketInvalidArgument:
            LOG_NOTICE("EMSGType_StreamingError_UDPSocketInvalidArgument msg comes, session %08lx, transmiter filter %08lx, streaming server %08lx\n", msg.p0, msg.p1, msg.p2);
            if (mpStreamingServerManager) {
                mpStreamingServerManager->RemoveSession((IStreamingServer *) msg.p2, (void *) msg.p0);
            }
            break;

        case EMSGType_OpenSourceFail:
            LOG_NOTICE("engine EMSGType_OpenSourceFail\n");
            PostAppMsg(msg);
            break;
        case EMSGType_OpenSourceDone:
            LOG_NOTICE("engine EMSGType_OpenSourceDone\n");
            PostAppMsg(msg);
            break;

        case EMSGType_AudioPrecacheReadyNotify: {
                SPlaybackPipeline *p_pipeline = queryPlaybackPipelineFromARID((TGenericID) msg.owner_id);
                if (p_pipeline && p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
                    p_pipeline->p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_audio_precache_notify, 1, NULL);
                }
            }
            break;

        default:
            LOGM_ERROR("unknown msg code %x\n", msg.code);
            break;
    }

    return true;
}

bool CLWMDEngine::processGenericCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
    //LOGM_FLOW("[cmd flow]: cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {

        case ECMDType_RunProcessing:
            err = doRunProcessing();
            mpWorkQ->CmdAck(err);
            break;

        case ECMDType_ExitProcessing:
            err = doExitProcessing();
            mpWorkQ->CmdAck(err);
            break;

        case ECMDType_Start:
            err = doStart();
            mpWorkQ->CmdAck(err);
            break;

        case ECMDType_Stop:
            err = doStop();
            mpWorkQ->CmdAck(err);
            break;

        case ECMDType_PrintCurrentStatus:
            doPrintCurrentStatus((TGenericID)cmd.res32_1, (TULong)cmd.res64_1);
            break;

        case ECMDType_ExitRunning:
            mbRun = false;
            break;

        default:
            LOGM_ERROR("unknown cmd %x.\n", cmd.code);
            return false;
    }

    return true;
}

void CLWMDEngine::processCMD(SCMD &oricmd)
{
    TU32 remaingCmd;
    SCMD cmd1;
    DASSERT(mpCmdQueue);

    if (oricmd.repeatType == CMD_TYPE_SINGLETON) {
        processGenericCmd(oricmd);
        return;
    }

    remaingCmd = mpCmdQueue->GetDataCnt();
    if (remaingCmd == 0) {
        processGenericCmd(oricmd);
        return;
    } else if (remaingCmd == 1) {
        processGenericCmd(oricmd);
        mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1));
        processGenericCmd(cmd1);
        return;
    }

    //process multi cmd
    if (oricmd.repeatType == CMD_TYPE_REPEAT_LAST) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                processGenericCmd(oricmd);
                processGenericCmd(cmd1);
                return;
            }
        }
        processGenericCmd(cmd1);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_CNT) {
        //to do
        processGenericCmd(oricmd);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_AVATOR) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                processGenericCmd(oricmd);
                processGenericCmd(cmd1);
                return;
            } else {
                //to do
                processGenericCmd(cmd1);
                return;
            }
        }
    } else {
        LOGM_FATAL("must not come here.\n");
        return;
    }
}

EECode CLWMDEngine::initializeFilter(SComponent *component, TU8 type)
{
    EECode err = EECode_OK;
    IFilter *p_filter = NULL;
    DASSERT(component);

    p_filter = component->p_filter;
    DASSERT(p_filter);

    switch (type) {

        case EGenericComponentType_Demuxer:
            if (component->url) {
                err = p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, (void *) component->url);
                DASSERT_OK(err);
            }
            err = p_filter->Initialize(component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoDecoder:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoEncoder:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioEncoder:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioDecoder:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_Muxer:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);

            err = p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, component->url);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoRenderer:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioRenderer:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_StreamingTransmiter:
            err = p_filter->Initialize((TChar *)component->prefer_string);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_FlowController:
            err = p_filter->Initialize();
            break;

        default:
            LOGM_FATAL("unknown filter type %d\n", type);
            break;
    }

    if (DLikely(EECode_OK != err)) {
        LOGM_FATAL("filter->Initialize() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CLWMDEngine::initializeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOG_INFO(" initialize all filters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_not_alive == p_component->cur_state);

            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = initializeFilter(p_component, p_component->type);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    p_component->cur_state = EComponentState_initialized;
                } else {
                    LOGM_ERROR("initializeFilter(%s) fail, index %d, pipeline_id 0x%08x\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index, pipeline_id);
                    return err;
                }
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" initialize all filters end\n");
    } else {
        LOGM_ERROR("initializeAllFilters: TODO!\n");
    }

    return EECode_OK;
}

EECode CLWMDEngine::finalizeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOG_INFO(" finalizeAllFilters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_initialized == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = p_filter->Finalize();
                DASSERT_OK(err);
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" finalizeAllFilters end\n");
    } else {
        LOGM_ERROR("finalizeAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CLWMDEngine::runAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOG_INFO(" run all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_initialized == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Run();
                p_component->cur_state = EComponentState_activated;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" run all fiters end\n");
    } else {
        LOGM_ERROR("runAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CLWMDEngine::exitAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOG_INFO(" exit all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_activated == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Exit();
                p_component->cur_state = EComponentState_initialized;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" exit all fiters end\n");
    } else {
        LOGM_ERROR("exitAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CLWMDEngine::startAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOG_INFO(" start all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_activated == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Start();
                LOG_PRINTF("[Flow]: component start (0x%08x)\n", p_component->id);
                p_component->cur_state = EComponentState_started;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" start all fiters end\n");
    } else {
        LOGM_ERROR("startAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CLWMDEngine::stopAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_DEBUG(" stop all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_started == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            LOGM_DEBUG("before stop, %08x\n", p_component->id);
            if (p_filter) {
                p_filter->Stop();
                p_component->cur_state = EComponentState_activated;
            }
            LOGM_DEBUG("after stop\n");
            pnode = mpListFilters->NextNode(pnode);
        }
        LOG_INFO(" stop all fiters end\n");
    } else {
        LOGM_ERROR("stopAllFilters: TODO\n");
    }

    return EECode_OK;
}

bool CLWMDEngine::allFiltersFlag(TU32 flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                if (!(flag & p_component->status_flag)) {
                    return false;
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        return true;
    } else {
        LOGM_ERROR("allFiltersFlag: TODO\n");
        return true;
    }
}

void CLWMDEngine::clearAllFiltersFlag(TU32 flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                p_component->status_flag &= (~ flag);
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mpListFilters->NextNode(pnode);
        }
    } else {
        LOGM_ERROR("clearAllFiltersFlag: TODO\n");
    }
}

void CLWMDEngine::setFiltersEOS(IFilter *pfilter)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (pfilter == p_component->p_filter) {
                p_component->status_flag |= DCAL_BITMASK(DFLAG_STATUS_EOS);
                return;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOG_WARN("do not find filter %p\n", pfilter);
}

EECode CLWMDEngine::createSchedulers(void)
{
    TU32 i = 0, number = 0;

    DASSERT(!mbSchedulersCreated);
    if (!mbSchedulersCreated) {

        if (mPersistMediaConfig.number_scheduler_network_reciever) {
            number = mPersistMediaConfig.number_scheduler_network_reciever;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOG_NOTICE("udp receiver number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_reciever[i]);
                LOG_INFO("p_scheduler_network_reciever, gfSchedulerFactory(ESchedulerType_PriorityPreemptive, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_Preemptive, i);
                //mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_PriorityPreemptive, i);
                if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
                    mPersistMediaConfig.p_scheduler_network_reciever[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_network_tcp_reciever) {
            number = mPersistMediaConfig.number_scheduler_network_tcp_reciever;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOG_NOTICE("tcp receiver number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]);
                LOG_INFO("p_scheduler_network_tcp_reciever, gfSchedulerFactory(ESchedulerType_PriorityPreemptive, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_tcp_reciever[i] = gfSchedulerFactory(ESchedulerType_Preemptive, i);
                //mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_PriorityPreemptive, i);
                if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
                    mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_network_sender) {
            number = mPersistMediaConfig.number_scheduler_network_sender;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOG_NOTICE("network sender number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_sender[i]);
                LOG_INFO("p_scheduler_network_sender, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_sender[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
                    mPersistMediaConfig.p_scheduler_network_sender[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_io_reader) {
            number = mPersistMediaConfig.number_scheduler_io_reader;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOG_NOTICE("io reader number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_io_reader[i]);
                LOG_INFO("p_scheduler_io_reader, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_io_reader[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
                    mPersistMediaConfig.p_scheduler_io_reader[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_io_writer) {
            number = mPersistMediaConfig.number_scheduler_io_writer;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOG_NOTICE("io writer number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_io_writer[i]);
                LOG_INFO("p_scheduler_io_writer, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_io_writer[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
                    mPersistMediaConfig.p_scheduler_io_writer[i]->StartScheduling();
                }
            }
        }

        mbSchedulersCreated = 1;
    } else {
        LOGM_ERROR("already created?\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CLWMDEngine::destroySchedulers(void)
{
    TU32 i = 0;

    DASSERT(mbSchedulersCreated);

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_reciever[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
            mPersistMediaConfig.p_scheduler_network_sender[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_sender[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_sender[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
            mPersistMediaConfig.p_scheduler_io_reader[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_reader[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_io_reader[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
            mPersistMediaConfig.p_scheduler_io_writer[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_writer[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_io_writer[i] = NULL;
        }
    }

    mbSchedulersCreated = 0;

    return EECode_OK;
}

EECode CLWMDEngine::doRunProcessing()
{
    EECode err;

    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doRunProcessing: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbProcessingRunning)) {
        LOGM_ERROR("doRunProcessing: already in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(mRuningCount)) {
        LOGM_ERROR("doRunProcessing: already invoke ExitProcessing.\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doRunProcessing(), start\n");

    if (DLikely(EEngineState_idle == msState)) {
        //start running
        LOGM_INFO("doRunProcessing(), before initializeAllFilters(0)\n");
        err = initializeAllFilters(0);
        if (DUnlikely(err != EECode_OK)) {
            LOGM_ERROR("initializeAllFilters() fail, err %d\n", err);
            return err;
        }
        LOGM_INFO("doRunProcessing(), before runAllFilters(0)\n");

        err = runAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("runAllFilters() fail, err %d\n", err);
            return err;
        }

        mbProcessingRunning = 1;
        mRuningCount ++;
        msState = EEngineState_running;
        LOGM_NOTICE("doRunProcessing(), done\n");
    } else if (EEngineState_running == msState) {
        //already running
        LOGM_ERROR("engine is already running state?\n");
        return EECode_BadState;
    } else {
        //bad state
        LOGM_ERROR("BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CLWMDEngine::doExitProcessing()
{
    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doExitProcessing: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doExitProcessing: not in running state\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doExitProcessing(), start\n");

    if (mbProcessingStarted) {
        LOGM_WARN("doExitProcessing: not stopped yet, stop here\n");
        doStop();
    }

    if (DLikely(EEngineState_running == msState)) {
        mPersistMediaConfig.app_start_exit = 1;
        mpClockManager->UnRegisterClockReference(mpSystemClockReference);
        mpSystemClockReference->ClearAllTimers();
        exitServers();
        exitAllFilters(0);
        finalizeAllFilters(0);
        msState = EEngineState_idle;
        LOGM_NOTICE("doExitProcessing(), done\n");
    } else {
        LOGM_ERROR("BAD state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CLWMDEngine::doStart()
{
    EECode err;

    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doStart: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doStart: not in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbProcessingStarted)) {
        LOGM_ERROR("doStart: already started\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doStart(), start\n");

    if (DLikely(EEngineState_running == msState)) {

        LOG_INFO("doStart(), before startAllFilters(0)\n");
        err = startAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("startAllFilters() fail, err %d\n", err);
            stopAllFilters(0);
            return err;
        }
        LOG_INFO("doStart(), after startAllFilters(0)\n");

        startServers();

        LOGM_NOTICE("doStart(): done\n");
    } else {
        //bad state
        LOGM_ERROR("doStart(): BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CLWMDEngine::doStop()
{
    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doStop: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doStop: not in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingStarted)) {
        LOGM_ERROR("doStop: not started yet\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doStop(), start\n");

    if (DLikely(EEngineState_running == msState)) {
        stopServers();
        stopAllFilters(0);

        if (!mbExternalClock) {
            if (mpClockManager && mbClockManagerStarted) {
                mpClockManager->Stop();
                mbClockManagerStarted = 0;
            }
        }
        LOGM_NOTICE("doStop(), done\n");
    } else {
        LOGM_ERROR("doStop(): BAD state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

void CLWMDEngine::reconnectAllStreamingServer()
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (EGenericComponentType_Demuxer == p_component->type) {
            if (p_component->p_filter) {
                p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, NULL);
            }
        }
        pnode = mpListFilters->NextNode(pnode);
    }
}

void CLWMDEngine::doPrintSchedulersStatus()
{
}

void CLWMDEngine::doPrintCurrentStatus(TGenericID id, TULong print_flag)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    TU32 i = 0, max = 0;

    if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                p_component->p_filter->GetObject0()->PrintStatus();
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mpListFilters->NextNode(pnode);
        }

        if (mpStreamingServerManager) {
            mpStreamingServerManager->PrintStatus0();
            CIDoubleLinkedList::SNode *p_n = mpStreamingServerList->FirstNode();
            if (DLikely(p_n && p_n->p_context)) {
                IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                p_streaming_server->GetObject0()->PrintStatus();
            }
        }

        if (mpClockSource) {
            mpClockSource->GetObject0()->PrintStatus();
        }

        if (mpClockManager) {
            mpClockManager->PrintStatus();
        }

        if (mpSystemClockReference) {
            mpSystemClockReference->PrintStatus();
        }

        doPrintSchedulersStatus();

        SStreamingSessionContent *p_content = NULL;
        max = mnStreamingPipelinesCurrentNumber;
        for (i = 0; i < max; i++) {
            p_content = mpStreamingContent + i;
            LOG_PRINTF("[%d] streaming url %s\n", i, p_content->session_name);
        }

    } else {
        doPrintComponentStatus(id);

        if (mpStreamingServerManager) {
            mpStreamingServerManager->PrintStatus0();
            CIDoubleLinkedList::SNode *p_n = mpStreamingServerList->FirstNode();
            if (DLikely(p_n && p_n->p_context)) {
                IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                p_streaming_server->GetObject0()->PrintStatus();
            }
        }

        if (mpClockSource) {
            mpClockSource->GetObject0()->PrintStatus();
        }

        if (mpClockManager) {
            mpClockManager->PrintStatus();
        }

        if (mpSystemClockReference) {
            mpSystemClockReference->PrintStatus();
        }

        doPrintSchedulersStatus();

    }

}

void CLWMDEngine::doPrintComponentStatus(TGenericID id)
{
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);
    TComponentType index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);

    switch (type) {

        case EGenericPipelineType_Streaming: {
                if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnStreamingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely((NULL == mpStreamingPipelines) || (NULL == mpStreamingContent))) {
                    LOGM_FATAL("NULL mpStreamingPipelines %p or mpStreamingContent %p\n", mpStreamingPipelines, mpStreamingContent);
                    return;
                }

                SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;
                SStreamingSessionContent *p_content = mpStreamingContent + index;

                LOG_PRINTF("@@streaming url %s\n", p_content->session_name);

                if (p_pipeline->p_video_source[0]) {
                    if (p_pipeline->p_video_source[0]->url) {
                        LOG_PRINTF("@@video source url %s\n", p_pipeline->p_video_source[0]->url);
                    }
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (p_pipeline->p_audio_source[0]->url) {
                        LOG_PRINTF("@@audio source url %s\n", p_pipeline->p_audio_source[0]->url);
                    }
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_data_transmiter) {
                    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
                        p_pipeline->p_data_transmiter->p_filter->GetObject0()->PrintStatus();
                    }
                }

            }
            break;

        case EGenericPipelineType_Recording: {
                if (DUnlikely(index >= mnRecordingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnRecordingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpRecordingPipelines)) {
                    LOGM_FATAL("NULL mpRecordingPipelines %p\n", mpRecordingPipelines);
                    return;
                }

                SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;

                if (p_pipeline->p_video_source[0]) {
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_sink) {
                    if (DLikely(p_pipeline->p_sink->p_filter)) {
                        p_pipeline->p_sink->p_filter->GetObject0()->PrintStatus();
                    }
                }

            }
            break;

        case EGenericPipelineType_Playback: {
                if (DUnlikely(index >= mnPlaybackPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnPlaybackPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpPlaybackPipelines)) {
                    LOGM_FATAL("NULL mpPlaybackPipelines %p\n", mpPlaybackPipelines);
                    return;
                }

                SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;

                if (p_pipeline->p_video_source) {
                    if (DLikely(p_pipeline->p_video_source->p_filter)) {
                        p_pipeline->p_video_source->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source) {
                    if (DLikely(p_pipeline->p_audio_source->p_filter)) {
                        p_pipeline->p_audio_source->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_video_decoder) {
                    if (DLikely(p_pipeline->p_video_decoder->p_filter)) {
                        p_pipeline->p_video_decoder->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_decoder) {
                    if (DLikely(p_pipeline->p_audio_decoder->p_filter)) {
                        p_pipeline->p_audio_decoder->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_video_renderer) {
                    if (DLikely(p_pipeline->p_video_renderer->p_filter)) {
                        p_pipeline->p_video_renderer->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_renderer) {
                    if (DLikely(p_pipeline->p_audio_renderer->p_filter)) {
                        p_pipeline->p_audio_renderer->p_filter->GetObject0()->PrintStatus();
                    }
                }
            }
            break;

        default:
            LOGM_ERROR("TO DO 0x%08x, %s\n", id, gfGetComponentStringFromGenericComponentType(type));
            break;
    }
}

void CLWMDEngine::pausePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
        p_pipeline->p_video_source->p_filter->Pause();
    }

    if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
        p_pipeline->p_audio_source->p_filter->Pause();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->Pause();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->Pause();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->Pause();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->Pause();
    }
}

void CLWMDEngine::resumePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
        LOGM_INFO("resume video source\n");
        p_pipeline->p_video_source->p_filter->Resume();
    }

    if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
        LOGM_INFO("resume audio source\n");
        p_pipeline->p_audio_source->p_filter->Resume();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        LOGM_INFO("resume video renderer\n");
        p_pipeline->p_video_renderer->p_filter->Resume();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        LOGM_INFO("resume audio renderer\n");
        p_pipeline->p_audio_renderer->p_filter->Resume();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        LOGM_INFO("resume video decoder\n");
        p_pipeline->p_video_decoder->p_filter->Resume();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        LOGM_INFO("resume audio decoder\n");
        p_pipeline->p_audio_decoder->p_filter->Resume();
    }
}

void CLWMDEngine::purgePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
        LOGM_INFO("flush video source\n");
        p_pipeline->p_video_source->p_filter->Flush();
    }
    if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
        LOGM_INFO("flush audio source\n");
        p_pipeline->p_audio_source->p_filter->Flush();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->Flush();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->Flush();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->Flush();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->Flush();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->ResumeFlush();
    }
}

