/*******************************************************************************
 * mesh.cpp
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

#include "common_hierarchy_object.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIMesh
//
//-----------------------------------------------------------------------

CIMesh *CIMesh::Create()
{
    CIMesh *result = new CIMesh();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIMesh::CIMesh()
{
}

EECode CIMesh::Construct()
{
    memset(&mMeshData, 0x0, sizeof(VCOM_SMesh));
    mbLoaded = 0;
    return EECode_OK;
}

CIMesh::~CIMesh()
{
    FreeMeshData();
}

//load singal mesh object's version
EECode CIMesh::LoadDataFromFile(TChar *filename)
{
    CIHierarchyObject fileIO;
    VCOM_SFileIOFile *p_file = NULL;
    TU32 length = 0, i = 0, num_of_sections = 0;
    TU32 index = 0, *p_index = NULL, *p_current = NULL;
    void *p_data = NULL;
    EECode ret_status = EECode_OK;
    VCOM_SMeshDeformVert *p_dvert = NULL;
    VCOM_SFileIOObject *p_ob = NULL;

    if (filename == NULL) {
        LOG_ERROR("LoadDataFromFile: filename is NULL?.\n");
        //printf("Error: CIMesh::LoadDataFromFile, filename is NULL?.\n");
        return EECode_Error;
    }

    ret_status = fileIO.open_file(filename, VCOM_DFileIOWorkMode_read | VCOM_DFileIOWorkMode_binary);
    ret_status = fileIO.read_dataFile();
    p_file = fileIO.get_fileStruct();

    if (p_file == NULL) {
        LOG_ERROR("LoadDataFromFile: load file fail.\n");
        //printf("Error: CIMesh::LoadDataFromFile, cant load file %s.\n",filename);
        return EECode_Error;
    }

    //find first mesh ob
    p_ob = p_file->p_objects;
    for (i = 0; i < p_file->tail.object_number; i++, p_ob++) {
        if (p_ob->head.object_type == DFOURCC('m', 'e', 's', 'h')) {
            break;
        }
    }

    if (i >= p_file->tail.object_number || !p_ob) {
        LOG_ERROR("LoadDataFromFile: the file have not contain mesh data.\n");
        //printf("Error: CIMesh::LoadDataFromFile, the file have not contain mesh data.\n");
        return EECode_Error;
    }

    //if there's loaded first, free it
    if (mbLoaded) {
        FreeMeshData();
    }

    num_of_sections = p_ob->tail.total_section_number;
    for (i = 0; i < num_of_sections; i++) {
        length = p_ob->p_sections[i].length;
        switch (p_ob->p_sections[i].section_name) {
            case DFOURCC('v', 'e', 'r', 't'):
                mMeshData.p_verts = (VCOM_SMeshVert *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('e', 'd', 'g', 'e'):
                mMeshData.p_edges = (VCOM_SMeshEdge *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('f', 'a', 'c', 'e'):
                mMeshData.p_faces = (VCOM_SMeshFace *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('n', 'u', 'm', 'v'):
                mMeshData.num_of_verts = *((TU32 *)(p_ob->p_sections[i].p_data));
                break;
            case DFOURCC('n', 'u', 'm', 'f'):
                mMeshData.num_of_faces = *((TU32 *)(p_ob->p_sections[i].p_data));
                break;
            case DFOURCC('n', 'u', 'm', 'e'):
                mMeshData.num_of_edges = *((TU32 *)(p_ob->p_sections[i].p_data));
                break;
            case DFOURCC('t', 'e', 'x', 'f'):
                mMeshData.p_tfaces = (VCOM_SMeshTFace *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('c', 'o' , 'l' , 'f'):
                mMeshData.p_colors = (VCOM_SMeshColFace *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('d', 'e' , 'f' , 'w'):
                mMeshData.p_dweights = (VCOM_SMeshDeformWeight *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('d', 'e' , 'f' , 'v'):
                mMeshData.p_dverts = (VCOM_SMeshDeformVert *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                break;
            case DFOURCC('e', 'x', 't', 'd'):
                p_index = (TU32 *)p_ob->p_sections[i].p_data;
                p_ob->p_sections[i].p_data = NULL;
                //extra data here,p_verts and p_dverts must be read before
                p_dvert = mMeshData.p_dverts;
                p_current = p_index;
                for (index = 0; index < mMeshData.num_of_verts; index++, p_dvert++) {
                    p_dvert->dw_index = (TU32 *)malloc(p_dvert->totweight * sizeof(TU32));
                    memcpy(p_dvert->dw_index, p_current, p_dvert->totweight * sizeof(TU32));
                    p_current += p_dvert->totweight;
                }
                free(p_index);
                break;
        }
    }

    mbLoaded = 1;

    return EECode_OK;
}

EECode CIMesh::LoadDataFromMemory(void *p_data)
{
    return EECode_Error;
}

EECode CIMesh::FreeMeshData()
{
    mbLoaded = 0;
    TU32 i = 0;

    if (mMeshData.p_verts) {
        free(mMeshData.p_verts);
    }
    if (mMeshData.p_edges) {
        free(mMeshData.p_edges);
    }
    if (mMeshData.p_faces) {
        free(mMeshData.p_faces);
    }
    if (mMeshData.p_tfaces) {
        free(mMeshData.p_tfaces);
    }
    if (mMeshData.p_colors) {
        free(mMeshData.p_colors);
    }
    if (mMeshData.p_dweights) {
        free(mMeshData.p_dweights);
    }
    if (mMeshData.p_dverts) {
        for (i = 0; i < mMeshData.num_of_verts; i++) {
            if (mMeshData.p_dverts[i].dw_index)
            { free(mMeshData.p_dverts[i].dw_index); }
        }
        free(mMeshData.p_dverts);
    }
    memset(&mMeshData, 0, sizeof(VCOM_SMesh));
    return EECode_OK;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


