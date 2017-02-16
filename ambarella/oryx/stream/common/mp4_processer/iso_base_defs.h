/*******************************************************************************
 * iso_base_defs.h
 *
 * History:
 *   2015-09-06 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/


#ifndef ISO_BASE_DEFS_
#define ISO_BASE_DEFS_
/*
 * simple layout of MP4 file (interleave, 32bit/64bit adaptive)
 * --ftyp (File Type Box)
 * --mdat (Media Data Box)
 * --moov (Movie Box)
 * ----mvhd (Movie Header Box)
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
  DISOM_TRACK_BOX_TAG = DISO_BOX_TAG_CREATE('t', 'r', 'a', 'k'),
  DISOM_TRACK_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('t', 'k', 'h', 'd'),
  DISOM_MEDIA_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'd', 'i', 'a'),
  DISOM_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'd', 'h', 'd'),
  DISOM_HANDLER_REFERENCE_BOX_TAG = DISO_BOX_TAG_CREATE('h', 'd', 'l', 'r'),
  DISOM_MEDIA_INFORMATION_BOX_TAG = DISO_BOX_TAG_CREATE('m', 'i', 'n', 'f'),
  DISOM_VIDEO_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('v', 'm', 'h', 'd'),
  DISOM_SOUND_MEDIA_HEADER_BOX_TAG = DISO_BOX_TAG_CREATE('s', 'm', 'h', 'd'),
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
  DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG = DISO_BOX_TAG_CREATE('e', 's', 'd', 's'),
  DISOM_VIDEO_HANDLER_REFERENCE_TAG = DISO_BOX_TAG_CREATE('v', 'i', 'd', 'e'),
  DISOM_AUDIO_HANDLER_REFERENCE_TAG = DISO_BOX_TAG_CREATE('s', 'o', 'u', 'n'),
  DISOM_HINT_HANDLER_REFERENCE_TAG = DISO_BOX_TAG_CREATE('h', 'i', 'n', 't'),
  DISOM_FREE_BOX_TAG = DISO_BOX_TAG_CREATE('f', 'r', 'e', 'e'),
};

enum DISO_BRAND_TAG{
  DISOM_BRAND_NULL = 0,
  DISOM_BRAND_V0_TAG = DISO_BOX_TAG_CREATE('i', 's', 'o', 'm'),
  DISOM_BRAND_V1_TAG = DISO_BOX_TAG_CREATE('i', 's', 'o', '2'),
  DISOM_BRAND_AVC_TAG = DISO_BOX_TAG_CREATE('a', 'v', 'c', '1'),
  DISOM_BRAND_MPEG4_TAG = DISO_BOX_TAG_CREATE('m', 'p', 'g', '4'),
};

#define DISOM_TAG_SIZE 4
#define DISOM_BOX_SIZE 8
#define DISOM_FULL_BOX_SIZE 12

#define DISOM_TRACK_ENABLED_FLAG 0x000001

struct WriteInfo {
  private :
    uint64_t m_video_duration;
    uint64_t m_audio_duration;
    uint32_t m_video_first_frame;
    uint32_t m_video_last_frame;
    uint32_t m_audio_first_frame;
    uint32_t m_audio_last_frame;
  public :
    WriteInfo();
    ~WriteInfo(){}
    bool set_video_duration(uint64_t video_duration);
    bool set_audio_duration(uint64_t audio_duration);
    bool set_video_first_frame(uint32_t video_first_frame);
    bool set_video_last_frame(uint32_t video_last_frame);
    bool set_audio_first_frame(uint32_t audio_first_frame);
    bool set_audio_last_frame(uint32_t audio_last_frame);
    uint64_t get_video_duration();
    uint64_t get_audio_duration();
    uint32_t get_video_first_frame();
    uint32_t get_video_last_frame();
    uint32_t get_audio_first_frame();
    uint32_t get_audio_last_frame();
};

struct BaseBox {
    uint32_t       m_size;
    DISOM_BOX_TAG m_type;
  public:
    BaseBox();
    ~BaseBox(){}
    bool write_base_box(Mp4FileWriter *file_writer);
    bool read_base_box(Mp4FileReader *file_reader);
    uint32_t get_base_box_length();
};

struct FullBox {
    BaseBox     m_base_box;
    uint32_t     m_flags; //24bits
    uint8_t      m_version;
  public:
    FullBox();
    ~FullBox(){}
    bool write_full_box(Mp4FileWriter* file_writer);
    bool read_full_box(Mp4FileReader *file_reader);
    uint32_t get_full_box_length();
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
    BaseBox       m_base_box;
    uint32_t       m_compatible_brands_number;
    uint32_t       m_minor_version;
    DISO_BRAND_TAG m_major_brand;
    DISO_BRAND_TAG m_compatible_brands[DISOM_MAX_COMPATIBLE_BRANDS];
  public:
    FileTypeBox();
    ~FileTypeBox(){}
    bool write_file_type_box(Mp4FileWriter* file_writer);
    bool read_file_type_box(Mp4FileReader *file_reader);
};

struct MediaDataBox {
    BaseBox m_base_box;
  public:
    MediaDataBox(){}
    ~MediaDataBox(){}
    bool write_media_data_box(Mp4FileWriter* file_writer);
    bool read_media_data_box(Mp4FileReader *file_reader);
};

struct MovieHeaderBox {
    FullBox     m_base_full_box;
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
  public:
    MovieHeaderBox();
    ~MovieHeaderBox(){}
    bool write_movie_header_box(Mp4FileWriter* file_writer);
    bool read_movie_header_box(Mp4FileReader *file_reader);
    bool get_proper_movie_header_box(MovieHeaderBox& movie_header_box,
                                     WriteInfo& write_info);
};
#define DISOMConstMovieHeaderSize 108
#define DISOMConstMovieHeader64Size 120

struct TrackHeaderBox {
    FullBox m_base_full_box;
    //version specific
    uint64_t m_creation_time;// 32 bits in version 0
    uint64_t m_modification_time;// 32 bits in version 0
    uint64_t m_duration; // 32 bits in version 0
    uint32_t m_track_ID;
    uint32_t m_reserved;
    uint32_t m_reserved_1[2];
    uint32_t m_matrix[9];
    uint32_t m_width;
    uint32_t m_height;
    uint16_t m_layer;
    uint16_t m_alternate_group;
    uint16_t m_volume;
    uint16_t m_reserved_2;
  public:
    TrackHeaderBox();
    ~TrackHeaderBox(){}
    bool write_movie_track_header_box(Mp4FileWriter* file_writer);
    bool read_movie_track_header_box(Mp4FileReader *file_reader);
    bool get_proper_track_header_box(TrackHeaderBox& track_header_box,
                                     WriteInfo& write_info);
};

#define DISOMConstTrackHeaderSize 92
#define DISOMConstTrackHeader64Size 104

struct MediaHeaderBox {//contained in media box
    FullBox m_base_full_box;
    //version specific
    uint64_t m_creation_time;// 32 bits in version 0
    uint64_t m_modification_time;// 32 bits in version 0
    uint64_t m_duration; // 32 bits in version 0
    uint32_t m_timescale;
    uint16_t m_pre_defined;
    uint8_t  m_pad; // 1bit
    uint8_t  m_language[3]; //5bits
  public:
    MediaHeaderBox();
    ~MediaHeaderBox(){}
    bool write_media_header_box(Mp4FileWriter* file_writer);
    bool read_media_header_box(Mp4FileReader *file_reader);
    bool get_proper_media_header_box(MediaHeaderBox& media_header_box,
                                     WriteInfo& write_info);
};
#define DISOMConstMediaHeaderSize 32
#define DISOMConstMediaHeader64Size 44

struct HandlerReferenceBox {
    FullBox        m_base_full_box;
    DISOM_BOX_TAG m_handler_type;
    uint32_t       m_pre_defined;
    uint32_t       m_reserved[3];
    char          *m_name;
  public:
    HandlerReferenceBox();
    ~HandlerReferenceBox();
    bool write_handler_reference_box(Mp4FileWriter* file_writer);
    bool read_handler_reference_box(Mp4FileReader *file_reader);
    bool get_proper_handler_reference_box(
        HandlerReferenceBox& handler_reference_box, WriteInfo& write_info);
};
#define DISOMConstHandlerReferenceSize 32

struct VideoMediaInformationHeaderBox {//contained in media information box
    FullBox  m_base_full_box;
    uint16_t  m_graphicsmode;
    uint16_t  m_opcolor[3];
  public:
    VideoMediaInformationHeaderBox();
    ~VideoMediaInformationHeaderBox(){}
    bool write_video_media_information_header_box(Mp4FileWriter* file_writer);
    bool read_video_media_information_header_box(Mp4FileReader *file_reader);
    bool get_proper_video_media_information_header_box(
        VideoMediaInformationHeaderBox& video_media_information_header_box,
        WriteInfo& write_info);
};
#define DISOMConstVideoMediaHeaderSize 20

struct SoundMediaInformationHeaderBox {
    FullBox m_base_full_box;
    uint16_t m_balanse;
    uint16_t m_reserved;
  public:
    SoundMediaInformationHeaderBox();
    ~SoundMediaInformationHeaderBox(){}
    bool write_sound_media_information_header_box(Mp4FileWriter* file_writer);
    bool read_sound_media_information_header_box(Mp4FileReader *file_reader);
    bool get_proper_sound_media_information_header_box(
        SoundMediaInformationHeaderBox& sound_media_information_header_box,
        WriteInfo& write_info);
};
#define DISOMConstSoundMediaHeaderSize 16

struct _STimeEntry {
  uint32_t sample_count;
  uint32_t sample_delta;
};

struct DecodingTimeToSampleBox {
    FullBox      m_base_full_box;
    uint32_t      m_entry_buf_count;
    uint32_t      m_entry_count;
    _STimeEntry *m_p_entry;
  public:
    DecodingTimeToSampleBox();
    ~DecodingTimeToSampleBox();
    bool write_decoding_time_to_sample_box(Mp4FileWriter* file_writer);
    bool read_decoding_time_to_sample_box(Mp4FileReader *file_reader);
    void update_decoding_time_to_sample_box_size();
    bool get_proper_video_decoding_time_to_sample_box(
        DecodingTimeToSampleBox& decoding_time_to_sample_box,
        WriteInfo& write_info);
    bool get_proper_audio_decoding_time_to_sample_box(
            DecodingTimeToSampleBox& decoding_time_to_sample_box,
            WriteInfo& write_info);
};

struct CompositionTimeToSampleBox {
    FullBox      m_base_full_box;
    uint32_t      m_entry_count;
    uint32_t      m_entry_buf_count;
    _STimeEntry *m_p_entry;
  public:
    CompositionTimeToSampleBox();
    ~CompositionTimeToSampleBox();
    bool write_composition_time_to_sample_box(Mp4FileWriter* file_writer);
    bool read_composition_time_to_sample_box(Mp4FileReader *file_reader);
    void update_composition_time_to_sample_box_size();
    bool get_proper_composition_time_to_sample_box(
        CompositionTimeToSampleBox& composition_time_to_sample_box,
        WriteInfo& write_info);
};

struct AVCDecoderConfigurationRecord {
    BaseBox     m_base_box;
    uint8_t     *m_p_pps;
    uint8_t     *m_p_sps;
    uint16_t     m_picture_parameters_set_length;
    uint16_t     m_sequence_parameters_set_length;
    uint8_t      m_configuration_version;
    uint8_t      m_AVC_profile_indication;
    uint8_t      m_profile_compatibility;
    uint8_t      m_AVC_level_indication;
    uint8_t      m_reserved; // 6bits 1
    uint8_t      m_length_size_minus_one; // 2bits
    uint8_t      m_reserved_1; // 3bits 1
    uint8_t      m_num_of_sequence_parameter_sets; // 5bits
    uint8_t      m_num_of_picture_parameter_sets;
  public:
    AVCDecoderConfigurationRecord();
    ~AVCDecoderConfigurationRecord();
    bool write_avc_decoder_configuration_record_box(
                       Mp4FileWriter* file_writer);
    bool read_avc_decoder_configuration_record_box(
                           Mp4FileReader* file_reader);
    bool get_proper_avc_decoder_configuration_record_box(
        AVCDecoderConfigurationRecord& avc_decoder_configuration_record_box,
        WriteInfo& write_info);
};
#define DISOMConstAVCDecoderConfiurationRecordSize_FixPart 19

struct _SVisualSampleEntry {
  BaseBox                         m_base_box;
  AVCDecoderConfigurationRecord m_avc_config;// for avc
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
  public:
  _SVisualSampleEntry();
  ~_SVisualSampleEntry(){}
  bool write_visual_sample_entry(Mp4FileWriter* file_writer);
  bool read_visual_sample_entry(Mp4FileReader *file_reader);
  bool get_proper_visual_sample_entry(
      _SVisualSampleEntry& visual_sample_entry, WriteInfo& write_info);
};
#define DISOMConstVisualSampleEntrySize_FixPart 86

struct AACElementaryStreamDescriptorBox {
    FullBox m_base_full_box;
    /*ES descriptor*/
    uint16_t m_es_id;
    uint8_t  m_es_descriptor_type_tag;
    uint8_t  m_es_descriptor_type_length;
    uint8_t  m_stream_priority;
    /*decoder config descriptor*/
    uint32_t m_buffer_size;// 24bits
    uint32_t m_max_bitrate;
    uint32_t m_avg_bitrate;
    uint8_t  m_decoder_config_descriptor_type_tag;
    uint8_t  m_decoder_descriptor_type_length;
    uint8_t  m_object_type;
    uint8_t  m_stream_flag;
    /*decoder specific descriptor*/
    uint16_t m_audio_spec_config;
    uint8_t  m_decoder_specific_descriptor_type_tag;
    uint8_t  m_decoder_specific_descriptor_type_length;
    /*SL descriptor*/
    uint8_t  m_SL_descriptor_type_tag;
    uint8_t  m_SL_descriptor_type_length;
    uint8_t  m_SL_value;
  public:
    AACElementaryStreamDescriptorBox();
    ~AACElementaryStreamDescriptorBox(){}
    bool write_aac_elementary_stream_description_box(
        Mp4FileWriter* file_writer);
    bool read_aac_elementary_stream_description_box(
            Mp4FileReader* file_reader);
    bool get_proper_aac_elementary_stream_descriptor_box(
        AACElementaryStreamDescriptorBox& aac_elementary_box,
        WriteInfo& write_info);
};
#define DISOMConstAACElementaryStreamDescriptorSize_FixPart 50

