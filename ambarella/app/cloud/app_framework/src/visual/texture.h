/**
 * texture.h
 *
 * History:
 *  2014/11/15 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

