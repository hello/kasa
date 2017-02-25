/*******************************************************************************
 * iso_base_defs.h
 *
 * History:
 *   2016-01-04 - [ccjing] created file
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


#ifndef ISO_BASE_DEFS_
#define ISO_BASE_DEFS_
/*
 * simple layout of MP4 file (interleave, 32bit/64bit adaptive)
 * --ftyp (File Type Box)
 * --mdat (Media Data Box)
 * --free (Free box)(option)
 * --moov (Movie Box)
 * ----mvhd (Movie Header Box)
 * ----iods (Object Description Box)(option)
 * ----trak (Track Box)
 * ------tkhd (Track Header Box)
 * ------tref (Track Reference Container)(not add)
 * ------edts (Edit List Container)(not add)
 * --------elst (An Edit List)(not add)
 * ------mdia (Container for Media information of the track)
 * --------mdhd (Media header, overall information about the media)
 * --------hdlr (Media handler)
 * --------minf (Media information container)
 * ----------vmhd (Video Media Header)
 * ----------smhd (Sound Media Header)
 * ----------stbl (Sample Table Box)
 * ------------stsd (Sample Descriptions)
 * ------------stts (Decoding Time Stamp)
 * ------------stsc (Sample to Chunk, partial data-offset information)
 * ------------stco (Chunk offset, partial data-offset information)
 * ------------co64 (Chunk offset(64 bits), partial data-offset information)
 * ------------stss (sync sample table)//I frame location
 */

#define DISO_BOX_TAG_CREATE(X,Y,Z,N) \
         X | (Y << 8) | (Z << 16) | (N << 24)
class Mp4FileWriter;
class Mp4FileReader;

enum DISOM_BOX_TAG
{
  DISOM_BOX_TAG_NULL = 0,
  DISOM_FILE_TYPE_BOX_TAG = DISO_BOX_TAG_CREATE('f', 't', 'y', 'p'),
  DISOM_MEDIA_DATA_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'd', 'a', 't'),
  DISOM_MOVIE_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'o', 'o', 'v'),
  DISOM_MOVIE_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'v', 'h', 'd'),
  DIDOM_OBJECT_DESC_BOX_TAG = DISO_BOX_TAG_CREATE('i', 'o', 'd', 's'),
  DISOM_TRACK_BOX_TAG = DISO_BOX_TAG_CREATE('t', 'r', 'a', 'k'),
  DISOM_TRACK_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('t', 'k', 'h', 'd'),
  DISOM_MEDIA_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'd', 'i', 'a'),
  DISOM_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'd', 'h', 'd'),
  DISOM_HANDLER_REFERENCE_BOX_TAG = DISO_BOX_TAG_CREATE('h', 'd', 'l', 'r'),
  DISOM_MEDIA_INFORMATION_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'i', 'n', 'f'),
  DISOM_VIDEO_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('v', 'm', 'h', 'd'),
  DISOM_SOUND_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('s', 'm', 'h', 'd'),
  DISOM_NULL_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('n', 'm', 'h', 'd'),
  DISOM_DATA_INFORMATION_BOX_TAG = DISO_BOX_TAG_CREATE('d', 'i', 'n', 'f'),
  DISOM_DATA_REFERENCE_BOX_TAG = DISO_BOX_TAG_CREATE('d', 'r', 'e', 'f'),
  DISOM_DATA_ENTRY_URL_BOX_TAG = DISO_BOX_TAG_CREATE('u', 'r', 'l', ' '),
  DISOM_SAMPLE_TABLE_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 'b', 'l'),
  DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 't', 's'),
  DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG = DISO_BOX_TAG_CREATE('c', 't', 't', 's'),
  DISOM_SAMPLE_DESCRIPTION_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 's', 'd'),
  DISOM_SAMPLE_SIZE_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 's', 'z'),
  DISOM_COMPACT_SAMPLE_SIZE_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 'z', '2'),
  DISOM_SAMPLE_TO_CHUNK_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 's', 'c'),
  DISOM_CHUNK_OFFSET_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 'c', 'o'),
  DISOM_CHUNK_OFFSET64_BOX_TAG = DISO_BOX_TAG_CREATE('c', 'o', '6', '4'),
  DISOM_SYNC_SAMPLE_TABLE_BOX_TAG = DISO_BOX_TAG_CREATE('s', 't', 's', 's'),
  DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG = DISO_BOX_TAG_CREATE('a', 'v', 'c', 'C'),
  DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG = DISO_BOX_TAG_CREATE('h', 'v', 'c', 'C'),
  DISOM_AAC_DESCRIPTOR_BOX_TAG = DISO_BOX_TAG_CREATE('e', 's', 'd', 's'),
  DISOM_OPUS_DESCRIPTOR_BOX_TAG = DISO_BOX_TAG_CREATE('d', 'O', 'p', 's'),
  DISOM_FREE_BOX_TAG = DISO_BOX_TAG_CREATE('f', 'r', 'e', 'e'),
  DISOM_OPUS_ENTRY_TAG = DISO_BOX_TAG_CREATE('O', 'p', 'u', 's'),
  DISOM_AAC_ENTRY_TAG = DISO_BOX_TAG_CREATE('m', 'p', '4', 'a'),
  DISOM_G726_ENTRY_TAG = DISO_BOX_TAG_CREATE('g', '7', '2', '6'),
  DISOM_H264_ENTRY_TAG = DISO_BOX_TAG_CREATE('a', 'v', 'c', '1'),
  DISOM_H265_ENTRY_TAG = DISO_BOX_TAG_CREATE('h', 'v', 'c', '1'),
  DISOM_META_ENTRY_TAG = DISO_BOX_TAG_CREATE('m', 'e', 't', 't'),
};

