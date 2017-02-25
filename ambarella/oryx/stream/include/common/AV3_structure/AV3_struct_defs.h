/*******************************************************************************
 * av3_struct_defs.h
 *
 * History:
 *   2016-08-24 - [ccjing] created file
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


#ifndef AV3_BASE_DEFS_
#define AV3_BASE_DEFS_
/*
 * simple layout of MP4 file (interleave, 32bit/64bit adaptive)
 * --ftyp (File Type Box)
 * --mdat (Media Data Box)
 * --moov (Movie Box)
 * ----mvhd (Movie Header Box)
 * ----trak (Track Box)
 * ------tkhd (Track Header Box)
 * ------mdia (Container for Media information of the track)
 * --------mdhd (Media header, overall information about the media)
 * --------hdlr (Media handler)
 * --------minf (Media information container)
 * ----------vmhd (Video Media Header) (for video track)
 * ----------smhd (Sound Media Header) (for audio track)
 * ----------dinf (data information box)
 * ------------dref (data reference box, declares source of metadata items)
 * --------------url
 * ----------stbl (Sample Table Box)
 * ------------stsd (Sample Descriptions)
 * ------------stts (Decoding Time Stamp)
 * ------------stsc (Sample to Chunk, partial data-offset information)
 * ------------stsz (sample size box)
 * ------------stco (Chunk offset, partial data-offset information)
 * ------------stss (sync sample table)//I frame location
 */

#define AV3_BOX_TAG_CREATE(X,Y,Z,N) \
         X | (Y << 8) | (Z << 16) | (N << 24)
class AV3FileWriter;
class AV3FileReader;

enum AV3_BOX_TAG
{
  AV3_BOX_TAG_NULL = 0,
  AV3_FILE_TYPE_BOX_TAG = AV3_BOX_TAG_CREATE('f', 't', 'y', 'p'),
  AV3_MEDIA_DATA_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'd', 'a', 't'),
  AV3_MOVIE_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'o', 'o', 'v'),
  AV3_MOVIE_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'v', 'h', 'd'),
  DIDOM_OBJECT_DESC_BOX_TAG = AV3_BOX_TAG_CREATE('i', 'o', 'd', 's'),
  AV3_TRACK_BOX_TAG = AV3_BOX_TAG_CREATE('t', 'r', 'a', 'k'),
  AV3_TRACK_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('t', 'k', 'h', 'd'),
  AV3_MEDIA_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'd', 'i', 'a'),
  AV3_MEDIA_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'd', 'h', 'd'),
  AV3_HANDLER_REFERENCE_BOX_TAG = AV3_BOX_TAG_CREATE('h', 'd', 'l', 'r'),
  AV3_MEDIA_INFORMATION_BOX_TAG = AV3_BOX_TAG_CREATE('m', 'i', 'n', 'f'),
  AV3_VIDEO_MEDIA_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('v', 'm', 'h', 'd'),
  AV3_SOUND_MEDIA_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('s', 'm', 'h', 'd'),
  AV3_NULL_MEDIA_HEADER_BOX_TAG = AV3_BOX_TAG_CREATE('n', 'm', 'h', 'd'),
  AV3_DATA_INFORMATION_BOX_TAG = AV3_BOX_TAG_CREATE('d', 'i', 'n', 'f'),
  AV3_DATA_REFERENCE_BOX_TAG = AV3_BOX_TAG_CREATE('d', 'r', 'e', 'f'),
  AV3_DATA_ENTRY_URL_BOX_TAG = AV3_BOX_TAG_CREATE('u', 'r', 'l', ' '),
  AV3_SAMPLE_TABLE_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 'b', 'l'),
  AV3_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 't', 's'),
  AV3_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG = AV3_BOX_TAG_CREATE('c', 't', 't', 's'),
  AV3_SAMPLE_DESCRIPTION_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 's', 'd'),
  AV3_SAMPLE_SIZE_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 's', 'z'),
  AV3_COMPACT_SAMPLE_SIZE_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 'z', '2'),
  AV3_SAMPLE_TO_CHUNK_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 's', 'c'),
  AV3_CHUNK_OFFSET_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 'c', 'o'),
  AV3_CHUNK_OFFSET64_BOX_TAG = AV3_BOX_TAG_CREATE('c', 'o', '6', '4'),
  AV3_SYNC_SAMPLE_TABLE_BOX_TAG = AV3_BOX_TAG_CREATE('s', 't', 's', 's'),
  AV3_AVC_DECODER_CONFIGURATION_RECORD_TAG = AV3_BOX_TAG_CREATE('a', 'v', 'c', 'C'),
  AV3_HEVC_DECODER_CONFIGURATION_RECORD_TAG = AV3_BOX_TAG_CREATE('h', 'v', 'c', 'C'),
  AV3_AAC_DESCRIPTOR_BOX_TAG = AV3_BOX_TAG_CREATE('e', 's', 'd', 's'),
  AV3_OPUS_DESCRIPTOR_BOX_TAG = AV3_BOX_TAG_CREATE('d', 'O', 'p', 's'),
  AV3_UUID_BOX_TAG = AV3_BOX_TAG_CREATE('u', 'u', 'i', 'd'),
  AV3_PSMT_BOX_TAG = AV3_BOX_TAG_CREATE('p', 's', 'm', 't'),
  AV3_PSMH_BOX_TAG = AV3_BOX_TAG_CREATE('p', 's', 'm', 'h'),
  AV3_PSFM_BOX_TAG = AV3_BOX_TAG_CREATE('p', 's', 'f', 'm'),
  AV3_XML_BOX_TAG = AV3_BOX_TAG_CREATE('x', 'm', 'l', ' '),
  AV3_FREE_BOX_TAG = AV3_BOX_TAG_CREATE('f', 'r', 'e', 'e'),
  AV3_OPUS_ENTRY_TAG = AV3_BOX_TAG_CREATE('O', 'p', 'u', 's'),
  AV3_AAC_ENTRY_TAG = AV3_BOX_TAG_CREATE('m', 'p', '4', 'a'),
  AV3_H264_ENTRY_TAG = AV3_BOX_TAG_CREATE('a', 'v', 'c', '1'),
  AV3_H265_ENTRY_TAG = AV3_BOX_TAG_CREATE('h', 'v', 'c', '1'),
  AV3_META_ENTRY_TAG = AV3_BOX_TAG_CREATE('m', 'e', 't', 't'),
};

