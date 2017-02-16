/*
 * am_api_video.h
 *
 *  History:
 *    Nov 18, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_api_video.h
 *  @brief This file defines Oryx Video Services related data structures
 */
#ifndef _AM_API_VIDEO_H_
#define _AM_API_VIDEO_H_

#include "commands/am_api_cmd_video.h"

// Setting item masks
/*! @defgroup airapi-datastructure-video Data Structure of Video Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Video Service related method call data structures
 *  @{
 */

/*
 * VIN
 */
/******************************VIN Parameters**********************************/
/*! @defgroup airapi-datastructure-video-vin VIN Parameters
 *  @ingroup airapi-datastructure-video
 *  @brief VIN related parameters,
 *         refer to @ref airapi-commandid-video-vin "VIN Commands" to see
 *         related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_VIN_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_VIN_SET
 *  @{
 *//* Start of VIN Parameter Section */

/*! @enum AM_VIN_CONFIG_BITS
 *  @brief VIN configuration type to indicate which config needs to be modified
 */
enum AM_VIN_CONFIG_BITS
{
  AM_VIN_CONFIG_WIDTH_HEIGHT_EN_BIT  = 0,//!< Change VIN Width and Height
  AM_VIN_CONFIG_FLIP_EN_BIT          = 1,//!< Change VIN Flip config
  AM_VIN_CONFIG_HDR_MODE_EN_BIT      = 2,//!< Change VIN HDR mode
  AM_VIN_CONFIG_FPS_EN_BIT           = 3,//!< Change VIN FPS config
  AM_VIN_CONFIG_BAYER_PATTERN_EN_BIT = 4,//!< Change VIN bayer pattern
};

/*! @struct am_vin_config_s
 *  @brief For VIN config file GET and SET
 */
/*! @typedef am_vin_config_t
 *  @brief For VIN config file GET and SET
 */
typedef struct am_vin_config_s
{
    /*!
     * @sa AM_VIN_CONFIG_BITS
     */
    uint32_t enable_bits; //!< AM_VIN_CONFIG_BITS

    uint32_t vin_id; //!< VIN ID

    uint32_t width; //!< VIN Width
    uint32_t height; //!< VIN Height

    /*!
     * - 0:   not flip
     * - 1:   flipv
     * - 2:   fliph
     * - 3:   flip both V & h
     * - 255: auto
     */
    uint8_t flip; //!< VIN flip

    /*!
     * - 0: single exposure, no HDR
     * - 1: 2x hdr exposure
     * - 2: 3x hdr exposure
     * - 3: 4x hdr exposure
     */
    uint8_t hdr_mode; //!< VIN HDR Mode
    uint16_t reserved0; //!< Reserved space to keep align

    /*!
     * - 0:     auto
     * - x:     x
     * - 1000:   29.97
     * - 1001:   59.94
     * - 1002:   23.976
     * - 1003:   12.5
     * - 1004:   6.25
     * - 1005:   3.125
     * - 1006:   7.5
     * - 1007:   3.75
     */
    uint16_t fps; //!< VIN FPS

    /*!
     * - 0: auto
     * - 1: RG
     * - 2: BH
     * - 3: GR
     * - 4: GB
     */
    uint8_t bayer_pattern; //!< VIN bayer pattern
    uint8_t reserved1; //!< Reserved space to keep align
} am_vin_config_t;

/*! @struct am_vin_info_s
 *  @brief For dynamically GET VIN info
 */
/*! @typedef am_vin_info_t
 *  @brief For dynamically GET VIN info
 */
