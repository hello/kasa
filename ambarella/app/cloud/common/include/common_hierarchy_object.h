/**
 * common_hierarchy_object.h
 *
 * History:
 *  2014/11/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_HIERARCHY_OBJECT_H__
#define __COMMON_HIERARCHY_OBJECT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//fileIO related start
#define VCOM_DFileIOWorkMode_defalt 0
#define VCOM_DFileIOWorkMode_instantToFile  1
#define VCOM_DFileIOWorkMode_freeMemSelf    2
#define VCOM_DFileIOWorkMode_write  4
#define VCOM_DFileIOWorkMode_read 8
#define VCOM_DFileIOWorkMode_binary  16
#define VCOM_DFileIOWorkMode_text 32

#define VCOM_DFileIOSectionHeadLength 28
#define VCOM_DFileIOObjectHeadLength  16
#define VCOM_DFileIOFileHeadLength  32

class CIHierarchyObject
{
public:
    CIHierarchyObject();
    ~CIHierarchyObject();

    //open (read or write)file
    EECode open_file(TChar *filename, TU32 flag);
    //close readed file or written file, write file to disk
    EECode close_file(TU32 flag);

    //read file, append the file data
    EECode read_dataFile();
    //write all data to file
    EECode write_dataFile();
    //return the file stuct
    VCOM_SFileIOFile *get_fileStruct();

    //read memory
    EECode read_memFile(void *p_mem, TU64 length);
    //write data to memory, alloc memory
    EECode write_memFile();
    //get the memory buffer
    void *get_memStruct(TU64 *p_length);

    VCOM_SFileIOObject *add_object(TU32 objectname, TU32 index);
    //type indicate the section is data section or subsection entry
    VCOM_SFileIOSection *add_section(VCOM_SFileIOObject *parent_object, TU32 sectionname, TU64 length, void *p_data, TU16 size, TU16 type);
    //add subsection: may be section entry too
    VCOM_SFileIOSection *add_subSection(VCOM_SFileIOSection *parent_section, TU32 sectionname, TU64 length, void *p_data, TU16 size, TU16 type);


    //        TU64 retreive_sectionData(void* des_buf,TU64 object_index,TU64 section_index);

    VCOM_TFileIOStatus get_status();
    TU32 get_error();

private:
    //recalculate the file information, update the lengthes, indexes. must be called before write file to disk or memory
    void calculate_file();
    //calculate objects,updates its lengthes
    void calculate_object(VCOM_SFileIOObject *p_ob);
    //calculate objects,updates its lengthes
    void calculate_section(VCOM_SFileIOSection *p_sec);

    EECode write_section(VCOM_SFileIOSection *p_section);

    //expend section if it have subsection
    VCOM_TFileIOErrors expand_section(VCOM_SFileIOSection *p_section, TU8 *p_data, TU64 length);
    //free section
    void free_section(VCOM_SFileIOSection *p_section);

protected:
    //file
    FILE  *read_fd, *write_fd;
    //memory
    void *p_read_mem, *p_write_mem; //p_read_mem is from outside, need not to be freed
    TU64 read_mem_len, write_mem_len;

    //        VCOM_TFileIOErrors error;
    TU32  error;
    VCOM_TFileIOStatus status;
    VCOM_SFileIOFile file;

    TU32 calculate_done;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