enum AV3_BRAND_TAG{
  AV3_BRAND_NULL = 0,
  AV3_BRAND_TAG_0 = AV3_BOX_TAG_CREATE('m', 'p', '4', '2'),
  AV3_BRAND_TAG_1 = AV3_BOX_TAG_CREATE('a', 'v', 'c', '1'),
};

enum AV3_HANDLER_TYPE{
  NULL_TRACK            = 0,
  VIDEO_TRACK           = AV3_BOX_TAG_CREATE('v', 'i', 'd', 'e'),
  AUDIO_TRACK           = AV3_BOX_TAG_CREATE('s', 'o', 'u', 'n'),
  HINT_TRACK            = AV3_BOX_TAG_CREATE('h', 'i', 'n', 't'),
  TIMED_METADATA_TRACK  = AV3_BOX_TAG_CREATE('m', 'e', 't', 'a'),
  AUXILIARY_VIDEO_TRACK = AV3_BOX_TAG_CREATE('a', 'u', 'x', 'v'),
};

#define AV3_TRACK_ENABLED_FLAG 0x000001

struct AV3SubMediaInfo
{
  private :
    uint64_t m_duration    = 0;
    uint32_t m_first_frame = 0;
    uint32_t m_last_frame  = 0;
  public :
    AV3SubMediaInfo();
    bool set_duration(uint64_t duration);
    bool set_first_frame(uint32_t first_frame);
    bool set_last_frame(uint32_t last_frame);
    uint64_t get_duration();
    uint32_t get_first_frame();
    uint32_t get_last_frame();
};

struct AV3MediaInfo
{
  private :
    uint64_t m_video_duration     = 0;
    uint64_t m_audio_duration     = 0;
    uint64_t m_gps_duration       = 0;
    uint32_t m_video_first_frame  = 0;//count from 1
    uint32_t m_video_last_frame   = 0;
    uint32_t m_audio_first_frame  = 0;//count from 1
    uint32_t m_audio_last_frame   = 0;
    uint32_t m_gps_first_frame    = 0;//count from 1
    uint32_t m_gps_last_frame     = 0;
  public :
    AV3MediaInfo();
    ~AV3MediaInfo(){}
    bool set_video_duration(uint64_t video_duration);
    bool set_audio_duration(uint64_t audio_duration);
    bool set_gps_duration(uint64_t gps_duration);
    bool set_video_first_frame(uint32_t video_first_frame);
    bool set_video_last_frame(uint32_t video_last_frame);
    bool set_audio_first_frame(uint32_t audio_first_frame);
    bool set_audio_last_frame(uint32_t audio_last_frame);
    bool set_gps_first_frame(uint32_t gps_first_frame);
    bool set_gps_last_frame(uint32_t gps_last_frame);
    uint64_t get_video_duration();
    uint64_t get_audio_duration();
    uint64_t get_gps_duration();
    uint32_t get_video_first_frame();
    uint32_t get_video_last_frame();
    uint32_t get_audio_first_frame();
    uint32_t get_audio_last_frame();
    uint32_t get_gps_first_frame();
    uint32_t get_gps_last_frame();
    bool get_video_sub_info(AV3SubMediaInfo& video_sub_info);
    bool get_audio_sub_info(AV3SubMediaInfo& audio_sub_info);
    bool get_gps_sub_info(AV3SubMediaInfo& gps_sub_info);
};

#define AV3_TAG_SIZE 4
#define AV3_BASE_BOX_SIZE 8
#define AV3_FULL_BOX_SIZE 12
struct AV3BaseBox {
    uint32_t       m_size    = 0;
    AV3_BOX_TAG    m_type    = AV3_BOX_TAG_NULL;
    bool           m_enable  = false;
  public:
    AV3BaseBox();
    ~AV3BaseBox(){}
    bool write_base_box(AV3FileWriter *file_writer, bool copy_to_mem = false);
    bool read_base_box(AV3FileReader *file_reader);
    void copy_base_box(AV3BaseBox& base_box);
};

struct AV3FullBox {
    uint32_t     m_flags     = 0;//24bits
    uint8_t      m_version   = 0;
    AV3BaseBox   m_base_box;
  public:
    AV3FullBox();
    ~AV3FullBox(){}
    bool write_full_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_full_box(AV3FileReader *file_reader);
    void copy_full_box(AV3FullBox& box);
};

#define AV3_MAX_COMPATIBLE_BRANDS 2
struct AV3FileTypeBox {
    uint32_t       m_compatible_brands_number = 0;
    uint32_t       m_minor_version            = 0;
    AV3_BRAND_TAG  m_major_brand              = AV3_BRAND_NULL;
    AV3_BRAND_TAG  m_compatible_brands[AV3_MAX_COMPATIBLE_BRANDS] = {AV3_BRAND_NULL};
    AV3BaseBox     m_base_box;
  public:
    AV3FileTypeBox();
    ~AV3FileTypeBox(){}
    bool write_file_type_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_file_type_box(AV3FileReader *file_reader);
    void copy_file_type_box(AV3FileTypeBox& box);
};

