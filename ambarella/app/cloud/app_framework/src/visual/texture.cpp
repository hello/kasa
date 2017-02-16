/*
 * texture.cpp
 *
 * History:
 *    2014/10/13 - [Zhi He] create file
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

#include "app_framework_if.h"

#include "platform_al_if.h"

#include "texture.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CGUIArea *gfCreateGUIAreaYUVTexture(IVisualDirectRendering *&p_direct_rendering)
{
    CYUVTexture *thiz = CYUVTexture::Create();
    if (thiz) {
        p_direct_rendering = (IVisualDirectRendering *) thiz;
        return ((CGUIArea *) thiz);
    }

    LOG_FATAL("CYUVTexture::Create() fail\n");
    p_direct_rendering = NULL;
    return NULL;
}

//-----------------------------------------------------------------------
//
// CYUVTexture
//
//-----------------------------------------------------------------------
//#define DCOPY_BUFFER

CYUVTexture *CYUVTexture::Create()
{
    CYUVTexture *result = new CYUVTexture();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CYUVTexture::CYUVTexture()
    : mpMutex(NULL)
    , mpTexture(NULL)
    , mpCopiedBufferBase(NULL)
    , mCopiedBufferSize(0)
    , mbContextSetup(0)
    , mbNeedUpdate(0)
    , mbTextureSetup(0)
    , mbNeedPrepare(0)
{
}

EECode CYUVTexture::Construct()
{
    mpTexture = gfCreate2DTexture(ETextureSourceType_YUV420p);
    if (DUnlikely(!mpTexture)) {
        LOG_FATAL("gfCreate2DTexture(ETextureSourceType_YUV420p) fail\n");
        return EECode_NoMemory;
    }

    memset(&mDataSource, 0x0, sizeof(STextureDataSource));
    return EECode_OK;
}

CYUVTexture::~CYUVTexture()
{
    if (mpCopiedBufferBase) {
        free(mpCopiedBufferBase);
        mpCopiedBufferBase = NULL;
    }
}

EECode CYUVTexture::SetupContext()
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(!mbTextureSetup)) {
        mpTexture->SetupContext();
        mbTextureSetup = 1;
    }

    return EECode_OK;
}

EECode CYUVTexture::PrepareDrawing()
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbNeedPrepare)) {
        EECode err = mpTexture->PrepareTexture();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("mpTexture->PrepareTexture() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        mbNeedPrepare = 0;
    }

    return EECode_OK;
}

EECode CYUVTexture::Draw(TU32 &updated)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpTexture)) {
        if (DLikely(mbNeedUpdate)) {
            mbNeedUpdate = 0;
            updated |= 0x1;
            mFlags &= ~DGUIAreaFlags_needredraw;
            return mpTexture->DrawTexture();
        }
    }

    return EECode_OK;
}

EECode CYUVTexture::Action(EInputEvent event)
{
    LOG_FATAL("CYUVTexture::Action TO DO\n");
    return EECode_NoImplement;
}

void CYUVTexture::NeedReDraw(TU32 &needed) const
{
    AUTO_LOCK(mpMutex);

    if (mFlags & DGUIAreaFlags_needredraw) {
        needed = 1;
    }
}

EECode CYUVTexture::Render(SVisualDirectRenderingContent *p_context, TU32 reuse)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(!mbTextureSetup)) {
        LOG_FATAL("DirectUpdate: context not setup yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbNeedUpdate)) {
        //LOG_NOTICE("DirectUpdate: device busy\n");
        return EECode_Busy;
    }

    mDataSource.width = p_context->size_x;
    mDataSource.height = p_context->size_y;

#ifdef DCOPY_BUFFER
    if (DUnlikely(!mpCopiedBufferBase)) {
        EECode err = allocateCopiedBuffer();
        if (DUnlikely(EECode_OK != err)) {
            LOG_FATAL("CYUVTexture::DirectUpdate: allocateCopiedBuffer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        mDataSource.linesize[0] = mDataSource.width;
        mDataSource.linesize[1] = mDataSource.linesize[2] = mDataSource.width / 2;
    }
    copyBuffer(p_context->data[0], p_context->data[1], p_context->data[2], p_context->linesize[0], p_context->linesize[1], p_context->linesize[2]);
#else
    mDataSource.pdata[0] = p_context->data[0];
    mDataSource.pdata[1] = p_context->data[1];
    mDataSource.pdata[2] = p_context->data[2];
    mDataSource.linesize[0] = p_context->linesize[0];
    mDataSource.linesize[1] = p_context->linesize[1];
    mDataSource.linesize[2] = p_context->linesize[2];
#endif

#ifdef DCONFIG_TEST_END2END_DELAY
    mDataSource.debug_time = p_context->debug_time;
    mDataSource.debug_count = p_context->debug_count;
#endif

    mpTexture->UpdateData(&mDataSource);
    mFlags |= DGUIAreaFlags_needredraw;
    mbNeedUpdate = 1;
    mbNeedPrepare = 1;

    return EECode_OK;
}

void CYUVTexture::Destroy()
{
    delete this;
}

EECode CYUVTexture::allocateCopiedBuffer()
{
    if (DUnlikely(mpCopiedBufferBase)) {
        LOG_FATAL("CYUVTexture::allocateCopiedBuffer(), already allocated?\n");
        return EECode_InternalLogicalBug;
    }

    mCopiedBufferSize = mDataSource.width * mDataSource.height + ((mDataSource.width + 1) / 2) * ((mDataSource.height + 1) / 2) * 2;
    mpCopiedBufferBase = (TU8 *) DDBG_MALLOC(mCopiedBufferSize, "FTCP");

    if (DUnlikely(!mpCopiedBufferBase)) {
        LOG_FATAL("CYUVTexture::allocateCopiedBuffer(), no memory, request size %d\n", mCopiedBufferSize);
        return EECode_NoMemory;
    }

    mDataSource.pdata[0] = mpCopiedBufferBase;
    mDataSource.pdata[1] = mDataSource.pdata[0] + mDataSource.width * mDataSource.height;
    mDataSource.pdata[2] = mDataSource.pdata[1] + ((mDataSource.width + 1) / 2) * ((mDataSource.height + 1) / 2);
    return EECode_OK;
}

void CYUVTexture::copyBuffer(TU8 *py, TU8 *pcb, TU8 *pcr, TU32 linesize_y, TU32 linesize_cb, TU32 linesize_cr)
{
    TU32 i = 0;
    TU32 w = mDataSource.width, h = mDataSource.height;
    TU8 *pdest = mDataSource.pdata[0];

    for (i = 0; i < h; i ++) {
        memcpy(pdest, py, w);
        pdest += w;
        py += linesize_y;
    }

    h = (h + 1) / 2;
    w = (w + 1) / 2;
    pdest = mDataSource.pdata[1];
    for (i = 0; i < h; i ++) {
        memcpy(pdest, pcb, w);
        pdest += w;
        pcb += linesize_cb;
    }

    pdest = mDataSource.pdata[2];
    for (i = 0; i < h; i ++) {
        memcpy(pdest, pcr, w);
        pdest += w;
        pcr += linesize_cr;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