typedef struct am_vin_info_s {
    uint32_t  width;  //!< VIN width
    uint32_t  height; //!< VIN height
    uint32_t  vin_id; //!< VIN ID

    /*!
     * - 0:     auto
     * - x:     x
     * - 1000:   29.97
     * - 1001:   59.94
     * - 1002:   23.976
     * - 1003:   12.5
     * - 1004:   6.25
     * - 1005:   3.125
     * - 1006:   7.5
     * - 1007:   3.75
     */
    uint16_t   max_fps; //!< VIN fps
    uint16_t   fps;     //!< The same as max_fps

    /*!
     * - 0: auto
     * - 1: Progressive
     * - 2: Interlace
     */
    uint8_t   format; //!< VIN format

    /*!
     * - 0: AUTO
     * - 1: YUV BT601
     * - 2: YUV BT656
     * - 3: YUV BT1120
     * - 4: RGB BT601
     * - 5: RGB BT656
     * - 6: RGB RAW
     * - 7: RGB BT1120
     */
    uint8_t   type;  //!< VIN type

    /*!
     * - 0: auto
     * - X: X bits
     */
    uint8_t   bits; //!< VIN bits

    /*!
     * - 0: auto
     * - 1: 4:3
     * - 2: 16:9
     */
    uint8_t   ratio; //!< VIN aspect ratio

    /*!
     * - 0: auto
     * - 1: NTSC
     * - 2: PAL
     * - 3: SECAM
     * - 4: ALL
     */
    uint8_t   system; //!< VIN system

    /*!
     * - 0: none
     * - 1: auto
     * - 2: FLIP_V
     * - 3: FLIP_H
     * - 4: FLIP both V & H
     */
    uint8_t   flip; //!< VIN flip type

    /*!
     * - 0: linear mode
     * - 1: 2x
     * - 2: 3x
     * - 3: 4x
     */
    uint8_t   hdr_mode; //!< VIN HDR mode
    uint8_t   reserved; //!< Reserved place to keep align
} am_vin_info_t;

/*! @} */ /* End of VIN Parameters */
/******************************************************************************/

/*
 * Stream Format
 */
/***************************Stream Format Parameters***************************/
/*! @defgroup airapi-datastructure-video-stream-fmt Stream Format Parameters
 *  @ingroup airapi-datastructure-video
 *  @brief Stream format related parameters,
 *         refer to @ref airapi-commandid-video-stream-fmt
 *         "Stream Format Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET
 *  @{
 */ /* Start of Stream Format Parameters */

/*! @enum AM_STREAM_FMT_BITS
 *  @brief Stream format type to indicate which config needs to be modified
 */
enum AM_STREAM_FMT_BITS {
  AM_STREAM_FMT_ENABLE_EN_BIT       = 0,  //!< Bit0
  AM_STREAM_FMT_TYPE_EN_BIT         = 1,  //!< Bit1
  AM_STREAM_FMT_SOURCE_EN_BIT       = 2,  //!< Bit2
  AM_STREAM_FMT_FRAME_NUM_EN_BIT    = 3,  //!< Bit3
  AM_STREAM_FMT_FRAME_DEN_EN_BIT    = 4,  //!< Bit4
  AM_STREAM_FMT_WIDTH_EN_BIT        = 5,  //!< Bit5
  AM_STREAM_FMT_HEIGHT_EN_BIT       = 6,  //!< Bit6
  AM_STREAM_FMT_OFFSET_X_EN_BIT     = 7,  //!< Bit7
  AM_STREAM_FMT_OFFSET_Y_EN_BIT     = 8,  //!< Bit8
  AM_STREAM_FMT_HFLIP_EN_BIT        = 9,  //!< Bit9
  AM_STREAM_FMT_VFLIP_EN_BIT        = 10, //!< Bit10
  AM_STREAM_FMT_ROTATE_EN_BIT       = 11, //!< Bit11
};

/*! @struct am_stream_fmt_s
 *  @brief For stream format file GET and SET
 */
/*! @typedef am_stream_fmt_t
 *  @brief For stream format file GET and SET
 */
