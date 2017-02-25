/*******************************************************************************
 * iso_base_media_file_defs.h
 *
 * History:
 *    2014/12/02 - [Zhi He] Create File
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

#ifndef __ISO_BASE_MEDIA_FILE_DEFS__
#define __ISO_BASE_MEDIA_FILE_DEFS__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;


/*
 * simple layout of MP4 file (interleave, 32bit/64bit adaptive)
 * ----ftyp (File Type Box)
 * ----mdat (Media Data Box)
 * ----moov (Movie Box)
 * --------mvhd (Movie Header Box)
 * --------trak (Track Box)
 * ------------tkhd (Track Header Box)
 * ------------tref (Track Reference Container)
 * ------------edts (Edit List Container)
 * ----------------elst (An Edit List)
 * ------------mdia (Container for Media information of the track)
 * ----------------mdhd (Media header, overall information about the media)
 * ----------------hdlr (Media handler)
 * ----------------minf (Media information container)
 * --------------------vmhd (Video Media Header)
 * --------------------smhd (Sound Media Header)
 * --------------------stbl (Sample Table Box)
 * ------------------------stsd (Sample Descriptions)
 * ------------------------stts (Decoding Time Stamp)
 * ------------------------stsc (Sample to Chunk, partial data-offset information)
 * ------------------------stco (Chunk offset, partial data-offset information)
 * ------------------------co64 (Chunk offset(64 bits), partial data-offset information)
 *
*/

#define DISOM_FILE_TYPE_BOX_TAG                     "ftyp"
#define DISOM_MEDIA_DATA_BOX_TAG                "mdat"
#define DISOM_MOVIE_BOX_TAG                         "moov"
#define DISOM_MOVIE_HEADER_BOX_TAG          "mvhd"
#define DISOM_TRACK_BOX_TAG                         "trak"
#define DISOM_TRACK_HEADER_BOX_TAG          "tkhd"
#define DISOM_TRACK_REFERENCE_BOX_TAG      "tref"
#define DISOM_MEDIA_BOX_TAG                             "mdia"
#define DISOM_MEDIA_HEADER_BOX_TAG             "mdhd"
#define DISOM_HANDLER_REFERENCE_BOX_TAG   "hdlr"
#define DISOM_MEDIA_INFORMATION_BOX_TAG   "minf"
#define DISOM_VIDEO_MEDIA_HEADER_BOX_TAG   "vmhd"
#define DISOM_SOUND_MEDIA_HEADER_BOX_TAG   "smhd"
#define DISOM_DATA_INFORMATION_BOX_TAG        "dinf"
#define DISOM_DATA_REFERENCE_BOX_TAG            "dref"
#define DISOM_DATA_ENTRY_URL_BOX_TAG            "url "
#define DISOM_SAMPLE_TABLE_BOX_TAG                          "stbl"
#define DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG      "stts"
#define DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG      "ctts"
#define DISOM_SAMPLE_DESCRIPTION_BOX_TAG                         "stsd"
#define DISOM_SAMPLE_SIZE_BOX_TAG                                       "stsz"
#define DISOM_COMPACT_SAMPLE_SIZE_BOX_TAG                     "stz2"
#define DISOM_SAMPLE_TO_CHUNK_BOX_TAG                           "stsc"
#define DISOM_CHUNK_OFFSET_BOX_TAG                                  "stco"
#define DISOM_CHUNK_OFFSET64_BOX_TAG                               "co64"
#define DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG      "avcC"
#define DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG      "hvcC"
#define DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG        "esds"
#define DISOM_USER_DATA_BOX_TAG                         "udta"
#define DISOM_EDIT_LIST_CONTAINER_BOX_TAG     "edts"
#define DISOM_EDIT_LIST_BOX_TAG     "elst"

#define DISOM_VIDEO_HANDLER_REFERENCE_TAG                   "vide"
#define DISOM_AUDIO_HANDLER_REFERENCE_TAG                   "soun"
#define DISOM_HINT_HANDLER_REFERENCE_TAG                   "hint"

#define DISOM_BRAND_V0_TAG                     "isom"
#define DISOM_BRAND_V1_TAG                     "iso2"
#define DISOM_BRAND_AVC_TAG                    "avc1"
#define DISOM_BRAND_HEVC_TAG                    "hvc1"
#define DISOM_BRAND_MPEG4_TAG               "mpg4"
#define DISOM_BRAND_MSMPEG4_TAG               "mp42"

#define DISOM_CODEC_AVC_NAME                "avc1"
#define DISOM_CODEC_HEVC_NAME                "hvc1"
#define DISOM_CODEC_AAC_NAME                "mp4a"

#define DISOM_TAG_SIZE 4
#define DISOM_BOX_SIZE 8
#define DISOM_FULL_BOX_SIZE 12

#define DISOM_TRACK_ENABLED_FLAG 0x000001

typedef struct {
    TU32 size;
    TChar type[DISOM_TAG_SIZE];
} SISOMBox;

