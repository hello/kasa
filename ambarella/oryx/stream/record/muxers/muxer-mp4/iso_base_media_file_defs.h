/*
 * iso_base_media_file_defs.h
 *
 * History:
 *    2014/12/02 - [Zhi He] Create File
 *    2015/04/14 - [ccJing] Modified File
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef ISO_BASE_MEDIA_FILE_DEFS__
#define ISO_BASE_MEDIA_FILE_DEFS__
#include "am_amf_types.h"
#include "am_muxer_mp4_file_writer.h"
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

struct SISOMBaseBox {
    uint32_t     m_size;
    DISOM_BOX_TAG m_type;
  public:
    SISOMBaseBox();
    ~SISOMBaseBox(){}
    AM_STATE write_base_box(AMMp4FileWriter* file_writer);
    uint32_t get_base_box_length();
};

struct SISOMFullBox {
    SISOMBaseBox m_base_box;
    uint32_t     m_flags; //24bits
    uint8_t      m_version;
  public:
    SISOMFullBox();
    ~SISOMFullBox(){}
    AM_STATE write_full_box(AMMp4FileWriter* file_writer);
    uint32_t get_full_box_length();
};

struct SISOMFreeBox {
    SISOMBaseBox m_base_box;
    SISOMFreeBox(){}
    ~SISOMFreeBox(){}
    AM_STATE write_free_box(AMMp4FileWriter* file_writer);
};
#define DISOM_MAX_COMPATIBLE_BRANDS 8
struct SISOMFileTypeBox {
    SISOMBaseBox   m_base_box;
    uint32_t       m_compatible_brands_number;
    uint32_t       m_minor_version;
    DISO_BRAND_TAG m_major_brand;
    DISO_BRAND_TAG m_compatible_brands[DISOM_MAX_COMPATIBLE_BRANDS];
  public:
    SISOMFileTypeBox();
    ~SISOMFileTypeBox(){}
    AM_STATE write_file_type_box(AMMp4FileWriter* file_writer);
};

struct SISOMMediaDataBox {
    SISOMBaseBox m_base_box;
  public:
    SISOMMediaDataBox(){}
    ~SISOMMediaDataBox(){}
    AM_STATE write_media_data_box(AMMp4FileWriter* file_writer);
};

struct SISOMMovieHeaderBox {
    SISOMFullBox m_base_full_box;
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
    SISOMMovieHeaderBox();
    ~SISOMMovieHeaderBox(){}
    AM_STATE write_movie_header_box(AMMp4FileWriter* file_writer);
};
#define DISOMConstMovieHeaderSize 108
#define DISOMConstMovieHeader64Size 120

struct SISOMTrackHeaderBox {
    SISOMFullBox m_base_full_box;
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
    SISOMTrackHeaderBox();
    ~SISOMTrackHeaderBox(){}
    AM_STATE write_movie_track_header_box(AMMp4FileWriter* file_writer);
};

#define DISOMConstTrackHeaderSize 92
#define DISOMConstTrackHeader64Size 104

struct SISOMMediaHeaderBox {//contained in mdia box
    SISOMFullBox m_base_full_box;
    //version specific
    uint64_t m_creation_time;// 32 bits in version 0
    uint64_t m_modification_time;// 32 bits in version 0
    uint64_t m_duration; // 32 bits in version 0
    uint32_t m_timescale;
    uint16_t m_pre_defined;
    uint8_t  m_pad; // 1bit
    uint8_t  m_language[3]; //5bits
  public:
    SISOMMediaHeaderBox();
    ~SISOMMediaHeaderBox(){}
    AM_STATE write_media_header_box(AMMp4FileWriter* file_writer);
};
#define DISOMConstMediaHeaderSize 32
#define DISOMConstMediaHeader64Size 44

struct SISOMHandlerReferenceBox {
    SISOMFullBox m_base_full_box;
    DISOM_BOX_TAG m_handler_type;
    uint32_t     m_pre_defined;
    uint32_t     m_reserved[3];
    const char*  m_name;
  public:
    SISOMHandlerReferenceBox();
    ~SISOMHandlerReferenceBox(){}
    AM_STATE write_hander_reference_box(AMMp4FileWriter* file_writer);
};
#define DISOMConstHandlerReferenceSize 32

struct SISOMVideoMediaInformationHeaderBox {//contained in media information box
    SISOMFullBox m_base_full_box;
    uint16_t     m_graphicsmode;
    uint16_t     m_opcolor[3];
  public:
    SISOMVideoMediaInformationHeaderBox();
    ~SISOMVideoMediaInformationHeaderBox(){}
    AM_STATE write_video_media_information_header_box(
                                  AMMp4FileWriter* file_writer);
};
#define DISOMConstVideoMediaHeaderSize 20

struct SISOMSoundMediaInformationHeaderBox {
    SISOMFullBox m_base_full_box;
    uint16_t     m_balanse;
    uint16_t     m_reserved;
  public:
    SISOMSoundMediaInformationHeaderBox();
    ~SISOMSoundMediaInformationHeaderBox(){}
    AM_STATE write_sound_media_information_header_box(
                                         AMMp4FileWriter* file_writer);
};
#define DISOMConstSoundMediaHeaderSize 16

struct _STimeEntry {
  uint32_t sample_count;
  uint32_t sample_delta;
};

struct SISOMDecodingTimeToSampleBox {
    SISOMFullBox m_base_full_box;
    uint32_t     m_entry_buf_count;
    uint32_t     m_entry_count;
    _STimeEntry* m_p_entry;
  public:
    SISOMDecodingTimeToSampleBox();
    ~SISOMDecodingTimeToSampleBox();
    AM_STATE write_decoding_time_to_sample_box(AMMp4FileWriter* file_writer);
    void update_decoding_time_to_sample_box_size();
};

struct SISOMCompositionTimeToSampleBox {
    SISOMFullBox m_base_full_box;
    uint32_t     m_entry_count;
    uint32_t     m_entry_buf_count;
    _STimeEntry* m_p_entry;
  public:
    SISOMCompositionTimeToSampleBox();
    ~SISOMCompositionTimeToSampleBox();
    AM_STATE write_composition_time_to_sample_box(AMMp4FileWriter* file_writer);
    void update_composition_time_to_sample_box_size();
};

struct SISOMAVCDecoderConfigurationRecord {
    SISOMBaseBox m_base_box;
    uint8_t*     m_p_pps;
    uint8_t*     m_p_sps;
    uint16_t     m_pictureParametersSetLength;
    uint16_t     m_sequenceParametersSetLength;
    uint8_t      m_configurationVersion;
    uint8_t      m_AVCProfileIndication;
    uint8_t      m_profile_compatibility;
    uint8_t      m_AVCLevelIndication;
    uint8_t      m_reserved; // 6bits 1
    uint8_t      m_lengthSizeMinusOne; // 2bits
    uint8_t      m_reserved_1; // 3bits 1
    uint8_t      m_numOfSequenceParameterSets; // 5bits
    uint8_t      m_numOfPictureParameterSets;
  public:
    SISOMAVCDecoderConfigurationRecord();
    ~SISOMAVCDecoderConfigurationRecord(){}
    AM_STATE write_avc_decoder_configuration_record_box(
                       AMMp4FileWriter* file_writer);
};
#define DISOMConstAVCDecoderConfiurationRecordSize_FixPart 19

struct _SVisualSampleEntry {
  SISOMBaseBox                       m_base_box;
  SISOMAVCDecoderConfigurationRecord m_avc_config;// for avc
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
  AM_STATE write_visual_sample_entry(AMMp4FileWriter* file_writer);
};
#define DISOMConstVisualSampleEntrySize_FixPart 86

struct SISOMAACElementaryStreamDescriptorBox {
    SISOMFullBox m_base_full_box;
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
    SISOMAACElementaryStreamDescriptorBox();
    ~SISOMAACElementaryStreamDescriptorBox(){}
    AM_STATE write_aac_elementary_stream_description_box(
        AMMp4FileWriter* file_writer);
};
#define DISOMConstAACElementaryStreamDescriptorSize_FixPart 50

struct _SAudioSampleEntry {
  SISOMBaseBox m_base_box;
  SISOMAACElementaryStreamDescriptorBox m_esd;
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
  AM_STATE write_audio_sample_entry(AMMp4FileWriter* file_writer);
} ;
#define DISOMConstAudioSampleEntrySize 36

struct SISOMVisualSampleDescriptionBox {
    SISOMFullBox        m_base_full_box;
    _SVisualSampleEntry m_visual_entry;
    uint32_t            m_entry_count;
  public:
    SISOMVisualSampleDescriptionBox();
    ~SISOMVisualSampleDescriptionBox(){}
    AM_STATE write_visual_sample_description_box(AMMp4FileWriter* file_writer);
};

struct SISOMAudioSampleDescriptionBox {
    SISOMFullBox m_base_full_box;
    _SAudioSampleEntry m_audio_entry;
    uint32_t m_entry_count;
  public:
    SISOMAudioSampleDescriptionBox();
    ~SISOMAudioSampleDescriptionBox(){}
    AM_STATE write_audio_sample_description_box(AMMp4FileWriter* file_writer);
};

struct SISOMSampleSizeBox {
  SISOMFullBox m_base_full_box;
  uint32_t  m_sample_size;
  uint32_t  m_sample_count;
  uint32_t *m_entry_size;
  uint32_t  m_entry_buf_count;
  public:
  SISOMSampleSizeBox();
  ~SISOMSampleSizeBox();
  AM_STATE write_sample_size_box(AMMp4FileWriter* file_writer);
  void update_sample_size_box_size();
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
    AM_STATE write_sample_chunk_entry(AMMp4FileWriter* file_writer);
};

struct SISOMSampleToChunkBox {
    SISOMFullBox          m_base_full_box;
    _SSampleToChunkEntry *m_entrys;
    uint32_t              m_entry_count;
  public:
    SISOMSampleToChunkBox();
    ~SISOMSampleToChunkBox();
    AM_STATE write_sample_to_chunk_box(AMMp4FileWriter* file_writer);
    void update_sample_to_chunk_box_size();
};

struct SISOMChunkOffsetBox {
    SISOMFullBox m_base_full_box;
    uint32_t     m_entry_count;
    uint32_t    *m_chunk_offset;
    uint32_t     m_entry_buf_count;
  public:
    SISOMChunkOffsetBox();
    ~SISOMChunkOffsetBox();
    AM_STATE write_chunk_offset_box(AMMp4FileWriter* file_writer);
    void update_chunk_offset_box_size();
};

//struct SISOMChunkOffset64Box {
//  SISOMFullBox m_base_full_box;
//  uint64_t    *m_chunk_offset;
//  uint32_t     m_entry_count;
//};

struct SISOMSyncSampleTableBox {   //sync sample table  stss
    SISOMFullBox m_base_full_box;
    uint32_t    *m_sync_sample_table;
    uint32_t     m_stss_count;
    uint32_t     m_stss_buf_count;
  public:
    SISOMSyncSampleTableBox();
    ~SISOMSyncSampleTableBox();
    AM_STATE write_sync_sample_table_box(AMMp4FileWriter* file_writer);
    void update_sync_sample_box_size();
};

struct SISOMDataEntryBox
{
    SISOMFullBox m_base_full_box;
  public :
    SISOMDataEntryBox(){};
    ~SISOMDataEntryBox(){};
    AM_STATE write_data_entry_box(AMMp4FileWriter* file_writer);
};

struct SISOMDataReferenceBox {
  SISOMFullBox m_base_full_box;
  SISOMDataEntryBox m_url;
  uint32_t     m_entry_count;
  public:
  SISOMDataReferenceBox();
  ~SISOMDataReferenceBox(){}
  AM_STATE write_data_reference_box(AMMp4FileWriter* file_writer);
};

struct SISOMDataInformationBox {
    SISOMBaseBox          m_base_box;
    SISOMDataReferenceBox m_data_ref;
  public:
    SISOMDataInformationBox(){}
    ~SISOMDataInformationBox(){}
    AM_STATE write_data_information_box(AMMp4FileWriter* file_writer);
};

struct SISOMSampleTableBox {
    SISOMBaseBox                    m_base_box;
    SISOMDecodingTimeToSampleBox    m_stts;
    SISOMCompositionTimeToSampleBox m_ctts;
    SISOMVisualSampleDescriptionBox m_visual_sample_description;
    SISOMAudioSampleDescriptionBox  m_audio_sample_description;
    SISOMSampleSizeBox              m_sample_size;
    SISOMSampleToChunkBox           m_sample_to_chunk;
    SISOMChunkOffsetBox             m_chunk_offset;
    SISOMSyncSampleTableBox         m_sync_sample;
  public:
    SISOMSampleTableBox(){}
    ~SISOMSampleTableBox(){}
    AM_STATE write_video_sample_table_box(AMMp4FileWriter* file_writer);
    AM_STATE write_audio_sample_table_box(AMMp4FileWriter* file_writer);
    void update_video_sample_table_box_size();
    void update_audio_sample_table_box_size();
};

struct SISOMMediaInformationBox {
  SISOMBaseBox             m_base_box;
  SISOMVideoMediaInformationHeaderBox m_video_information_header;
  SISOMSoundMediaInformationHeaderBox m_sound_information_header;
  SISOMDataInformationBox  m_data_info;
  SISOMSampleTableBox      m_sample_table;
  public:
  SISOMMediaInformationBox(){}
  ~SISOMMediaInformationBox(){}
  AM_STATE write_video_media_information_box(AMMp4FileWriter* file_writer);
  AM_STATE write_audio_media_information_box(AMMp4FileWriter* file_writer);
  void update_video_media_information_box_size();
  void update_audio_media_information_box_size();
};

struct SISOMMediaBox {
  SISOMBaseBox             m_base_box;
  SISOMMediaHeaderBox      m_media_header;
  SISOMHandlerReferenceBox m_media_handler;
  SISOMMediaInformationBox m_media_info;
  public:
  SISOMMediaBox(){}
  ~SISOMMediaBox(){}
  AM_STATE write_video_media_box(AMMp4FileWriter* file_writer);
  AM_STATE write_audio_media_box(AMMp4FileWriter* file_writer);
  void update_video_media_box_size();
  void update_audio_media_box_size();
};

struct SISOMTrackBox {
  SISOMBaseBox        m_base_box;
  SISOMTrackHeaderBox m_track_header;
  SISOMMediaBox       m_media;
  public:
  SISOMTrackBox(){}
  ~SISOMTrackBox(){}
  AM_STATE write_video_movie_track_box(AMMp4FileWriter* file_writer);
  AM_STATE write_audio_movie_track_box(AMMp4FileWriter* file_writer);
  void update_video_movie_track_box_size();
  void update_audio_movie_track_box_size();
};

struct SISOMMovieBox {
  SISOMBaseBox        m_base_box;
  SISOMMovieHeaderBox m_movie_header_box;
  SISOMTrackBox       m_video_track;
  SISOMTrackBox       m_audio_track;
  public:
  SISOMMovieBox(){}
  ~SISOMMovieBox(){};
  AM_STATE write_movie_box(AMMp4FileWriter* file_writer);
  void update_movie_box_size();
};

#endif