typedef struct am_stream_fmt_s {
    /*!
     * @sa AM_STREAM_FMT_BITS
     */
    uint32_t  enable_bits;
    uint32_t  stream_id;

    /*!
     * - 0: Disable
     * - 1: Enable
     */
    uint32_t  enable;

    /*!
     * - 0: None
     * - 1: H264
     * - 2: Mjpeg
     */
    uint32_t  type;

    /*!
     * - 0: Main buffer
     * - 1: 2nd buffer
     * - 2: 3rd buffer
     * - 3: 4th buffer
     */
    uint32_t  source;

    /*!
     * Frame factor: frame_fact_num/frame_fact_den, for example 1/2, 1/1
     * frame_fact_num must be no bigger than frame_fact_den
     */
    uint32_t  frame_fact_num;

    /*!
     * Frame factor: frame_fact_num/frame_fact_den, for example 1/2, 1/1
     * frame_fact_num must be no bigger than frame_fact_den
     */
    uint32_t  frame_fact_den;

    /*!
     * Video width.
     * multiple of 16, -1 for auto configure
     */
    uint32_t  width;

    /*!
     * Video height.
     * multiple of 8, -1 for auto configure
     */
    uint32_t  height;

    /*!
     * Video offset x.
     * multiple of 4
     */
    uint32_t  offset_x;

    /*!
     * Video offset y.
     * multiple of 4
     */
    uint32_t  offset_y;

    /*! horizontal flip.
     * - 0: disable hflip
     * - 1: enable hflip
     */
    uint32_t  hflip;

    /*! vertical flip.
     * - 0: disable vflip
     * - 1: enable vflip
     */
    uint32_t  vflip;

    /*!
     * - 0: disable rotate
     * - 1: enable rotate
     */
    uint32_t  rotate;
} am_stream_fmt_t;

/*! @} */ /* End of Stream Format Paramaters */
/******************************************************************************/

/*
 * Stream Config
 */
/********************************Stream Config*********************************/
/*! @defgroup airapi-datastructure-video-stream-cfg Stream Config Parameters
 *  @ingroup airapi-datastructure-video
 *  @brief Stream config related parameters,
 *         refer to @ref airapi-commandid-video-stream-cfg
 *         "Stream Config Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET
 *  @{
 */ /* Start of Stream Config Parameters */

/*! @enum AM_H264_CFG_BITS
 *  @brief H264 config type to indicate which config needs to be modified
 */
enum AM_H264_CFG_BITS {
  AM_H264_CFG_BITRATE_CTRL_EN_BIT     = 0,
  AM_H264_CFG_PROFILE_EN_BIT          = 1,
  AM_H264_CFG_AU_TYPE_EN_BIT          = 2,
  AM_H264_CFG_CHROMA_EN_BIT           = 3,
  AM_H264_CFG_M_EN_BIT                = 4,
  AM_H264_CFG_N_EN_BIT                = 5,
  AM_H264_CFG_IDR_EN_BIT              = 6,
  AM_H264_CFG_BITRATE_EN_BIT          = 7,
  AM_H264_CFG_MV_THRESHOLD_EN_BIT     = 8,
  AM_H264_CFG_ENC_IMPROVE_EN_BIT      = 9,
  AM_H264_CFG_MULTI_REF_P_EN_BIT      = 10,
  AM_H264_CFG_LONG_TERM_INTVL_EN_BIT  = 11,
};

/*! @struct am_h264_cfg_s
 *  @brief For h264 config file GET and SET
 */
/*! @typedef am_h264_cfg_t
 *  @brief For h264 config file GET and SET
 */