enum DISO_BRAND_TAG{
  DISOM_BRAND_NULL = 0,
  DISOM_BRAND_V0_TAG = DISO_BOX_TAG_CREATE('i', 's', 'o', 'm'),
  DISOM_BRAND_V1_TAG = DISO_BOX_TAG_CREATE('i', 's', 'o', '2'),
  DISOM_BRAND_AVC_TAG = DISO_BOX_TAG_CREATE('a', 'v', 'c', '1'),
  DISOM_BRAND_MPEG4_TAG = DISO_BOX_TAG_CREATE('m', 'p', 'g', '4'),
};

enum DISO_HANDLER_TYPE{
  NULL_TRACK            = 0,
  VIDEO_TRACK           = DISO_BOX_TAG_CREATE('v', 'i', 'd', 'e'),
  AUDIO_TRACK           = DISO_BOX_TAG_CREATE('s', 'o', 'u', 'n'),
  HINT_TRACK            = DISO_BOX_TAG_CREATE('h', 'i', 'n', 't'),
  TIMED_METADATA_TRACK  = DISO_BOX_TAG_CREATE('m', 'e', 't', 'a'),
  AUXILIARY_VIDEO_TRACK = DISO_BOX_TAG_CREATE('a', 'u', 'x', 'v'),
};

#define DISOM_TAG_SIZE 4
#define DISOM_BASE_BOX_SIZE 8
#define DISOM_FULL_BOX_SIZE 12

#define DISOM_TRACK_ENABLED_FLAG 0x000001

struct SubMediaInfo
{
  private :
    uint64_t m_duration;
    uint32_t m_first_frame;
    uint32_t m_last_frame;
  public :
    SubMediaInfo();
    bool set_duration(uint64_t duration);
    bool set_first_frame(uint32_t first_frame);
    bool set_last_frame(uint32_t last_frame);
    uint64_t get_duration();
    uint32_t get_first_frame();
    uint32_t get_last_frame();
};

struct MediaInfo
{
  private :
    uint64_t m_video_duration;
    uint64_t m_audio_duration;
    uint64_t m_gps_duration;
    uint32_t m_video_first_frame;//count from 1
    uint32_t m_video_last_frame;
    uint32_t m_audio_first_frame;//count from 1
    uint32_t m_audio_last_frame;
    uint32_t m_gps_first_frame;//count from 1
    uint32_t m_gps_last_frame;
  public :
    MediaInfo();
    ~MediaInfo(){}
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
    bool get_video_sub_info(SubMediaInfo& video_sub_info);
    bool get_audio_sub_info(SubMediaInfo& audio_sub_info);
    bool get_gps_sub_info(SubMediaInfo& gps_sub_info);
};

struct BaseBox {
    uint32_t       m_size;
    DISOM_BOX_TAG  m_type;
    bool           m_enable;
  public:
    BaseBox();
    ~BaseBox(){}
    bool write_base_box(Mp4FileWriter *file_writer);
    bool read_base_box(Mp4FileReader *file_reader);
    void copy_base_box(BaseBox& base_box);
};

struct FullBox {
    uint32_t     m_flags; //24bits
    uint8_t      m_version;
    BaseBox      m_base_box;
  public:
    FullBox();
    ~FullBox(){}
    bool write_full_box(Mp4FileWriter* file_writer);
    bool read_full_box(Mp4FileReader *file_reader);
    void copy_full_box(FullBox& box);
};

struct FreeBox {
    BaseBox m_base_box;
    FreeBox(){}
    ~FreeBox(){}
    bool write_free_box(Mp4FileWriter* file_writer);
    bool read_free_box(Mp4FileReader *file_reader);
};
#define DISOM_MAX_COMPATIBLE_BRANDS 8
struct FileTypeBox {
    uint32_t       m_compatible_brands_number;
    uint32_t       m_minor_version;
    DISO_BRAND_TAG m_major_brand;
    DISO_BRAND_TAG m_compatible_brands[DISOM_MAX_COMPATIBLE_BRANDS];
    BaseBox        m_base_box;
  public:
    FileTypeBox();
    ~FileTypeBox(){}
    bool write_file_type_box(Mp4FileWriter* file_writer);
    bool read_file_type_box(Mp4FileReader *file_reader);
    void copy_file_type_box(FileTypeBox& box);
};

struct MediaDataBox {
    BaseBox m_base_box;
  public:
    MediaDataBox(){}
    ~MediaDataBox(){}
    bool write_media_data_box(Mp4FileWriter* file_writer);
    bool read_media_data_box(Mp4FileReader *file_reader);
};

#define DISOM_MOVIE_HEADER_SIZE 108
#define DISOM_MOVIE_HEADER_64_SIZE 120
struct MovieHeaderBox {
    //version specific
    uint64_t     m_creation_time;// 32 bits in version 0
    uint64_t     m_modification_time;// 32 bits in version 0
    uint64_t     m_duration; // 32 bits in version 0
    uint32_t     m_timescale;
    uint32_t     m_rate;
    uint32_t     m_reserved_1;
    uint32_t     m_reserved_2;
    uint32_t     m_matrix[9];
    uint32_t     m_pre_defined[6];
    uint32_t     m_next_track_ID;
    uint16_t     m_volume;
    uint16_t     m_reserved;
    FullBox      m_full_box;
  public:
    MovieHeaderBox();
    ~MovieHeaderBox(){}
    bool write_movie_header_box(Mp4FileWriter* file_writer);
    bool read_movie_header_box(Mp4FileReader *file_reader);
    bool get_proper_movie_header_box(MovieHeaderBox& box,
                                     MediaInfo& media_info);
    bool get_combined_movie_header_box(MovieHeaderBox& source_box,
                                       MovieHeaderBox& combined_box);
    bool copy_movie_header_box(MovieHeaderBox& box);
};