struct _SAudioSampleEntry {
  BaseBox                             m_base_box;
  AACElementaryStreamDescriptorBox  m_esd;
  uint32_t m_reserved[2]; // = 0
  uint32_t m_samplerate;
  uint16_t m_data_reference_index;
  uint16_t m_channelcount; // = 2
  uint16_t m_samplesize; // = 16
  uint16_t m_pre_defined; // = 0
  uint16_t m_reserved_1; // = 0
  uint8_t  m_reserved_0[6];
  public:
  _SAudioSampleEntry();
  ~_SAudioSampleEntry(){}
  bool write_audio_sample_entry(Mp4FileWriter* file_writer);
  bool read_audio_sample_entry(Mp4FileReader *file_reader);
  bool get_proper_audio_sample_entry(_SAudioSampleEntry& audio_sample_entry,
                                     WriteInfo& write_info);
};
#define DISOMConstAudioSampleEntrySize 36

struct VisualSampleDescriptionBox {
    FullBox              m_base_full_box;
    _SVisualSampleEntry m_visual_entry;
    uint32_t              m_entry_count;
  public:
    VisualSampleDescriptionBox();
    ~VisualSampleDescriptionBox(){}
    bool write_visual_sample_description_box(Mp4FileWriter* file_writer);
    bool read_visual_sample_description_box(Mp4FileReader* file_reader);
    bool get_proper_visual_sample_description_box(
        VisualSampleDescriptionBox& visual_sample_description_box,
        WriteInfo& write_info);
};

