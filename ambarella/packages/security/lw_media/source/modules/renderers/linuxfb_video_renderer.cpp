/*
 * linuxfb_video_renderer.cpp
 *
 * History:
 *    2015/09/14 - [Zhi He] create file
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
#include "internal_asm.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"

#ifdef BUILD_MODULE_LINUX_FB

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "linuxfb_video_renderer.h"

#define DUSE_NEON_SIMD_FUNCTION

static void __scale_yuv420p_to_vyu565(TU8 *src_y, TU8 *src_cb, TU8 *src_cr, TU16 *des, TU32 src_stride_y, TU32 src_stride_cbcr, TU32 des_stride, TU32 src_width, TU32 src_height, TU32 des_width, TU32 des_height, TU32 *index_x, TU32 *index_y)
{
    TU32 i = 0, j = 0;
    TU8 *srcy, *srccb, *srccr;

    TU32 desty, destcbcr;
    TU16 *dest = NULL;

    dest = des;
    srcy = src_y;
    srccb = src_cb;
    srccr = src_cr;

    for (j = 0; j< des_height; j++) {

        srcy = src_y + index_y[j] * src_stride_y;
        srccb = src_cb + (index_y[j] >> 1) * src_stride_cbcr;
        srccr = src_cr + (index_y[j] >> 1) * src_stride_cbcr;

        for(i=0; i< des_width; i++) {
            desty = ((TU32)(srcy[index_x[i]]) >> 2) << 5;
            destcbcr = ((((TU32)srccr[index_x[i] >> 1]) >> 3) << 11) | (((TU32)srccb[index_x[i] >> 1]) >> 3);
            dest[i] = desty | destcbcr;
        }
        dest += des_stride;
    }

}

#ifndef DUSE_NEON_SIMD_FUNCTION
static void __yuv420p_to_vyu565(TU8 *src_y, TU8 *src_cb, TU8 *src_cr, TU16 *des, TU32 src_stride_y, TU32 src_stride_cbcr, TU32 des_stride, TU32 src_width, TU32 src_height)
{
    TU32 i = 0, j = 0;
    TU8 *srcy, *srccb, *srccr;

    TU32 desty, destcbcr;
    TU16 *dest = NULL;

    dest = des;
    srcy = src_y;
    srccb = src_cb;
    srccr = src_cr;

    for (j = 0; j< src_height; j++) {

        srcy = src_y + j * src_stride_y;
        srccb = src_cb + (j >> 1) * src_stride_cbcr;
        srccr = src_cr + (j >> 1) * src_stride_cbcr;

        for(i=0; i< src_width; i++) {
            desty = ((TU32)(srcy[i]) >> 2) << 5;
            destcbcr = ((((TU32)srccr[i >> 1]) >> 3) << 11) | (((TU32)srccb[i >> 1]) >> 3);
            dest[i] = desty | destcbcr;
        }
        dest += des_stride;
    }

}

static void __yuv420p_to_vyu565_v2(TU8 *src_y, TU8 *src_cb, TU8 *src_cr, TU16 *des, TU32 src_stride_y, TU32 src_stride_cbcr, TU32 des_stride, TU32 src_width, TU32 src_height)
{
    TU32 i = 0, j = 0, k = 0;
    TU8 *srcy, *srcy1, *srccb, *srccr;

    TU32 destcbcr;
    TU16 *dest = NULL, *dest1 = NULL;

    dest = des;
    dest1 = des + des_stride;
    srcy = src_y;
    srcy1 = src_y + src_stride_y;
    srccb = src_cb;
    srccr = src_cr;

    des_stride <<= 1;
    src_stride_y <<= 1;

    for (j = 0; j< src_height; j += 2) {
        for(i = 0, k = 0; i< src_width; i += 2, k++) {
            destcbcr = ((((TU32)srccr[k]) >> 3) << 11) | (((TU32)srccb[k]) >> 3);
            dest[i] = (((TU32)(srcy[i]) >> 2) << 5) | destcbcr;
            dest[i + 1] = (((TU32)(srcy[i + 1]) >> 2) << 5) | destcbcr;
            dest1[i] = (((TU32)(srcy1[i]) >> 2) << 5) | destcbcr;
            dest1[i + 1] = (((TU32)(srcy1[i + 1]) >> 2) << 5) | destcbcr;
        }
        dest += des_stride;
        dest1 += des_stride;
        srcy += src_stride_y;
        srcy1 += src_stride_y;
        srccb += src_stride_cbcr;
        srccr += src_stride_cbcr;
    }

}
#endif

static void __scale_yuv420p_to_ayuv8888(TU8 *src_y, TU8 *src_cb, TU8 *src_cr, TU32 *des, TU32 src_stride_y, TU32 src_stride_cbcr, TU32 des_stride, TU32 src_width, TU32 src_height, TU32 des_width, TU32 des_height, TU32 *index_x, TU32 *index_y)
{
    TU32 i = 0, j = 0;
    TU8 *srcy, *srccb, *srccr;

    TU32 desty, destcbcr;
    TU32 *dest = NULL;

    dest = des;
    srcy = src_y;
    srccb = src_cb;
    srccr = src_cr;

    for (j = 0; j< des_height; j++) {

        srcy = src_y + index_y[j] * src_stride_y;
        srccb = src_cb + (index_y[j] >> 1) * src_stride_cbcr;
        srccr = src_cr + (index_y[j] >> 1) * src_stride_cbcr;

        for(i=0; i< des_width; i++) {
            desty = (TU32)(srcy[index_x[i]]) << 16;
            destcbcr = 0xff000000 | (((TU32)srccb[index_x[i] >> 1]) << 8) | ((TU32)srccr[index_x[i] >> 1]);
            dest[i] = desty | destcbcr;
        }
        dest += des_stride;
    }

}

#ifndef DUSE_NEON_SIMD_FUNCTION
static void __yuv420p_to_ayuv8888(TU8 *src_y, TU8 *src_cb, TU8 *src_cr, TU32 *des, TU32 src_stride_y, TU32 src_stride_cbcr, TU32 des_stride, TU32 src_width, TU32 src_height)
{
    TU32 i = 0, j = 0, k = 0;
    TU8 *srcy, *srcy1, *srccb, *srccr;

    TU32 destcbcr;
    TU32 *dest = NULL, *dest1 = NULL;

    dest = des;
    dest1 = des + des_stride;
    srcy = src_y;
    srcy1 = src_y + src_stride_y;
    srccb = src_cb;
    srccr = src_cr;

    des_stride <<= 1;
    src_stride_y <<= 1;

    for (j = 0; j< src_height; j += 2) {
        for(i = 0, k = 0; i< src_width; i += 2, k++) {
            destcbcr = 0xff000000 | (((TU32)srccb[k]) << 8) | ((TU32)srccr[k]);
            dest[i] = ((TU32)(srcy[i]) << 16) | destcbcr;
            dest[i + 1] = ((TU32)(srcy[i + 1]) << 16) | destcbcr;
            dest1[i] = ((TU32)(srcy1[i]) << 16) | destcbcr;
            dest1[i + 1] = ((TU32)(srcy1[i + 1]) << 16) | destcbcr;
        }
        dest += des_stride;
        dest1 += des_stride;
        srcy += src_stride_y;
        srcy1 += src_stride_y;
        srccb += src_stride_cbcr;
        srccr += src_stride_cbcr;
    }
}
#endif

IVideoRenderer *gfCreateLinuxFBVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CLinuxFBVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CLinuxFBVideoRenderer
//
//-----------------------------------------------------------------------
CLinuxFBVideoRenderer::CLinuxFBVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mFd(-1)
    , mWidth(0)
    , mHeight(0)
    , mLinesize(0)
    , mBufferSize(0)
    , mBitsPerPixel(0)
    , mpBuffers(NULL)
    , mSrcOffsetX(0)
    , mSrcOffsetY(0)
    , mSrcWidth(0)
    , mSrcHeight(0)
    , mLastDisplayPTS(0)
    , mpScaleIndexX(NULL)
    , mpScaleIndexY(NULL)
{
    mTotalDisplayFrameCount = 0;
    mFirstFrameTime = 0;
    mCurFrameTime= 0;

    mpBuffer[0] = NULL;
    mpBuffer[1] = NULL;
    mpBuffer[2] = NULL;
    mpBuffer[3] = NULL;

    mbScaled = 0;
    mFrameCount = 2;
    mCurFrameIndex = 0;
    mbYUV16BitMode = 0;
}

IVideoRenderer *CLinuxFBVideoRenderer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CLinuxFBVideoRenderer *result = new CLinuxFBVideoRenderer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CLinuxFBVideoRenderer::Destroy()
{
    if (mpScaleIndexX) {
        free(mpScaleIndexX);
        mpScaleIndexX = NULL;
    }

    if (mpScaleIndexY) {
        free(mpScaleIndexY);
        mpScaleIndexY = NULL;
    }

    Delete();
}

EECode CLinuxFBVideoRenderer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaVideoRenderer);

    return EECode_OK;
}

CLinuxFBVideoRenderer::~CLinuxFBVideoRenderer()
{
}

EECode CLinuxFBVideoRenderer::SetupContext(SVideoParams *param)
{
    DASSERT(mpPersistMediaConfig);
    mFd = open("/dev/fb0", O_RDWR);
    if (0 > mFd) {
        LOGM_ERROR("Error: cannot open!\n");
        return EECode_OSError;
    } else {
        LOGM_NOTICE("The framebuffer device was opened successfully.\n");
    }

    if (ioctl(mFd, FBIOGET_FSCREENINFO, &mFinfo)) {
        LOGM_ERROR("Error reading fixed information.\n");
        return EECode_OSError;
    }

    if (ioctl(mFd, FBIOGET_VSCREENINFO, &mVinfo)) {
        LOGM_ERROR("Error reading variable information.\n");
        return EECode_OSError;
    } else {
        LOGM_NOTICE("%dx%d, %dbpp\n", mVinfo.xres, mVinfo.yres, mVinfo.bits_per_pixel);
    }

    mWidth = mVinfo.xres;
    mHeight = mVinfo.yres;
    mBitsPerPixel = mVinfo.bits_per_pixel;
    if (16 == mBitsPerPixel) {
        mbYUV16BitMode = 1;
    } else {
        mbYUV16BitMode = 0;
    }
    mLinesize = mFinfo.line_length;
    mBufferSize = mHeight * mLinesize;
    mLinesize = (mLinesize << 3) / mBitsPerPixel;
    LOGM_NOTICE("mBufferSize %d, mHeight %d, mLinesize %d, mbYUV16BitMode %d\n", mBufferSize, mHeight, mLinesize, mbYUV16BitMode);

    mFrameCount = 2;
    mpBuffers = (TU8 *)mmap(0, mBufferSize * mFrameCount, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);
    if (!mpBuffer) {
        LOGM_ERROR("Error: failed to map framebuffer device to memory.\n");
        return EECode_OSError;
    }

    for (TU32 i = 0; i < mFrameCount; i ++) {
        mpBuffer[i] = mpBuffers + (i * mBufferSize);
    }

    DASSERT(param);
    if (param) {
        mSrcOffsetX = param->pic_offset_x;
        mSrcOffsetY = param->pic_offset_y;
        mSrcWidth = param->pic_width;
        mSrcHeight = param->pic_height;

        if ((mSrcWidth != mWidth) || (mSrcHeight != mHeight)) {
            if (mpScaleIndexX) {
                free(mpScaleIndexX);
                mpScaleIndexX = NULL;
            }
            if (mpScaleIndexY) {
                free(mpScaleIndexY);
                mpScaleIndexY = NULL;
            }
            mpScaleIndexX = (TU32 *) malloc(mWidth * sizeof(TU32));
            mpScaleIndexY = (TU32 *) malloc(mHeight * sizeof(TU32));
            if ((!mpScaleIndexX) || (!mpScaleIndexY)) {
                LOGM_ERROR("no memory\n");
                return EECode_NoMemory;
            }

            TU32 current = 0, residue = mSrcWidth >> 1;
            TU32 i = 0;

            for (i = 0; i < mWidth; i++) {
                mpScaleIndexX[i] = current;
                residue += mSrcWidth;
                while (residue > mWidth ) {
                    current ++;
                    residue -= mWidth;
                }
            }

            current = 0;
            residue = mSrcHeight >> 1;
            for (i = 0; i < mHeight; i++) {
                mpScaleIndexY[i] = current;
                residue += mSrcHeight;
                while (residue > mHeight ) {
                    current ++;
                    residue -= mHeight;
                }
            }

            mbScaled = 1;
        } else {
            mbScaled = 0;
        }
    }

    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::DestroyContext()
{
    if (mpBuffers && mBufferSize) {
        munmap(mpBuffers, mBufferSize);
    }
    mpBuffers = NULL;
    mBufferSize = 0;

    if (0 <= mFd) {
        close(mFd);
    }
    mFd = -1;

    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Start(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Stop(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Flush(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Render(CIBuffer *p_buffer, TU32 index)
{
    if (p_buffer) {
        mLastDisplayPTS = p_buffer->GetBufferNativePTS();
        if (mbYUV16BitMode) {
            if (!mbScaled) {
#ifdef DUSE_NEON_SIMD_FUNCTION
                SASMArguYUV420p2VYU param;
                param.src_y = p_buffer->GetDataPtr(0);
                param.src_cb = p_buffer->GetDataPtr(1);
                param.src_cr = p_buffer->GetDataPtr(2);
                param.des = mpBuffer[mCurFrameIndex];
                param.src_stride_y = p_buffer->GetDataLineSize(0);
                param.src_stride_cbcr = p_buffer->GetDataLineSize(1);
                param.des_stride = mLinesize;
                param.src_width = mSrcWidth;
                param.src_height = mSrcHeight;
                asm_neon_yuv420p_to_vyu565(&param);
#else
                if (1) {
                    __yuv420p_to_vyu565_v2(p_buffer->GetDataPtr(0), p_buffer->GetDataPtr(1), p_buffer->GetDataPtr(2), (TU16 *)mpBuffer[mCurFrameIndex], (TU32) p_buffer->GetDataLineSize(0), (TU32) p_buffer->GetDataLineSize(1), mLinesize, mSrcWidth, mSrcHeight);
                } else {
                    __yuv420p_to_vyu565(p_buffer->GetDataPtr(0), p_buffer->GetDataPtr(1), p_buffer->GetDataPtr(2), (TU16 *)mpBuffer[mCurFrameIndex], (TU32) p_buffer->GetDataLineSize(0), (TU32) p_buffer->GetDataLineSize(1), mLinesize, mSrcWidth, mSrcHeight);
                }
#endif
            } else {
                __scale_yuv420p_to_vyu565(p_buffer->GetDataPtr(0), p_buffer->GetDataPtr(1), p_buffer->GetDataPtr(2), (TU16 *)mpBuffer[mCurFrameIndex], (TU32) p_buffer->GetDataLineSize(0), (TU32) p_buffer->GetDataLineSize(1), mLinesize, mSrcWidth, mSrcHeight, mWidth, mHeight, mpScaleIndexX, mpScaleIndexY);
            }
        } else {
            if (!mbScaled) {
#ifdef DUSE_NEON_SIMD_FUNCTION
                SASMArguYUV420p2VYU param;
                param.src_y = p_buffer->GetDataPtr(0);
                param.src_cb = p_buffer->GetDataPtr(1);
                param.src_cr = p_buffer->GetDataPtr(2);
                param.des = mpBuffer[mCurFrameIndex];
                param.src_stride_y = p_buffer->GetDataLineSize(0);
                param.src_stride_cbcr = p_buffer->GetDataLineSize(1);
                param.des_stride = mLinesize;
                param.src_width = mSrcWidth;
                param.src_height = mSrcHeight;
                asm_neon_yuv420p_to_ayuv8888(&param);
#else
                __yuv420p_to_ayuv8888(p_buffer->GetDataPtr(0), p_buffer->GetDataPtr(1), p_buffer->GetDataPtr(2), (TU32 *)mpBuffer[mCurFrameIndex], (TU32) p_buffer->GetDataLineSize(0), (TU32) p_buffer->GetDataLineSize(1), mLinesize, mSrcWidth, mSrcHeight);
#endif
            } else {
                __scale_yuv420p_to_ayuv8888(p_buffer->GetDataPtr(0), p_buffer->GetDataPtr(1), p_buffer->GetDataPtr(2), (TU32 *)mpBuffer[mCurFrameIndex], (TU32) p_buffer->GetDataLineSize(0), (TU32) p_buffer->GetDataLineSize(1), mLinesize, mSrcWidth, mSrcHeight, mWidth, mHeight, mpScaleIndexX, mpScaleIndexY);
            }
        }
        mVinfo.yoffset = mCurFrameIndex * mHeight;
        ioctl(mFd, FBIOPAN_DISPLAY, &mVinfo);

        mCurFrameIndex ++;
        if (mCurFrameIndex >= mFrameCount) {
            mCurFrameIndex = 0;
        }

        if (!mTotalDisplayFrameCount) {
            mFirstFrameTime = mpPersistMediaConfig->p_system_clock_reference->GetCurrentTime();
            mTotalDisplayFrameCount ++;
        } else {
            mCurFrameTime = mpPersistMediaConfig->p_system_clock_reference->GetCurrentTime();
            mTotalDisplayFrameCount ++;
            if (!(mTotalDisplayFrameCount & 0xff)) {
                LOGM_PRINTF("display %d frames, avg fps %f\n", mTotalDisplayFrameCount, (float)(((float) mTotalDisplayFrameCount * (float) 1000000)/((float)(mCurFrameTime - mFirstFrameTime))));
            }
        }
    }
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Pause(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::Resume(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::StepPlay(TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::SyncTo(TTime pts, TU32 index)
{
    return EECode_OK;
}

EECode CLinuxFBVideoRenderer::QueryLastShownTimeStamp(TTime &pts, TU32 index)
{
    pts = mLastDisplayPTS;
    return EECode_OK;
}

#endif

