/*******************************************************************************
 * am_export_if.h
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-21 - [Shupeng Ren] modified file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

/*! @file am_export_if.h
 *  @brief Data Export Interface
 */

#ifndef __AM_EXPORT_IF_H__
#define __AM_EXPORT_IF_H__

/*! Export Unix Domain Socket File Path */
#define DEXPORT_PATH ("/run/oryx/export.socket")

/*! @enum AM_EXPORT_PACKET_TYPE
 *  @brief Packet types indicate which packet you have received,
 *         video or audio, info or data.
 */
enum AM_EXPORT_PACKET_TYPE {
  AM_EXPORT_PACKET_TYPE_INVALID = 0x00, //!< Invalid Packet
  AM_EXPORT_PACKET_TYPE_VIDEO_DATA,     //!< Video Data Packet
  AM_EXPORT_PACKET_TYPE_AUDIO_DATA,     //!< Audio Data Packet
  AM_EXPORT_PACKET_TYPE_VIDEO_INFO,     //!< Video Info Packet
  AM_EXPORT_PACKET_TYPE_AUDIO_INFO,     //!< Audio Info Packet
};

/*! @enum AM_EXPORT_PACKET_FORMAT
 *  @brief Packet format types indicate which kind of data is in the packet
 */
enum AM_EXPORT_PACKET_FORMAT {
  AM_EXPORT_PACKET_FORMAT_INVALID     = 0x00, //!< Invalid format

  //For Video
  AM_EXPORT_PACKET_FORMAT_AVC         = 0x01, //!< H.264 Video
  AM_EXPORT_PACKET_FORMAT_HEVC        = 0x02, //!< H.265 Video
  AM_EXPORT_PACKET_FORMAT_MJPEG       = 0x03, //!< MJPEG Video

  //For Audio
  AM_EXPORT_PACKET_FORMAT_AAC         = 0x10, //!< AAC ADTS Audio
  AM_EXPORT_PACKET_FORMAT_G711MuLaw   = 0x11, //!< G.711 MuLaw Audio
  AM_EXPORT_PACKET_FORMAT_G711ALaw    = 0x12, //!< G.711 ALaw Audio
  AM_EXPORT_PACKET_FORMAT_G726_40     = 0x13, //!< G.726 40kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_32     = 0x14, //!< G.726 32kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_24     = 0x15, //!< G.726 24kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_16     = 0x16, //!< G.726 16kbps Audio
  AM_EXPORT_PACKET_FORMAT_PCM         = 0x17, //!< S16LE PCM Audio
  AM_EXPORT_PACKET_FORMAT_OPUS        = 0x18, //!< OPUS Audio
  AM_EXPORT_PACKET_FORMAT_BPCM        = 0x19, //!< S16BE PCM Audio
  AM_EXPORT_PACKET_FORMAT_SPEEX       = 0x1A, //!< SPEEX Audio
};

/*! @enum AM_EXPORT_VIDEO_FRAME_TYPE
 *  @brief Frame type just used for video packet
 */
enum AM_EXPORT_VIDEO_FRAME_TYPE {
  AM_EXPORT_VIDEO_FRAME_TYPE_INVALID  = 0x00, //!< Invalid Frame Type
  AM_EXPORT_VIDEO_FRAME_TYPE_IDR      = 0x01, //!< IDR Frame
  AM_EXPORT_VIDEO_FRAME_TYPE_I        = 0x02, //!< I Frame
  AM_EXPORT_VIDEO_FRAME_TYPE_P        = 0x03, //!< P Frame
  AM_EXPORT_VIDEO_FRAME_TYPE_B        = 0x04, //!< B Frame
  AM_EXPORT_VIDEO_FRAME_TYPE_PSKIP    = 0x05, //!< P Skip Frame
};

/*! @struct AMExportVideoInfo
 *  @brief Video Information
 */
struct AMExportVideoInfo {
    uint32_t width; //!< Video Width
    uint32_t height; //!< Video Height
    uint32_t framerate_num; //!< Numerator of Frame Factor
    uint32_t framerate_den; //!< Denominator of Frame Factor
};

/*! @struct AMExportAudioInfo
 *  @brief Audio Information
 */
struct AMExportAudioInfo {
    uint32_t samplerate; //!< Audio Samplerate
    uint32_t frame_size; //!< Audio Frame Size, number of bytes
    uint32_t bitrate; //!< Audio Bitrate

