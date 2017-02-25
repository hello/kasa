/*
 * raw2dng.c
 * the program can convert Ambarella RAW format into Adobe DNG format.
 * this is a code to illustrate the basic idea to see RAW file quickily, but not a
 * formal code to let user to use this DNG format in real product.
 * user should understand this raw2dng code is provided for test purpose AS IS
 * and no warranty. so if you feel it does not work for you, please use formal RAW
 * tools instead.
 * This tool also just takes a subset of DNG, so it does not contain full features of
 * Adobe DNG.
 *
 * History:
 *	2014/01/10 - [Louis Sun] create it
 *
 *
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "dng_format.h"

static dng_pic_format_t  G_dng_format;
static dng_pic_writer_t     G_dng_writer;

#define TOTAL_NUMBER_OF_ENTRIES      21


#define COLOR_MATRIX1_SIZE      (2 * sizeof(long) * 3 * 3)

#define CFA_PATTERN_RGGB    0x02010100          //bayer pattern 0
#define CFA_PATTERN_BGGR    0x00010102          //bayer pattern 1
#define CFA_PATTERN_GRBG    0x01020001          //bayer pattern 2
#define CFA_PATTERN_GBRG    0x01000201          //bayer pattern 3


//use a XYZ to CIE RGB-1 matrix,
//this the tool does not know what CC is used in sensor, use identify matrix for sensor CC
static int colormatrix1[][2] ={
            //XYZ to CIE RGB-1 matrix
            {23707, 10000},
            {-9000, 10000},
            {-4706, 10000},
            {-5139, 10000},
            {14253, 10000},
            {886, 10000},
            {53, 10000},
            {-147, 10000},
            {10094, 10000},
};



static int as_shot_neutral[][2] ={
            {4258, 10000},
            {10000, 10000},
             {7196, 10000},
};

#define AS_SHOT_NEUTRAL_SIZE  (2 * sizeof(long) * 3 )



int dng_write_init(dng_pic_format_t * dng_format,  char * file_name)
{
    memcpy(&G_dng_format, dng_format, sizeof(G_dng_format));
    memset(&G_dng_writer, 0, sizeof(G_dng_writer));
    strcpy(G_dng_writer.dng_filename,  file_name);
    G_dng_writer.fp = fopen(file_name, "wb");
    if (!G_dng_writer.fp) {
            printf("unable to create dng file %s \n", file_name);
            return -1;
    }

    G_dng_writer.dng_file_buffer_size = G_dng_format.pic_width * G_dng_format.pic_height  * 3 + 256; //allocate a buffer with enough size to hold the RAW
    G_dng_writer.dng_file_buffer = malloc (G_dng_writer.dng_file_buffer_size);
    if (!G_dng_writer.dng_file_buffer) {
            printf("unable to allocate dng file buffer \n");
            return -1;
    }

    memset(G_dng_writer.dng_file_buffer , 0, G_dng_writer.dng_file_buffer_size);
    return 0;
}

int dng_write_close(void)
{

    if (!G_dng_writer.fp)
        fclose(G_dng_writer.fp);

    if (!G_dng_writer.dng_file_buffer)
        free(G_dng_writer.dng_file_buffer);

    return 0;
}



int dng_write_file_header()
{
      dng_header_t file_header;
      memset(&file_header, 0, sizeof(file_header));
      strcpy(file_header.type_identifier, "II*");
      file_header.first_ifd_offset =  sizeof(dng_header_t);

      memcpy(G_dng_writer.dng_file_buffer,  &file_header,  sizeof(file_header));
      //move offset
      G_dng_writer.dng_file_offset = sizeof(dng_header_t);

      return 0;
}

int dng_write_ifd_header(void)
{
        ifd_header_t  ifd_header;
        unsigned char * ifd_position;
        ifd_header.num_dir_entries =  TOTAL_NUMBER_OF_ENTRIES;
        ifd_position = G_dng_writer.dng_file_buffer+ sizeof(dng_header_t);
        //copy 2 bytes for number of ifds
        memcpy(ifd_position,  &ifd_header,  2);
        //move offset
        G_dng_writer.dng_file_offset = sizeof(dng_header_t)  + 2;

        return 0;
}




int dng_write_ifd_dir_entries()
{
    dir_entry_t  cur_entry;
    unsigned char * dir_position =  G_dng_writer.dng_file_buffer +  sizeof(dng_header_t)  + 2;
    int * strip_offset_pos;
    int * dng_color_matrix_pos;
    int * dng_as_shot_neutral_pos;
    int cfa_pattern = 0;

    //newSubfiletype
    cur_entry.dng_tag = DNG_NewSubfileType;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    cur_entry.data_value = 0;       //not a reduce size image, but original full size image
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //ImageWidth
    cur_entry.dng_tag = DNG_ImageWidth;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    cur_entry.data_value = G_dng_format.pic_width;
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //ImageWidth
    cur_entry.dng_tag = DNG_ImageLength;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    cur_entry.data_value = G_dng_format.pic_height;
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //BitsPerSample
    cur_entry.dng_tag = DNG_BitsPerSample;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 16;// the RAW will always use 16 bit per pixel to be simpler in storage, if the source is not 16-bit, then we will do conversion
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //Compression
    cur_entry.dng_tag = DNG_Compression;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 1;       //uncompressed
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //PhotometricInterpretation
    cur_entry.dng_tag = DNG_PhotometricInterpretation;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 0x8023;       //CFA
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //Make
    cur_entry.dng_tag = DNG_Make;
    cur_entry.data_type = DNG_DATA_ASCII;
    cur_entry.data_count = 4;       //count of 4 chars
    cur_entry.data_value = 0;
    strcpy((char*)&cur_entry.data_value, "AMB");
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //Model
    cur_entry.dng_tag = DNG_Model;
    cur_entry.data_type = DNG_DATA_ASCII;
    cur_entry.data_count = 3;       //count of 3 chars
    cur_entry.data_value = 0;
    memcpy(&cur_entry.data_value, "S2", 2);
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //StripOffsets:
    cur_entry.dng_tag = DNG_StripOffsets;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    strip_offset_pos =  (int *) ( dir_position + offsetof(dir_entry_t, data_value));
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


 //DNG_Orientation
    cur_entry.dng_tag = DNG_Orientation;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 1;       //regular
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //SamplesPerPixel
    cur_entry.dng_tag = DNG_SamplesPerPixel;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 1;       //CFA
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //RowsPerStrip
    cur_entry.dng_tag = DNG_RowsPerStrip;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    cur_entry.data_value = G_dng_format.pic_height;
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //StripByteCounts
    cur_entry.dng_tag = DNG_StripByteCounts;
    cur_entry.data_type = DNG_DATA_LONG;
    cur_entry.data_count = 1;
    cur_entry.data_value = G_dng_format.pic_width * G_dng_format.pic_height * 2 ;   // consider the bits per sample to be 16.
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //PlanarConfiguration
    cur_entry.dng_tag = DNG_PlanaConfiguration;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 1;
    cur_entry.data_value = 1;       //continuously
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_CFARepeatPatternDim
    cur_entry.dng_tag = DNG_CFARepeatPatternDim;
    cur_entry.data_type = DNG_DATA_SHORT;
    cur_entry.data_count = 2;
    cur_entry.data_value = 0x00020002;
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);

    //DNG_CFAPattern
    cur_entry.dng_tag = DNG_CFAPattern;
    cur_entry.data_type = DNG_DATA_BYTE;
    cur_entry.data_count = 4;

    switch(G_dng_format.bayer_pattern)
    {
    case 0:
        cfa_pattern = CFA_PATTERN_RGGB;
        printf("Use Bayer pattern RG\n");
        break;
    case 1:
        cfa_pattern = CFA_PATTERN_BGGR;
        printf("Use Bayer pattern BG\n");
        break;
    case 2:
        cfa_pattern = CFA_PATTERN_GRBG;
        printf("Use Bayer pattern GR\n");
        break;
    case 3:
        cfa_pattern = CFA_PATTERN_GBRG;
        printf("Use Bayer pattern GB\n");
        break;
     default:
         printf("cfa pattern error %d \n", G_dng_format.bayer_pattern);
         return -1;
      }
    cur_entry.data_value = cfa_pattern; //IMX104


    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_Version
    cur_entry.dng_tag = DNG_Version;
    cur_entry.data_type = DNG_DATA_BYTE;
    cur_entry.data_count = 4;
    cur_entry.data_value = 0x00000201;  //version 1.2.0.0
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_UniqueCameraModel
    cur_entry.dng_tag = DNG_UniqueCameraModel;
    cur_entry.data_type = DNG_DATA_ASCII;
    cur_entry.data_count = 4;
    cur_entry.data_value = 0;
    strcpy((char*)&cur_entry.data_value,  "EVK");
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_CFAPlaneColor
    cur_entry.dng_tag = DNG_CFAPlaneColor;
    cur_entry.data_type = DNG_DATA_BYTE;
    cur_entry.data_count = 3;
    cur_entry.data_value = 0x0020100;
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_ColorMatrix1
    cur_entry.dng_tag = DNG_ColorMatrix1;
    cur_entry.data_type = DNG_DATA_SRATIONAL;
    cur_entry.data_count = 9;
    dng_color_matrix_pos = (int *)(  dir_position + offsetof(dir_entry_t, data_value));
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);


    //DNG_AsShotNeutral
    cur_entry.dng_tag = DNG_AsShotNeutral;
    cur_entry.data_type = DNG_DATA_RATIONAL;
    cur_entry.data_count = 3;
    dng_as_shot_neutral_pos = (int *)( dir_position + offsetof(dir_entry_t, data_value));
    memcpy(dir_position, &cur_entry, sizeof(cur_entry));
    dir_position += sizeof(cur_entry);




    //IFD Terminator by 4 zero bytes
    memset(dir_position, 0, sizeof(int));   // 4 bytes for ifd terminator
    dir_position += sizeof(int);


    //fill the Color Matrix 1 now
    *dng_color_matrix_pos = dir_position - G_dng_writer.dng_file_buffer;
    memcpy(dir_position, colormatrix1, COLOR_MATRIX1_SIZE);
    dir_position += COLOR_MATRIX1_SIZE;

    //fill As Shot Neutral:
    *dng_as_shot_neutral_pos = dir_position - G_dng_writer.dng_file_buffer;
    memcpy(dir_position, as_shot_neutral,  AS_SHOT_NEUTRAL_SIZE);
    dir_position += AS_SHOT_NEUTRAL_SIZE;



    //now update the STRIPOFFSET, because it's referring to later
   *strip_offset_pos = dir_position - G_dng_writer.dng_file_buffer;

    //after StripOffset, it's the image data, update dng_file_offset
    G_dng_writer.dng_file_offset = dir_position - G_dng_writer.dng_file_buffer;

    return 0;
}



int dng_write_image_data(dng_pic_format_t  * dng_format, unsigned char * raw_data,  int raw_data_size)
{
     unsigned short * img_addr =  (unsigned short*) (G_dng_writer.dng_file_buffer + G_dng_writer.dng_file_offset);
     unsigned short   source_data;
     int row, col;

    //AMBA RAW always use 14-bit to store data, if the data is 12-bit, then it's shifted to be 14-bit.
    //which means G_dng_format.bits_per_sample  MUST be 14.

    for (row = 0; row < dng_format->pic_height;  row++)
    {
             for(col = 0; col < dng_format->pic_width; col++)
            {
                    //get source data from original pic
                    source_data  =  *(unsigned short *)(raw_data +  col * 2 + dng_format->pic_width * row * 2);
                    //convert source 14-bit into 16-bit
                    *img_addr =  (source_data<<2);
                    img_addr++;
            }
    }

    //update offset
    G_dng_writer.dng_file_offset = (unsigned char *)img_addr  - G_dng_writer.dng_file_buffer;

    return 0;
}


int dng_flush_file(void)
{
    if (!G_dng_writer.fp) {
        printf("dng file fp wrong \n");
        return -1;
    }

    if (fwrite(G_dng_writer.dng_file_buffer, 1,  G_dng_writer.dng_file_offset, G_dng_writer.fp) <  G_dng_writer.dng_file_offset)  {
        printf("actual file write count not equal to size %d \n", G_dng_writer.dng_file_offset);
        return -2;
    }



    return 0;
}


int amba_raw_get_data(char * source_raw_filename, int width, int height,  unsigned char ** source_raw_data,  int * source_raw_size)
{
    FILE * source_raw = NULL;
    int length;
    int ret = 0;
    unsigned char * ambaraw_data =NULL;

    do {
        source_raw = fopen(source_raw_filename, "rb");
        if (!source_raw) {
            printf("unable to open Amba RAW file %s \n", source_raw_filename);
           ret = -1;
           break;
        }

        fseek(source_raw, 0, SEEK_END);
        length = ftell(source_raw);
        rewind(source_raw);

        if (length <  width * height *2) {
            printf("Warning! Amba RAW file size is %d,  not equal to width* height * 2\n", length);
            ret = -2;
            break;
        }

        //only use the first size of the file, if it's bigger than usual.
        length = width * height *2;
        ambaraw_data = (unsigned char *)malloc (length);
        if (!ambaraw_data) {
                printf("unable to allocate mem for amba raw , size %d \n", length);
            ret = -3;
            break;
        }

        if (fread(ambaraw_data, 1, length, source_raw)!= length) {
                printf("actual read out size not equal to file length \n");
            ret = -4;
            break;
         }

    }while(0);


    if (source_raw)
        fclose(source_raw);

    if (ret == 0) {
        if (source_raw_data)
            *source_raw_data = ambaraw_data;

        if (source_raw_size)
            *source_raw_size = length;
    }


    return ret;
}


int main(int argc ,  char * argv[])
{

    dng_pic_format_t  dng_format;
    int ret = 0;
    int source_raw_size;
    unsigned char * source_raw_data = NULL;

    if (argc < 6) {
        printf("raw2dng ver 1.15,  developed for demo purpose.\n");
        printf("raw2dng is a simple tool to convert Ambarella RAW format into Adobe DNG.\n");
        printf("you must specify bayer pattern correctly, otherwise, it will not generate correct DNG.\n");
        printf("this tool takes 5 args:\n");
        printf("\nsensor list with bayer pattern type :\n");
        printf("IMX035\t\t\t\tRG \n");
        printf("IMX036\t\t\t\tRG \n");
        printf("IMX104/IMX138/IMX238\t\tRG \n");
        printf("IMX121\t\t\t\tRG \n");
        printf("IMX122/IMX222\t\t\tGB \n");
        printf("IMX123/IMX124\t\t\tRG \n");
        printf("IMX136/IMX236\t\t\tRG \n");
        printf("IMX172\t\t\t\tRG \n");
        printf("IMX178\t\t\t\tRG \n");
        printf("IMX185\t\t\t\tRG \n");
        printf("IMX226\t\t\t\tRG \n");
        printf("MT9P031/MT9P001\t\t\tGR \n");
        printf("MT9P006\t\t\t\tGR \n");
        printf("MT9M034\t\t\t\tRG \n");
        printf("AR0130\t\t\t\tRG \n");
        printf("MT9T002(AR0330)\t\t\tGR \n");
        printf("AR0331\t\t\t\tGR \n");
        printf("AR0835\t\t\t\tRG \n");
        printf("MT9J001\t\t\t\tGR \n");
        printf("OV9715/OV9710\t\t\tBG \n");
        printf("OV2715/OV2710\t\t\tGR \n");
        printf("OV9718\t\t\t\tRG \n");
        printf("OV9726\t\t\t\tBG \n");
        printf("OV5653\t\t\t\tBG \n");
        printf("OV5658\t\t\t\tBG \n");
        printf("OV14810\t\t\t\tBG \n");
        printf("OV4688/OV4689\t\t\tBG \n");
        printf("MN34031\t\t\t\tBG \n");
        printf("MN34041\t\t\t\tBG \n");
        printf("MN34210\t\t\t\tRG \n");
        printf("MN34220\t\t\t\tRG \n");
        printf("\nExample:\nraw2dng 1.RAW  1.dng  width height  bayerpattern(0~3 or RG/BG/GR/GB) \n\n");
        return -1;
    }

    do {
        memset(&dng_format, 0, sizeof(dng_format));
        dng_format.pic_width = atoi(argv[3]);
        dng_format.pic_height = atoi(argv[4]);
        printf("create DNG with filename %s,  width %d, height %d \n", argv[2], dng_format.pic_width, dng_format.pic_height);
        dng_format.bits_per_sample = 14;

       if (!strcmp(argv[5], "RG") ||  !strcmp(argv[5], "rg")) {
             dng_format.bayer_pattern = 0;
       }else if (!strcmp(argv[5], "BG") ||  !strcmp(argv[5], "bg")) {
             dng_format.bayer_pattern = 1;
       }else if (!strcmp(argv[5], "GR") ||  !strcmp(argv[5], "gr")) {
             dng_format.bayer_pattern = 2;
       }else if (!strcmp(argv[5], "GB") ||  !strcmp(argv[5], "gb")) {
             dng_format.bayer_pattern = 3;
        }else {
            dng_format.bayer_pattern = atoi(argv[5]);
        }

        dng_format.pic_type = 0; //CFA

       if (amba_raw_get_data(argv[1],  dng_format.pic_width, dng_format.pic_height, &source_raw_data, &source_raw_size) < 0) {
            printf("amba raw get data failed \n");
            ret = -1;
            break;
       }

       if (dng_write_init(&dng_format,  argv[2]) < 0) {
            printf("dng write init failed \n");
            ret = -2;
            break;
       }

       if (dng_write_file_header() < 0) {
            printf("dng write file header failed \n");
            ret = -3;
            break;
        }


       if (dng_write_ifd_header() < 0) {
            printf("dng write ifd header failed \n");
            ret = -4;
            break;
        }

        if (dng_write_ifd_dir_entries() < 0) {
            printf("dng write ifd dir entries failed \n");
            ret = -5;
            break;
        }

        if (dng_write_image_data(&dng_format, source_raw_data, source_raw_size) < 0) {
            printf("dng write image data failed \n");
            ret = -6;
            break;
        }


       if (dng_flush_file() < 0 ) {
            printf("dng flush file failed \n");
            ret = -7;
            break;
      }

      if (dng_write_close() < 0) {
            printf("dng close file failed \n");
            ret = -8;
            break;
     }


    }while(0);



    if (source_raw_data)
        free(source_raw_data);

    return ret;

}


