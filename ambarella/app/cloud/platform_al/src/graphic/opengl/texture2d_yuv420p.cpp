/*******************************************************************************
 * texture2d_yuv420p.cpp
 *
 * History:
 *  2014/11/14- [Zhi He] create file
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

#include "platform_al_if.h"

#ifdef BUILD_OS_WINDOWS
#include <GL/glew.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "texture2d_yuv420p.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static const TChar *gsVShader =
    "attribute vec4 vertexIn;\n"
    "attribute vec2 textureIn;\n"
    "varying vec2 textureOut;\n"
    "void main(void)\n"
    "{\n"
    "gl_Position = vertexIn;\n"
    "textureOut = textureIn;\n"
    "}\n";

//yuv format
#if 0
static const TChar *gsFShader =
    "varying vec2 textureOut;\n"
    "uniform sampler2D tex_y;\n"
    "uniform sampler2D tex_u;\n"
    "uniform sampler2D tex_v;\n"
    "void main(void)\n"
    "{\n"
    "vec3 yuv;\n"
    "vec3 rgb;\n"
    "yuv.x = texture2D(tex_y, textureOut).r;\n"
    "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
    "yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
    "rgb = mat3( 1,       1,         1,\n"
    "0,       -0.39465,  2.03211,\n"
    "1.13983, -0.58060,  0) * yuv;\n"
    "gl_FragColor = vec4(rgb, 1);\n"
    "}\n";
#endif

//ycbcr bt 709 format
static const TChar *gsFShader =
    "varying vec2 textureOut;\n"
    "uniform sampler2D tex_y;\n"
    "uniform sampler2D tex_u;\n"
    "uniform sampler2D tex_v;\n"
    "void main(void)\n"
    "{\n"
    "vec3 yuv;\n"
    "vec3 rgb;\n"
    "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
    "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
    "yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
    "rgb = mat3( 1.164,       1.164,         1.164,\n"
    "0,       -0.213,  2.112,\n"
    "1.793, -0.533,  0) * yuv;\n"
    "gl_FragColor = vec4(rgb, 1);\n"
    "}\n";

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

#ifdef DCONFIG_TEST_END2END_DELAY
extern CIClockReference *gpSystemClockReference;
#endif

ITexture2D *gfCreate2DTextureYUV420p()
{
    return CTexture2DYUV420p::Create();
}

//-----------------------------------------------------------------------
//
// CTexture2DYUV420p
//
//-----------------------------------------------------------------------

CTexture2DYUV420p *CTexture2DYUV420p::Create()
{
    CTexture2DYUV420p *result = new CTexture2DYUV420p();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        delete result;
        result = NULL;
    }
    return result;
}

void CTexture2DYUV420p::Destroy()
{

}

CTexture2DYUV420p::CTexture2DYUV420p()
    : mbContextSetup(0)
    , mbTextureDataAssigned(0)
    , mWidth(0)
    , mHeight(0)
    , mUVWidth(0)
    , mUVHeight(0)
    , mLineSize(0)
    , mProgram(0)
    , mTextureY(0)
    , mTextureCb(0)
    , mTextureCr(0)
    , mTextureUniformY(0)
    , mTextureUniformCb(0)
    , mTextureUniformCr(0)
{
    mpData[0] = NULL;
    mpData[1] = NULL;
    mpData[2] = NULL;

#ifdef DCONFIG_TEST_END2END_DELAY
    mOverallDebugTime = 0;
    mOverallDebugFrames = 0;
#endif
}

EECode CTexture2DYUV420p::Construct()
{
    mVertexVertices[0][0] = -1.0f;
    mVertexVertices[0][1] = -1.0f;

    mVertexVertices[1][0] = 1.0f;
    mVertexVertices[1][1] = -1.0f;

    mVertexVertices[2][0] = -1.0f;
    mVertexVertices[2][1] = 1.0f;

    mVertexVertices[3][0] = 1.0f;
    mVertexVertices[3][1] = 1.0f;

    mTextureVertices[0][0] = 0.0f;
    mTextureVertices[0][1] = 1.0f;

    mTextureVertices[1][0] = 1.0f;
    mTextureVertices[1][1] = 1.0f;

    mTextureVertices[2][0] = 0.0f;
    mTextureVertices[2][1] = 0.0f;

    mTextureVertices[3][0] = 1.0f;
    mTextureVertices[3][1] = 0.0f;

    return EECode_OK;
}



CTexture2DYUV420p::~CTexture2DYUV420p()
{
}

EECode CTexture2DYUV420p::SetupContext()
{
    return EECode_OK;
}

void CTexture2DYUV420p::DestroyContext()
{
}

EECode CTexture2DYUV420p::UpdateSourceRect(SRect *crop_rect)
{
    return EECode_OK;
}

EECode CTexture2DYUV420p::UpdateDisplayQuad(SQuanVertext3F *quan_vert)
{
    return EECode_OK;
}

EECode CTexture2DYUV420p::PrepareTexture()
{
    DCHECK_OPENGL_ERROR_DECLARE

    if (DUnlikely(!mbTextureDataAssigned)) {
        LOG_PRINTF("data not assigned yet, not draw texture\n");
        return EECode_OK;
    }

    if (DUnlikely(!mbContextSetup)) {
        EECode err = setupTexture();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setupTexture() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        mbContextSetup = 1;
    }

    DCHECK_OPENGL_ERROR("PrepareTexture begin");

    glActiveTexture(GL_TEXTURE0);
    DCHECK_OPENGL_ERROR("glActiveTexture(GL_TEXTURE0)");

    glBindTexture(GL_TEXTURE_2D, mTextureY);
    DCHECK_OPENGL_ERROR("glBindTexture(GL_TEXTURE_2D, mTextureY)");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mWidth, mHeight, 0, GL_RED, GL_UNSIGNED_BYTE, mpData[0]);
    DCHECK_OPENGL_ERROR("glTexImage2D");

    glUniform1i(mTextureUniformY, 0);
    DCHECK_OPENGL_ERROR("glUniform1i(mTextureUniformY, 0)");

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureCb);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mUVWidth, mUVHeight, 0, GL_RED, GL_UNSIGNED_BYTE, mpData[1]);
    glUniform1i(mTextureUniformCb, 1);
    DCHECK_OPENGL_ERROR("after cb");

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextureCr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mUVWidth, mUVHeight, 0, GL_RED, GL_UNSIGNED_BYTE, mpData[2]);
    glUniform1i(mTextureUniformCr, 2);
    DCHECK_OPENGL_ERROR("after cr");

    return EECode_OK;
}

EECode CTexture2DYUV420p::DrawTexture()
{
    GLenum glerror = 0;
#ifdef DCONFIG_TEST_END2END_DELAY
    TTime time = 0;
#endif

    if (DUnlikely(!mbTextureDataAssigned)) {
        LOG_PRINTF("data not assigned yet, not draw texture\n");
        return EECode_OK;
    }

#ifdef DCONFIG_TEST_END2END_DELAY
    if (gpSystemClockReference) {
        time = gpSystemClockReference->GetCurrentTime();
    }
#endif

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: DrawTexture, after glDrawArrays: glerror %d\n", glerror);
        return EECode_GLError;
    }

#ifdef DCONFIG_TEST_END2END_DELAY
    if (gpSystemClockReference) {
        LOG_NOTICE("\ttime gap %lld, frame cnt %lld\n", time - mDebugTime, mDebugCount);
        mOverallDebugTime += time - mDebugTime;
        mOverallDebugFrames ++;
        if (!(mOverallDebugFrames & 0xff) && mOverallDebugFrames) {
            LOG_NOTICE("avg dec delay %lld, frames %lld\n", mOverallDebugTime / mOverallDebugFrames , mOverallDebugFrames);
        }
    }
#endif

    return EECode_OK;
}

EECode CTexture2DYUV420p::UpdateData(STextureDataSource *p_data_source)
{
    if (DLikely(p_data_source)) {
        if (DLikely(p_data_source->pdata[0] && p_data_source->pdata[1] && p_data_source->pdata[2])) {

            mpData[0] = p_data_source->pdata[0];
            mpData[1] = p_data_source->pdata[1];
            mpData[2] = p_data_source->pdata[2];

            mWidth = p_data_source->linesize[0]; //p_data_source->width;
            mHeight = p_data_source->height;
            mUVWidth = mWidth / 2;
            mUVHeight = mHeight / 2;

            if (DUnlikely(mLineSize != p_data_source->linesize[0])) {
                mLineSize = p_data_source->linesize[0];
                if (mLineSize > p_data_source->width) {
                    assignTexCords((float)((float)p_data_source->width / (float)mLineSize), 1.0f);
                } else if (mLineSize < p_data_source->width) {
                    LOG_FATAL("linesize less than width?\n");
                    return EECode_BadParam;
                }
            }

            mbTextureDataAssigned = 1;
        } else {
            LOG_FATAL("NULL data pointers\n");
            return EECode_BadParam;
        }

#ifdef DCONFIG_TEST_END2END_DELAY
        mDebugTime = p_data_source->debug_time;
        mDebugCount = p_data_source->debug_count;
#endif
    } else {
        LOG_FATAL("NULL p_data_source\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CTexture2DYUV420p::setupTexture()
{
    GLint vert_compiled, flag_compiled, linked;
    GLint v = 0, f = 0;
    DCHECK_OPENGL_ERROR_DECLARE

    if (DUnlikely(NULL == __glewCreateShader)) {
        LOG_ERROR("system do not support shader? please install latest driver to support latest OpenGL\n");
        return EECode_NotSupported;
    }

    v = glCreateShader(GL_VERTEX_SHADER);
    DCHECK_OPENGL_ERROR("glCreateShader(GL_VERTEX_SHADER)");

    f = glCreateShader(GL_FRAGMENT_SHADER);
    DCHECK_OPENGL_ERROR("glCreateShader(GL_FRAGMENT_SHADER)");

    glShaderSource(v, 1, &gsVShader, NULL);
    glShaderSource(f, 1, &gsFShader, NULL);

    glCompileShader(v);

    glGetShaderiv(v, GL_COMPILE_STATUS, &vert_compiled);
    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &flag_compiled);

    mProgram = glCreateProgram();
    DCHECK_OPENGL_ERROR("glCreateProgram()");

    glAttachShader(mProgram, v);
    glAttachShader(mProgram, f);
    DCHECK_OPENGL_ERROR("glAttachShader");

    glBindAttribLocation(mProgram, ATTRIB_VERTEX, "vertexIn");
    glBindAttribLocation(mProgram, ATTRIB_TEXTURE, "textureIn");
    DCHECK_OPENGL_ERROR("glBindAttribLocation");

    glLinkProgram(mProgram);
    DCHECK_OPENGL_ERROR("glLinkProgram");

    glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
    DCHECK_OPENGL_ERROR("glGetProgramiv");

    glUseProgram(mProgram);
    DCHECK_OPENGL_ERROR("glUseProgram");

    //Get Uniform Variables Location
    mTextureUniformY = glGetUniformLocation(mProgram, "tex_y");
    mTextureUniformCb = glGetUniformLocation(mProgram, "tex_u");
    mTextureUniformCr = glGetUniformLocation(mProgram, "tex_v");
    DCHECK_OPENGL_ERROR("glGetUniformLocation");

    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, mVertexVertices);
    DCHECK_OPENGL_ERROR("glVertexAttribPointer");

    glEnableVertexAttribArray(ATTRIB_VERTEX);
    DCHECK_OPENGL_ERROR("glEnableVertexAttribArray");

    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, mTextureVertices);
    DCHECK_OPENGL_ERROR("glVertexAttribPointer");

    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    DCHECK_OPENGL_ERROR("glEnableVertexAttribArray");

    glGenTextures(1, &mTextureY);
    DCHECK_OPENGL_ERROR("glGenTextures");

    glBindTexture(GL_TEXTURE_2D, mTextureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    DCHECK_OPENGL_ERROR("glTexParameteri y");

    glGenTextures(1, &mTextureCb);
    glBindTexture(GL_TEXTURE_2D, mTextureCb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    DCHECK_OPENGL_ERROR("glTexParameteri cb");

    glGenTextures(1, &mTextureCr);
    glBindTexture(GL_TEXTURE_2D, mTextureCr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    DCHECK_OPENGL_ERROR("glTexParameteri cr");

    return EECode_OK;
}

EECode CTexture2DYUV420p::assignTexCords(float xscale, float yscale)
{
    mTextureVertices[0][0] = 0.0f;
    mTextureVertices[0][1] = yscale;

    mTextureVertices[1][0] = xscale;
    mTextureVertices[1][1] = yscale;

    mTextureVertices[2][0] = 0.0f;
    mTextureVertices[2][1] = 0.0f;

    mTextureVertices[3][0] = xscale;
    mTextureVertices[3][1] = 0.0f;

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