struct ObjectDescBox
{
    uint8_t      m_iod_type_tag;//0x10
    uint8_t      m_extended_desc_type[3];//3 bytes, 0x808080
    uint8_t      m_desc_type_length;
    uint8_t      m_od_id[2];
    uint8_t      m_od_profile_level;
    uint8_t      m_scene_profile_level;
    uint8_t      m_audio_profile_level;
    uint8_t      m_video_profile_level;
    uint8_t      m_graphics_profile_level;
    FullBox      m_full_box;
  public :
    ObjectDescBox();
    ~ObjectDescBox(){}
    bool write_object_desc_box(Mp4FileWriter* file_writer);
    bool read_object_desc_box(Mp4FileReader* file_reader);
    bool get_proper_ojbect_desc_box(ObjectDescBox& box,
                                    MediaInfo& media_info);
    bool get_combined_object_desc_box(ObjectDescBox& source_box,
                                      ObjectDescBox& combined_box);
    bool copy_object_desc_box(ObjectDescBox& box);
};

#define DISOM_TRACK_HEADER_SIZE 92
#define DISOM_TRACK_HEADER_64_SIZE 104
struct TrackHeaderBox {
    //version specific
    uint64_t m_creation_time;// 32 bits in version 0
    uint64_t m_modification_time;// 32 bits in version 0
    uint64_t m_duration; // 32 bits in version 0
    uint32_t m_track_ID;
    uint32_t m_reserved;
    uint32_t m_reserved_1[2];
    uint32_t m_matrix[9];
    uint16_t m_width_integer;//16.16 fixed point
    uint16_t m_width_decimal;
    uint16_t m_height_integer;//16.16 fixed point
    uint16_t m_height_decimal;
    uint16_t m_layer;
    uint16_t m_alternate_group;
    uint16_t m_volume;
    uint16_t m_reserved_2;
    FullBox  m_full_box;
  public:
    TrackHeaderBox();
    ~TrackHeaderBox(){}
    bool write_movie_track_header_box(Mp4FileWriter* file_writer);
    bool read_movie_track_header_box(Mp4FileReader *file_reader);
    bool get_proper_track_header_box(TrackHeaderBox& track_header_box,
                                     SubMediaInfo& sub_media_info);
    bool get_combined_track_header_box(TrackHeaderBox& source_box,
                                       TrackHeaderBox& combined_box);
    bool copy_track_header_box(TrackHeaderBox& track_header_box);
};

#define DISOM_MEDIA_HEADER_SIZE 32
#define DISOM_MEDIA_HEADER_64_SIZE 44
struct MediaHeaderBox {//contained in media box
    uint64_t m_creation_time;// 32 bits in version 0
    uint64_t m_modification_time;// 32 bits in version 0
    uint64_t m_duration; // 32 bits in version 0
    uint32_t m_timescale;
    uint16_t m_pre_defined;
    uint8_t  m_pad; // 1bit
    uint8_t  m_language[3]; //5bits
    FullBox  m_full_box;
  public:
    MediaHeaderBox();
    ~MediaHeaderBox(){}
    bool write_media_header_box(Mp4FileWriter* file_writer);
    bool read_media_header_box(Mp4FileReader *file_reader);
    bool get_proper_media_header_box(MediaHeaderBox& media_header_box,
                                     SubMediaInfo& sub_media_info);
    bool get_combined_media_header_box(MediaHeaderBox& source_box,
                                       MediaHeaderBox& combined_box);
    bool copy_media_header_box(MediaHeaderBox& media_header_box);
};

#define DISOM_HANDLER_REF_SIZE 32 //except name length
struct HandlerReferenceBox {
    char              *m_name;
    DISO_HANDLER_TYPE  m_handler_type;
    uint32_t           m_pre_defined;
    uint32_t           m_reserved[3];
    //do not write into file. Specifice the length of m_name
    uint32_t           m_name_size;
    FullBox            m_full_box;
  public:
    HandlerReferenceBox();
    ~HandlerReferenceBox();
    bool write_handler_reference_box(Mp4FileWriter* file_writer);
    bool read_handler_reference_box(Mp4FileReader *file_reader);
    bool get_proper_handler_reference_box(
        HandlerReferenceBox& handler_reference_box, SubMediaInfo& sub_media_info);
    bool get_combined_handler_reference_box(HandlerReferenceBox& source_box,
                                            HandlerReferenceBox& combined_box);
    bool copy_handler_reference_box(HandlerReferenceBox& handler_reference_box);
};

#define DISOM_VIDEO_MEDIA_INFO_HEADER_SIZE 20
struct VideoMediaInfoHeaderBox {//contained in media information box
    uint16_t  m_graphicsmode;
    uint16_t  m_opcolor[3];
    FullBox   m_full_box;
  public:
    VideoMediaInfoHeaderBox();
    ~VideoMediaInfoHeaderBox(){}
    bool write_video_media_info_header_box(Mp4FileWriter* file_writer);
    bool read_video_media_info_header_box(Mp4FileReader *file_reader);
    bool get_proper_video_media_info_header_box(
        VideoMediaInfoHeaderBox& box, SubMediaInfo& video_media_info);
    bool get_combined_video_media_info_header_box(
        VideoMediaInfoHeaderBox& source_box,
        VideoMediaInfoHeaderBox& combined_box);
    bool copy_video_media_info_header_box(VideoMediaInfoHeaderBox& box);
};