struct AV3MediaDataBox {
    AV3BaseBox m_base_box;
    uint32_t   m_flags         = 0;
    char       m_hash[512]     = {0};
    uint32_t   m_padding_count = 0;
  public:
    AV3MediaDataBox(){}
    ~AV3MediaDataBox(){}
    bool write_media_data_box(AV3FileWriter *file_writer, bool copy_to_mem = false);
    bool read_media_data_box(AV3FileReader *file_reader);
};

#define AV3_MOVIE_HEADER_SIZE 108
struct AV3MovieHeaderBox {
    //version specific
    uint32_t     m_creation_time     = 0;// 32 bits in version 0
    uint32_t     m_modification_time = 0;// 32 bits in version 0
    uint32_t     m_duration          = 0; // 32 bits in version 0
    uint32_t     m_timescale         = 0;
    uint32_t     m_rate              = 0;
    uint32_t     m_reserved_1        = 0;
    uint32_t     m_reserved_2        = 0;
    uint32_t     m_matrix[9]         = {0};
    uint32_t     m_pre_defined[6]    = {0};
    uint32_t     m_next_track_ID     = 0;
    uint16_t     m_volume            = 0;
    uint16_t     m_reserved          = 0;
    AV3FullBox   m_full_box;
  public:
    AV3MovieHeaderBox();
    ~AV3MovieHeaderBox(){}
    bool write_movie_header_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_movie_header_box(AV3FileReader *file_reader);
    bool get_proper_movie_header_box(AV3MovieHeaderBox& box,
                                     AV3MediaInfo& media_info);
    bool get_combined_movie_header_box(AV3MovieHeaderBox& source_box,
                                       AV3MovieHeaderBox& combined_box);
    bool copy_movie_header_box(AV3MovieHeaderBox& box);
};

#define AV3_TRACK_HEADER_SIZE 92
struct AV3TrackHeaderBox {
    //version specific
    uint32_t    m_creation_time     = 0;// 32 bits in version 0
    uint32_t    m_modification_time = 0;// 32 bits in version 0
    uint32_t    m_duration          = 0; // 32 bits in version 0
    uint32_t    m_track_ID          = 0;
    uint32_t    m_reserved          = 0;
    uint32_t    m_reserved_1[2]     = {0};
    uint32_t    m_matrix[9]         = {0};
    uint16_t    m_width_integer     = 0;//16.16 fixed point
    uint16_t    m_width_decimal     = 0;
    uint16_t    m_height_integer    = 0;//16.16 fixed point
    uint16_t    m_height_decimal    = 0;
    uint16_t    m_layer             = 0;
    uint16_t    m_alternate_group   = 0;
    uint16_t    m_volume            = 0;
    uint16_t    m_reserved_2        = 0;
    AV3FullBox  m_full_box;
  public:
    AV3TrackHeaderBox();
    ~AV3TrackHeaderBox(){}
    bool write_movie_track_header_box(AV3FileWriter* file_writer,
                                      bool copy_to_mem = false);
    bool read_movie_track_header_box(AV3FileReader *file_reader);
    bool get_proper_track_header_box(AV3TrackHeaderBox& track_header_box,
                                     AV3SubMediaInfo& sub_media_info);
    bool get_combined_track_header_box(AV3TrackHeaderBox& source_box,
                                       AV3TrackHeaderBox& combined_box);
    bool copy_track_header_box(AV3TrackHeaderBox& track_header_box);
};

#define AV3_MEDIA_HEADER_SIZE 32
struct AV3MediaHeaderBox {//contained in media box
    uint32_t    m_creation_time     = 0;
    uint32_t    m_modification_time = 0;
    uint32_t    m_duration          = 0;
    uint32_t    m_timescale         = 0;
    uint16_t    m_pre_defined       = 0;
    uint16_t    m_language          = 0;
    AV3FullBox  m_full_box;
  public:
    AV3MediaHeaderBox();
    ~AV3MediaHeaderBox(){}
    bool write_media_header_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_media_header_box(AV3FileReader *file_reader);
    bool get_proper_media_header_box(AV3MediaHeaderBox& media_header_box,
                                     AV3SubMediaInfo& sub_media_info);
    bool get_combined_media_header_box(AV3MediaHeaderBox& source_box,
                                       AV3MediaHeaderBox& combined_box);
    bool copy_media_header_box(AV3MediaHeaderBox& media_header_box);
};

#define AV3_HANDLER_REF_SIZE 32 //except name length
struct AV3HandlerReferenceBox {
    char             *m_name          = nullptr;
    AV3_HANDLER_TYPE  m_handler_type  = NULL_TRACK;
    uint32_t          m_pre_defined   = 0;
    uint32_t          m_reserved[3]   = {0};
    //do not write into file. Just record the length of m_name
    uint32_t          m_name_size     = 0;
    AV3FullBox        m_full_box;
  public:
    AV3HandlerReferenceBox();
    ~AV3HandlerReferenceBox();
    bool write_handler_reference_box(AV3FileWriter* file_writer,
                                     bool copy_to_mem = false);
    bool read_handler_reference_box(AV3FileReader *file_reader);
    bool get_proper_handler_reference_box(
        AV3HandlerReferenceBox& handler_reference_box, AV3SubMediaInfo& sub_media_info);
    bool get_combined_handler_reference_box(AV3HandlerReferenceBox& source_box,
                                            AV3HandlerReferenceBox& combined_box);
    bool copy_handler_reference_box(AV3HandlerReferenceBox& handler_reference_box);
};