typedef struct {
    SISOMBox base_box;
    TU8 version;
    TU32 flags; //24bits
} SISOMFullBox;

#define DISOM_MAX_COMPATIBLE_BRANDS 8
typedef struct {
    SISOMBox base_box;

    TChar major_brand[DISOM_TAG_SIZE];
    TU32 minor_version;
    TChar compatible_brands[DISOM_MAX_COMPATIBLE_BRANDS][DISOM_TAG_SIZE];

    TU32 compatible_brands_number;
} SISOMFileTypeBox;

typedef struct {
    SISOMBox base_box;
} SISOMMediaDataBox;

typedef struct {
    SISOMFullBox base_full_box;

    //version specific
    TU64 creation_time;// 32 bits in version 0
    TU64 modification_time;// 32 bits in version 0
    TU32 timescale;
    TU64 duration; // 32 bits in version 0

    TU32 rate;
    TU16 volume;
    TU16 reserved;

    TU32 reserved_1;
    TU32 reserved_2;

    TU32 matrix[9];
    TU32 pre_defined[6];
    TU32 next_track_ID;
} SISOMMovieHeaderBox;
#define DISOMConstMovieHeaderSize 108
#define DISOMConstMovieHeader64Size 120

typedef struct {
    SISOMFullBox base_full_box;

    //version specific
    TU64 creation_time;// 32 bits in version 0
    TU64 modification_time;// 32 bits in version 0
    TU32 track_ID;
    TU32 reserved;
    TU64 duration; // 32 bits in version 0

    TU32 reserved_1[2];
    TU16 layer;
    TU16 alternate_group;
    TU16 volume;
    TU16 reserved_2;

    TU32 matrix[9];
    TU32 width;
    TU32 height;
} SISOMTrackHeader;
#define DISOMConstTrackHeaderSize 92
#define DISOMConstTrackHeader64Size 104

typedef struct {
    SISOMFullBox base_full_box;

    //version specific
    TU64 creation_time;// 32 bits in version 0
    TU64 modification_time;// 32 bits in version 0
    TU32 timescale;
    TU64 duration; // 32 bits in version 0

    TU8 pad; // 1bit
    TU8 language[3]; //5bits
    TU16 pre_defined;
} SISOMMediaHeaderBox;
#define DISOMConstMediaHeaderSize 32
#define DISOMConstMediaHeader64Size 44

typedef struct {
    SISOMFullBox base_full_box;

    TU32 pre_defined;
    TChar handler_type[DISOM_TAG_SIZE];
    TU32 reserved[3];

    TChar *name;
} SISOMHandlerReferenceBox;
#define DISOMConstHandlerReferenceSize 32

typedef struct {
    SISOMFullBox base_full_box;

    TU16 graphicsmode;
    TU16 opcolor[3];
} SISOMVideoMediaHeaderBox;
#define DISOMConstVideoMediaHeaderSize 20

typedef struct {
    SISOMFullBox base_full_box;

    TU16 balanse;
    TU16 reserved;
} SISOMSoundMediaHeaderBox;
#define DISOMConstSoundMediaHeaderSize 16

typedef struct {
    TU32 sample_count;
    TU32 sample_delta;
} _STimeEntry;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 entry_count;
    _STimeEntry *p_entry;

    TU32 entry_buf_count;
} SISOMDecodingTimeToSampleBox;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 entry_count;
    _STimeEntry *p_entry;
} SISOMCompositionTimeToSampleBox;

typedef struct {
    SISOMBox base_box;

    TU8 configurationVersion;
    TU8 AVCProfileIndication;
    TU8 profile_compatibility;
    TU8 AVCLevelIndication;

    TU8 reserved; // 6bits 1
    TU8 lengthSizeMinusOne; // 2bits
    TU8 reserved_1; // 3bits 1
    TU8 numOfSequenceParameterSets; // 5bits

    TU16 sequenceParametersSetLength;
    TU8 *p_sps;

    TU8 numOfPictureParameterSets;
    TU16 pictureParametersSetLength;
    TU8 *p_pps;
} SISOMAVCDecoderConfigurationRecordBox;
#define DISOMConstAVCDecoderConfiurationRecordSize_FixPart 19

typedef struct {
    TU8 array_completeness; // 1bit
    TU8 reserved; // 1bit
    TU8 nal_type; // 6bits
    TU16 numNalus;

    //TU16 nalUintLength;
    // data ...
} _SNALArray;

typedef struct {
    SISOMBox base_box;

    SHEVCDecoderConfigurationRecord record;

    TU8 *vps;
    TU32 vps_length;
    TU8 *sps;
    TU32 sps_length;
    TU8 *pps;
    TU32 pps_length;

    TU8 *p_seis[8];
    TU32 sei_lengths[8];
    TU32 num_of_sei;
} SISOMHEVCDecoderConfigurationRecordBox;
#define DISOMConstHEVCDecoderConfiurationRecordSize_FixPart 31