#define DISOM_SOUND_MEDIA_HEADER_SIZE 16
struct SoundMediaInfoHeaderBox {
    uint16_t m_balanse;
    uint16_t m_reserved;
    FullBox  m_full_box;
  public:
    SoundMediaInfoHeaderBox();
    ~SoundMediaInfoHeaderBox(){}
    bool write_sound_media_info_header_box(Mp4FileWriter* file_writer);
    bool read_sound_media_info_header_box(Mp4FileReader *file_reader);
    bool get_proper_sound_media_info_header_box(
        SoundMediaInfoHeaderBox& box, SubMediaInfo& audio_media_info);
    bool get_combined_sound_media_info_header_box(
        SoundMediaInfoHeaderBox& source_box,
        SoundMediaInfoHeaderBox& combined_box);
    bool copy_sound_media_info_header_box(
            SoundMediaInfoHeaderBox& box);
};

struct NullMediaInfoHeaderBox
{
    FullBox m_full_box;  // type is "nmhd" version = 0
  public :
    NullMediaInfoHeaderBox() {}
    ~NullMediaInfoHeaderBox() {}
    bool write_null_media_info_header_box(Mp4FileWriter* file_writer);
    bool read_null_media_info_header_box(Mp4FileReader* file_reader);
    bool get_proper_null_media_info_header_box(
        NullMediaInfoHeaderBox& box, SubMediaInfo& sub_media_info);
    bool get_combined_null_media_info_header_box(
        NullMediaInfoHeaderBox& source_box,
        NullMediaInfoHeaderBox& combined_box);
    bool copy_null_media_info_header_box(
            NullMediaInfoHeaderBox& box);
};

#define TIME_ENTRY_SIZE 8
struct TimeEntry {
    uint32_t sample_count;
    uint32_t sample_delta;
};

struct DecodingTimeToSampleBox {
    TimeEntry    *m_entry_ptr;
    uint32_t      m_entry_buf_count;
    uint32_t      m_entry_count;
    FullBox       m_full_box;
  public:
    DecodingTimeToSampleBox();
    ~DecodingTimeToSampleBox();
    bool write_decoding_time_to_sample_box(Mp4FileWriter* file_writer);
    bool read_decoding_time_to_sample_box(Mp4FileReader *file_reader);
    void update_decoding_time_to_sample_box_size();
    bool get_proper_decoding_time_to_sample_box(
        DecodingTimeToSampleBox& box,
        SubMediaInfo& sub_media_info);
    bool get_combined_decoding_time_to_sample_box(
        DecodingTimeToSampleBox& source_box,
        DecodingTimeToSampleBox& combined_box);
    bool copy_decoding_time_to_sample_box(
            DecodingTimeToSampleBox& box);
};

struct CompositionTimeToSampleBox {
    TimeEntry    *m_entry_ptr;
    uint32_t      m_entry_count;
    uint32_t      m_entry_buf_count;
    uint32_t      m_last_i_p_count;//do not write it into file, last I or P frame
    FullBox       m_full_box;
  public:
    CompositionTimeToSampleBox();
    ~CompositionTimeToSampleBox();
    bool write_composition_time_to_sample_box(Mp4FileWriter* file_writer);
    bool read_composition_time_to_sample_box(Mp4FileReader *file_reader);
    void update_composition_time_to_sample_box_size();
    bool get_proper_composition_time_to_sample_box(
        CompositionTimeToSampleBox& box, SubMediaInfo& sub_media_info);
    bool get_combined_composition_time_to_sample_box(
        CompositionTimeToSampleBox& source_box,
        CompositionTimeToSampleBox& combined_box);
    bool copy_composition_time_to_sample_box(
        CompositionTimeToSampleBox& box);
};

#define DISOM_AVC_DECODER_CONFIG_RECORD_SIZE 19
struct AVCDecoderConfigurationRecord {
    uint8_t     *m_pps;
    uint8_t     *m_sps;
    uint16_t     m_pps_len;//pps length
    uint16_t     m_sps_len;//sps length
    uint8_t      m_config_version;
    uint8_t      m_profile_indication;
    uint8_t      m_profile_compatibility;
    uint8_t      m_level_indication;
    uint8_t      m_reserved; // 6bits 1
    uint8_t      m_len_size_minus_one; // 2bits
    uint8_t      m_reserved_1; // 3bits 1
    uint8_t      m_sps_num; // 5bits
    uint8_t      m_pps_num;
    BaseBox      m_base_box;
  public:
    AVCDecoderConfigurationRecord();
    ~AVCDecoderConfigurationRecord();
    bool write_avc_decoder_configuration_record_box(
        Mp4FileWriter* file_writer);
    bool read_avc_decoder_configuration_record_box(
        Mp4FileReader* file_reader);
    bool get_proper_avc_decoder_configuration_record_box(
        AVCDecoderConfigurationRecord& box, SubMediaInfo& sub_media_info);
    bool get_combined_avc_decoder_configuration_record_box(
        AVCDecoderConfigurationRecord& source_box,
        AVCDecoderConfigurationRecord& combined_box);
    bool copy_avc_decoder_configuration_record_box(
           AVCDecoderConfigurationRecord& box);
};

