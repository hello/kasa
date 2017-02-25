/*******************************************************************************
 * base.cpp
 *
 * History:
 *    2014/10/11 - [Zhi He] create file
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

#include "app_framework_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CGUIArea
//
//-----------------------------------------------------------------------
CGUIArea::CGUIArea()
    : mpMutex(NULL)
    , mPosX(0)
    , mPosY(0)
    , mSizeX(0)
    , mSizeY(0)
    , mRelativePosX(0)
    , mRelativePosY(0)
    , mRelativeSizeX(0)
    , mRelativeSizeY(0)
    , mFlags(0)
{
}

EECode CGUIArea::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("no memory? gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CGUIArea::~CGUIArea()
{
}

EECode CGUIArea::SetupContext()
{
    LOG_FATAL("should be overwrited\n");
    return EECode_InternalLogicalBug;
}

EECode CGUIArea::PrepareDrawing()
{
    LOG_FATAL("should be overwrited\n");
    return EECode_InternalLogicalBug;
}

EECode CGUIArea::Draw(TU32 &updated)
{
    LOG_FATAL("should be overwrited\n");
    return EECode_InternalLogicalBug;
}

EECode CGUIArea::Action(EInputEvent event)
{
    LOG_FATAL("should be overwrited\n");
    return EECode_InternalLogicalBug;
}

void CGUIArea::NeedReDraw(TU32 &needed) const
{
    LOG_FATAL("should be overwrited\n");
}

EECode CGUIArea::SetDisplayArea(TDimension pos_x, TDimension pos_y, TDimension size_x, TDimension size_y)
{
    mPosX = pos_x;
    mPosY = pos_y;
    mSizeX = size_x;
    mSizeY = size_y;
    return EECode_OK;
}

EECode CGUIArea::SetDisplayAreaRelative(float pos_x, float pos_y, float size_x, float size_y)
{
    mRelativePosX = pos_x;
    mRelativePosY = pos_y;
    mRelativeSizeX = size_x;
    mRelativeSizeY = size_y;
    return EECode_OK;
}

void CGUIArea::Focus()
{
    mFlags |= DGUIAreaFlags_focus;
}

void CGUIArea::UnFocus()
{
    mFlags &= ~DGUIAreaFlags_focus;
}

TU32 CGUIArea::IsFocused() const
{
    if (DGUIAreaFlags_focus & mFlags) {
        return 1;
    }

    return 0;
}

void CGUIArea::Destroy()
{
    if (mpMutex) {
        mpMutex->Delete();
    }

    delete this;
}

//-----------------------------------------------------------------------
//
// CIGUILayer
//
//-----------------------------------------------------------------------
CIGUILayer *CIGUILayer::Create()
{
    CIGUILayer *result = new CIGUILayer();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIGUILayer::CIGUILayer()
{
}

EECode CIGUILayer::Construct()
{
    mpAreaList = new CIDoubleLinkedList();
    if (DUnlikely(!mpAreaList)) {
        LOG_FATAL("alloc CIDoubleLinkedList fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CIGUILayer::~CIGUILayer()
{
    if (mpAreaList) {
        delete mpAreaList;
        mpAreaList = NULL;
    }
}

EECode CIGUILayer::AddArea(CGUIArea *area)
{
    if (DLikely(mpAreaList)) {
        mpAreaList->InsertContent(NULL, (void *) area, 0);
    } else {
        LOG_FATAL("mpAreaList is NULL\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

void CIGUILayer::RemoveArea(CGUIArea *area)
{
    if (DLikely(mpAreaList)) {
        mpAreaList->RemoveContent((void *) area);
    } else {
        LOG_FATAL("mpAreaList is NULL\n");
    }
}

EECode CIGUILayer::SetupContexts()
{
    CGUIArea *p_area = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpAreaList->FirstNode();
    while (p_node) {
        p_area = (CGUIArea *) p_node->p_context;
        if (DLikely(p_area)) {
            err = p_area->SetupContext();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_area(%p)->SetupContext(), ret %d, %s\n", p_area, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpAreaList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CIGUILayer::PrepareDrawing()
{
    CGUIArea *p_area = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpAreaList->FirstNode();
    while (p_node) {
        p_area = (CGUIArea *) p_node->p_context;
        if (DLikely(p_area)) {
            err = p_area->PrepareDrawing();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_area(%p)->PrepareDrawing(), ret %d, %s\n", p_area, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpAreaList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CIGUILayer::Draw(TU32 &updated)
{
    CGUIArea *p_area = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpAreaList->FirstNode();
    while (p_node) {
        p_area = (CGUIArea *) p_node->p_context;
        if (DLikely(p_area)) {
            err = p_area->Draw(updated);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_area(%p)->Draw(), ret %d, %s\n", p_area, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpAreaList->NextNode(p_node);
    }

    return EECode_OK;
}

void CIGUILayer::NeedReDraw(TU32 &needed)
{
    CGUIArea *p_area = NULL;

    CIDoubleLinkedList::SNode *p_node = mpAreaList->FirstNode();
    while (p_node) {
        p_area = (CGUIArea *) p_node->p_context;
        if (DLikely(p_area)) {
            p_area->NeedReDraw(needed);
            if (DUnlikely(needed)) {
                return;
            }
        }
        p_node = mpAreaList->NextNode(p_node);
    }
}

void CIGUILayer::Destroy()
{
    delete this;
}

//-----------------------------------------------------------------------
//
// CISoundTrack
//
//-----------------------------------------------------------------------
CISoundTrack *CISoundTrack::Create(TU32 buffer_size)
{
    CISoundTrack *result = new CISoundTrack();
    if (DUnlikely((result) && (EECode_OK != result->Construct(buffer_size)))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CISoundTrack::CISoundTrack()
    : mpMutex(NULL)
    , mpComposer(NULL)
    , mpDataBuffer(NULL)
    , mCurrentSize(0)
    , mDataBufferSize(0)
    , mbEnabled(1)
{
}

EECode CISoundTrack::Construct(TU32 buffer_size)
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("CISoundTrack: gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpDataBuffer = (TU8 *) DDBG_MALLOC(buffer_size, "FRST");
    if (DUnlikely(!mpDataBuffer)) {
        LOG_FATAL("CISoundTrack: alloc(%d) fail\n", buffer_size);
        return EECode_NoMemory;
    }

    mDataBufferSize = buffer_size;

    return EECode_OK;
}

CISoundTrack::~CISoundTrack()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpDataBuffer) {
        DDBG_FREE(mpDataBuffer, "FRST");
        mpDataBuffer = NULL;
    }
}

EECode CISoundTrack::SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format)
{
    if (mpComposer) {
        mpComposer->SetConfig(samplerate, framesize, channel_number, sample_format);
    }

    return EECode_OK;
}

EECode CISoundTrack::Render(SSoundDirectRenderingContent *p_context, TU32 reuse)
{
    AUTO_LOCK(mpMutex);

    if (!mbEnabled) {
        return EECode_OK;
    }

    wirteData(p_context->data[0], p_context->size);
    mpComposer->DataNotify();

    return EECode_OK;
}

EECode CISoundTrack::ControlOutput(TU32 control_flag)
{
    mpComposer->ControlNotify(control_flag);
    return EECode_OK;
}

void CISoundTrack::Enable()
{
    mbEnabled = 1;
}

void CISoundTrack::Disable()
{
    mbEnabled = 0;
}

void CISoundTrack::SetSoundComposer(ISoundComposer *composer)
{
    mpComposer = composer;
}

void CISoundTrack::Lock()
{
    __LOCK(mpMutex);
}

void CISoundTrack::Unlock()
{
    __UNLOCK(mpMutex);
}

void CISoundTrack::QueryData(TU8 *&p, TU32 &size)
{
    p = mpDataBuffer;
    size = mCurrentSize;
}

void CISoundTrack::ClearData()
{
    mCurrentSize = 0;
}

void CISoundTrack::Destroy()
{
    delete this;
}

void CISoundTrack::wirteData(TU8 *p, TU32 size)
{
    if (DUnlikely((size + mCurrentSize) > mDataBufferSize)) {
        LOG_ERROR("audio track data overflow, size %d, cur size %d, buffer size %d\n", size, mCurrentSize, mDataBufferSize);
        return;
    }

    memcpy(mpDataBuffer + mCurrentSize, p, size);
    mCurrentSize += size;
}

//-----------------------------------------------------------------------
//
// CIScene
//
//-----------------------------------------------------------------------
CIScene *CIScene::Create()
{
    CIScene *result = new CIScene();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIScene::CIScene()
{
}

EECode CIScene::Construct()
{
    mpLayerList =  new CIDoubleLinkedList();
    if (DUnlikely(!mpLayerList)) {
        LOG_FATAL("alloc CIDoubleLinkedList fail\n");
        return EECode_NoMemory;
    }
    return EECode_OK;
}

CIScene::~CIScene()
{
    if (mpLayerList) {
        delete mpLayerList;
        mpLayerList = NULL;
    }
}

EECode CIScene::AddLayer(CIGUILayer *layer)
{
    if (DLikely(mpLayerList)) {
        mpLayerList->InsertContent(NULL, (void *) layer, 0);
    } else {
        LOG_FATAL("mpLayerList is NULL\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

void CIScene::RemoveLayer(CIGUILayer *layer)
{
    if (DLikely(mpLayerList)) {
        mpLayerList->RemoveContent((void *) layer);
    } else {
        LOG_FATAL("mpLayerList is NULL\n");
    }
}

EECode CIScene::SetupContexts()
{
    CIGUILayer *p_layer = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpLayerList->FirstNode();
    while (p_node) {
        p_layer = (CIGUILayer *) p_node->p_context;
        if (DLikely(p_layer)) {
            err = p_layer->SetupContexts();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_layer(%p)->SetupContexts(), ret %d, %s\n", p_layer, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpLayerList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CIScene::PrepareDrawing()
{
    CIGUILayer *p_layer = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpLayerList->FirstNode();
    while (p_node) {
        p_layer = (CIGUILayer *) p_node->p_context;
        if (DLikely(p_layer)) {
            err = p_layer->PrepareDrawing();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_layer(%p)->PrepareDrawing(), ret %d, %s\n", p_layer, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpLayerList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CIScene::Draw(TU32 &updated)
{
    CIGUILayer *p_layer = NULL;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *p_node = mpLayerList->FirstNode();
    while (p_node) {
        p_layer = (CIGUILayer *) p_node->p_context;
        if (DLikely(p_layer)) {
            err = p_layer->Draw(updated);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("p_layer(%p)->Draw(), ret %d, %s\n", p_layer, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        p_node = mpLayerList->NextNode(p_node);
    }

    return EECode_OK;
}

EECode CIScene::Hide()
{
    LOG_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CIScene::NeedReDraw(TU32 &needed)
{
    CIGUILayer *p_layer = NULL;

    CIDoubleLinkedList::SNode *p_node = mpLayerList->FirstNode();
    while (p_node) {
        p_layer = (CIGUILayer *) p_node->p_context;
        if (DLikely(p_layer)) {
            p_layer->NeedReDraw(needed);
            if (DUnlikely(needed)) {
                return;
            }
        }
        p_node = mpLayerList->NextNode(p_node);
    }
}

void CIScene::Destroy()
{
    delete this;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