struct AudioSampleDescriptionBox {
    FullBox m_base_full_box;
    _SAudioSampleEntry m_audio_entry;
    uint32_t m_entry_count;
  public:
    AudioSampleDescriptionBox();
    ~AudioSampleDescriptionBox(){}
    bool write_audio_sample_description_box(Mp4FileWriter* file_writer);
    bool read_audio_sample_description_box(Mp4FileReader* file_reader);
    bool get_proper_audio_sample_description_box(
        AudioSampleDescriptionBox& audio_sample_description_box,
        WriteInfo& write_info);
};

struct SampleSizeBox {
  FullBox  m_base_full_box;
  uint32_t  m_sample_size;//0
  uint32_t  m_sample_count;
  uint32_t *m_entry_size;
  uint32_t  m_entry_buf_count;
  public:
  SampleSizeBox();
  ~SampleSizeBox();
  bool write_sample_size_box(Mp4FileWriter* file_writer);
  bool read_sample_size_box(Mp4FileReader *file_reader);
  void update_sample_size_box_size();
  bool get_proper_video_sample_size_box(SampleSizeBox& sample_size_box,
                                  WriteInfo& write_info);
  bool get_proper_audio_sample_size_box(SampleSizeBox& sample_size_box,
                                        WriteInfo& write_info);
};

