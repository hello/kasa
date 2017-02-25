/*******************************************************************************
 * am_export_if.h
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-21 - [Shupeng Ren] modified file
 *   2016-07-08 - [Guohua Zheng] modified file
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

/*! @file am_export_if.h
 *  @brief Data Export Interface
 */

#ifndef __AM_EXPORT_IF_H__
#define __AM_EXPORT_IF_H__

#include "am_result.h"

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
  AM_EXPORT_PACKET_FORMAT_INVALID         = 0x00, //!< Invalid format

  //For Video
  AM_EXPORT_PACKET_FORMAT_AVC             = 0x01, //!< H.264 Video
  AM_EXPORT_PACKET_FORMAT_HEVC            = 0x02, //!< H.265 Video
  AM_EXPORT_PACKET_FORMAT_MJPEG           = 0x03, //!< MJPEG Video

  //For Audio HZ:sample rate
  AM_EXPORT_PACKET_FORMAT_AAC_8KHZ        = 0x08, //!< AAC ADTS Audio
  AM_EXPORT_PACKET_FORMAT_AAC_16KHZ       = 0x09,
  AM_EXPORT_PACKET_FORMAT_AAC_48KHZ       = 0x0A,

  AM_EXPORT_PACKET_FORMAT_G711MuLaw_8KHZ  = 0x0C, //!< G.711 MuLaw Audio
  AM_EXPORT_PACKET_FORMAT_G711MuLaw_16KHZ = 0x0D,
  AM_EXPORT_PACKET_FORMAT_G711MuLaw_48KHZ = 0x0E,

  AM_EXPORT_PACKET_FORMAT_G711ALaw_8KHZ   = 0x10, //!< G.711 ALaw Audio
  AM_EXPORT_PACKET_FORMAT_G711ALaw_16KHZ  = 0x11,
  AM_EXPORT_PACKET_FORMAT_G711ALaw_48KHZ  = 0x12,

  AM_EXPORT_PACKET_FORMAT_G726_40KBPS     = 0x14, //!< G.726 40kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_32KBPS     = 0x15, //!< G.726 32kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_24KBPS     = 0x16, //!< G.726 24kbps Audio
  AM_EXPORT_PACKET_FORMAT_G726_16KBPS     = 0x17, //!< G.726 16kbps Audio

  AM_EXPORT_PACKET_FORMAT_PCM_8KHZ        = 0x20, //!< S16LE PCM Audio
  AM_EXPORT_PACKET_FORMAT_PCM_16KHZ       = 0x21,
  AM_EXPORT_PACKET_FORMAT_PCM_48KHZ       = 0x22,

  AM_EXPORT_PACKET_FORMAT_OPUS_8KHZ       = 0x24, //!< OPUS Audio
  AM_EXPORT_PACKET_FORMAT_OPUS_16KHZ      = 0x25,
  AM_EXPORT_PACKET_FORMAT_OPUS_48KHZ      = 0x26,

  AM_EXPORT_PACKET_FORMAT_BPCM_8KHZ       = 0x28, //!< S16BE PCM Audio
  AM_EXPORT_PACKET_FORMAT_BPCM_16KHZ      = 0x29,
  AM_EXPORT_PACKET_FORMAT_BPCM_48KHZ      = 0x2A,

  AM_EXPORT_PACKET_FORMAT_SPEEX_8KHZ      = 0x2C, //!< SPEEX Audio
  AM_EXPORT_PACKET_FORMAT_SPEEX_16KHZ     = 0x2D,
  AM_EXPORT_PACKET_FORMAT_SPEEX_48KHZ     = 0x2E,
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
    uint16_t reserved1; //!< Reserved Bytes
    uint8_t  channels; //!< Audio Channels
    uint8_t  sample_size; //!< Audio Sample Size
};

/*! @struct AMExportPacket
 *  @brief Export Data Packet
 */
struct AMExportPacket {

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

    /*! Audio Sample Rate
     * The measure unit of audio sample rate is kHz
     */
    uint8_t     audio_sample_rate;

    /*! Used for Video, if this is a IDR frame
     * - 0: IPB Frame,
     * - 1: IDR Frame
     */
    uint8_t     is_key_frame;

    /*! Ignore these members for users
     */
    uint8_t     is_direct_mode;
    uint8_t     user_alloc_memory;
    uint8_t     reserved1;
};

/*! @struct AMExportConfig
 *  @brief Export Muxer configuration
 */
struct AMExportConfig
{
    /*! Specify the type of audio to export by bits mask.
     *@sa enum AM_EXPORT_PACKET_FORMAT
     */
    uint64_t audio_map;

    /*! Specify the type of video to export by bits mask.
     *@sa enum AM_EXPORT_PACKET_FORMAT
     */
    uint32_t video_map;

    /*! Config if need to sort video and audio packets by PTS.
     *  - 0: no sort
     *  - 1: sort
     */
    uint32_t need_sort;
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
    virtual AM_RESULT connect_server(const char *url)    = 0;

    /*! Disconnect to export server
     *  @sa connect_server()
     */
    virtual void disconnect_server()                = 0;

    /*! Receive one packet from export server
     *  @param packet @ref AMExportPacket pointer
     *  @return true if success, otherwise return false
     *  @sa release()
     */
    virtual AM_RESULT receive(AMExportPacket *packet)    = 0;

    /*! Set config to export clients
     *  @return true if success, otherwise return false
     *  @sa set_config
     */
    virtual AM_RESULT set_config(AMExportConfig *config) = 0;

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
