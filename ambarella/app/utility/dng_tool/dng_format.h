/* dng_format.h
 *
 * History:
 *	2014/01/10 - [Louis Sun] create it
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

#ifndef __DNG_FORMAT_H__
#define __DNG_FORMAT_H__




//DNG Tag definitions from DNG spec (by Adobe, as public document)
enum DNG_TAG{
    DNG_NewSubfileType  = 0xFE,
    DNG_ImageWidth = 0x100,
    DNG_ImageLength = 0x101,
    DNG_BitsPerSample = 0x102,
    DNG_Compression  = 0x103,

    DNG_PhotometricInterpretation = 0x106,      //value 2 : RGB, 6: YCbCr   0x8023: CFA  0x884C: LinearRAW

    DNG_Make    = 0x10f,
    DNG_Model   = 0x110,
    DNG_StripOffsets = 0x111,
    DNG_Orientation = 0x112,
    DNG_SamplesPerPixel = 0x115,        // value 1: for CFA or grayscale,   3 for RGB or YCbCr,    1~7 can be used for LinearRAW


    DNG_RowsPerStrip   = 0x116,
    DNG_StripByteCounts = 0x117,
    DNG_XResolution = 0x11a,
    DNG_YResolution = 0x11b,
    DNG_PlanaConfiguration = 0x11c,
    DNG_ResolutionUnit  = 0x128,
    DNG_Software   = 0x131,
    DNG_DateTime    = 0x132,
    DNG_subIFDs =   0x14a,
    DNG_XMP         = 0x2bc,
    DNG_CFARepeatPatternDim = 0x828d,
    DNG_CFAPattern = 0x828e,
    DNG_TimeZoneOffset = 0x882a,
    DNG_ImageNumber = 0x9211,
    DNG_Version = 0xc612,
    DNG_BackwardVersion = 0xc613,
    DNG_UniqueCameraModel = 0xc614,
    DNG_CFAPlaneColor = 0xc616,
    DNG_CFALayout = 0xc617,
    DNG_ColorMatrix1    = 0xc621,
    DNG_ColorMatrix2    = 0xc622,
    DNG_CameraCalibration1 = 0xc623,
    DNG_CameraCalibration2 = 0xc624,
    DNG_AnalogBalance = 0xc627,
    DNG_AsShotNeutral = 0xc628,
    DNG_BaselineExposure = 0xc62a,
    DNG_BaselineNoise = 0xc62b,
    DNG_BaselineSharpness = 0xc62c,
    DNG_LinearResponseLimit = 0xc62e,
    DNG_CameraSerialNumber = 0xc62f,
    DNG_LensInfo    = 0xc630,
    DNG_ShadowScale = 0xc633,
    DNG_PrivateData = 0xc634,
    DNG_CalibrationIlluminant1 = 0xc65a,
    DNG_CalibrationIlluminant2 = 0xc65b,
    DNG_RawDataUniqueID = 0xc65d,
    DNG_OriginalRawFileName = 0xc68b,
    DNG_CameraCalibrationSignature = 0xc6f3,
    DNG_ProfileCalibrationSignature = 0xc6f4,
    DNG_ProfileName = 0xc6f8,
    DNG_ProfileEmbedPolicy = 0xc6fd,
    DNG_ProfileCopyright = 0xc6fe,
    DNG_ForwardMatrix1 = 0xc714,
    DNG_ForwardMatrix2 = 0xc715,
    DNG_PreviewApplicationName = 0xc716,
    DNG_PreviewApplicationVersion = 0xc717,
    DNG_PreviewSettingsDigest = 0xc719,
    DNG_PreviewColorSpace = 0xc71a,
    DNG_PreviewDataTime = 0xc71b,
    DNG_RawImageDigest = 0xc71c,


};

enum DNG_Datatype{
    DNG_DATA_BYTE    = 1,
    DNG_DATA_ASCII   = 2,
    DNG_DATA_SHORT  = 3,  // 2-byte value
    DNG_DATA_LONG   = 4,  // 4-byte value
    DNG_DATA_RATIONAL = 5,  // 8-byte value, first is numerator, second is denominator
    DNG_DATA_SBYTE = 6,    //signed 8-bit integer
    DNG_DATA_UNDEFINED = 7,
    DNG_DATA_SSHORT = 8,
    DNG_DATA_SLONG = 9,
    DNG_DATA_SRATIONAL = 10,
    DNG_DATA_FLOAT = 11,    //single precision 4-byte float
    DNG_DATA_DOUBLE = 12,   //double precision 8-byte float
    DNG_DATA_IFD = 13,
    DNG_DATA_LONG8 = 16,
    DNG_DATA_SLONG8 = 17,
    DNG_DATA_IFD8 = 18,
};

//DNG is extension from TIFF,
//so here we use TIFF 6 header according to TIFF spec
typedef struct dng_header_s {
    char type_identifier[4];        //usually comes with 'II*' followed by '\0'
    int    first_ifd_offset;          //first IFD offset from file start
}dng_header_t;

//ifd has a 2-byte file header
typedef struct ifd_header_s {
    short  num_dir_entries;
}ifd_header_t;


//dir entry is 12-bytes always,
// if data cannot be put into it, then put an offset of data instead
typedef struct dir_entry_s{
    short dng_tag;
    short data_type;
    int     data_count;  //count of the data_type.  if datatype is short, then it's "number of shorts"
    unsigned int data_value;    //the value is either a data value or offset to data value if the value is above 4 bytes
}dir_entry_t;


//picture format struct, not file format.
//the data is used to generate the file format.
typedef struct dng_pic_format_s{
    int pic_type;       //0 means RBG RAW
    int bayer_pattern;
    int bits_per_sample;        // if it's 12-bit, it's 12,  if it's 14-bit,  it's 14
    int pic_width;
    int pic_height;
}dng_pic_format_t;

typedef struct dng_pic_writer_s{
    char   dng_filename[256];
    FILE * fp;
//create memory buffer before write to file
    unsigned char * dng_file_buffer;
    int     dng_file_buffer_size;
    int     dng_file_size;              //final size of dng
    int     dng_file_offset;        //offset used to write dng file buffer
}dng_pic_writer_t;



int dng_write_file_header();
int dng_write_ifd_header();
int dng_write_ifd_dir_entries();
int dng_write_image_data();





#endif