struct HEVCConArrayNal
{
    uint8_t *m_nal_unit;
    uint16_t m_nal_unit_length;
  public :
    HEVCConArrayNal();
    ~HEVCConArrayNal();
    bool write_HEVC_con_array_nal(Mp4FileWriter* file_writer);
    bool read_HEVC_con_array_nal(Mp4FileReader* file_reader);
    bool get_proper_HEVC_con_array_nal(HEVCConArrayNal& HEVC_con_array_nal,
                                       SubMediaInfo& sub_media_info);
    bool get_combined_HEVC_con_array_nal(HEVCConArrayNal& source_nal,
                                         HEVCConArrayNal& combined_nal);
    uint32_t get_HEVC_array_nal_size();
    bool copy_HEVC_con_array_nal(HEVCConArrayNal& HEVC_con_array_nal);
};

struct HEVCConArray
{
    HEVCConArrayNal *m_array_units; // m_num_nalus
    uint16_t         m_num_nalus;
    uint8_t          m_array_completeness;//1bits
    uint8_t          m_reserved;//1 bits, vaule = 0;
    uint8_t          m_NAL_unit_type;// 6 bits
  public :
    HEVCConArray();
    ~HEVCConArray();
    bool write_HEVC_con_array(Mp4FileWriter* file_writer);
    bool read_HEVC_con_array(Mp4FileReader* file_reader);
    bool get_proper_HEVC_con_array(HEVCConArray& HEVC_con_array,
                                   SubMediaInfo& sub_media_info);
    bool get_combined_HEVC_con_array(HEVCConArray& source_array,
                                     HEVCConArray& combined_array);
    uint32_t get_HEVC_con_array_size();
    bool copy_HEVC_con_array(HEVCConArray& HEVC_con_array);
};

#define DISOM_HEVC_DECODER_CONFIG_RECORD_SIZE 31
struct HEVCDecoderConfigurationRecord
{
    HEVCConArray *m_HEVC_config_array;// m_num_of_arrays, order: vps, sps, pps,SEI
    uint32_t      m_general_profile_compatibility_flags;
    uint32_t      m_general_constraint_indicator_flags_low;//low 32bits
    uint16_t      m_general_constraint_indicator_flags_high;//high 16bits
    uint16_t      m_min_spatial_segmentation_idc;//12bits,highest 4 bits is 1111.
    uint16_t      m_avg_frame_rate;
    uint8_t       m_configuration_version;// value is 1
    uint8_t       m_general_profile_space;// 2bits
    uint8_t       m_general_tier_flag;  //1bits
    uint8_t       m_general_profile_idc;//5bits
    uint8_t       m_general_level_idc;
    uint8_t       m_parallelism_type;//2bits, highest 6 bits is 111111
    uint8_t       m_chroma_format;//2bits, highest 6 bits is 111111
    uint8_t       m_bit_depth_luma_minus8;//3 bits, highest 5 bits is 11111
    uint8_t       m_bit_depth_chroma_minus8; // 3bits, highest 5 bits is 11111
    uint8_t       m_constant_frame_rate;// 2 bits
    uint8_t       m_num_temporal_layers;//3 bits
    uint8_t       m_temporal_id_nested;//1 bit
    uint8_t       m_length_size_minus_one;// 2 bits
    uint8_t       m_num_of_arrays;
    BaseBox       m_base_box;
  public :
    HEVCDecoderConfigurationRecord();
    ~HEVCDecoderConfigurationRecord();
    bool write_HEVC_decoder_configuration_record(Mp4FileWriter* file_writer);
    bool read_HEVC_decoder_configuration_record(Mp4FileReader* file_reader);
    bool get_proper_HEVC_decoder_configuration_record(
        HEVCDecoderConfigurationRecord& HEVC_record_box,
        SubMediaInfo& sub_media_info);
    bool get_combined_HEVC_decoder_configuration_record(
        HEVCDecoderConfigurationRecord& source_box,
        HEVCDecoderConfigurationRecord& combined_box);
    void update_HEVC_decoder_configuration_record_size();
    bool copy_HEVC_decoder_configuration_record(
            HEVCDecoderConfigurationRecord& HEVC_record_box);
};

#define VISUAL_SAMPLE_ENTRY_SIZE 86
struct VisualSampleEntry {
    uint32_t m_horizresolution;
    uint32_t m_vertresolution;
    uint32_t m_reserved_1;
    uint32_t m_pre_defined_1[3]; // = 0
    uint16_t m_data_reference_index;
    uint16_t m_pre_defined; // = 0
    uint16_t m_reserved; // = 0
    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_frame_count;
    uint16_t m_depth; // = 0x0018
    int16_t  m_pre_defined_2; // = -1
    uint8_t  m_reserved_0[6];
    char     m_compressorname[32];
    BaseBox                         m_base_box;
    AVCDecoderConfigurationRecord   m_avc_config;// for avc
    HEVCDecoderConfigurationRecord  m_HEVC_config;// for HEVC
  public:
    VisualSampleEntry();
    ~VisualSampleEntry(){}
    bool write_visual_sample_entry(Mp4FileWriter* file_writer);
    bool read_visual_sample_entry(Mp4FileReader *file_reader);
    bool get_proper_visual_sample_entry(
        VisualSampleEntry& entry, SubMediaInfo& sub_media_info);
    bool get_combined_visual_sample_entry(
        VisualSampleEntry& source_entry, VisualSampleEntry& combined_entry);
    void update_visual_sample_entry_size();
    bool copy_visual_sample_entry(VisualSampleEntry& entry);
};

