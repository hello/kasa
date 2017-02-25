/*******************************************************************************
 * texture.h
 *
 * History:
 *  2014/11/15 - [Zhi He] create file
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

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CYUVTexture
//
//-----------------------------------------------------------------------
class CYUVTexture : public CGUIArea, public IVisualDirectRendering
{
    typedef CGUIArea inherited;

public:
    static CYUVTexture *Create();

protected:
    CYUVTexture();

    EECode Construct();
    virtual ~CYUVTexture();

public:
    virtual EECode SetupContext();
    virtual EECode PrepareDrawing();

    virtual EECode Draw(TU32 &updated);
    void NeedReDraw(TU32 &needed) const;
    virtual EECode Action(EInputEvent event);

public:
    virtual EECode Render(SVisualDirectRenderingContent *p_context, TU32 reuse);

public:
    virtual void Destroy();

private:
    EECode allocateCopiedBuffer();
    void copyBuffer(TU8 *py, TU8 *pcb, TU8 *pcr, TU32 linesize_y, TU32 linesize_cb, TU32 linesize_cr);

private:
    IMutex *mpMutex;
    ITexture2D *mpTexture;

    STextureDataSource mDataSource;

private:
    TU8 *mpCopiedBufferBase;
    TU32 mCopiedBufferSize;

private:
    TU8 mbContextSetup;
    TU8 mbNeedUpdate;
    TU8 mbTextureSetup;
    TU8 mbNeedPrepare;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