    uint32_t pts_increment; //!< PTS duration of a audio packet
    uint8_t  channels; //!< Audio Channels
    uint8_t  sample_size; //!< Audio Sample Size
    uint16_t reserved1; //!< Reserved Bytes
};

/*! @struct AMExportPacket
 *  @brief Export Data Packet
 */
struct AMExportPacket {
    /*! Video or Audio Stream Index
     */
    uint8_t     stream_index;
    /*! Export Packet Type
     *  @sa enum AM_EXPORT_PACKET_TYPE
     */
    uint8_t     packet_type;

    /*! Packet Data format
     *  @sa enum AM_EXPORT_PACKET_FORMAT
     */
    uint8_t     packet_format;

    /*! Video Frame Type
     *  @sa enum AM_EXPORT_VIDEO_FRAME_TYPE
     */
    uint8_t     frame_type;

    /*! Used for Video, if this is a IDR frame
     * - 0: IPB Frame,
     * - 1: IDR Frame
     */
    uint8_t     is_key_frame;

    /*! Ignore this member for users
     */
    uint8_t     is_direct_mode;
    uint8_t     user_alloc_memory;
    uint8_t     reserved1;

    /*! Packet PTS
     */
    uint64_t    pts;
    uint64_t    seq_num;

    /*! Raw Data Pointer
     *
     * if (packet_type == AM_EXPORT_PACKET_TYPE_(VIDEO/AUDIO)_INFO),
     * then you can get:
     *
     *    AMExportVideoInfo or AMExportAudioInfo
     *
     * from (uint8_t*)data_ptr.
     *
     * if (packet_type == AM_EXPORT_PACKET_TYPE_(VIDEO/AUDIO)_DATA),
     * then you can get data_size bytes:
     *
     *    Video or Audio raw data
     *
     * from (uint8_t*)data_ptr.
     *
     * Video/Audio INFO will be received before Video/Audio DATA
     * @sa data_size
     * @sa AMExportVideoInfo
     * @sa AMExportAudioInfo
     */
    uint8_t    *data_ptr;

    /*! Raw Data Size
     *  @sa data_ptr
     */
    uint32_t    data_size;
};

/*! @class AMIExportClient
 *  @brief Export Client Interface
 */
class AMIExportClient
{
  public:
    /*! Connect to export server if it has already run
     *  @param url C style string indicates export server
     *         UNIX domain socket path, the default path is
     *         @ref DEXPORT_PATH
     *  @return true if success, otherwise return false
     *  @sa disconnect_server()
     *  @sa DEXPORT_PATH
     */
    virtual bool connect_server(const char *url)    = 0;

    /*! Disconnect to export server
     *  @sa connect_server()
     */
    virtual void disconnect_server()                = 0;

    /*! Receive one packet from export server
     *  @param packet @ref AMExportPacket pointer
     *  @return true if success, otherwise return false
     *  @sa release()
     */
    virtual bool receive(AMExportPacket *packet)    = 0;

    /*! Release one packet if you have processed it
     *  @param packet @ref AMExportPacket pointer
     *  @sa receive()
     */
    virtual void release(AMExportPacket *packet)    = 0;

  public:
    /*! Destroy this
     *  @sa am_create_export_client()
     */
    virtual void destroy()                          = 0;
  protected:
    /*! Destructor
     */
    virtual ~AMIExportClient() {}
};

/*! @enum AM_EXPORT_CLIENT_TYPE
 *  @brief export's client type
 */
enum AM_EXPORT_CLIENT_TYPE
{
  AM_EXPORT_TYPE_INVALID            = 0x00, //!< Invalid Client Type

  /*! This client uses UNIX Domain Socket
   *  to transfer data with data export agent
   */
  AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET = 0x01,
};

/*! @struct AMExportConfig
 *  @brief Export Muxer configuration
 */
struct AMExportConfig
{
    /*! Config if need to sort video and audio packets by PTS.
     *  - 0: no sort
     *  - 1: sort
     */
    uint8_t need_sort;
};

/*!
 * Create export client by type
 * @param type client type defined by @ref AM_EXPORT_CLIENT_TYPE
 * @param config Export muxer config defined by @ref AMExportConfig
 * @return AMIExportClient* if success, otherwise return null
 */
extern AMIExportClient* am_create_export_client(AM_EXPORT_CLIENT_TYPE type,
                                                AMExportConfig *config);

/*! @example test_oryx_data_export.cpp
 *  Test program of AMIExportClient.
 */
#endif