#define DISOM_AAC_DESCRIPTOR_SIZE 50
struct AACDescriptorBox {
    uint32_t m_buffer_size;// 24bits
    uint32_t m_max_bitrate;
    uint32_t m_avg_bitrate;
    uint16_t m_es_id;
    uint16_t m_audio_spec_config;
    uint8_t  m_es_descriptor_type_tag;
    uint8_t  m_es_descriptor_type_length;
    uint8_t  m_stream_priority;
    uint8_t  m_decoder_config_descriptor_type_tag;
    uint8_t  m_decoder_descriptor_type_length;
    uint8_t  m_object_type;
    uint8_t  m_stream_flag;
    uint8_t  m_decoder_specific_descriptor_type_tag;
    uint8_t  m_decoder_specific_descriptor_type_length;
    uint8_t  m_SL_descriptor_type_tag;
    uint8_t  m_SL_descriptor_type_length;
    uint8_t  m_SL_value;
    FullBox  m_full_box;
  public:
    AACDescriptorBox();
    ~AACDescriptorBox(){}
    bool write_aac_descriptor_box(
        Mp4FileWriter* file_writer);
    bool read_aac_descriptor_box(
        Mp4FileReader* file_reader);
    bool get_proper_aac_descriptor_box(
        AACDescriptorBox& box, SubMediaInfo& sub_media_info);
    bool get_combined_aac_descriptor_box(
        AACDescriptorBox& source_box,
        AACDescriptorBox& combined_box);
    bool copy_aac_descriptor_box(AACDescriptorBox& box);
};

#define OPUS_SPECIFIC_BOX_SIZE 18
struct OpusSpecificBox {
    //if m_channel_mapping_family != 0, m_output_channel_count;
    uint8_t     *m_channel_mapping;
    uint32_t     m_input_sample_rate;
    uint16_t     m_pre_skip;
    int16_t      m_output_gain;
    uint8_t      m_version;
    uint8_t      m_output_channel_count;
    uint8_t      m_channel_mapping_family;
    uint8_t      m_stream_count;//if m_channel_mapping_family != 0
    uint8_t      m_couple_count;//if m_channel_mapping_family != 0
    BaseBox      m_base_box;
    OpusSpecificBox();
    ~OpusSpecificBox();
    bool write_opus_specific_box(Mp4FileWriter* file_writer);
    bool read_opus_specific_box(Mp4FileReader* file_reader);
    bool get_proper_opus_specific_box(OpusSpecificBox& box,
                                      SubMediaInfo& media_info);
    bool get_combined_opus_specific_box(OpusSpecificBox& source_box,
                                        OpusSpecificBox& combined_box);
    void update_opus_specific_box_size();
    bool copy_opus_specific_box(OpusSpecificBox& box);
};

#define DISOM_AUDIO_SAMPLE_ENTRY_SIZE 36
struct AudioSampleEntry {
    uint32_t m_reserved[2]; // = 0
    uint16_t m_sample_rate_integer;//16.16 fixed-point
    uint16_t m_sample_rate_decimal;
    uint16_t m_data_reference_index;
    uint16_t m_channelcount; // = 2
    uint16_t m_sample_size; // = 16
    uint16_t m_pre_defined; // = 0
    uint16_t m_reserved_1; // = 0
    uint8_t  m_reserved_0[6];
    BaseBox           m_base_box;
    AACDescriptorBox  m_aac;
    OpusSpecificBox   m_opus;
  public:
    AudioSampleEntry();
    ~AudioSampleEntry(){}
    bool write_audio_sample_entry(Mp4FileWriter* file_writer);
    bool read_audio_sample_entry(Mp4FileReader *file_reader);
    bool get_proper_audio_sample_entry(AudioSampleEntry& entry,
                                       SubMediaInfo& media_info);
    bool get_combined_audio_sample_entry(AudioSampleEntry& source_entry,
                                         AudioSampleEntry& combined_entry);
    void update_audio_sample_entry_size();
    bool copy_audio_sample_entry(AudioSampleEntry& entry);
};

#define TEXT_METADATA_SAMPLE_ENTRY_SIZE 16
struct TextMetadataSampleEntry
{
    uint16_t     m_data_reference_index;
    uint8_t      m_reserved[6];
    BaseBox      m_base_box;  //type "mett"
    std::string  m_mime_format;  //"text/GPS"
  public :
    TextMetadataSampleEntry();
    ~TextMetadataSampleEntry() {}
    bool write_text_metadata_sample_entry(Mp4FileWriter* file_writer);
    bool read_text_metadata_sample_entry(Mp4FileReader* file_reader);
    bool get_proper_text_medadata_sample_entry(TextMetadataSampleEntry& entry,
                                               SubMediaInfo& media_info);
    bool get_combined_text_medadata_sample_entry(TextMetadataSampleEntry&
                       source_entry, TextMetadataSampleEntry& combined_entry);
    void update_text_metadata_sample_entry_size();
    bool copy_text_medadata_sample_entry(TextMetadataSampleEntry& entry);
};

struct SampleDescriptionBox {
    uint32_t                m_entry_count;
    FullBox                 m_full_box;
    VisualSampleEntry       m_visual_entry;
    AudioSampleEntry        m_audio_entry;
    TextMetadataSampleEntry m_metadata_entry;
  public:
    SampleDescriptionBox();
    ~SampleDescriptionBox(){}
    bool write_sample_description_box(Mp4FileWriter* file_writer);
    bool read_sample_description_box(Mp4FileReader* file_reader);
    bool get_proper_sample_description_box(
        SampleDescriptionBox& box, SubMediaInfo& media_info);
    bool get_combined_sample_description_box(
        SampleDescriptionBox& source_box,
        SampleDescriptionBox& combined_box);
    void update_sample_description_box();
    bool copy_sample_description_box(
        SampleDescriptionBox& box);
};