#define AV3_VIDEO_MEDIA_INFO_HEADER_SIZE 20
struct AV3VideoMediaInfoHeaderBox {//contained in media information box
    uint16_t    m_graphicsmode = 0;
    uint16_t    m_opcolor[3]   = {0};
    AV3FullBox  m_full_box;
  public:
    AV3VideoMediaInfoHeaderBox();
    ~AV3VideoMediaInfoHeaderBox(){}
    bool write_video_media_info_header_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem = false);
    bool read_video_media_info_header_box(AV3FileReader *file_reader);
    bool get_proper_video_media_info_header_box(
        AV3VideoMediaInfoHeaderBox& box, AV3SubMediaInfo& video_media_info);
    bool get_combined_video_media_info_header_box(
        AV3VideoMediaInfoHeaderBox& source_box,
        AV3VideoMediaInfoHeaderBox& combined_box);
    bool copy_video_media_info_header_box(AV3VideoMediaInfoHeaderBox& box);
};

#define AV3_SOUND_MEDIA_HEADER_SIZE 16
struct AV3SoundMediaInfoHeaderBox {
    uint16_t    m_balanse  = 0;
    uint16_t    m_reserved = 0;
    AV3FullBox  m_full_box;
  public:
    AV3SoundMediaInfoHeaderBox();
    ~AV3SoundMediaInfoHeaderBox(){}
    bool write_sound_media_info_header_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem = false);
    bool read_sound_media_info_header_box(AV3FileReader *file_reader);
    bool get_proper_sound_media_info_header_box(
        AV3SoundMediaInfoHeaderBox& box, AV3SubMediaInfo& audio_media_info);
    bool get_combined_sound_media_info_header_box(
        AV3SoundMediaInfoHeaderBox& source_box,
        AV3SoundMediaInfoHeaderBox& combined_box);
    bool copy_sound_media_info_header_box(
        AV3SoundMediaInfoHeaderBox& box);
};

#define AV3_DATA_ENTRY_BOX_SIZE 12
struct AV3DataEntryBox
{
    AV3FullBox m_full_box;
  public :
    AV3DataEntryBox(){};
    ~AV3DataEntryBox(){};
    bool write_data_entry_box(AV3FileWriter* file_writer,
                              bool copy_to_mem = false);
    bool read_data_entry_box(AV3FileReader *file_reader);
    bool get_proper_data_entry_box(AV3DataEntryBox& box,
                                   AV3SubMediaInfo& media_info);
    bool get_combined_data_entry_box(AV3DataEntryBox& source_box,
                                     AV3DataEntryBox& combined_box);
    bool copy_data_entry_box(AV3DataEntryBox& box);
};

struct AV3DataReferenceBox {
    uint32_t        m_entry_count  = 0;
    AV3FullBox      m_full_box;
    AV3DataEntryBox m_url;
  public:
    AV3DataReferenceBox();
    ~AV3DataReferenceBox(){}
    bool write_data_reference_box(AV3FileWriter* file_writer,
                                  bool copy_to_mem = false);
    bool read_data_reference_box(AV3FileReader *file_reader);
    bool get_proper_data_reference_box(AV3DataReferenceBox& box,
                                       AV3SubMediaInfo& media_info);
    bool get_combined_data_reference_box(AV3DataReferenceBox& source_box,
                                         AV3DataReferenceBox& combined_box);
    bool copy_data_reference_box(AV3DataReferenceBox& box);
};

struct AV3DataInformationBox {
    AV3BaseBox           m_base_box;
    AV3DataReferenceBox  m_data_ref;
  public:
    AV3DataInformationBox(){}
    ~AV3DataInformationBox(){}
    bool write_data_info_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_data_info_box(AV3FileReader *file_reader);
    bool get_proper_data_info_box(
        AV3DataInformationBox& box, AV3SubMediaInfo& media_info);
    bool get_combined_data_info_box(AV3DataInformationBox& source_box,
                                    AV3DataInformationBox& combined_box);
    bool copy_data_info_box(AV3DataInformationBox& box);
};

#define AV3_TIME_ENTRY_SIZE 8
struct AV3TimeEntry {
    uint32_t sample_count  = 0;
    int32_t  sample_delta  = 0;
};

struct AV3DecodingTimeToSampleBox { // stts
    AV3TimeEntry    *m_entry_ptr        = nullptr;
    uint32_t         m_entry_buf_count  = 0;
    uint32_t         m_entry_count      = 0;
    AV3FullBox       m_full_box;
  public:
    AV3DecodingTimeToSampleBox();
    ~AV3DecodingTimeToSampleBox();
    bool write_decoding_time_to_sample_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem = false);
    bool read_decoding_time_to_sample_box(AV3FileReader *file_reader);
    bool optimize_time_entry();
    void update_decoding_time_to_sample_box_size();
    bool get_proper_decoding_time_to_sample_box(
        AV3DecodingTimeToSampleBox& box,
        AV3SubMediaInfo& sub_media_info);
    bool get_combined_decoding_time_to_sample_box(
        AV3DecodingTimeToSampleBox& source_box,
        AV3DecodingTimeToSampleBox& combined_box);
    bool copy_decoding_time_to_sample_box(
        AV3DecodingTimeToSampleBox& box);
};