typedef struct am_h264_cfg_s {
    /*!
     * @sa AM_H264_CFG_BITS
     */
    uint32_t    enable_bits;

    /*!
     * - 0: CBR
     * - 1: VBR
     * - 2: CBR QUALITY
     * - 3: VBR QUALITY
     * - 4: CBR2
     * - 5: VBR2
     * - 6: LBR
     */
    uint32_t    bitrate_ctrl;

    /*!
     * - 0: Baseline
     * - 1: Main
     * - 2: High
     */
    uint32_t    profile;

    /*!
     * - 0: NO_AUD_NO_SEI
     * - 1: AUD_BEFORE_SPS_WITH_SEI
     * - 2: AUD_AFTER_SPS_WITH_SEI
     * - 3: NO_AUD_WITH_SEI
     */
    uint32_t    au_type;

    /*!
     * - 0: 420
     * - 1: 422
     * - 2: Mono
     */
    uint32_t    chroma;

    /*!
     * 1 ~ 3
     */
    uint32_t    M;

    /*!
     * 15 ~ 1800
     */
    uint32_t    N;

    /*!
     * 1 ~ 4
     */
    uint32_t    idr_interval;

    /*!
     * bit per second@30FPS
     * 6400 ~ 12000000
     */
    uint32_t    target_bitrate;

    /*!
     * 0 ~ 64
     */
    uint32_t    mv_threshold;

    /*! encode improve
     * - 1: enable
     * - 0: disable
     */
    uint32_t    encode_improve;

    /*!
     * enable multi-reference P frame
     *
     * - 1: enable
     * - 0: disable
     */
    uint32_t    multi_ref_p;

    /*!
     * specify long term reference P frame interval
     *
     * 0 ~ 63
     */
    uint32_t    long_term_intvl;
} am_h264_cfg_t;

/*! @enum AM_MJPEG_CFG_BITS
 *  @brief MJPEG config type to indicate which config needs to be modified
 */
enum AM_MJPEG_CFG_BITS {
  AM_MJPEG_CFG_QUALITY_EN_BIT    = 0,
  AM_MJPEG_CFG_CHROMA_EN_BIT     = 1,
};

/*! @struct am_mjpeg_cfg_s
 *  @brief For mjpeg config file GET and SET
 */
/*! @typedef am_mjpeg_cfg_t
 *  @brief For mjpeg config file GET and SET
 */
typedef struct am_mjpeg_cfg_s {
    /*!
     * @sa AM_MJPEG_CFG_BITS
     */
    uint32_t    enable_bits;

    /*!
     * 1 ~ 99
     */
    uint32_t    quality;

    /*!
     * - 0: 420
     * - 1: 422
     * - 2: Mono
     */
    uint32_t    chroma;
} am_mjpeg_cfg_t;

/*! @enum AM_STREAM_CFG_BITS */
enum AM_STREAM_CFG_BITS
{
  AM_STREAM_CFG_H264_EN_BIT = 0, //!< Config stream to H.264
  AM_STREAM_CFG_MJPEG_EN_BIT = 1, //!< Config stream to MJPEG
};

/*! @struct am_stream_cfg_s
 *  @brief For stream config file GET and SET
 */
/*! @typedef am_stream_cfg_t
 *  @brief For stream config file GET and SET
 */
typedef struct am_stream_cfg_s {
    /*!
     * @sa AM_STREAM_CFG_BITS
     */
    uint32_t        enable_bits;

    uint32_t        stream_id;

    /*!
     * @sa am_h264_cfg_s
     */
    am_h264_cfg_t   h264;

    /*!
     * @sa am_mjpeg_cfg_s
     */
    am_mjpeg_cfg_t  mjpeg;
} am_stream_cfg_t;

/*! @} */ /* End of Stream Config Parameters */
/******************************************************************************/

/*
 * Source buffer
 */
/******************************Source Buffer Format****************************/
/*! @defgroup airapi-datastructure-video-src-buf-fmt Source Buffer Format
 *  @ingroup airapi-datastructure-video
 *  @brief Source buffer format related parameters,
 *         refer to @ref airapi-commandid-video-src-buf-fmt
 *         "Source Buffer Format Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET
 *  @{
 */ /* Start of Source Buffer Format */

/*! @enum AM_BUFFER_FMT_BITS
 *  @brief Buffer format type to indicate which config needs to be modified
 */