//struct SISOMCompactSampleSizeBox {
//  SISOMFullBox m_base_full_box;
//  uint32_t     m_reserved; // 24 bits
//  uint32_t     m_field_size; //8 bits
//  uint32_t     m_sample_count;
//  uint32_t    *m_entry_size; // adaptive bits
//};

struct _SSampleToChunkEntry {
    uint32_t m_first_entry;
    uint32_t m_sample_per_chunk;
    uint32_t m_sample_description_index;
  public:
    _SSampleToChunkEntry();
    ~_SSampleToChunkEntry(){}
    bool write_sample_chunk_entry(Mp4FileWriter* file_writer);
    bool read_sample_chunk_entry(Mp4FileReader *file_reader);
};

struct SampleToChunkBox {
    FullBox                m_base_full_box;
    _SSampleToChunkEntry *m_entrys;
    uint32_t                m_entry_count;
  public:
    SampleToChunkBox();
    ~SampleToChunkBox();
    bool write_sample_to_chunk_box(Mp4FileWriter* file_writer);
    bool read_sample_to_chunk_box(Mp4FileReader *file_reader);
    void update_sample_to_chunk_box_size();
    bool get_proper_sample_to_chunk_box(SampleToChunkBox& sample_to_chunk_box,
                                        WriteInfo& write_info);
};