#define DISOM_SAMPLE_SIZE_BOX_SIZE 20
/*if m_sample_size 0,then the samples have different sizes, and those sizes
 * are stored in the sample size table. If this field is not 0, it specifies
 * the constant sample size, and no array follows.
 */
struct SampleSizeBox {
    uint32_t *m_size_array;
    uint32_t  m_sample_size;
    uint32_t  m_sample_count;
    uint32_t  m_buf_count;
    FullBox   m_full_box;
  public:
    SampleSizeBox();
    ~SampleSizeBox();
    bool write_sample_size_box(Mp4FileWriter* file_writer);
    bool read_sample_size_box(Mp4FileReader *file_reader);
    void update_sample_size_box_size();
    bool get_proper_sample_size_box(SampleSizeBox& sample_size_box,
                                    SubMediaInfo& media_info);
    bool get_combined_sample_size_box(SampleSizeBox& source_box,
                                      SampleSizeBox& combined_box);
    bool copy_sample_size_box(SampleSizeBox& sample_size_box);
};

//struct SISOMCompactSampleSizeBox {
//  SISOMFullBox m_full_box;
//  uint32_t     m_reserved; // 24 bits
//  uint32_t     m_field_size; //8 bits
//  uint32_t     m_sample_count;
//  uint32_t    *m_entry_size; // adaptive bits
//};

#define SAMPLE_TO_CHUNK_ENTRY_SIZE 12
struct SampleToChunkEntry {
    uint32_t m_first_chunk;
    uint32_t m_sample_per_chunk;
    uint32_t m_sample_description_index;
  public:
    SampleToChunkEntry();
    ~SampleToChunkEntry(){}
    bool write_sample_chunk_entry(Mp4FileWriter* file_writer);
    bool read_sample_chunk_entry(Mp4FileReader *file_reader);
};

struct SampleToChunkBox {
    SampleToChunkEntry  *m_entrys;
    uint32_t             m_entry_count;
    FullBox              m_full_box;
  public:
    SampleToChunkBox();
    ~SampleToChunkBox();
    bool write_sample_to_chunk_box(Mp4FileWriter* file_writer);
    bool read_sample_to_chunk_box(Mp4FileReader *file_reader);
    void update_sample_to_chunk_box_size();
    bool get_proper_sample_to_chunk_box(SampleToChunkBox& box,
                                        SubMediaInfo& media_info);
    bool get_combined_sample_to_chunk_box(SampleToChunkBox& source_box,
                                          SampleToChunkBox& combined_box);
    bool copy_sample_to_chunk_box(SampleToChunkBox& box);
};

struct ChunkOffsetBox {
    uint32_t    *m_chunk_offset;
    uint32_t     m_entry_count;
    uint32_t     m_buf_count;
    FullBox      m_full_box;
  public:
    ChunkOffsetBox();
    ~ChunkOffsetBox();
    bool write_chunk_offset_box(Mp4FileWriter* file_writer);
    bool read_chunk_offset_box(Mp4FileReader *file_reader);
    void update_chunk_offset_box_size();
    bool get_proper_chunk_offset_box(ChunkOffsetBox& box,
                                     SubMediaInfo& media_info);
    bool get_combined_chunk_offset_box(ChunkOffsetBox& source_box,
                                       ChunkOffsetBox& combined_box);
    bool copy_chunk_offset_box(ChunkOffsetBox& box);
};

struct ChunkOffset64Box {
  uint64_t    *m_chunk_offset;
  uint32_t     m_entry_count;
  uint32_t     m_buf_count;
  FullBox      m_full_box;
  public:
  ChunkOffset64Box();
  ~ChunkOffset64Box();
  bool write_chunk_offset64_box(Mp4FileWriter* file_writer);
  bool read_chunk_offset64_box(Mp4FileReader *file_reader);
  void update_chunk_offset64_box_size();
  bool get_proper_chunk_offset64_box(ChunkOffset64Box& box,
                                     SubMediaInfo& media_info);
  bool get_combined_chunk_offset64_box(ChunkOffset64Box& source_box,
                                       ChunkOffset64Box& combined_box);
  bool copy_chunk_offset64_box(ChunkOffset64Box& box);
};

struct SyncSampleTableBox {   //sync sample table  stss
    uint32_t    *m_sync_sample_table;
    uint32_t     m_stss_count;
    uint32_t     m_buf_count;
    FullBox      m_full_box;
  public:
    SyncSampleTableBox();
    ~SyncSampleTableBox();
    bool write_sync_sample_table_box(Mp4FileWriter* file_writer);
    bool read_sync_sample_table_box(Mp4FileReader *file_reader);
    void update_sync_sample_box_size();
    bool get_proper_sync_sample_table_box(
        SyncSampleTableBox& sync_sample_table_box, SubMediaInfo& media_info);
    bool get_combined_sync_sample_table_box(
        SyncSampleTableBox& source_box,
        SyncSampleTableBox& combined_box);
    bool copy_sync_sample_table_box(
            SyncSampleTableBox& sync_sample_table_box);
};

