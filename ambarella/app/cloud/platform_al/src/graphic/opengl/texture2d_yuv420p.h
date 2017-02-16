/**
 * texture2d_yuv420p.h
 *
 * History:
 *  2014/11/14 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __TEXTURE2D_YUV420P_H__
#define __TEXTURE2D_YUV420P_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CTexture2DYUV420p
//
//-----------------------------------------------------------------------

class CTexture2DYUV420p : public ITexture2D
{
public:
    static CTexture2DYUV420p *Create();
    void Destroy();

protected:
    CTexture2DYUV420p();

    EECode Construct();
    virtual ~CTexture2DYUV420p();

public:
    virtual EECode SetupContext();
    virtual void DestroyContext();

public:
    virtual EECode UpdateSourceRect(SRect *crop_rect);
    virtual EECode UpdateDisplayQuad(SQuanVertext3F *quan_vert);
    virtual EECode PrepareTexture();
    virtual EECode DrawTexture();
    virtual EECode UpdateData(STextureDataSource *p_data_source);

private:
    EECode setupTexture();
    EECode assignTexCords(float xscale, float yscale);

private:
    TU8 mbContextSetup;
    TU8 mbTextureDataAssigned;
    TU8 mReserved0;
    TU8 mReserved1;

#ifdef DCONFIG_TEST_END2END_DELAY
    TTime mDebugTime;
    TU64 mDebugCount;

    TTime mOverallDebugTime;
    TU64 mOverallDebugFrames;
#endif

private:
    TDimension mWidth;
    TDimension mHeight;
    TDimension mUVWidth;
    TDimension mUVHeight;

    TDimension mLineSize;

    TU8 *mpData[3];
    GLuint mProgram;
    GLuint mTextureY;
    GLuint mTextureCb;
    GLuint mTextureCr;
    GLuint mTextureUniformY;
    GLuint mTextureUniformCb;
    GLuint mTextureUniformCr;

private:
    GLfloat mVertexVertices[4][2];
    GLfloat mTextureVertices[4][2];
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