enum AM_BUFFER_FMT_BITS {
  AM_BUFFER_FMT_TYPE_EN_BIT           = 0,
  AM_BUFFER_FMT_INPUT_CROP_EN_BIT     = 1,
  AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT    = 2,
  AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT   = 3,
  AM_BUFFER_FMT_INPUT_X_EN_BIT        = 4,
  AM_BUFFER_FMT_INPUT_Y_EN_BIT        = 5,
  AM_BUFFER_FMT_WIDTH_EN_BIT          = 6,
  AM_BUFFER_FMT_HEIGHT_EN_BIT         = 7,
  AM_BUFFER_FMT_PREWARP_EN_BIT        = 8,
};

/*! @struct am_buffer_fmt_s
 *  @brief For buffer format file GET and SET
 */
/*! @typedef am_buffer_fmt_t
 *  @brief For buffer format file GET and SET
 */
typedef struct am_buffer_fmt_s {
    /*!
     * @sa AM_BUFFER_FMT_BITS
     */
    uint32_t    enable_bits;

    /*!
     * - 0: Main buffer
     * - 1: 2nd buffer
     * - 2: 3rd buffer
     * - 3: 4th buffer
     */
    uint32_t    buffer_id;

    /*!
     * - 0: off
     * - 1: encode
     * - 2: preview
     */
    uint32_t    type;

    /*!
     * - 0: false
     * - 1: true
     */
    uint32_t    input_crop;

    /*!
     * input window:
     * {input_width, input_height, input_offset_x, input_offset_y}
     * Only use in input_crop is true
     */
    uint32_t    input_width;

    /*!
     * input window:
     * {input_width, input_height, input_offset_x, input_offset_y}
     * Only use in input_crop is true
     */
    uint32_t    input_height;

    /*!
     * input window:
     * {input_width, input_height, input_offset_x, input_offset_y}
     * Only use in input_crop is true
     */
    uint32_t    input_offset_x;

    /*!
     * input window:
     * {input_width, input_height, input_offset_x, input_offset_y}
     * Only use in input_crop is true
     */
    uint32_t    input_offset_y;

    /*!
     * buffer size: (width, height)
     */
    uint32_t    width;

    /*!
     * buffer size: (width, height)
     */
    uint32_t    height;

    /*!
     * - 0: false
     * - 1: true
     */
    uint32_t    prewarp;
} am_buffer_fmt_t;
/*! @} */ /* End of Source Buffer Format */
/******************************************************************************/

/*
 * Source buffer allocate style
 */
/*****************************Buffer Allocation Style**************************/
/*! @defgroup airapi-datastructure-video-src-buf-style Buffer Allocation Style
 *  @ingroup airapi-datastructure-video
 *  @brief Buffer allocation style related parameters,
 *         refer to @ref airapi-commandid-video-buf-style
 *         "Buffer Allocation Style Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET
 *  @{
 */ /* Start of Buffer Allocation Style */

/*! @struct am_buffer_alloc_style_s
 *  @brief For buffer allocation style GET and SET
 */
/*! @typedef am_buffer_alloc_style_t
 *  @brief For buffer allocation style GET and SET
 */
typedef struct am_buffer_alloc_style_s {
    /*!
     * - 0: manual
     * - 1: auto
     */
    uint32_t    alloc_style;
} am_buffer_alloc_style_t;

/*! @struct am_stream_status_s
 *  @brief For stream status GET
 */
/*! @typedef am_stream_status_t
 *  @brief For stream status GET
 */
typedef struct am_stream_status_s {
    /*!
     * bit OR'ed of all streams,
     * stream[0] is at bit 0, stream[x] is at bit x,
     * bit = 1 means stream is encoding,
     * otherwise it's not encoding.
     */
    uint32_t status;
} am_stream_status_t;
/*! @} */ /* End of Buffer Allocation Style */
/******************************************************************************/