#define ISOM_DATA_ENTRY_BOX_SIZE 12
struct DataEntryBox
{
    FullBox m_full_box;
  public :
    DataEntryBox(){};
    ~DataEntryBox(){};
    bool write_data_entry_box(Mp4FileWriter* file_writer);
    bool read_data_entry_box(Mp4FileReader *file_reader);
    bool get_proper_data_entry_box(DataEntryBox& box,
                                   SubMediaInfo& media_info);
    bool get_combined_data_entry_box(DataEntryBox& source_box,
                                     DataEntryBox& combined_box);
    bool copy_data_entry_box(DataEntryBox& box);
};

struct DataReferenceBox {
    uint32_t     m_entry_count;
    FullBox      m_full_box;
    DataEntryBox m_url;
  public:
    DataReferenceBox();
    ~DataReferenceBox(){}
    bool write_data_reference_box(Mp4FileWriter* file_writer);
    bool read_data_reference_box(Mp4FileReader *file_reader);
    bool get_proper_data_reference_box(DataReferenceBox& box,
                                       SubMediaInfo& media_info);
    bool get_combined_data_reference_box(DataReferenceBox& source_box,
                                         DataReferenceBox& combined_box);
    bool copy_data_reference_box(DataReferenceBox& box);
};

struct DataInformationBox {
    BaseBox           m_base_box;
    DataReferenceBox  m_data_ref;
  public:
    DataInformationBox(){}
    ~DataInformationBox(){}
    bool write_data_info_box(Mp4FileWriter* file_writer);
    bool read_data_info_box(Mp4FileReader *file_reader);
    bool get_proper_data_info_box(
        DataInformationBox& box, SubMediaInfo& media_info);
    bool get_combined_data_info_box(DataInformationBox& source_box,
                                    DataInformationBox& combined_box);
    bool copy_data_info_box(DataInformationBox& box);
};

struct SampleTableBox {
    BaseBox                     m_base_box;
    DecodingTimeToSampleBox     m_stts;
    CompositionTimeToSampleBox  m_ctts;
    SampleDescriptionBox        m_sample_description;
    SampleSizeBox               m_sample_size;
    SampleToChunkBox            m_sample_to_chunk;
    ChunkOffsetBox              m_chunk_offset;
    ChunkOffset64Box            m_chunk_offset64;
    SyncSampleTableBox          m_sync_sample;
  public:
    SampleTableBox(){}
    ~SampleTableBox(){}
    bool write_sample_table_box(Mp4FileWriter* file_writer);
    bool read_sample_table_box(Mp4FileReader *file_reader);
    void update_sample_table_box_size();
    bool get_proper_sample_table_box(
        SampleTableBox& box, SubMediaInfo& media_info);
    bool get_combined_sample_table_box(SampleTableBox& source_box,
                                       SampleTableBox& combined_box);
    bool copy_sample_table_box(SampleTableBox& box);
};

struct MediaInformationBox {
    BaseBox                   m_base_box;
    VideoMediaInfoHeaderBox   m_video_info_header;
    SoundMediaInfoHeaderBox   m_sound_info_header;
    NullMediaInfoHeaderBox    m_null_info_header;
    DataInformationBox        m_data_info;
    SampleTableBox            m_sample_table;
  public:
    MediaInformationBox(){}
    ~MediaInformationBox(){}
    bool write_media_info_box(Mp4FileWriter* file_writer);
    bool read_media_info_box(Mp4FileReader *file_reader);
    void update_media_info_box_size();
    bool get_proper_media_info_box(
        MediaInformationBox& box, SubMediaInfo& media_info);
    bool get_combined_media_info_box(MediaInformationBox& source_box,
                                     MediaInformationBox& combined_box);
    bool copy_media_info_box(MediaInformationBox& box);
};

struct MediaBox {
    BaseBox             m_base_box;
    MediaHeaderBox      m_media_header;
    HandlerReferenceBox m_media_handler;
    MediaInformationBox m_media_info;
  public:
    MediaBox(){}
    ~MediaBox(){}
    bool write_media_box(Mp4FileWriter* file_writer);
    bool read_media_box(Mp4FileReader *file_reader);
    void update_media_box_size();
    bool get_proper_media_box(MediaBox& media_box,
                              SubMediaInfo& media_info);
    bool get_combined_media_box(MediaBox& source_box,
                                      MediaBox& combined_box);
    bool copy_media_box(MediaBox& media_box);
};

struct TrackBox {
    BaseBox        m_base_box;
    TrackHeaderBox m_track_header;
    MediaBox       m_media;
  public:
    TrackBox(){}
    ~TrackBox(){}
    bool write_track_box(Mp4FileWriter* file_writer);
    bool read_track_box(Mp4FileReader *file_reader);
    void update_track_box_size();
    bool get_proper_track_box(TrackBox& track_box, SubMediaInfo& media_info);
    bool get_combined_track_box(TrackBox& source_box, TrackBox& combined_box);
    bool copy_track_box(TrackBox& track_box);
};

struct MovieBox {
    BaseBox        m_base_box;
    MovieHeaderBox m_movie_header_box;
    ObjectDescBox  m_iods_box;
    TrackBox       m_video_track;
    TrackBox       m_audio_track;
    TrackBox       m_gps_track;
  public:
    MovieBox(){}
    ~MovieBox(){};
    bool write_movie_box(Mp4FileWriter* file_writer);
    bool read_movie_box(Mp4FileReader *file_reader);
    void update_movie_box_size();
    bool get_proper_movie_box(MovieBox& movie_box, MediaInfo& media_info);
    bool get_combined_movie_box(MovieBox& source_box, MovieBox& combined_box);
    bool copy_movie_box(MovieBox& movie_box);
  private :
    bool read_track_box(Mp4FileReader *file_reader);
};

#endif