struct ChunkOffsetBox {
    FullBox     m_base_full_box;
    uint32_t     m_entry_count;
    uint32_t    *m_chunk_offset;
    uint32_t     m_entry_buf_count;
  public:
    ChunkOffsetBox();
    ~ChunkOffsetBox();
    bool write_chunk_offset_box(Mp4FileWriter* file_writer);
    bool read_chunk_offset_box(Mp4FileReader *file_reader);
    void update_chunk_offset_box_size();
    bool get_proper_video_chunk_offset_box(ChunkOffsetBox& chunk_offset_box,
                                     WriteInfo& write_info);
    bool get_proper_audio_chunk_offset_box(ChunkOffsetBox& chunk_offset_box,
                                           WriteInfo& write_info);
};

//struct SISOMChunkOffset64Box {
//  SISOMFullBox m_base_full_box;
//  uint64_t    *m_chunk_offset;
//  uint32_t     m_entry_count;
//};

struct SyncSampleTableBox {   //sync sample table  stss
    FullBox     m_base_full_box;
    uint32_t    *m_sync_sample_table;
    uint32_t     m_stss_count;
    uint32_t     m_stss_buf_count;
  public:
    SyncSampleTableBox();
    ~SyncSampleTableBox();
    bool write_sync_sample_table_box(Mp4FileWriter* file_writer);
    bool read_sync_sample_table_box(Mp4FileReader *file_reader);
    void update_sync_sample_box_size();
    bool get_proper_sync_sample_table_box(
        SyncSampleTableBox& sync_sample_table_box, WriteInfo& write_info);
};

struct DataEntryBox
{
    FullBox m_base_full_box;
  public :
    DataEntryBox(){};
    ~DataEntryBox(){};
    bool write_data_entry_box(Mp4FileWriter* file_writer);
    bool read_data_entry_box(Mp4FileReader *file_reader);
    bool get_proper_data_entry_box(DataEntryBox& data_entry_box,
                                   WriteInfo& write_info);
};

struct DataReferenceBox {
  FullBox m_base_full_box;
  DataEntryBox m_url;
  uint32_t m_entry_count;
  public:
  DataReferenceBox();
  ~DataReferenceBox(){}
  bool write_data_reference_box(Mp4FileWriter* file_writer);
  bool read_data_reference_box(Mp4FileReader *file_reader);
  bool get_proper_data_reference_box(DataReferenceBox& data_reference_box,
                                     WriteInfo& write_info);
};

struct DataInformationBox {
    BaseBox           m_base_box;
    DataReferenceBox m_data_ref;
  public:
    DataInformationBox(){}
    ~DataInformationBox(){}
    bool write_data_information_box(Mp4FileWriter* file_writer);
    bool read_data_information_box(Mp4FileReader *file_reader);
    bool get_proper_data_information_box(
        DataInformationBox& data_information_box, WriteInfo& write_info);
};