/*
 * DPTZ Warp
 */
/********************************DPTZ Warp*************************************/
/*! @defgroup airapi-datastructure-video-dptz_warp DPTZ Warp
 *  @ingroup airapi-datastruture-video
 *  @brief DPTZ Warp related parameters used,
 *         refer to @ref airapi-commandid-video-dptz_warp "DPTZ Warp Commands"
 *         to see related commands.
 *  @sa AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET
 *  @{
 */ /* Start of DPTZ Warp */

/*! @enum AM_DPTZ_WARP_BITS
 *  @brief DPTZ Warp type to indicate which configure needs to be modified
 */
enum AM_DPTZ_WARP_BITS {
  AM_DPTZ_WARP_LDC_STRENGTH_EN_BIT = 0,
  AM_DPTZ_WARP_PAN_RATIO_EN_BIT    = 1,
  AM_DPTZ_WARP_TILT_RATIO_EN_BIT   = 2,
  AM_DPTZ_WARP_ZOOM_RATIO_EN_BIT   = 3,
  AM_DPTZ_WARP_PANO_HFOV_DEGREE_EN_BIT = 4
};

/*! @struct am_dptz_warp_s
 *  @brief For DPTZ Warp GET and SET
 */

/*! @typedef am_dptz_warp_t
 *  @brief For DPTZ Warp GET and SET
 */
typedef struct am_dptz_warp_s {
    /*!
     * @sa AM_DPTZ_WARP_BITS
     */
    uint32_t enable_bits;
    /*!
     * 0~3, 0: main buffer (DPTZ-I),  1-3: sub buffer (DPTZ-II)
     * */
    uint32_t buf_id;
    /*!
     * -1.0~1.0, -1.0: left most,  1.0: right most
     * */
    float pan_ratio;
    /*!
     * -1.0~1.0, -1.0: top most,  1.0: bottom most
     * */
    float tilt_ratio;
    /*!
     * 0.1~8.0, 0.1: zoom in most, 1: original, 8.0: zoom out most
     */
    float zoom_ratio;
    /*!
     * 0.0~20.0, 0.0: do not do lens distortion calibration,
     *  20: maximum lens distortion calibration
     */
    float ldc_strenght;
    /*!
     * 1.0~180.0, only for panorama mode
     */
    float pano_hfov_degree;
} am_dptz_warp_t;

/*! @} */ /* End of DPTZ Warp */
/******************************************************************************/

/*
 * H.264 encode low bite rate control
 */
/********************************H264 LBR Control******************************/
/*! @defgroup airapi-datastructure-video-lbr H.264 LBR Control
 *  @ingroup airapi-datastructure-video
 *  @brief H.264 LBR control related parameters used,
 *         refer to @ref airapi-commandid-video-lbr "H.264 LBR Control Commands"
 *         to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET
 *  @{
 */ /* Start of H264 LBR Control */

/*! @enum AM_ENCODE_H264_LBR_CTRL_BITS
 *  @brief H.264 LBR control type to indicate which config needs to be modified
 */
enum AM_ENCODE_H264_LBR_CTRL_BITS {
  AM_ENCODE_H264_LBR_ENABLE_LBR_EN_BIT              = 0,
  AM_ENCODE_H264_LBR_AUTO_BITRATE_CEILING_EN_BIT    = 1,
  AM_ENCODE_H264_LBR_BITRATE_CEILING_EN_BIT         = 2,
  AM_ENCODE_H264_LBR_DROP_FRAME_EN_BIT              = 3,
};

/*! @struct am_encode_h264_lbr_ctrl_s
 *  @brief For h264 lbr control GET and SET
 */
/*! @typedef am_encode_h264_lbr_ctrl_t
 *  @brief For h264 lbr control GET and SET
 */
