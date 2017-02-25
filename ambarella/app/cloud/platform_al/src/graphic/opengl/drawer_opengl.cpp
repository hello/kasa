/*******************************************************************************
 * drawer_opengl.cpp
 *
 * History:
 *  2014/11/09 - [Zhi He] create file
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

#include "drawer_opengl.h"

#ifdef BUILD_OS_WINDOWS
#include <GL/glew.h>
#include <GL/wglew.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIGraphicDrawerOpenGL
//
//-----------------------------------------------------------------------

CIGraphicDrawerOpenGL *CIGraphicDrawerOpenGL::Create()
{
    CIGraphicDrawerOpenGL *result = new CIGraphicDrawerOpenGL();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIGraphicDrawerOpenGL::CIGraphicDrawerOpenGL()
{
}

EECode CIGraphicDrawerOpenGL::Construct()
{
    return EECode_OK;
}

CIGraphicDrawerOpenGL::~CIGraphicDrawerOpenGL()
{
}

EECode CIGraphicDrawerOpenGL::DrawMesh(CIMesh *mesh, VCOM_SMeshVert *pDeformedVerts)
{
#if 0
    TU32 i = 0, num_of_faces = 0;
    TS32 glmode = -1, new_glmode = -1;
    VCOM_SMeshFace *p_face = NULL;
    VCOM_SMeshVert *p_vert = pDeformedVerts;

    if (!mesh) {
        LOG_ERROR("DrawMesh: NULL pointer meshob.\n");
        return EECode_Error;
    } else if (!mesh->mbLoaded) {
        LOG_ERROR("DrawMesh: but the mesh mesh is not loaded.\n");
        return EECode_Error;
    }

    num_of_faces = mesh->mMeshData.num_of_faces;

    //draw original mesh
    if (p_vert == NULL) {
        p_vert = mesh->mMeshData.p_verts;
    }

    glBegin(glmode = GL_QUADS);
    for (i = 0; i < num_of_faces; i ++) {
        p_face = &mesh->mMeshData.p_faces[i];
        new_glmode = p_face->v4 ? GL_QUADS : GL_TRIANGLES;
        if (glmode != new_glmode) {
            glEnd();
            glmode = new_glmode;
            glBegin(glmode);
        }
        glVertex3fv(p_vert[p_face->v1].coordinate);
        glVertex3fv(p_vert[p_face->v2].coordinate);
        glVertex3fv(p_vert[p_face->v3].coordinate);
        if (p_face->v4) {
            glVertex3fv(p_vert[p_face->v4].coordinate);
        }
    }
    glEnd();
#endif
    return EECode_OK;
}

EECode CIGraphicDrawerOpenGL::DrawTextureMesh(CIMesh *mesh, VCOM_SMeshVert *pDeformedVerts)
{
#if 0
    TU32 i = 0, num_of_faces = 0;
    TS32 glmode = -1, new_glmode = -1;
    VCOM_SMeshFace *p_face = NULL;
    VCOM_SMeshTFace *p_texFace = NULL;
    VCOM_SMeshVert *p_vert = pDeformedVerts;

    if (!mesh) {
        LOG_ERROR("DrawTextureMesh: NULL pointer meshob.\n");
        return EECode_Error;
    } else if (!mesh->mbLoaded) {
        LOG_ERROR("DrawTextureMesh: but the mesh mesh is not loaded.\n");
        return EECode_Error;
    }

    if (!mesh->mMeshData.p_tfaces) {
        LOG_ERROR("DrawTextureMesh: there's no tex vert data, draw mesh instead.\n");
        return DrawMesh(mesh, pDeformedVerts);
    }

    num_of_faces = mesh->mMeshData.num_of_faces;

    //draw original mesh
    if (p_vert == NULL) {
        p_vert = mesh->mMeshData.p_verts;
    }

    glBegin(glmode = GL_QUADS);
    for (i = 0; i < num_of_faces; i ++) {
        p_face = &mesh->mMeshData.p_faces[i];
        p_texFace = &mesh->mMeshData.p_tfaces[i];
        new_glmode = p_face->v4 ? GL_QUADS : GL_TRIANGLES;
        if (glmode != new_glmode) {
            glEnd();
            glmode = new_glmode;
            glBegin(glmode);
        }
        if (p_texFace) { glTexCoord2fv(p_texFace->uv[0]); }
        glVertex3fv(p_vert[p_face->v1].coordinate);
        if (p_texFace) { glTexCoord2fv(p_texFace->uv[1]); }
        glVertex3fv(p_vert[p_face->v2].coordinate);
        if (p_texFace) { glTexCoord2fv(p_texFace->uv[2]); }
        glVertex3fv(p_vert[p_face->v3].coordinate);
        if (p_face->v4) {
            if (p_texFace) { glTexCoord2fv(p_texFace->uv[3]); }
            glVertex3fv(p_vert[p_face->v4].coordinate);
        }
    }
    glEnd();
#endif
    return EECode_OK;
}

EECode CIGraphicDrawerOpenGL::DrawRawDataRect(VCOM_SImageData *p_imgData, SRect *p_rect)
{
    TS32 draw_width = 0, draw_height = 0;
    TS32 pos_x = 0, pos_y = 0;

    if (!p_imgData || !p_rect) {
        LOG_ERROR("DrawRawDataRect: NULL pointer argumnts.\n");
        return EECode_Error;
    } else if (!p_imgData->width || !p_imgData->height || !p_imgData->p_data) {
        LOG_ERROR("DrawRawDataRect: but the imagedata is not valid.\n");
        return EECode_Error;
    }

    if (p_imgData->format != VCOM_EImageDataFormat_RGBA32) {
        LOG_ERROR("DrawRawDataRect: the imagedata has unsupport format.\n");
        return EECode_Error;
    }

    draw_width = p_rect->width;
    draw_height = p_rect->height;
    if (draw_width == p_imgData->width) {
        draw_height = (draw_height > p_imgData->height ? p_imgData->height : draw_height);
        glPixelZoom(1.0,  1.0);
        glRasterPos2i(p_rect->pos_x, p_rect->pos_y);
        glDrawPixels(draw_width, draw_height, GL_RGBA, GL_UNSIGNED_BYTE, p_imgData->p_data);
        return EECode_OK;
    }

    //todo
    if (draw_width < 1 || draw_height < 1 || draw_width > p_imgData->width || draw_height > p_imgData->height) {
        LOG_ERROR("DrawRawDataRect: scale the imagedata is not supported now.\n");
    }

    return EECode_Error;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