typedef struct {
    SISOMBox base_box;
    TU8 reserved_0[6];
    TU16 data_reference_index;

    TU16 pre_defined; // = 0
    TU16 reserved; // = 0
    TU32 pre_defined_1[3]; // = 0
    TU16 width;
    TU16 height;

    TU32 horizresolution;
    TU32 vertresolution;
    TU32 reserved_1;
    TU16 frame_count;
    TChar compressorname[32];
    TU16 depth; // = 0x0018
    TS16 pre_defined_2; // = -1

    SISOMAVCDecoderConfigurationRecordBox avc_config;
    SISOMHEVCDecoderConfigurationRecordBox hevc_config;
} _SVisualSampleEntry;
#define DISOMConstVisualSampleEntrySize_FixPart 86

typedef struct {
    SISOMFullBox base_full_box;

    TU8 es_descriptor_type_tag;
    TU32 strange_size_0;
    TU16 track_id;
    TU8 flags;

    TU8 decoder_descriptor_type_tag;
    TU32 strange_size_1;
    TU8 object_type;
    TU8 stream_flag;
    TU32 buffer_size;// 24bits
    TU32 max_bitrate;
    TU32 avg_bitrate;

    TU8 decoder_config_type_tag;
    TU32 strange_size_2;
    TU8 config[4]; //hard code here

    TU8 SL_descriptor_type_tag;
    TU32 strange_size_3;
    TU8 SL_value;
} SISOMAACElementaryStreamDescriptor;
#define DISOMConstAACElementaryStreamDescriptorSize_FixPart 49

typedef struct {
    SISOMBox base_box;
    TU8 reserved_0[6];
    TU16 data_reference_index;

    TU32 reserved[2]; // = 0
    TU16 channelcount; // = 2
    TU16 samplesize; // = 16

    TU16 pre_defined; // = 0
    TU16 reserved_1; // = 0
    TU32 samplerate;

    SISOMAACElementaryStreamDescriptor esd;
} _SAudioSampleEntry;
#define DISOMConstAudioSampleEntrySize 36

typedef struct {
    SISOMFullBox base_full_box;
    TU32 entry_count;
    _SVisualSampleEntry visual_entry;
} SISOMVisualSampleDescriptionBox;

typedef struct {
    SISOMFullBox base_full_box;
    TU32 entry_count;
    _SAudioSampleEntry audio_entry;
} SISOMAudioSampleDescriptionBox;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 sample_size;
    TU32 sample_count;
    TU32 *entry_size;

    TU32 entry_buf_count;
} SISOMSampleSizeBox;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 reserved; // 24 bits
    TU32 field_size; //8 bits
    TU32 sample_count;
    TU32 *entry_size; // adaptive bits
} SISOMCompactSampleSizeBox;

typedef struct {
    TU32 first_chunk;
    TU32 sample_per_chunk;
    TU32 sample_description_index;
} _SSampleToChunkEntry;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 entry_count;
    _SSampleToChunkEntry *entrys;
} SISOMSampleToChunkBox;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 entry_count;
    TU32 *chunk_offset;

    TU32 entry_buf_count;
} SISOMChunkOffsetBox;

typedef struct {
    SISOMFullBox base_full_box;

    TU32 entry_count;
    TU64 *chunk_offset;
} SISOMChunkOffset64Box;

typedef struct {
    SISOMFullBox base_full_box;
    TU32 entry_count;

    SISOMFullBox url;
} SISOMDataReferenceBox;

typedef struct {
    SISOMBox base_box;

    SISOMDataReferenceBox data_ref;
} SISOMDataInformationBox;

typedef struct {
    SISOMBox base_box;

    SISOMDecodingTimeToSampleBox stts;
    SISOMCompositionTimeToSampleBox ctts;

    SISOMVisualSampleDescriptionBox visual_sample_description;
    SISOMAudioSampleDescriptionBox audio_sample_description;

    SISOMSampleSizeBox sample_size;
    SISOMSampleToChunkBox sample_to_chunk;
    SISOMChunkOffsetBox chunk_offset;
} SISOMSampleTableBox;

typedef struct {
    SISOMBox base_box;

    SISOMVideoMediaHeaderBox video_header;
    SISOMSoundMediaHeaderBox sound_header;

    SISOMDataInformationBox data_info;

    SISOMSampleTableBox sample_table;
} SISOMMediaInformationBox;

typedef struct {
    SISOMBox base_box;
    SISOMMediaHeaderBox media_header;
    SISOMHandlerReferenceBox media_handler;
    SISOMMediaInformationBox media_info;
} SISOMMediaBox;

typedef struct {
    SISOMBox base_box;
    SISOMTrackHeader track_header;
    SISOMMediaBox media;
} SISOMTrackBox;

typedef struct {
    SISOMBox base_box;

    SISOMMovieHeaderBox movie_header_box;

    SISOMTrackBox video_track;
    SISOMTrackBox audio_track;
} SISOMMovieBox;

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