typedef struct am_encode_h264_lbr_ctrl_s {
    /*!
     * @sa AM_ENCODE_H264_LBR_CTRL_BITS
     */
    uint32_t enable_bits;
    uint32_t stream_id;

    /*!
     * - 0: disable LBR and use CBR
     * - 1: enable LBR
     */
    bool enable_lbr;

    /*!
     * - false: close auto bitrate ceiling and need to set manually
     * - true: auto bitrate ceiling, can not set bitrate ceiling now.
     */
    bool auto_bitrate_ceiling;

    /*!
     * when auto_bitrate_seiling is false, you can set it.
     */
    uint32_t bitrate_ceiling;

    /*!
     * - false: do not drop frame when motion is large
     * - true: drop frame auto when motion is large
     */
    bool drop_frame;
} am_encode_h264_lbr_ctrl_t;
/*! @} */ /* End of H264 LBR Control */
/******************************************************************************/

/*
 * Overlay
 */
/************************************Overlay**********************************/
/*! @defgroup airapi-datastructure-video-overlay
 *  @ingroup airapi-datastruture-video
 *  @brief Overlay related parameters used,
 *         refer to @ref airapi-commandid-video-overlay "Overlay Commands"
 *         to see related commands.
 *  @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 *  @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 *  @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 *  @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 *  @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 *  @{
 */ /* Start of OVERLAY Control */

/*! @enum AM_OVERLAY_BITS
 *  @brief OVERLAY control type to indicate which config needs to be modified
 */
enum AM_OVERLAY_BITS {
  AM_REMOVE_EN_BIT = 0,           //!< remove overlay
  AM_ENABLE_EN_BIT = 1,           //!< enable overlay
  AM_DISABLE_EN_BIT = 2,          //!< disable overlay
  AM_ADD_EN_BIT = 3,              //!< add overlay
  AM_WIDTH_EN_BIT = 4,            //!< overlay area width for add
  AM_HEIGHT_EN_BIT = 5,           //!< overlay area height for add
  AM_LAYOUT_EN_BIT = 6,           //!< overlay area layout for add
  AM_OFFSETX_EN_BIT = 7,          //!< overlay area start x for add
  AM_OFFSETY_EN_BIT = 8,          //!< overlay area start y for add
  AM_TYPE_EN_BIT = 9,             //!< overlay area overlay type for add
  AM_ROTATE_EN_BIT = 10,          //!< overlay area rotate attribute for add
  AM_FONT_TYPE_EN_BIT = 11,       //!< font type if overlay type is string for add
  AM_FONT_SIZE_EN_BIT = 12,       //!< font size if overlay type is string for add
  AM_FONT_COLOR_EN_BIT = 13,      //!< font color if overlay type is string for add
  AM_FONT_OUTLINE_W_EN_BIT = 14,  //!< font outline width if overlay type is string for add
  AM_FONT_HOR_BOLD_EN_BIT = 15,   //!< font hor_bold if overlay type is string for add
  AM_FONT_VER_BOLD_EN_BIT = 16,   //!< font ver_bold if overlay type is string for add
  AM_FONT_ITALIC_EN_BIT = 17,     //!< font italic if overlay type is string for add
  AM_STRING_EN_BIT = 18,          //!< string if overlay type is string for add
  AM_COLOR_KEY_EN_BIT = 19,       //!< color used to transparent if overlay type is picture for add
  AM_COLOR_RANGE_EN_BIT = 20,     //!< color range used to transparent with color key for add
  AM_BMP_EN_BIT = 21,             //!< bmp file path if overlay type is picture for add
};


/*! @macros OSD_MAX_STRING
 *  @brief OVERLAY max char number which overlay type is string
 */
#define OSD_MAX_STRING    (256)  //!<just for test, if it's big size, the total size maybe large than
                                //!<AM_MAX_IPC_MESSAGE_SIZE which may cause AIR API call failed

/*! @macros OSD_MAX_FILENAME
  *  @brief OVERLAY max size number of bmp file and font file full name
 */