struct SampleTableBox {
    BaseBox                      m_base_box;
    DecodingTimeToSampleBox    m_stts;
    CompositionTimeToSampleBox m_ctts;
    VisualSampleDescriptionBox m_visual_sample_description;
    AudioSampleDescriptionBox  m_audio_sample_description;
    SampleSizeBox               m_sample_size;
    SampleToChunkBox            m_sample_to_chunk;
    ChunkOffsetBox              m_chunk_offset;
    SyncSampleTableBox          m_sync_sample;
  public:
    SampleTableBox(){}
    ~SampleTableBox(){}
    bool write_video_sample_table_box(Mp4FileWriter* file_writer);
    bool read_video_sample_table_box(Mp4FileReader *file_reader);
    bool write_audio_sample_table_box(Mp4FileWriter* file_writer);
    bool read_audio_sample_table_box(Mp4FileReader *file_reader);
    void update_video_sample_table_box_size();
    void update_audio_sample_table_box_size();
    bool get_proper_video_sample_table_box(
        SampleTableBox& video_sample_table_box, WriteInfo& write_info);
    bool get_proper_audio_sample_table_box(
        SampleTableBox& audio_sample_table_box, WriteInfo& write_info);
};

struct MediaInformationBox {
  BaseBox             m_base_box;
  VideoMediaInformationHeaderBox m_video_information_header;
  SoundMediaInformationHeaderBox m_sound_information_header;
  DataInformationBox  m_data_info;
  SampleTableBox      m_sample_table;
  public:
  MediaInformationBox(){}
  ~MediaInformationBox(){}
  bool write_video_media_information_box(Mp4FileWriter* file_writer);
  bool read_video_media_information_box(Mp4FileReader *file_reader);
  bool write_audio_media_information_box(Mp4FileWriter* file_writer);
  bool read_audio_media_information_box(Mp4FileReader *file_reader);
  void update_video_media_information_box_size();
  void update_audio_media_information_box_size();
  bool get_proper_video_media_information_box(
      MediaInformationBox& media_information_box, WriteInfo& write_info);
  bool get_proper_audio_media_information_box(
        MediaInformationBox& media_information_box, WriteInfo& write_info);
};

struct MediaBox {
  BaseBox             m_base_box;
  MediaHeaderBox      m_media_header;
  HandlerReferenceBox m_media_handler;
  MediaInformationBox m_media_info;
  public:
  MediaBox(){}
  ~MediaBox(){}
  bool write_video_media_box(Mp4FileWriter* file_writer);
  bool read_video_media_box(Mp4FileReader *file_reader);
  bool write_audio_media_box(Mp4FileWriter* file_writer);
  bool read_audio_media_box(Mp4FileReader *file_reader);
  void update_video_media_box_size();
  void update_audio_media_box_size();
  bool get_proper_video_media_box(MediaBox& media_box,
                                  WriteInfo& write_info);
  bool get_proper_audio_media_box(MediaBox& media_box,
                                  WriteInfo& write_info);
};

struct TrackBox {
  BaseBox        m_base_box;
  TrackHeaderBox m_track_header;
  MediaBox       m_media;
  public:
  TrackBox(){}
  ~TrackBox(){}
  bool write_video_movie_track_box(Mp4FileWriter* file_writer);
  bool read_video_movie_track_box(Mp4FileReader *file_reader);
  bool write_audio_movie_track_box(Mp4FileWriter* file_writer);
  bool read_audio_movie_track_box(Mp4FileReader *file_reader);
  void update_video_movie_track_box_size();
  void update_audio_movie_track_box_size();
  bool get_proper_video_movie_track_box(TrackBox& track_box,
                                        WriteInfo& write_info);
  bool get_proper_audio_movie_track_box(TrackBox& track_box,
                                        WriteInfo& write_info);
};

struct MovieBox {
  BaseBox         m_base_box;
  MovieHeaderBox m_movie_header_box;
  TrackBox        m_video_track;
  TrackBox       m_audio_track;
  public:
  MovieBox(){}
  ~MovieBox(){};
  bool write_movie_box(Mp4FileWriter* file_writer);
  bool read_movie_box(Mp4FileReader *file_reader);
  void update_movie_box_size();
  bool get_proper_movie_box(MovieBox& movie_box, WriteInfo& write_info);
};

#endif