struct AV3CompositionTimeToSampleBox {
    AV3TimeEntry    *m_entry_ptr       = nullptr;
    uint32_t         m_entry_count     = 0;
    uint32_t         m_entry_buf_count = 0;
    AV3FullBox       m_full_box;
  public:
    AV3CompositionTimeToSampleBox();
    ~AV3CompositionTimeToSampleBox();
    bool write_composition_time_to_sample_box(AV3FileWriter* file_writer,
                                              bool copy_to_mem = false);
    bool read_composition_time_to_sample_box(AV3FileReader *file_reader);
    void update_composition_time_to_sample_box_size();
    bool get_proper_composition_time_to_sample_box(
        AV3CompositionTimeToSampleBox& box, AV3SubMediaInfo& sub_media_info);
    bool get_combined_composition_time_to_sample_box(
        AV3CompositionTimeToSampleBox& source_box,
        AV3CompositionTimeToSampleBox& combined_box);
    bool copy_composition_time_to_sample_box(
        AV3CompositionTimeToSampleBox& box);
};

#define AV3_AVC_DECODER_CONFIG_RECORD_SIZE 19
struct AV3AVCDecoderConfigurationRecord {   //avcC
    uint8_t        *m_pps                   = nullptr;
    uint8_t        *m_sps                   = nullptr;
    uint16_t        m_pps_len               = 0;//pps length
    uint16_t        m_sps_len               = 0;//sps length
    uint8_t         m_config_version        = 0;
    uint8_t         m_profile_indication    = 0;
    uint8_t         m_profile_compatibility = 0;
    uint8_t         m_level_indication      = 0;
    uint8_t         m_len_size_minus_one    = 0; // 2bits
    uint8_t         m_sps_num               = 0; // 5bits
    uint8_t         m_pps_num               = 0;
    uint8_t         m_padding_count         = 0;
    AV3BaseBox      m_base_box;
  public:
    AV3AVCDecoderConfigurationRecord();
    ~AV3AVCDecoderConfigurationRecord();
    bool write_avc_decoder_configuration_record_box(
        AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_avc_decoder_configuration_record_box(
        AV3FileReader* file_reader);
    bool get_proper_avc_decoder_configuration_record_box(
        AV3AVCDecoderConfigurationRecord& box, AV3SubMediaInfo& sub_media_info);
    bool get_combined_avc_decoder_configuration_record_box(
        AV3AVCDecoderConfigurationRecord& source_box,
        AV3AVCDecoderConfigurationRecord& combined_box);
    bool copy_avc_decoder_configuration_record_box(
        AV3AVCDecoderConfigurationRecord& box);
};

#define AV3_VISUAL_SAMPLE_ENTRY_SIZE 86
struct AV3VisualSampleEntry {  //avc1
    uint32_t m_horizresolution        = 0;
    uint32_t m_vertresolution         = 0;
    uint32_t m_reserved_1             = 0;
    uint32_t m_pre_defined_1[3]       = {0}; // = 0
    uint16_t m_data_reference_index   = 0;
    uint16_t m_pre_defined            = 0; // = 0
    uint16_t m_reserved               = 0; // = 0
    uint16_t m_width                  = 0;
    uint16_t m_height                 = 0;
    uint16_t m_frame_count            = 0;
    uint16_t m_depth                  = 0; // = 0x0018
    int16_t  m_pre_defined_2          = 0; // = -1
    uint8_t  m_reserved_0[6]          = {0};
    char     m_compressorname[32]     = {0};
    AV3BaseBox                         m_base_box;
    AV3AVCDecoderConfigurationRecord   m_avc_config;// for avc
  public:
    AV3VisualSampleEntry();
    ~AV3VisualSampleEntry(){}
    bool write_visual_sample_entry(AV3FileWriter* file_writer,
                                   bool copy_to_mem = false);
    bool read_visual_sample_entry(AV3FileReader *file_reader);
    bool get_proper_visual_sample_entry(
        AV3VisualSampleEntry& entry, AV3SubMediaInfo& sub_media_info);
    bool get_combined_visual_sample_entry(
        AV3VisualSampleEntry& source_entry, AV3VisualSampleEntry& combined_entry);
    void update_visual_sample_entry_size();
    bool copy_visual_sample_entry(AV3VisualSampleEntry& entry);
};

#define AV3_AAC_DESCRIPTOR_SIZE 40
struct AV3AACDescriptorBox {    //esds
    uint32_t    m_buffer_size_DB                               = 0; // 24bits
    uint32_t    m_max_bitrate                                  = 0;
    uint32_t    m_avg_bitrate                                  = 0;
    uint16_t    m_es_id                                        = 0;
    uint8_t     m_es_descriptor_tag                            = 0;
    uint8_t     m_es_descriptor_tag_size                       = 0; //7 bits

    uint8_t     m_stream_dependency_flag                       = 0; //1 bit
    uint8_t     m_URL_flag                                     = 0; //1 bit
    uint8_t     m_OCR_stream_flag                              = 0; //1 bit
    uint8_t     m_stream_priority                              = 0; //5 bits

    uint8_t     m_decoder_config_descriptor_tag                = 0;
    uint8_t     m_decoder_config_descriptor_tag_size           = 0;
    uint8_t     m_object_type_id                               = 0;

    uint8_t     m_stream_type                                  = 0; //6 bits
    uint8_t     m_up_stream                                    = 0; //1 bit
    uint8_t     m_reserved                                     = 0; //1 bit