#define OSD_MAX_FILENAME  (128)  //!<just for test, if it's big size, the total size maybe large than
                                //!<AM_MAX_IPC_MESSAGE_SIZE which may cause AIR API call failed

/*! @struct am_overlay_id_s
 *  @brief For overlay Get input parameter and SET
 */
/*! @typedef am_overlay_id_t
 *  @brief For overlay Get input parameter and SET
 */
typedef struct am_overlay_id_s {
    uint32_t stream_id;
    uint32_t area_id;
} am_overlay_id_t;

/*! @struct am_overlay_set_s
 *  @brief For Overlay SET
 */
/*! @typedef am_overlay_set_t
 *  @brief For Overlay SET
 */
typedef struct am_overlay_set_s {
    /*!
     * @sa AM_MANIPULATE_BITS
     */
    uint32_t enable_bits;

    /*!
     * overlay area to manipulate
     */
    am_overlay_id_t osd_id;

} am_overlay_set_t;

/*! @struct am_overlay_area_s
 *  @brief For overlay GET and ADD
 */
/*! @typedef am_overlay_area_t
 *  @brief For overlay GET and ADD
 */
typedef struct am_overlay_area_s {
    /*!
     * whether enable area when add, when used in GET,
     * 0 = disable, 1 = enable, other value = not created
     */
    uint32_t enable;

    /*!
     * area width to add
     */
    uint32_t width;

    /*!
     * area height to add
     */
    uint32_t height;

    /*!
     * area layout to add, 0:left top; 1:right top; 2:left bottom;
     * 3:right bottom; 4:manual set by offset_x and offset_y
     */
    uint32_t layout;

    /*!
     * area offset x to add
     */
    uint32_t offset_x;

    /*!
     * area offset y to add
     */
    uint32_t offset_y;

    /*!
     * area type to add, 0:generic string; 1:string time;
     * 2:bmp picture;3:test pattern
     */
    uint32_t type;

    /*!
     * area rotate attribute to add
     */
    uint32_t  rotate;

    /*!
     * text font type name
     */
    char font_type[OSD_MAX_FILENAME];

    /*!
     * string font size
     */
    uint32_t font_size;

    /*!
     * string font color0-7 is predefine color: 0,white;1,black;2,red;3,blue;
     * 4,green;5,yellow;6,cyan;7,magenta; >7,user custom color(VUYA format)
     */
    uint32_t font_color;

    /*!
     * string font outline width
     */
    uint32_t font_outline_w;

    /*!
     * string font hor bold size
     */
    uint32_t font_hor_bold;

    /*!
     * string font ver bold size
     */
    uint32_t font_ver_bold;

    /*!
     * string font italic
     */
    uint32_t font_italic;

    /*!
     * color used to transparent in picture type(VUYA format):
     * v:bit[24-31],u:bit[16-23],y:bit[8-15],a:bit[0-7]
     */
    uint32_t color_key;

    /*!
     * color range used to transparent with color_key
     */
    uint32_t color_range;

    /*!
     * string to be insert
     */
    char str[OSD_MAX_STRING];

    /*!
     * bmp full path name to be insert
     */
    char bmp[OSD_MAX_FILENAME];
} am_overlay_area_t;

/*! @struct am_overlay_s
 *  @brief For Overlay ADD
 */
/*! @typedef am_overlay_t
 *  @brief For overlay ADD
 */
typedef struct am_overlay_s {
    /*!
     * @sa AM_OVERLAY_BITS
     */
    uint32_t enable_bits;

    uint32_t stream_id;
    /*!
     * osd area parameter(add of SET and GET)
     */
    am_overlay_area_t area;
} am_overlay_t;

/*! @} */ /* End of Overlay */
/******************************************************************************/

/*! @} */ /* End of defgroup airapi-video */

#endif /* _AM_API_VIDEO_H_ */
