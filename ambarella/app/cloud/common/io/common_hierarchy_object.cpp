/*******************************************************************************
 * common_hierarchy_object.cpp
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

#ifdef DCONFIG_COMPILE_OBSOLETE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_hierarchy_object.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define VCOM_DFileIOOffsetBit64

//log write length
//#define __D_vcomfileio_log_write_calculate_length__
#ifdef __D_vcomfileio_log_write_calculate_length__
#include <fstream>
using namespace std;

static int __gi_vcomfileio_tot_write_length = 0;
static int __gi_vcomfileio_tot_calculate_length = 0;
static int __gi_vcomfileio_file_index = 0;
static char __gi_vcomfileio_writefilename_tmp[25] = "write_%d.log";
static char __gi_vcomfileio_calculatefilename_tmp[25] = "calculate_%d.log";
static char __gi_vcomfileio_filename[100];

static ofstream __gf_vcomfileio_write;
static ofstream __gf_vcomfileio_calculate;

#endif

void CIHierarchyObject::calculate_section(VCOM_SFileIOSection *p_section)
{
    TU32 index = 0, i = 0;
    VCOM_SFileIOSection *p_subsec = NULL;
    TU64 tot_length = 0;

    if (!p_section) {
        error |= VCOM_EFileIOError_null_pointer;
        return;
    }

    if (p_section->type == VCOM_EFileIOSectionType_data && p_section->p_subsection == NULL && p_section->used_num_of_subsections == 0) {
#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_calculate << "sec_data " << endl;
        __gi_vcomfileio_tot_calculate_length += p_section->length;
        __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;
#endif

        p_section->section_index = 0;
    } else if (p_section->type == VCOM_EFileIOSectionType_sectionentry) {
        p_subsec = p_section->p_subsection;

        if (!p_subsec || !p_section->used_num_of_subsections) {
            p_section->length = 0;
            p_section->used_num_of_subsections = 0;
            p_section->p_subsection = NULL;
            printf("Warning: section section entry without subsection.\n");
            error |= VCOM_EFileIOError_warings;
            return;
        }

        for (i = 0; i < p_section->used_num_of_subsections; i++, p_subsec++) {
#ifdef __D_vcomfileio_log_write_calculate_length__
            __gf_vcomfileio_calculate << "sec_head " << endl;
            __gi_vcomfileio_tot_calculate_length += VCOM_DFileIOSectionHeadLength;
            __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;
#endif
            calculate_section(p_subsec);
            p_subsec->section_index = i;
            tot_length += p_subsec->length + VCOM_DFileIOSectionHeadLength;
        }
        p_section->length = tot_length;
    } else {
        //must have error if goes here
#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_calculate << "Error: currupt section " << endl;
#endif
        printf("Error:currupt section?\n");
        error |= VCOM_EFileIOError_internal_error;
    }
}

//update object's total length
void CIHierarchyObject::calculate_object(VCOM_SFileIOObject *p_ob)
{
    TU64 index = 0, i = 0, num_of_sections = 0;
    VCOM_SFileIOSection *p_subsec = NULL;
    void *p_data = NULL;
    TU64 tot_length = VCOM_DFileIOObjectHeadLength;

    if (!p_ob) {
        error |= VCOM_EFileIOError_null_pointer;
        return;
    }

    num_of_sections = p_ob->tail.total_section_number = p_ob->used_number;
    //recalculate the offsets
    if (p_ob->tail.p_section_offset) {
        DDBG_FREE(p_ob->tail.p_section_offset, "C0HO");
    }
    p_ob->tail.p_section_offset = (TU64 *) DDBG_MALLOC(num_of_sections * sizeof(TU64), "C0HO");
    memset(p_ob->tail.p_section_offset, 0, num_of_sections * sizeof(TU64));

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gf_vcomfileio_calculate << "object_head " << endl;
    __gi_vcomfileio_tot_calculate_length += VCOM_DFileIOObjectHeadLength;
    __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;
#endif

    p_subsec = p_ob->p_sections;
    for (i = 0; i < num_of_sections; i++, p_subsec++) {
#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_calculate << "sec_head " << endl;
        __gi_vcomfileio_tot_calculate_length += VCOM_DFileIOSectionHeadLength;
        __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;
#endif

        p_ob->tail.p_section_offset[i] = tot_length;
        calculate_section(p_subsec);
        tot_length += VCOM_DFileIOSectionHeadLength + p_subsec->length;
    }
    p_ob->head.tail_offset = tot_length;
    tot_length += sizeof(TU64) + (num_of_sections * sizeof(TU64));
    p_ob->total_length = tot_length;

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gf_vcomfileio_calculate << "object_tail " << endl;
    __gi_vcomfileio_tot_calculate_length += sizeof(TU64) + (num_of_sections * sizeof(TU64));
    __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;
#endif

}

void CIHierarchyObject::calculate_file()
{
    TU64 index = 0, i = 0, num_of_objects = 0;
    VCOM_SFileIOObject *p_ob = NULL;
    void *p_data = NULL;
    TU64 tot_length = VCOM_DFileIOFileHeadLength;

    num_of_objects = file.tail.object_number = file.used_number;
    p_ob = file.p_objects;
    if (!num_of_objects || !p_ob) {
        printf("no objects in file?,num_of_ob=%d,p_ob=%p.\n", num_of_objects, p_ob);
        error |= VCOM_EFileIOError_null_pointer;
        return;
    }
    //free previous offsets
    if (file.tail.p_object_offset) {
        free(file.tail.p_object_offset);
    }

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gi_vcomfileio_file_index++;
    __gi_vcomfileio_tot_calculate_length = 0;
    sprintf(__gi_vcomfileio_filename, __gi_vcomfileio_calculatefilename_tmp, __gi_vcomfileio_file_index);
    __gf_vcomfileio_calculate.open(__gi_vcomfileio_filename);

    __gf_vcomfileio_calculate << "file_head " << endl;
    __gi_vcomfileio_tot_calculate_length += VCOM_DFileIOFileHeadLength;
    __gf_vcomfileio_calculate << "current length=" << __gi_vcomfileio_tot_calculate_length << endl;

#endif

    p_data = malloc(num_of_objects * sizeof(TU64));
    memset(p_data, 0, num_of_objects * sizeof(TU64));
    file.tail.p_object_offset = (TU64 *)p_data;
    for (i = 0; i < num_of_objects; i++, p_ob++) {
        file.tail.p_object_offset[i] = tot_length;
        calculate_object(p_ob);
        tot_length += p_ob->total_length;
    }
    file.head.tail_offset = tot_length;
    file.total_length = tot_length + (num_of_objects + 1) * sizeof(TU64);

    calculate_done = 1;

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gf_vcomfileio_calculate.close();
    __gi_vcomfileio_tot_calculate_length = 0;
#endif
}

void CIHierarchyObject::free_section(VCOM_SFileIOSection *p_section)
{
    TU32 index = 0;
    VCOM_SFileIOSection *p_subsec = NULL;

    if (!p_section) {
        error |= VCOM_EFileIOError_null_pointer;
        return;
    }
    //data section
    if (p_section->type == VCOM_EFileIOSectionType_data && p_section->p_subsection == NULL && p_section->used_num_of_subsections == 0) {
        if (p_section->p_data) {
            free(p_section->p_data);
        }
        memset(p_section, 0, sizeof(VCOM_SFileIOSection));
    } else if (p_section->type == VCOM_EFileIOSectionType_sectionentry  && p_section->used_num_of_subsections) {
        p_subsec = p_section->p_subsection;
        if (p_subsec) {
            for (index = 0; index < p_section->used_num_of_subsections; index++, p_subsec++) {
                free_section(p_subsec);
            }
        }
        if (p_section->p_data) {
            free(p_section->p_data);
        }
        memset(p_section, 0, sizeof(VCOM_SFileIOSection));
    } else {
        //must have error if goes here
        //printf("currupt section?\n");
        error |= VCOM_EFileIOError_internal_error;
    }

    return ;
}

CIHierarchyObject::CIHierarchyObject()
{
    read_fd = NULL;
    write_fd = NULL;
    p_read_mem = NULL;
    p_write_mem = NULL;
    read_mem_len = 0;
    write_mem_len = 0;
    status = VCOM_EFileIOStatus_uninited;
    error = VCOM_EFileIOError_nothing;
    calculate_done = 0;
    memset(&file, 0, sizeof(VCOM_SFileIOFile));
}

CIHierarchyObject::~CIHierarchyObject()
{
    TU64 i = 0, j = 0;
    VCOM_SFileIOObject *p_ob = NULL;
    VCOM_SFileIOSection *p_section = NULL;
    TU64 num_of_ob = file.tail.object_number;

    if (write_fd) {
#ifdef VCOM_DFileIOOffsetBit64
        fclose64(write_fd);
#else
        fclose(write_fd);
#endif
    }
    if (read_fd) {
#ifdef VCOM_DFileIOOffsetBit64
        fclose64(read_fd);
#else
        fclose(read_fd);
#endif
    }
    if (p_write_mem) {
        free(p_write_mem);
    }

    if (file.p_objects) {
        p_ob = file.p_objects;
        for (i = 0; i < num_of_ob; i++, p_ob++) {
            p_section = p_ob->p_sections;
            for (j = 0; j < p_ob->used_number; j++, p_section++) {
                free_section(p_section);
            }
        }
        file.p_objects = NULL;
    }

    if (file.tail.p_object_offset) {
        free(file.tail.p_object_offset);
        file.tail.p_object_offset = NULL;
    }

}

EECode CIHierarchyObject::open_file(TChar *filename, TU32 flag)
{
    TChar mode[4][3] = {{'w', 't'}, {'w', 'b'}, {'r', 't'}, {'r', 'b'}};
    TU32 mode_index = 0;
    if (!filename)
    { return EECode_Error; }

    //need** more strict check
    if (flag & VCOM_DFileIOWorkMode_write) {
        if (flag & VCOM_DFileIOWorkMode_text)
        { mode_index = 0; }
        else if (flag & VCOM_DFileIOWorkMode_binary)
        { mode_index = 1; }
        else {
            error |= VCOM_EFileIOError_invalid_input;
            return EECode_Error;
        }

#ifdef  VCOM_DFileIOOffsetBit64
        if (write_fd) {
            fclose64(write_fd);
            error |= VCOM_EFileIOError_warings; //previous fd have not been closed
        }
        write_fd = fopen64(filename, mode[mode_index]);
#else
        if (write_fd) {
            fclose(write_fd);
            error |= VCOM_EFileIOError_warings; //previous fd have not been closed
        }
        write_fd = fopen(filename, mode[mode_index]);
#endif
        if (!write_fd) {
            error |= VCOM_EFileIOError_file_openfail;
            return EECode_Error;
        }
    } else if (flag & VCOM_DFileIOWorkMode_read) {
        if (flag & VCOM_DFileIOWorkMode_text)
        { mode_index = 2; }
        else if (flag & VCOM_DFileIOWorkMode_binary)
        { mode_index = 3; }
        else {
            error |= VCOM_EFileIOError_invalid_input;
            return EECode_Error;
        }

#ifdef  VCOM_DFileIOOffsetBit64
        if (read_fd) {
            fclose64(read_fd);
            error |= VCOM_EFileIOError_warings; //previous fd have not been closed
        }
        read_fd = fopen64(filename, mode[mode_index]);
#else
        if (read_fd) {
            fclose(read_fd);
            error |= VCOM_EFileIOError_warings; //previous fd have not been closed
        }
        read_fd = fopen(filename, mode[mode_index]);
#endif
        if (!read_fd) {
            error |= VCOM_EFileIOError_file_openfail;
            return EECode_Error;
        }
    } else {
        error |= VCOM_EFileIOError_invalid_input;
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIHierarchyObject:: close_file(TU32 flag)
{
    if (flag & VCOM_DFileIOWorkMode_write) {
        if (write_fd) {
#ifdef VCOM_DFileIOOffsetBit64
            fclose64(write_fd);
#else
            fclose(write_fd);
#endif
            write_fd = NULL;
        }
    } else if (flag & VCOM_DFileIOWorkMode_read) {
        if (read_fd) {
#ifdef VCOM_DFileIOOffsetBit64
            fclose64(read_fd);
#else
            fclose(read_fd);
#endif
            read_fd = NULL;
        }
    } else {
        error |= VCOM_EFileIOError_invalid_input;
        return EECode_Error;
    }

    return EECode_OK;
}


//ugly append: to to. now use the last file's file head
EECode CIHierarchyObject:: read_dataFile()
{
    TU32 buf32[5];
    TU64 buf64[2];
    TU16 buf16[6];
    TU32 offset32 = 0, size32 = 0;
    TU64 offset64 = 0, size64 = 0;
    TU64 number_of_object = 0, num_of_remained_objects = 0, number_of_sections = 0, i = 0, j = 0;
    void *p_data = NULL;

    if (!read_fd) {
        printf("Error: in CIHierarchyObject:: read_dataFile, try to read an unexist file.\n");
        return EECode_Error;
    }

#ifdef  VCOM_DFileIOOffsetBit64
    fseek64(read_fd, 0, SEEK_SET);
#else
    fseek(read_fd, 0, SEEK_SET);
#endif

    //file head
    num_of_remained_objects = file.tail.object_number;
    {
#ifdef  VCOM_DFileIOOffsetBit64
        fread64(buf32, 4, 4, read_fd);
#else
        fread(buf32, 4, 4, read_fd);
#endif
        file.head.reserved = buf32[0];
        file.head.type = buf32[1];
        file.head.type_ext = buf32[2];
        file.head.info = buf32[3];
#ifdef  VCOM_DFileIOOffsetBit64
        fread64(buf64, 8, 2, read_fd);
#else
        fread(buf64, 8, 2, read_fd);
#endif
        file.head.tail_offset = buf64[0];
        file.head.tail_length = buf64[1];
    }

    //file tail
#ifdef  VCOM_DFileIOOffsetBit64
    offset64 = file.head.tail_offset;
    fseek64(read_fd, offset64, SEEK_SET);
    fread64(buf64, 8, 1, read_fd);
    file.tail.object_number = buf64[0];
#else
    offset32 = (TU32)file.head.tail_offset;
    fseek(read_fd, offset32, SEEK_SET);
    fread(buf64, 8, 1, read_fd);
    file.tail.object_number = buf64[0];
#endif

    number_of_object = file.tail.object_number;
    size32 = (TU32)(number_of_object * (sizeof(TU64)));
    p_data = malloc(size32);
#ifdef  VCOM_DFileIOOffsetBit64
    fread64(p_data, sizeof(TU64), number_of_object, read_fd);
#else
    fread(p_data, sizeof(TU64), (TU32)number_of_object, read_fd);
#endif
    if (file.tail.p_object_offset)
    { free(file.tail.p_object_offset); }
    file.tail.p_object_offset = (TU64 *)p_data;

    //append objects
    if (num_of_remained_objects) {
        size32 = (TU32)((number_of_object + num_of_remained_objects) * (sizeof(VCOM_SFileIOObject)));
        p_data = malloc(size32);
        memset(p_data, 0, size32);
        memcpy(((TU8 *)p_data) + number_of_object * sizeof(VCOM_SFileIOObject), file.p_objects, num_of_remained_objects * sizeof(VCOM_SFileIOObject));
        free(file.p_objects);
        file.used_number = file.total_number_of_object = number_of_object + num_of_remained_objects;
    } else {
        size32 = (TU32)(number_of_object * (sizeof(VCOM_SFileIOObject)));
        p_data = malloc(size32);
        memset(p_data, 0, size32);
        file.used_number = file.total_number_of_object = number_of_object;
    }
    file.p_objects = (VCOM_SFileIOObject *)p_data;

    for (i = 0; i < number_of_object; i++) {
        //read object head
#ifdef  VCOM_DFileIOOffsetBit64
        offset64 = file.tail.p_object_offset[i];
        fseek64(read_fd, offset64, SEEK_SET);
        fread64(buf32, 4, 2, read_fd);
        fread64(buf64, 8, 1, read_fd);
#else
        offset32 = (TU32)file.tail.p_object_offset[i];
        fseek(read_fd, offset32, SEEK_SET);
        fread(buf32, 4, 2, read_fd);
        fread(buf64, 8, 1, read_fd);
#endif

        file.p_objects[i].head.object_type = buf32[0];
        file.p_objects[i].head.index = buf32[1];
        file.p_objects[i].head.tail_offset = buf64[0];

        //read object tail
#ifdef  VCOM_DFileIOOffsetBit64
        offset64 = file.tail.p_object_offset[i] + file.p_objects[i].head.tail_offset;
        fseek64(read_fd, offset64, SEEK_SET);
        fread64(buf64, 8, 1, read_fd);
        file.p_objects[i].tail.total_section_number = buf64[0];
#else
        offset32 = (TU32)(file.tail.p_object_offset[i] + file.p_objects[i].head.tail_offset);
        fseek(read_fd, offset32, SEEK_SET);
        fread(buf64, 8, 1, read_fd);
        file.p_objects[i].tail.total_section_number = buf64[0];
#endif

        number_of_sections = file.p_objects[i].tail.total_section_number;
        size32 = (TU32)(number_of_sections * (sizeof(TU64)));
        p_data = malloc(size32);
#ifdef  VCOM_DFileIOOffsetBit64
        fread64(p_data, sizeof(TU64), number_of_sections, read_fd);
#else
        fread(p_data, sizeof(TU64), (TU32)number_of_sections, read_fd);
#endif
        file.p_objects[i].tail.p_section_offset = (TU64 *)p_data;

        //read sections, no subsection now, todo
        size32 = (TU32)(number_of_sections * (sizeof(VCOM_SFileIOSection)));
        p_data = malloc(size32);
        memset(p_data, 0, size32);
        file.p_objects[i].p_sections = (VCOM_SFileIOSection *)p_data;
        file.p_objects[i].used_number = file.p_objects[i].total_number_of_sections = number_of_sections;
        for (j = 0; j < number_of_sections; j++) {
#ifdef  VCOM_DFileIOOffsetBit64
            offset64 = file.tail.p_object_offset[i] + file.p_objects[i].tail.p_section_offset[j];
            fseek64(read_fd, offset64, SEEK_SET);
            fread64(buf32, 4, 2, read_fd);
            fread64(buf16, 2, 4, read_fd);
            fread64(buf64, 8, 1, read_fd);
            fread64(&buf32[2], 4, 1, read_fd);
#else
            offset32 = (TU32)(file.tail.p_object_offset[i] + file.p_objects[i].tail.p_section_offset[j]);
            fseek(read_fd, offset32, SEEK_SET);
            fread(buf32, 4, 2, read_fd);
            fread(buf16, 2, 4, read_fd);
            fread(buf64, 8, 1, read_fd);
            fread(&buf32[2], 4, 1, read_fd);
#endif
            file.p_objects[i].p_sections[j].section_name = buf32[0];
            file.p_objects[i].p_sections[j].section_index = buf32[1];
            file.p_objects[i].p_sections[j].size = buf16[0];
            file.p_objects[i].p_sections[j].type = buf16[1];
            file.p_objects[i].p_sections[j].flag = buf16[2];
            file.p_objects[i].p_sections[j].recusive_depth = buf16[3];
            file.p_objects[i].p_sections[j].length = buf64[0];
            file.p_objects[i].p_sections[j].used_num_of_subsections = buf32[2];

            //read section data
            size32 = (TU32)buf64[0];
            p_data = malloc(size32);
            memset(p_data, 0, size32);
#ifdef  VCOM_DFileIOOffsetBit64
            fread64(p_data, 1, size32, read_fd);
#else
            fread(p_data, 1, size32, read_fd);
#endif
            //expand sub section
            if (file.p_objects[i].p_sections[j].type == VCOM_EFileIOSectionType_sectionentry) {
                expand_section(&file.p_objects[i].p_sections[j], (TU8 *)p_data, (TU64)size32);
                free(p_data);
            } else if (file.p_objects[i].p_sections[j].type == VCOM_EFileIOSectionType_data) {
                file.p_objects[i].p_sections[j].p_data = p_data;
            } else {
                free(p_data);
                printf("unknown type of section in read_dataFile?\n");
                error |= VCOM_EFileIOError_internal_error;
                return EECode_Error;
            }
        }
    }

    file.tail.object_number = number_of_object + num_of_remained_objects;
    return EECode_OK;
}

TU32 CIHierarchyObject::get_error()
{
    return error;
}

VCOM_TFileIOStatus CIHierarchyObject::get_status()
{
    return status;
}

VCOM_SFileIOObject  *CIHierarchyObject::add_object(TU32 objecttype, TU32 index)
{
    TU32 size32 = 0;
    void *p_data = NULL;

    if (file.used_number >= file.total_number_of_object) {
        if (file.total_number_of_object == 0) {
            file.total_number_of_object = 2;
            size32 = (TU32)(file.total_number_of_object * sizeof(VCOM_SFileIOObject));
            p_data = malloc(size32);
            if (!p_data) {
                printf("Fatal error, CIHierarchyObject::add_object malloc fail.\n");
                error |= VCOM_EFileIOError_memalloc_fail;
                file.total_number_of_object = 0;
                return NULL;
            }
            memset(p_data, 0, size32);
        } else {
            file.total_number_of_object = file.total_number_of_object * 2;
            size32 = (TU32)(file.total_number_of_object * sizeof(VCOM_SFileIOObject));
            p_data = malloc(size32);
            if (!p_data) {
                printf("Fatal error, CIHierarchyObject::add_object malloc fail.\n");
                error |= VCOM_EFileIOError_memalloc_fail;
                file.total_number_of_object /= 2;
                return NULL;
            }
            memset(p_data, 0, size32);
            memcpy(p_data, file.p_objects, file.used_number * sizeof(VCOM_SFileIOObject));
        }

        file.p_objects = (VCOM_SFileIOObject *)p_data;
    }
    file.p_objects[file.used_number].head.object_type = objecttype;
    file.p_objects[file.used_number].head.index = index;
    file.used_number++;
    return (VCOM_SFileIOObject *)(&file.p_objects[file.used_number - 1]);
}

VCOM_SFileIOSection *CIHierarchyObject::add_section(VCOM_SFileIOObject *parent_object, TU32 sectionname, TU64 length, void *p_data, TU16 size, TU16 type)
{
    TU32 size32 = 0;
    void *pdata = NULL;
    VCOM_SFileIOSection *p_sec = NULL;

    //strict check if it is a data section or section entry
    if (type == VCOM_EFileIOSectionType_data && p_data && length) {
        //data section
    } else if (type == VCOM_EFileIOSectionType_sectionentry && ! p_data && !length) {
        //subsection entry
    } else {
        printf("section type error?\n");
        error |= VCOM_EFileIOError_invalid_input;
        return NULL;
    }

    if (parent_object->used_number >= parent_object->total_number_of_sections) {
        if (parent_object->total_number_of_sections == 0) {
            parent_object->total_number_of_sections = 2;
            size32 = (TU32)(parent_object->total_number_of_sections * sizeof(VCOM_SFileIOSection));
            pdata = malloc(size32);
            if (!pdata) {
                printf("Fatal error, CIHierarchyObject::add_section malloc fail.\n");
                error |= VCOM_EFileIOError_memalloc_fail;
                parent_object->total_number_of_sections = 0;
                return NULL;
            }
            memset(pdata, 0, size32);
        } else {
            parent_object->total_number_of_sections *= 2;
            size32 = (TU32)(parent_object->total_number_of_sections * sizeof(VCOM_SFileIOSection));
            pdata = malloc(size32);
            if (!pdata) {
                printf("Fatal error, CIHierarchyObject::add_section malloc fail.\n");
                parent_object->total_number_of_sections /= 2;
                error |= VCOM_EFileIOError_memalloc_fail;
                return NULL;
            }
            memset(pdata, 0, size32);
            memcpy(pdata, (void *)parent_object->p_sections, size32 / 2);
        }
        parent_object->p_sections = (VCOM_SFileIOSection *)pdata;
    }

    p_sec = &parent_object->p_sections[parent_object->used_number];
    p_sec->section_name = sectionname;
    if (type == VCOM_EFileIOSectionType_data) {
        p_sec->p_data = malloc(length);
        memcpy(p_sec->p_data, p_data, length);
    } else {
        p_sec->p_data = NULL;
    }
    p_sec->length = length;
    p_sec->size = size;
    p_sec->type = type;
    parent_object->used_number++;
    return p_sec;
}

VCOM_SFileIOSection *CIHierarchyObject::add_subSection(VCOM_SFileIOSection *parent_section, TU32 sectionname, TU64 length, void *p_data, TU16 size, TU16 type)
{
    TU32 size32 = 0;
    void *pdata = NULL;
    VCOM_SFileIOSection *p_sec = NULL;

    //strict check if it is a data section or section entry
    if (type == VCOM_EFileIOSectionType_data && p_data && length) {
        //data section
    } else if (type == VCOM_EFileIOSectionType_sectionentry && ! p_data && !length) {
        //subsection entry
    } else {
        printf("section type error?\n");
        error |= VCOM_EFileIOError_invalid_input;
        return NULL;
    }

    if (parent_section->used_num_of_subsections >= parent_section->total_num_of_subsections) {
        if (parent_section->total_num_of_subsections == 0) {
            parent_section->total_num_of_subsections = 2;
            size32 = (TU32)(parent_section->total_num_of_subsections * sizeof(VCOM_SFileIOSection));
            pdata = malloc(size32);
            if (!pdata) {
                printf("Fatal error, CIHierarchyObject::add_subsection malloc fail.\n");
                error |= VCOM_EFileIOError_memalloc_fail;
                parent_section->total_num_of_subsections = 0;
                return NULL;
            }
            memset(pdata, 0, size32);
        } else {
            parent_section->total_num_of_subsections *= 2;
            size32 = (TU32)(parent_section->total_num_of_subsections * sizeof(VCOM_SFileIOSection));
            pdata = malloc(size32);
            if (!pdata) {
                printf("Fatal error, CIHierarchyObject::add_subsection malloc fail.\n");
                parent_section->total_num_of_subsections /= 2;
                error |= VCOM_EFileIOError_memalloc_fail;
                return NULL;
            }
            memset(pdata, 0, size32);
            memcpy(pdata, (void *)parent_section->p_subsection, size32 / 2);
        }
        parent_section->p_subsection = (VCOM_SFileIOSection *)pdata;
    }

    p_sec = &parent_section->p_subsection[parent_section->used_num_of_subsections];
    p_sec->section_name = sectionname;
    if (type == VCOM_EFileIOSectionType_data) {
        p_sec->p_data = malloc(length);
        memcpy(p_sec->p_data, p_data, length);
    } else {
        p_sec->p_data = NULL;
    }
    p_sec->length = length;
    p_sec->size = size;
    p_sec->type = type;
    parent_section->used_num_of_subsections++;
    return p_sec;
}

EECode CIHierarchyObject::write_section(VCOM_SFileIOSection *p_section)
{
    TU64 i = 0, j = 0, number_of_subsection = 0;
    TU16 buf16[6];
    TU32 buf32[5];
    TU64 buf64[2];
    TU16 type = 0;
    void *p_data = NULL;
    TU64 length = 0;
    VCOM_SFileIOSection *p_subsec = NULL;

    if (!p_section) {
        error |= VCOM_EFileIOError_null_pointer;
        return EECode_Error;
    }

    type = p_section->type;
    p_data = p_section->p_data;
    length = p_section->length;
    if (type == VCOM_EFileIOSectionType_data && p_data && length && !p_section->used_num_of_subsections && !p_section->p_subsection) {
        //data section
        buf32[0] = p_section->section_name;
        buf32[1] = p_section->section_index;
        buf16[0] = p_section->size;
        buf16[1] = p_section->type;
        buf16[2] = p_section->flag;
        buf16[3] = p_section->recusive_depth;
        buf64[0] = p_section->length;
        buf32[2] = 0;
#ifdef  VCOM_DFileIOOffsetBit64
        fwrite64(buf32, 4, 2, write_fd);
        fwrite64(buf16, 2, 4, write_fd);
        fwrite64(buf64, 8, 1, write_fd);
        fwrite64(&buf32[2], 4, 1, write_fd);
        fwrite64(p_data, 1, length, write_fd);

#else
        fwrite(buf32, 4, 2, write_fd);
        fwrite(buf16, 2, 4, write_fd);
        fwrite(buf64, 8, 1, write_fd);
        fwrite(&buf32[2], 4, 1, write_fd);

#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "sec_head " << endl;
        __gi_vcomfileio_tot_write_length = ftell(write_fd);
        __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif

        fwrite(p_data, 1, length, write_fd);

#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "sec_data " << endl;
        __gi_vcomfileio_tot_write_length = ftell(write_fd);
        __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif

#endif

    } else if (type == VCOM_EFileIOSectionType_sectionentry) {

        if (!p_section->p_subsection || !p_section->used_num_of_subsections) {
            p_section->length = 0;
            p_section->used_num_of_subsections = 0;
            p_section->p_subsection = NULL;
            printf("Warning: write section entry without subsection.\n");
            error |= VCOM_EFileIOError_warings;
        }

        //subsection entry
        buf32[0] = p_section->section_name;
        buf32[1] = p_section->section_index;
        buf16[0] = p_section->size;
        buf16[1] = p_section->type;
        buf16[2] = p_section->flag;
        buf16[3] = p_section->recusive_depth;
        buf64[0] = p_section->length;
        number_of_subsection = buf32[2] = p_section->used_num_of_subsections;
#ifdef  VCOM_DFileIOOffsetBit64
        fwrite64(buf32, 4, 2, write_fd);
        fwrite64(buf16, 2, 4, write_fd);
        fwrite64(buf64, 8, 1, write_fd);
        fwrite64(&buf32[2], 4, 1, write_fd);
#else
        fwrite(buf32, 4, 2, write_fd);
        fwrite(buf16, 2, 4, write_fd);
        fwrite(buf64, 8, 1, write_fd);
        fwrite(&buf32[2], 4, 1, write_fd);
#endif

#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "sec_head " << endl;
        __gi_vcomfileio_tot_write_length = ftell(write_fd);
        __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif

        //write subsections
        p_subsec = p_section->p_subsection;
        for (i = 0; i < number_of_subsection; i++, p_subsec++) {
            write_section(p_subsec);
        }

    } else {
#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "Error: write file section type error" << endl;
#endif
        printf("section type error?\n");
        error |= VCOM_EFileIOError_invalid_input;
        return EECode_Error;
    }
    return EECode_OK;
}

EECode CIHierarchyObject::write_dataFile()
{
    TU64 i = 0, j = 0, number_of_object = 0, number_of_sections = 0;
    //    TU16 buf16[6];
    TU32 buf32[5];
    TU64 buf64[2];
    VCOM_SFileIOObject *p_object = NULL;
    VCOM_SFileIOSection *p_section = NULL;

    if (!write_fd) {
        printf("Error: in CIHierarchyObject:: write_dataFile, try to write an unexist file.\n");
        error |= VCOM_EFileIOError_null_pointer;
        return EECode_Error;
    }

    if (!calculate_done) {
        calculate_file();
    }

    number_of_object = file.used_number;

    if (!file.tail.p_object_offset) {
        printf("Error in CIHierarchyObject::write_dataFile, the file.tail.p_object_offset==NULL.\n");
        return EECode_Error;
    }

#ifdef  VCOM_DFileIOOffsetBit64
    fseek64(write_fd, 0, SEEK_SET);
#else
    fseek(write_fd, 0, SEEK_SET);
#endif

    //file head
    buf32[0] = file.head.reserved;
    buf32[1] = file.head.type;
    buf32[2] = file.head.type_ext;
    buf32[3] = file.head.info;
#ifdef  VCOM_DFileIOOffsetBit64
    fwrite64(buf32, 4, 4, write_fd);
#else
    fwrite(buf32, 4, 4, write_fd);
#endif

    buf64[0] = file.head.tail_offset;
    buf64[1] = file.head.tail_length;

#ifdef  VCOM_DFileIOOffsetBit64
    fwrite64(buf64, 8, 2, write_fd);
#else
    fwrite(buf64, 8, 2, write_fd);
#endif

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gi_vcomfileio_tot_write_length = 0;
    sprintf(__gi_vcomfileio_filename, __gi_vcomfileio_writefilename_tmp, __gi_vcomfileio_file_index);
    __gf_vcomfileio_write.open(__gi_vcomfileio_filename);

    __gf_vcomfileio_write << "file_head " << endl;
    __gi_vcomfileio_tot_write_length = ftell(write_fd);
    __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif

    //objects
    number_of_object = file.used_number;

    for (i = 0; i < number_of_object; i++) {
        //write object head
        p_object = &file.p_objects[i];
        buf32[0] = file.p_objects[i].head.object_type;
        buf32[1] = file.p_objects[i].head.index;
        buf64[0] = file.p_objects[i].head.tail_offset;

#ifdef  VCOM_DFileIOOffsetBit64
        fwrite64(buf32, 4, 2, write_fd);
        fwrite64(buf64, 8, 1, write_fd);
#else
        fwrite(buf32, 4, 2, write_fd);
        fwrite(buf64, 8, 1, write_fd);
#endif

#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "object_head " << endl;
        __gi_vcomfileio_tot_write_length = ftell(write_fd);
        __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif

        //write sections, no subsection now, todo
        number_of_sections = file.p_objects[i].used_number;
        p_section = p_object->p_sections;
        for (j = 0; j < number_of_sections; j++, p_section++) {
            write_section(p_section);
        }

        //write object tail
        buf64[0] = file.p_objects[i].tail.total_section_number;
#ifdef  VCOM_DFileIOOffsetBit64
        fwrite64(buf64, 8, 1, write_fd);
        fwrite64(p_object->tail.p_section_offset, 8, number_of_sections, write_fd);
#else
        fwrite(buf64, 8, 1, write_fd);
        fwrite(p_object->tail.p_section_offset, 8, (TU32)number_of_sections, write_fd);
#endif

#ifdef __D_vcomfileio_log_write_calculate_length__
        __gf_vcomfileio_write << "object_tail " << endl;
        __gi_vcomfileio_tot_write_length = ftell(write_fd);
        __gf_vcomfileio_write << "current length=" << __gi_vcomfileio_tot_write_length << endl;
#endif
    }

    //write file tail
    buf64[0] = number_of_object;
#ifdef  VCOM_DFileIOOffsetBit64
    fwrite64(buf64, 8, 1, write_fd);
    fwrite64(file.tail.p_object_offset, 8, number_of_object, write_fd);
#else
    fwrite(buf64, 8, 1, write_fd);
    fwrite(file.tail.p_object_offset, 8, (TU32)number_of_object, write_fd);
#endif

#ifdef __D_vcomfileio_log_write_calculate_length__
    __gi_vcomfileio_tot_write_length = 0;
    __gf_vcomfileio_write.close();
#endif

    return EECode_OK;
}
VCOM_SFileIOFile *CIHierarchyObject:: get_fileStruct()
{
    return &file;
}

VCOM_TFileIOErrors CIHierarchyObject::expand_section(VCOM_SFileIOSection *p_section, TU8 *p_data, TU64 length)
{
    TU8 *p_cur = NULL;
    TU64 current = 0, subsec_data_length = 0;
    VCOM_SFileIOSection *p_subsec = NULL;
    TU32 index = 0, i = 0, num_of_subsection = 0;

    if (!p_section || !p_data || !length) {
        printf("Error: in CIHierarchyObject:: expand_section, NULL input.\n");
        error |= VCOM_EFileIOError_null_pointer;
        return VCOM_EFileIOError_null_pointer;
    }

    if (p_section->type == VCOM_EFileIOSectionType_sectionentry) {
        num_of_subsection = p_section->used_num_of_subsections;

        //free previous
        if (p_section->p_subsection) {
            p_subsec = p_section->p_subsection;
            for (i = 0; i < num_of_subsection; i++, p_subsec++) {
                free_section(p_subsec);
            }
            free(p_section->p_subsection);
        }

        p_section->p_subsection = (VCOM_SFileIOSection *)malloc(num_of_subsection * sizeof(VCOM_SFileIOSection));
        memset(p_section->p_subsection, 0, num_of_subsection * sizeof(VCOM_SFileIOSection));

        //expand subsection
        p_subsec = p_section->p_subsection;
        p_cur = p_data;
        for (i = 0; i < num_of_subsection; i++, p_subsec++) {
            //section head
            p_subsec->section_name = *((TU32 *)p_cur);
            p_cur += sizeof(TU32);
            p_subsec->section_index = *((TU32 *)p_cur);
            p_cur += sizeof(TU32);

            p_subsec->size = *((TU16 *)p_cur);
            p_cur += sizeof(TU16);
            p_subsec->type = *((TU16 *)p_cur);
            p_cur += sizeof(TU16);
            p_subsec->flag = *((TU16 *)p_cur);
            p_cur += sizeof(TU16);
            p_subsec->recusive_depth = *((TU16 *)p_cur);
            p_cur += sizeof(TU16);

            p_subsec->length = *((TU64 *)p_cur);
            p_cur += sizeof(TU64);
            p_subsec->used_num_of_subsections = *((TU32 *)p_cur);
            p_cur += sizeof(TU32);

            subsec_data_length = p_subsec->length;
            if (p_subsec->flag == VCOM_EFileIOSectionType_sectionentry) {
                expand_section(p_subsec, p_cur, subsec_data_length);
            } else if (p_subsec->flag == VCOM_EFileIOSectionType_data) {
                p_subsec->p_data = malloc(subsec_data_length);
                memcpy(p_subsec->p_data, p_cur, subsec_data_length);
            } else {
                //read data fail
                printf("expand_section fail.\n");
                error |= VCOM_EFileIOError_internal_error;
                return VCOM_EFileIOError_internal_error;
            }
            p_cur += subsec_data_length;
        }
        //check data
        if ((p_data + length) != p_cur) {
            printf("expand_section data length error,p_data=%p,p_cur=%p,length=%d.\n", p_data, p_cur, length);
            error |= VCOM_EFileIOError_internal_error;
        }
    }

    return VCOM_EFileIOError_nothing;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