    uint8_t     m_decoder_specific_info_tag                    = 0;
    uint8_t     m_decoder_specific_info_tag_size               = 0;
    uint8_t     m_audio_object_type                            = 0; //5 bits
    uint8_t     m_sampling_frequency_index                     = 0; //4 bits
    uint8_t     m_channel_configuration                        = 0; //4 bits
    uint8_t     m_frame_length_flag                            = 0; //1 bit
    uint8_t     m_depends_on_core_coder                        = 0; //1 bit
    uint8_t     m_extension_flag                               = 0; //1 bit
    uint8_t     m_SL_config_descriptor_tag                     = 0;
    uint8_t     m_SL_config_descriptor_tag_size                = 0;
    uint8_t     m_predefined                                   = 0;
    uint8_t     m_padding                                      = 0;
    AV3FullBox  m_full_box;
  public:
    AV3AACDescriptorBox();
    ~AV3AACDescriptorBox(){}
    bool write_aac_descriptor_box(
        AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_aac_descriptor_box(
        AV3FileReader* file_reader);
    bool get_proper_aac_descriptor_box(
        AV3AACDescriptorBox& box, AV3SubMediaInfo& sub_media_info);
    bool get_combined_aac_descriptor_box(
        AV3AACDescriptorBox& source_box,
        AV3AACDescriptorBox& combined_box);
    bool copy_aac_descriptor_box(AV3AACDescriptorBox& box);
};

#define AV3_AUDIO_SAMPLE_ENTRY_SIZE 36
struct AV3AudioSampleEntry {  //mp4a
    uint32_t m_time_scale           = 0;
    uint16_t m_data_reference_index = 0;
    uint16_t m_sound_version        = 0;
    uint16_t m_channels             = 0;
    uint16_t m_sample_size          = 0;
    uint16_t m_packet_size          = 0;
    uint16_t m_reserved_1           = 0;
    uint8_t  m_reserved_0[6]        = {0};
    AV3BaseBox           m_base_box;
    AV3AACDescriptorBox  m_aac;
  public:
    AV3AudioSampleEntry();
    ~AV3AudioSampleEntry(){}
    bool write_audio_sample_entry(AV3FileWriter* file_writer,
                                  bool copy_to_mem = false);
    bool read_audio_sample_entry(AV3FileReader *file_reader);
    bool get_proper_audio_sample_entry(AV3AudioSampleEntry& entry,
                                       AV3SubMediaInfo& media_info);
    bool get_combined_audio_sample_entry(AV3AudioSampleEntry& source_entry,
                                         AV3AudioSampleEntry& combined_entry);
    void update_audio_sample_entry_size();
    bool copy_audio_sample_entry(AV3AudioSampleEntry& entry);
};

struct AV3SampleDescriptionBox {  //stsd
    uint32_t                   m_entry_count = 0;
    AV3FullBox                 m_full_box;
    AV3VisualSampleEntry       m_visual_entry;
    AV3AudioSampleEntry        m_audio_entry;
  public:
    AV3SampleDescriptionBox();
    ~AV3SampleDescriptionBox(){}
    bool write_sample_description_box(AV3FileWriter* file_writer,
                                      bool copy_to_mem = false);
    bool read_sample_description_box(AV3FileReader* file_reader);
    bool get_proper_sample_description_box(
        AV3SampleDescriptionBox& box, AV3SubMediaInfo& media_info);
    bool get_combined_sample_description_box(
        AV3SampleDescriptionBox& source_box,
        AV3SampleDescriptionBox& combined_box);
    void update_sample_description_box();
    bool copy_sample_description_box(
        AV3SampleDescriptionBox& box);
};

#define AV3_SAMPLE_SIZE_BOX_SIZE 20
/*if m_sample_size 0,then the samples have different sizes, and those sizes
 * are stored in the sample size table. If this field is not 0, it specifies
 * the constant sample size, and no array follows.
 */
struct AV3SampleSizeBox { //stsz
    uint32_t    *m_size_array    = nullptr;
    uint32_t     m_sample_size   = 0;
    uint32_t     m_sample_count  = 0;
    uint32_t     m_buf_count     = 0;
    AV3FullBox   m_full_box;
  public:
    AV3SampleSizeBox();
    ~AV3SampleSizeBox();
    bool write_sample_size_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_sample_size_box(AV3FileReader *file_reader);
    void update_sample_size_box_size();
    bool get_proper_sample_size_box(AV3SampleSizeBox& sample_size_box,
                                    AV3SubMediaInfo& media_info);
    bool get_combined_sample_size_box(AV3SampleSizeBox& source_box,
                                      AV3SampleSizeBox& combined_box);
    bool copy_sample_size_box(AV3SampleSizeBox& sample_size_box);
};

#define AV3_SAMPLE_TO_CHUNK_ENTRY_SIZE 12
struct AV3SampleToChunkEntry {
    uint32_t m_first_chunk              = 0;
    uint32_t m_sample_per_chunk         = 0;
    uint32_t m_sample_description_index = 0;
  public:
    AV3SampleToChunkEntry();
    ~AV3SampleToChunkEntry(){}
    bool write_sample_chunk_entry(AV3FileWriter* file_writer,
                                  bool copy_to_mem = false);
    bool read_sample_chunk_entry(AV3FileReader *file_reader);
};

struct AV3SampleToChunkBox { //stsc
    AV3SampleToChunkEntry  *m_entrys          = nullptr;
    uint32_t                m_entry_count     = 0;
    uint32_t                m_entry_buf_count = 0;
    AV3FullBox              m_full_box;
  public:
    AV3SampleToChunkBox();
    ~AV3SampleToChunkBox();
    bool write_sample_to_chunk_box(AV3FileWriter* file_writer,
                                   bool copy_to_mem = false);
    bool read_sample_to_chunk_box(AV3FileReader *file_reader);
    bool optimize_sample_to_chunk_entry();
    void update_sample_to_chunk_box_size();
    bool get_proper_sample_to_chunk_box(AV3SampleToChunkBox& box,
                                        AV3SubMediaInfo& media_info);
    bool get_combined_sample_to_chunk_box(AV3SampleToChunkBox& source_box,
                                          AV3SampleToChunkBox& combined_box);
    bool copy_sample_to_chunk_box(AV3SampleToChunkBox& box);
};

struct AV3ChunkOffsetBox { //stco
    uint32_t       *m_chunk_offset = nullptr;
    uint32_t        m_entry_count  = 0;
    uint32_t        m_buf_count    = 0;
    AV3FullBox      m_full_box;
  public:
    AV3ChunkOffsetBox();
    ~AV3ChunkOffsetBox();
    bool write_chunk_offset_box(AV3FileWriter* file_writer,
                                bool copy_to_mem = false);
    bool read_chunk_offset_box(AV3FileReader *file_reader);
    void update_chunk_offset_box_size();
    bool get_proper_chunk_offset_box(AV3ChunkOffsetBox& box,
                                     AV3SubMediaInfo& media_info);
    bool get_combined_chunk_offset_box(AV3ChunkOffsetBox& source_box,
                                       AV3ChunkOffsetBox& combined_box);
    bool copy_chunk_offset_box(AV3ChunkOffsetBox& box);
};

struct AV3SyncSampleTableBox {   //stss
    uint32_t       *m_sync_sample_table = nullptr;
    uint32_t        m_stss_count        = 0;
    uint32_t        m_buf_count         = 0;
    AV3FullBox      m_full_box;
  public:
    AV3SyncSampleTableBox();
    ~AV3SyncSampleTableBox();
    bool write_sync_sample_table_box(AV3FileWriter* file_writer,
                                     bool copy_to_mem = false);
    bool read_sync_sample_table_box(AV3FileReader *file_reader);
    void update_sync_sample_box_size();
    bool get_proper_sync_sample_table_box(
        AV3SyncSampleTableBox& sync_sample_table_box, AV3SubMediaInfo& media_info);
    bool get_combined_sync_sample_table_box(
        AV3SyncSampleTableBox& source_box,
        AV3SyncSampleTableBox& combined_box);
    bool copy_sync_sample_table_box(
        AV3SyncSampleTableBox& sync_sample_table_box);
};

struct AV3SampleTableBox {
    AV3BaseBox                     m_base_box;
    AV3DecodingTimeToSampleBox     m_stts;
    AV3CompositionTimeToSampleBox  m_ctts;
    AV3SampleDescriptionBox        m_sample_description;
    AV3SampleSizeBox               m_sample_size;
    AV3SampleToChunkBox            m_sample_to_chunk;
    AV3ChunkOffsetBox              m_chunk_offset;
    AV3SyncSampleTableBox          m_sync_sample;
  public:
    AV3SampleTableBox(){}
    ~AV3SampleTableBox(){}
    bool write_sample_table_box(AV3FileWriter* file_writer,
                                bool copy_to_mem = false);
    bool read_sample_table_box(AV3FileReader *file_reader);
    void update_sample_table_box_size();
    bool get_proper_sample_table_box(
        AV3SampleTableBox& box, AV3SubMediaInfo& media_info);
    bool get_combined_sample_table_box(AV3SampleTableBox& source_box,
                                       AV3SampleTableBox& combined_box);
    bool copy_sample_table_box(AV3SampleTableBox& box);
};

struct AV3MediaInformationBox {//minf
    AV3BaseBox                   m_base_box;
    AV3VideoMediaInfoHeaderBox   m_video_info_header;
    AV3SoundMediaInfoHeaderBox   m_sound_info_header;
    AV3DataInformationBox        m_data_info;
    AV3SampleTableBox            m_sample_table;
  public:
    AV3MediaInformationBox(){}
    ~AV3MediaInformationBox(){}
    bool write_media_info_box(AV3FileWriter* file_writer,
                              bool copy_to_mem = false);
    bool read_media_info_box(AV3FileReader *file_reader);
    void update_media_info_box_size();
    bool get_proper_media_info_box(
        AV3MediaInformationBox& box, AV3SubMediaInfo& media_info);
    bool get_combined_media_info_box(AV3MediaInformationBox& source_box,
                                     AV3MediaInformationBox& combined_box);
    bool copy_media_info_box(AV3MediaInformationBox& box);
};

struct AV3MediaBox { //mdia
    AV3BaseBox             m_base_box;
    AV3MediaHeaderBox      m_media_header;
    AV3HandlerReferenceBox m_media_handler;
    AV3MediaInformationBox m_media_info;
  public:
    AV3MediaBox(){}
    ~AV3MediaBox(){}
    bool write_media_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_media_box(AV3FileReader *file_reader);
    void update_media_box_size();
    bool get_proper_media_box(AV3MediaBox& media_box,
                              AV3SubMediaInfo& media_info);
    bool get_combined_media_box(AV3MediaBox& source_box,
                                AV3MediaBox& combined_box);
    bool copy_media_box(AV3MediaBox& media_box);
};

struct AV3MetaDataEntry
{
    uint32_t m_sample_number = 0;
    int32_t  m_sec           = 0;
    int32_t  m_time_stamp    = 0;
    int16_t  m_msec          = 0;
    uint16_t m_reserved      = 0;
  public :
    bool write_meta_data_entry(AV3FileWriter* file_writer,
                               bool copy_to_mem = false);
    bool read_meta_data_entry(AV3FileReader *file_reader);
};

struct AV3PSMHBox
{
    uint32_t          m_reserved[2]   = {0};
    //m_meta_length = sizeof(m_entry_count) + m_entry_count * sizeof(AV3MetaDataEntry)
    uint32_t          m_meta_length   = 0;
    uint32_t          m_entry_count   = 0;
    uint16_t          m_meta_id       = 0;
    uint32_t          m_entry_buf_cnt = 0;
    AV3MetaDataEntry *m_entry         = nullptr;
    AV3BaseBox        m_base_box;
  public :
    AV3PSMHBox() {}
    ~AV3PSMHBox();
    bool write_psmh_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_psmh_box(AV3FileReader *file_reader);
    void update_psmh_box_size();
    bool get_proper_psmh_box(AV3PSMHBox& psmh_box, AV3SubMediaInfo& media_info);
    bool get_combined_psmh_box(AV3PSMHBox& source_box, AV3PSMHBox& combined_box);
    bool copy_psmh_box(AV3PSMHBox& psmh_box);
};

struct AV3PSMTBox
{
    AV3BaseBox         m_base_box;
    AV3PSMHBox         m_psmh_box;
    AV3SampleTableBox  m_meta_sample_table;
    AV3PSMTBox() {}
    ~AV3PSMTBox() {}
    bool write_psmt_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_psmt_box(AV3FileReader *file_reader);
    void update_psmt_box_size();
    bool get_proper_psmt_box(AV3PSMTBox& psmt_box, AV3SubMediaInfo& media_info);
    bool get_combined_psmt_box(AV3PSMTBox& source_box, AV3PSMTBox& combined_box);
    bool copy_psmt_box(AV3PSMTBox& psmt_box);
};

struct AV3TrackUUIDBox {
    uint8_t     m_uuid[16]      = {0};
    uint8_t     m_padding_count = 0;
    AV3BaseBox  m_base_box;
    AV3PSMTBox  m_psmt_box;
  public :
    AV3TrackUUIDBox() {}
    ~AV3TrackUUIDBox() {}
    bool write_track_uuid_box(AV3FileWriter* file_writer,
                              bool copy_to_mem = false);
    bool read_track_uuid_box(AV3FileReader *file_reader);
    void update_track_uuid_box_size();
    bool get_proper_track_uuid_box(AV3TrackUUIDBox& uuid_box,
                                   AV3SubMediaInfo& media_info);
    bool get_combined_track_uuid_box(AV3TrackUUIDBox& source_box,
                                     AV3TrackUUIDBox& combined_box);
    bool copy_track_uuid_box(AV3TrackUUIDBox& box);
};

struct AV3TrackBox {
    AV3BaseBox        m_base_box;
    AV3TrackHeaderBox m_track_header;
    AV3MediaBox       m_media;
    AV3TrackUUIDBox   m_uuid;
  public:
    AV3TrackBox(){}
    ~AV3TrackBox(){}
    bool write_track_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_track_box(AV3FileReader *file_reader);
    void update_track_box_size();
    bool get_proper_track_box(AV3TrackBox& track_box, AV3SubMediaInfo& media_info);
    bool get_combined_track_box(AV3TrackBox& source_box, AV3TrackBox& combined_box);
    bool copy_track_box(AV3TrackBox& track_box);
};

struct AV3MovieBox {
    AV3BaseBox        m_base_box;
    AV3MovieHeaderBox m_movie_header_box;
    AV3TrackBox       m_video_track;
    AV3TrackBox       m_audio_track;
  public:
    AV3MovieBox(){}
    ~AV3MovieBox(){};
    bool write_movie_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_movie_box(AV3FileReader *file_reader);
    void update_movie_box_size();
    bool get_proper_movie_box(AV3MovieBox& movie_box, AV3MediaInfo& media_info);
    bool get_combined_movie_box(AV3MovieBox& source_box, AV3MovieBox& combined_box);
    bool copy_movie_box(AV3MovieBox& movie_box);
  private :
    bool read_track_box(AV3FileReader *file_reader);
};

struct AV3PSFMBox
{
    AV3BaseBox  m_base_box;
    uint32_t    m_reserved = 0;
    AV3PSFMBox() {}
    ~AV3PSFMBox() {}
    bool write_psfm_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_psfm_box(AV3FileReader *file_reader);
    bool get_proper_psfm_box(AV3PSFMBox& psfm_box, AV3MediaInfo& media_info);
    bool get_combined_psfm_box(AV3PSFMBox& source_box, AV3PSFMBox& combined_box);
    bool copy_psfm_box(AV3PSFMBox& box);
};

struct AV3XMLBox
{
    AV3FullBox  m_full_box;
    uint32_t    m_data_length = 0;//not used currently
    AV3XMLBox() {}
    ~AV3XMLBox() {}
    bool write_xml_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_xml_box(AV3FileReader *file_reader);
    void update_xml_box_size();
    bool get_proper_xml_box(AV3XMLBox& xml_box, AV3MediaInfo& media_info);
    bool get_combined_xml_box(AV3XMLBox& source_box, AV3XMLBox& combined_box);
    bool copy_xml_box(AV3XMLBox& box);
};

struct AV3UUIDBox
{
    uint32_t    m_ip_address      = 0;
    uint32_t    m_num_event_log   = 0;
    uint8_t     m_uuid[16]        = {0};
    uint8_t     m_product_num[16] = {0};
    AV3BaseBox  m_base_box;
    AV3PSFMBox  m_psfm_box;
    AV3XMLBox   m_xml_box;
    AV3UUIDBox() {}
    ~AV3UUIDBox() {}
    bool write_uuid_box(AV3FileWriter* file_writer, bool copy_to_mem = false);
    bool read_uuid_box(AV3FileReader *file_reader);
    void update_uuid_box_size();
    bool get_proper_uuid_box(AV3UUIDBox& uuid_box, AV3MediaInfo& media_info);
    bool get_combined_uuid_box(AV3UUIDBox& source_box, AV3UUIDBox& combined_box);
    bool copy_uuid_box(AV3UUIDBox& box);
};

#endif

