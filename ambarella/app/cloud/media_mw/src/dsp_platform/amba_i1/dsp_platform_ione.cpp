/*******************************************************************************
 * dsp_platform_interface.cpp
 *
 * History:
 *    2012/12/21 - [Zhi He] create file
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "mw_internal.h"

#include "dsp_platform_interface.h"

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_transcode_drv.h"
#include "iav_duplex_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
}

#include "dsp_platform_ione.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IDSPAPI *gfCreateiOneDSPAPI(const volatile SPersistMediaConfig *p_config)
{
    IDSPAPI *api = NULL;

    CIOneDSPAPI *thiz = new CIOneDSPAPI(p_config);
    if (thiz) {
        api = (IDSPAPI *) thiz;
        return api;
    } else {
        LOG_FATAL("new CIOneDSPAPI() fail\n");
    }

    return NULL;
}

IDSPDecAPI *gfCreateiOneDSPDecAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
{
    IDSPDecAPI *api = NULL;

    if (EDSPOperationMode_UDEC == dsp_mode) {
        CIOneUdecDecAPI *thiz = new CIOneUdecDecAPI(iav_fd, dsp_mode, p_config);
        if (thiz) {
            api = (IDSPDecAPI *) thiz;
            return api;
        } else {
            LOG_FATAL("new CIOneUdecDecAPI() fail\n");
        }
    } else if (EDSPOperationMode_FullDuplex == dsp_mode) {
        CIOneDuplexDecAPI *thiz = new CIOneDuplexDecAPI(iav_fd, dsp_mode, p_config);
        if (thiz) {
            api = (IDSPDecAPI *) thiz;
            return api;
        } else {
            LOG_FATAL("new CIOneDuplexDecAPI() fail\n");
        }
    } else {
        LOG_FATAL("BAD request dsp_mode(%d)\n", dsp_mode);
    }

    return NULL;
}

IDSPEncAPI *gfCreateiOneDSPEncAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
{
    IDSPEncAPI *api = NULL;

    if (EDSPOperationMode_CameraRecording == dsp_mode) {
        CIOneRecordEncAPI *thiz = new CIOneRecordEncAPI(iav_fd, dsp_mode, p_config);
        if (thiz) {
            api = (IDSPEncAPI *) thiz;
            return api;
        } else {
            LOG_FATAL("new CIOneRecordEncAPI() fail\n");
        }
    } else if (EDSPOperationMode_MultipleWindowUDEC == dsp_mode || EDSPOperationMode_MultipleWindowUDEC_Transcode == dsp_mode) {
        CIOneMUdecEncAPI *thiz = new CIOneMUdecEncAPI(iav_fd, dsp_mode, p_config);
        if (thiz) {
            api = (IDSPEncAPI *) thiz;
            return api;
        } else {
            LOG_FATAL("new CIOneMUdecEncAPI() fail\n");
        }
    } else if (EDSPOperationMode_FullDuplex == dsp_mode) {
        CIOneDuplexEncAPI *thiz = new CIOneDuplexEncAPI(iav_fd, dsp_mode, p_config);
        if (thiz) {
            api = (IDSPEncAPI *) thiz;
            return api;
        } else {
            LOG_FATAL("new CIOneDuplexEncAPI() fail\n");
        }
    } else {
        LOG_FATAL("BAD request dsp_mode(%d)\n", dsp_mode);
    }

    return NULL;
}

static TU16 getDSPCodecType(StreamFormat codec_type)
{
    TU16 dec_type = 0;

    switch (codec_type) {
        case StreamFormat_H264:
            dec_type = UDEC_H264;
            break;
        case StreamFormat_VC1:
            dec_type = UDEC_VC1;
            break;
        case StreamFormat_MPEG12:
            dec_type = UDEC_MP12;
            break;
        case StreamFormat_MPEG4:
            dec_type = UDEC_MP4H;
            break;
        case StreamFormat_HybridMPEG4:
            dec_type = UDEC_MP4S;
            break;
        case StreamFormat_HybridRV40:
            dec_type = UDEC_RV40;
            break;
        case StreamFormat_VideoSW:
            dec_type = UDEC_SW;
            break;
        default:
            LOG_FATAL("BAD request video codec_type %d\n", codec_type);
            break;
    }

    return dec_type;
}

CIOneDuplexDecAPI::CIOneDuplexDecAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneDuplexDecAPI")
    , mIavFd(fd)
    , mDSPMode(dsp_mode)
    , mDecType(StreamFormat_H264)
    , mDSPDecType(UDEC_H264)
    , mTotalStreamNumber(1)
    , mpConfig(p_config)
{
    DASSERT(fd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == dsp_mode);
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPDec);
}

CIOneDuplexDecAPI::~CIOneDuplexDecAPI()
{
}

EECode CIOneDuplexDecAPI::InitDecoder(TUint &dec_id, SDecoderParam *param, TUint vout_start_index, TUint number_vout, const volatile SDSPVoutConfig *vout_config)
{
    TUint i = 0;
    iav_duplex_init_decoder_t decoder_t;
    iav_decoder_vout_config_t vout[EDisplayDevice_TotCount];
    DASSERT(number_vout <= EDisplayDevice_TotCount);
    DASSERT(mIavFd >= 0);

    if (number_vout > 2) {
        LOGM_ERROR("number_vout(%d) exceed max value 2.\n", number_vout);
        number_vout = 2;
    }

    mDSPMode = param->dsp_mode;
    mDecType = param->codec_type;
    mDSPDecType = getDSPCodecType(mDecType);

    memset(&decoder_t, 0, sizeof(decoder_t));

    decoder_t.dec_id = 0;
    decoder_t.dec_type = mDSPDecType;

    decoder_t.u.h264.enable_pic_info = 0;
    decoder_t.u.h264.use_tiled_dram = 1;

    //decoder_t.u.h264.rbuf_smem_size = 0;
    //decoder_t.u.h264.fbuf_dram_size = 0;
    //decoder_t.u.h264.pjpeg_buf_size = 0;
    decoder_t.u.h264.rbuf_smem_size = 0;
    decoder_t.u.h264.fbuf_dram_size = 40 * 1024 * 1024;
    decoder_t.u.h264.pjpeg_buf_size = 10 * 1024 * 1024;

    decoder_t.u.h264.svc_fbuf_dram_size = 0;
    decoder_t.u.h264.svc_pjpeg_buf_size = 0;
    decoder_t.u.h264.cabac_2_recon_delay = 0;
    decoder_t.u.h264.force_fld_tiled = 1;
    decoder_t.u.h264.ec_mode = 1;
    decoder_t.u.h264.svc_ext = 0;
    decoder_t.u.h264.warp_enable = 0;
    decoder_t.u.h264.max_frm_num_of_dpb = 0;    //18;
    decoder_t.u.h264.max_frm_buf_width = 0; //1920;
    decoder_t.u.h264.max_frm_buf_height = 0;    //1088;

    LOGM_INFO("set vout number_vout %d.\n", number_vout);
    for (i = 0; i < number_vout; i++) {
        vout[i].vout_id = vout_config[i].vout_id;
        vout[i].udec_id = dec_id;
        vout[i].disable = 0;
        vout[i].flip = vout_config[i].flip;
        vout[i].rotate = vout_config[i].rotate;
        vout[i].win_width = vout[i].target_win_width = vout_config[i].size_x;
        vout[i].win_height = vout[i].target_win_height = vout_config[i].size_y;
        vout[i].win_offset_x = vout[i].target_win_offset_x = vout_config[i].pos_x;
        vout[i].win_offset_y = vout[i].target_win_offset_y = vout_config[i].pos_y;
        vout[i].zoom_factor_x = vout[i].zoom_factor_y = 1;
        LOGM_INFO("  vout %d: vout_id %d, udec_id %d, flip %d, rotate %d.\n", i, vout[i].vout_id, vout[i].udec_id, vout[i].flip, vout[i].rotate);
        LOGM_INFO("                win_width %d, win_height %d, win_offset_x %d, win_offset_y %d.\n", vout[i].win_width, vout[i].win_height, vout[i].win_offset_x, vout[i].win_offset_y);
    }
    decoder_t.display_config.num_vout = number_vout;
    decoder_t.display_config.vout_config = &vout[0];

    decoder_t.display_config.first_pts_low = 0;
    decoder_t.display_config.first_pts_high = 0;
    decoder_t.display_config.input_center_x = param->pic_width / 2;
    decoder_t.display_config.input_center_y = param->pic_height / 2;

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_INIT_DECODER, &decoder_t) < 0) {
        perror("IAV_IOC_DUPLEX_INIT_DECODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_INIT_DECODER error.\n");
        return EECode_Error;
    }

    param->bits_fifo_start = decoder_t.bits_fifo_start;
    param->bits_fifo_size = decoder_t.bits_fifo_size;

    dec_id = 0;//hard code

    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);

    return EECode_OK;
}

EECode CIOneDuplexDecAPI::ReleaseDecoder(TUint dec_id)
{
    DASSERT(mIavFd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_RELEASE_DECODER, dec_id) < 0) {
        perror("IAV_IOC_DUPLEX_RELEASE_DECODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_RELEASE_DECODER error.\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIOneDuplexDecAPI::RequestBitStreamBuffer(TUint dec_id, TU8 *pstart, TUint room)
{
    DASSERT(mIavFd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);
    TInt ret;

    iav_wait_decoder_t wait;
    wait.flags = IAV_WAIT_BSB;
    wait.decoder_id = dec_id;
    wait.emptiness.start_addr = pstart;
    wait.emptiness.room = room;

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_WAIT_DECODER, &wait)) < 0) {
        perror("IAV_IOC_DUPLEX_WAIT_DECODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_WAIT_DECODER error.\n");
        if ((-EPERM) == ret) {
            return EECode_OSError;
        }
        return EECode_Error;
    }
    return EECode_OK;
}

EECode CIOneDuplexDecAPI::Decode(TUint dec_id, TU8 *pstart, TU8 *pend, TU32 number_of_frames)
{
    DASSERT(mIavFd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);
    TInt ret;

    iav_duplex_decode_t param;
    memset(&param, 0, sizeof(param));

    param.dec_id = dec_id;
    if (UDEC_H264 == mDSPDecType) {
        param.dec_type = UDEC_H264;
        param.u.h264.start_addr = pstart;
        param.u.h264.end_addr = pend;
        param.u.h264.first_pts_high = 0;//hard code
        param.u.h264.first_pts_low = 0;
        param.u.h264.num_pics = number_of_frames;
        param.u.h264.num_frame_decode = 0;
    } else {
        LOGM_ERROR("Add implementation.\n");
    }

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_DECODE, &param)) < 0) {
        LOGM_ERROR("IAV_IOC_DUPLEX_DECODE error.\n");
        perror("IAV_IOC_DUPLEX_DECODE");
        if ((-EPERM) == ret) {
            return EECode_OSError;
        }
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIOneDuplexDecAPI::Stop(TUint dec_id, TUint stop_flag)
{
    DASSERT(mIavFd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);

    iav_duplex_start_decoder_t stop;
    stop.dec_id = dec_id;
    stop.stop_flag = stop_flag;
    if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_DECODER, &stop) < 0) {
        perror("IAV_IOC_DUPLEX_STOP_DECODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_STOP_DECODER error.\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIOneDuplexDecAPI::PbSpeed(TUint dec_id, TU8 direction, TU8 feeding_rule, TU16 speed)
{
    LOGM_INFO("CIOneDuplexDecAPI::PbSpeed not implemented.\n");
    return EECode_NoImplement;
}

EECode CIOneDuplexDecAPI::QueryStatus(TUint dec_id, SDecStatus *p_status)
{
    DASSERT(mIavFd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == mDSPMode);
    DASSERT(UDEC_H264 == mDSPDecType);
    iav_duplex_get_decoder_status_t status;

    if ((!p_status) || (0 > mIavFd)) {
        LOGM_ERROR("NULL pointer %p, or invalid iav fd %d\n", p_status, mIavFd);
        return EECode_BadParam;
    }

    status.dec_id = dec_id;

    ioctl(mIavFd, IAV_IOC_DUPLEX_GET_DECODER_STATUS, &status);
    p_status->error_level = status.decoder_error_level;
    p_status->error_type = status.decoder_error_type;

    p_status->last_pts = ((TTime)status.last_pts_low) | (((TTime)status.last_pts_high) << 32);
    return EECode_OK;
}

EECode CIOneDuplexDecAPI::TuningPB(TUint dec_id, TU8 fw, TU16 frame_tick)
{
    LOGM_FATAL("TODO\n");
    return EECode_NotSupported;
}

CIOneUdecDecAPI::CIOneUdecDecAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneUdecDecAPI")
    , mIavFd(fd)
    , mDSPMode(dsp_mode)
    , mDecType(StreamFormat_H264)
    , mDSPDecType(UDEC_H264)
    , mTotalStreamNumber(1)
    , mpConfig(p_config)
{
    DASSERT(fd >= 0);
    DASSERT(p_config);
    DASSERT(EDSPOperationMode_UDEC == dsp_mode);
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPDec);

    //mConfigLogLevel = ELogLevel_Debug;
}

CIOneUdecDecAPI::~CIOneUdecDecAPI()
{
}

EECode CIOneUdecDecAPI::InitDecoder(TUint &dec_id, SDecoderParam *param, TUint vout_start_index, TUint number_vout, const volatile SDSPVoutConfig *vout_config)
{
    TUint i;
    iav_udec_info_ex_t mUdecInfo;
    iav_udec_vout_config_t mUdecVoutConfig[EDisplayDevice_TotCount];

    //DASSERT(number_vout);
    DASSERT((vout_start_index + number_vout) <= EDisplayDevice_TotCount);
    DASSERT(vout_config);

    if (((vout_start_index + number_vout) > EDisplayDevice_TotCount) || !vout_config) {
        LOGM_FATAL("Failed to InitDecoder!!\n");
        return EECode_BadParam;
    }

    memset(&mUdecInfo, 0, sizeof(mUdecInfo));
    memset(&mUdecVoutConfig[0], 0, EDisplayDevice_TotCount * sizeof(iav_udec_vout_config_t));

    mDSPMode = param->dsp_mode;
    mDecType = param->codec_type;
    mDSPDecType = getDSPCodecType(mDecType);

    for (i = vout_start_index; i < (vout_start_index + number_vout); i++) {
        mUdecVoutConfig[i].vout_id = vout_config[i].vout_id;
        mUdecVoutConfig[i].udec_id = dec_id;
        mUdecVoutConfig[i].disable = 0;
        mUdecVoutConfig[i].flip = vout_config[i].flip;
        mUdecVoutConfig[i].rotate = vout_config[i].rotate;
        mUdecVoutConfig[i].win_width = mUdecVoutConfig[i].target_win_width = vout_config[i].size_x;
        mUdecVoutConfig[i].win_height = mUdecVoutConfig[i].target_win_height = vout_config[i].size_y;
        mUdecVoutConfig[i].win_offset_x = mUdecVoutConfig[i].target_win_offset_x = vout_config[i].pos_x;
        mUdecVoutConfig[i].win_offset_y = mUdecVoutConfig[i].target_win_offset_y = vout_config[i].pos_y;
        mUdecVoutConfig[i].zoom_factor_x = mUdecVoutConfig[i].zoom_factor_y = 1;
        //LOGM_INFO("mUdecVoutConfig[%d]: vout_id %d\n", i, mUdecVoutConfig[i].vout_id);
    }

    mUdecInfo.udec_id = dec_id;
    mUdecInfo.enable_err_handle = mpConfig->dsp_config.errorHandlingConfig[dec_id].enable_udec_error_handling;

    mUdecInfo.enable_pp = 1;
    mUdecInfo.enable_deint = mpConfig->dsp_config.enable_deint;
    mUdecInfo.interlaced_out = 0;
    mUdecInfo.out_chroma_format = (1 == mpConfig->dsp_config.modeConfig.postp_mode) ? 1 : 0;
    mUdecInfo.packed_out = 0;
    if (mUdecInfo.enable_err_handle) {
        DASSERT(mpConfig->dsp_config.errorHandlingConfig[dec_id].enable_udec_error_handling);
        mUdecInfo.concealment_mode = mpConfig->dsp_config.errorHandlingConfig[dec_id].error_concealment_mode;
        mUdecInfo.concealment_ref_frm_buf_id = mpConfig->dsp_config.errorHandlingConfig[dec_id].error_concealment_frame_id;
        LOGM_INFO("enable udec error handling: concealment mode %d, frame id %d.\n", mUdecInfo.concealment_mode, mUdecInfo.concealment_ref_frm_buf_id);
    }

    mUdecInfo.vout_configs.num_vout = number_vout;
    mUdecInfo.vout_configs.vout_config = &mUdecVoutConfig[vout_start_index];

    if (EDSPOperationMode_UDEC == mpConfig->dsp_config.request_dsp_mode) {
        //debug assert
        DASSERT(mpConfig->dsp_config.voutConfigs.src_size_x);
        DASSERT(mpConfig->dsp_config.voutConfigs.src_size_y);

        mUdecInfo.vout_configs.input_center_x = (mpConfig->dsp_config.voutConfigs.src_size_x / 2) + mpConfig->dsp_config.voutConfigs.src_pos_x;
        mUdecInfo.vout_configs.input_center_y = (mpConfig->dsp_config.voutConfigs.src_size_y / 2) + mpConfig->dsp_config.voutConfigs.src_pos_y;
    }

    LOGM_INFO("mUdecInfo.vout_configs.num_vout %d(vout_start_index %d, number_vout %d), mUdecInfo.vout_configs.input_center_x %d, mUdecInfo.vout_configs.input_center_y %d\n", mUdecInfo.vout_configs.num_vout, vout_start_index, number_vout, mUdecInfo.vout_configs.input_center_x, mUdecInfo.vout_configs.input_center_y);

    //default zoom factor
    for (i = 0; i < EDisplayDevice_TotCount; i ++) {
        mUdecInfo.vout_configs.vout_config[i].zoom_factor_x = 1;
        mUdecInfo.vout_configs.vout_config[i].zoom_factor_y = 1;
    }

    LOGM_INFO("mpConfig->dsp_config.udecInstanceConfig[%d].bits_fifo_size %d\n", dec_id, mpConfig->dsp_config.udecInstanceConfig[dec_id].bits_fifo_size);

    mUdecInfo.bits_fifo_size = mpConfig->dsp_config.udecInstanceConfig[dec_id].bits_fifo_size;
    mUdecInfo.ref_cache_size = 0;
    mUdecInfo.noncachable_buffer = 0;
    mUdecInfo.udec_type = mDSPDecType;

    switch (mDSPDecType) {
        case UDEC_H264:
            mUdecInfo.u.h264.pjpeg_buf_size = 4 * 1024 * 1024;
            break;

        case UDEC_MP12:
        case UDEC_MP4H:
            mUdecInfo.u.mpeg.deblocking_flag = mpConfig->dsp_config.enable_deint;
            mUdecInfo.u.mpeg.pquant_mode = mpConfig->dsp_config.deblockingConfig.pquant_mode;
            for (i = 0; i < 32; i++) {
                mUdecInfo.u.mpeg.pquant_table[i] = (TU8)mpConfig->dsp_config.deblockingConfig.pquant_table[i];
            }
            //mUdecInfo.u.mpeg.is_avi_flag = mpSharedRes->is_avi_flag;
            LOGM_INFO("MPEG12/4 deblocking_flag %d, pquant_mode %d.\n", mUdecInfo.u.mpeg.deblocking_flag, mUdecInfo.u.mpeg.pquant_mode);
            for (i = 0; i < 4; i++) {
                LOGM_INFO(" pquant_table[%d - %d]:\n", i * 8, i * 8 + 7);
                LOGM_INFO(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
                          mUdecInfo.u.mpeg.pquant_table[i * 8], mUdecInfo.u.mpeg.pquant_table[i * 8 + 1], mUdecInfo.u.mpeg.pquant_table[i * 8 + 2], mUdecInfo.u.mpeg.pquant_table[i * 8 + 3], \
                          mUdecInfo.u.mpeg.pquant_table[i * 8 + 4], mUdecInfo.u.mpeg.pquant_table[i * 8 + 5], mUdecInfo.u.mpeg.pquant_table[i * 8 + 6], mUdecInfo.u.mpeg.pquant_table[i * 8 + 7] \
                         );
            }
            LOGM_INFO("MPEG4 is_avi_flag %d.\n", mUdecInfo.u.mpeg.is_avi_flag);
            break;

        case UDEC_VC1:
            break;

        default:
            LOGM_ERROR("udec type %d not implemented\n", mDSPDecType);
            return EECode_BadParam;
    }

    LOGM_INFO("start IAV_IOC_INIT_UDEC [%d]....\n", dec_id);
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo) < 0) {
        perror("IAV_IOC_INIT_UDEC");
        LOGM_ERROR("IAV_IOC_INIT_UDEC error.\n");
        return EECode_OSError;
    }
    LOGM_INFO("IAV_IOC_INIT_UDEC [%d] done.\n", dec_id);

    param->bits_fifo_start = mUdecInfo.bits_fifo_start;
    param->bits_fifo_size = mUdecInfo.bits_fifo_size;

    return EECode_OK;
}

EECode CIOneUdecDecAPI::ReleaseDecoder(TUint dec_id)
{
    LOGM_INFO("start ReleaseUdec %d...\n", dec_id);

    if (DUnlikely(ioctl(mIavFd, IAV_IOC_RELEASE_UDEC, dec_id) < 0)) {
        if (mpConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: IAV_IOC_RELEASE_UDEC return fail\n");
        } else {
            perror("IAV_IOC_DESTROY_UDEC");
            LOGM_ERROR("IAV_IOC_RELEASE_UDEC %d fail.\n", dec_id);
        }
        return EECode_OSError;
    }

    LOGM_INFO("end ReleaseUdec\n");
    return EECode_OK;
}

EECode CIOneUdecDecAPI::RequestBitStreamBuffer(TUint dec_id, TU8 *pstart, TUint room)
{
    iav_wait_decoder_t wait;
    TInt ret;

    wait.emptiness.room = room;
    wait.emptiness.start_addr = pstart;
    wait.flags = IAV_WAIT_BITS_FIFO;
    wait.decoder_id = dec_id;

    LOGM_DEBUG("request start.\n");
    if (DUnlikely((ret = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0)) {
        if (mpConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: IAV_IOC_WAIT_DECODER return fail\n");
        } else {
            perror("IAV_IOC_WAIT_DECODER");
            LOGM_ERROR("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                return EECode_OSError;
            }
        }
        return EECode_Error;
    }
    LOGM_DEBUG("request done.\n");

    return EECode_OK;
}

EECode CIOneUdecDecAPI::Decode(TUint dec_id, TU8 *pstart, TU8 *pend, TU32 number_of_frames)
{
    TInt ret;
    iav_udec_decode_t dec;

    memset(&dec, 0, sizeof(dec));
    dec.udec_type = mDSPDecType;
    dec.decoder_id = dec_id;
    dec.u.fifo.start_addr = pstart;
    dec.u.fifo.end_addr = pend;
    dec.num_pics = number_of_frames;
    //LOGM_VERBOSE("DecodeBuffer length %d, %p.\n", pend-pstart, pstart);

    if (DUnlikely((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec))) < 0) {
        if (mpConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: IAV_IOC_UDEC_DECODE return fail\n");
            return EECode_Error;
        } else {
            perror("IAV_IOC_UDEC_DECODE");
            LOGM_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                return EECode_OSError;
            }
            return EECode_Error;
        }
    }

    return EECode_OK;
}

EECode CIOneUdecDecAPI::Stop(TUint dec_id, TUint flag)
{
    TInt ret = 0;
    TUint stop_code = (((flag & 0xff) << 24) | dec_id);

    LOGM_INFO("IAV_IOC_UDEC_STOP start, 0x%x.\n", stop_code);
    if (DUnlikely((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0)) {
        LOGM_ERROR("IAV_IOC_UDEC_STOP error %d.\n", ret);
        return EECode_OSError;
    }

    LOGM_INFO("IAV_IOC_UDEC_STOP done.\n");

    return EECode_OK;
}

EECode CIOneUdecDecAPI::PbSpeed(TUint dec_id, TU8 direction, TU8 feeding_rule, TU16 speed)
{
    TInt ret;
    iav_udec_pb_speed_t speed_t;
    memset(&speed_t, 0x0, sizeof(speed_t));

    speed_t.speed = speed;
    speed_t.decoder_id = dec_id;
    switch (feeding_rule) {
        case DecoderFeedingRule_RefOnly:
            speed_t.scan_mode = 1;
            break;
        case DecoderFeedingRule_IOnly:
            speed_t.scan_mode = 2;
            break;
        default:
            speed_t.scan_mode = 0;
            break;
    }
    speed_t.direction = direction;

    LOGM_INFO("IAV_IOC_UDEC_PB_SPEED, decoder_id %hu, speed 0x%04x, scan_mode %hu, direction %hu.\n", speed_t.decoder_id, speed_t.speed, speed_t.scan_mode, speed_t.direction);
    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_PB_SPEED, &speed_t)) < 0) {
        LOGM_ERROR("IAV_IOC_UDEC_PB_SPEED error %d.\n", ret);
        if (ret == (-EPERM)) {
            return EECode_OSError;
        }
        return EECode_Error;
    }
    LOGM_INFO("IAV_IOC_UDEC_PB_SPEED done.\n");

    return EECode_OK;
}

EECode CIOneUdecDecAPI::QueryStatus(TUint dec_id, SDecStatus *p_status)
{
    DASSERT(mIavFd >= 0);
    DASSERT((EDSPOperationMode_UDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC == mDSPMode));

    TInt ret = 0;
    iav_udec_state_t state;
    UDECErrorCode code;

    memset(&state, 0, sizeof(state));
    state.decoder_id = (TU8)dec_id;
    state.flags = IAV_UDEC_STATE_BTIS_FIFO_ROOM;

    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
    if (ret) {
        perror("IAV_IOC_GET_UDEC_STATE");
        LOGM_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
        return EECode_Error;
    }

    p_status->dec_state = state.udec_state;
    p_status->vout_state = state.vout_state;
    code.mu32 = p_status->error_code = state.error_code;

    p_status->error_level = code.detail.error_level;
    p_status->error_type = code.detail.error_type;
    p_status->module_type = code.detail.decoder_type;
    p_status->free_zoom = state.bits_fifo_free_size;
    p_status->fullness = state.bits_fifo_total_size - state.bits_fifo_free_size;

    p_status->last_pts = ((TTime)state.current_pts_low) | (((TTime)state.current_pts_high) << 32);
    return EECode_OK;
}

EECode CIOneUdecDecAPI::TuningPB(TUint dec_id, TU8 fw, TU16 frame_tick)
{
    DASSERT(mIavFd >= 0);
    DASSERT((EDSPOperationMode_UDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC == mDSPMode));

    TInt ret = 0;
    iav_postp_buffering_control_t control;

    memset(&control, 0, sizeof(control));
    control.stream_id = dec_id;
    control.control_direction = fw;
    control.frame_time = frame_tick;

    ret = ioctl(mIavFd, IAV_IOC_POSTP_BUFFERING_CONTROL, &control);
    if (ret) {
        perror("IAV_IOC_POSTP_BUFFERING_CONTROL");
        LOGM_ERROR("IAV_IOC_POSTP_BUFFERING_CONTROL %d.\n", ret);
        return EECode_Error;
    }

    return EECode_OK;
}

CIOneMUdecEncAPI::CIOneMUdecEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneMUdecEncAPI")
    , mIavFd(fd)
    , mDSPMode(dsp_mode)
    , mMainStreamEnabled(1)
    , mSecondStreamEnabled(0)
    , mpConfig(p_config)
{
    DASSERT(fd >= 0);
    DASSERT(EDSPOperationMode_MultipleWindowUDEC == dsp_mode || EDSPOperationMode_MultipleWindowUDEC_Transcode == dsp_mode);
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPEnc);

    mDSPEncType = 0;
}

CIOneMUdecEncAPI::~CIOneMUdecEncAPI()
{
}

EECode CIOneMUdecEncAPI::InitEncoder(TUint &enc_id, SEncoderParam *param)
{
    TInt ret = 0;
    iav_udec_init_transcoder_t transcode;

    memset(&transcode, 0x0, sizeof(transcode));

    transcode.flags = 0;
    transcode.id = enc_id;
    transcode.stream_type = STREAM_TYPE_FULL_RESOLUTION;
    transcode.profile_idc = 0; //0/100 FREXT_HP, 66, base line, 77 main
    transcode.level_idc = 0;

    transcode.encoding_width = param->enc_width;
    transcode.encoding_height = param->enc_height;

    transcode.source_window_width = 1920;
    transcode.source_window_height = 1080;
    transcode.source_window_offset_x = 0;
    transcode.source_window_offset_y = 0;

    transcode.num_mbrows_per_bitspart = 0;
    transcode.M = param->M;
    transcode.N = param->N;
    transcode.idr_interval = param->idr_interval;
    transcode.gop_structure = param->gop_structure;
    transcode.numRef_P = param->numRef_P;
    transcode.numRef_B = param->numRef_B;
    transcode.use_cabac = param->use_cabac;
    transcode.quality_level = param->quality_level;

    transcode.average_bitrate = param->bitrate;

    transcode.vbr_setting = param->vbr_setting;
    transcode.calibration = param->calibration;

    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_INIT_TRANSCODER, &transcode)) < 0) {
        perror("IAV_IOC_UDEC_INIT_TRANSCODER");
        LOGM_ERROR("IAV_IOC_UDEC_INIT_TRANSCODER fail.\n");
        return EECode_Error;
    }

    param->bits_fifo_start = transcode.bits_fifo_start;
    param->bits_fifo_size = transcode.bits_fifo_size;

    return EECode_OK;
}

EECode CIOneMUdecEncAPI::ReleaseEncoder(TUint enc_id)
{
    DASSERT(0 == enc_id);
    if (ioctl(mIavFd, IAV_IOC_UDEC_RELEASE_TRANSCODER, enc_id) < 0) {
        perror("IAV_IOC_UDEC_RELEASE_TRANSCODER");
        LOGM_ERROR("IAV_IOC_UDEC_RELEASE_TRANSCODER fail.\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIOneMUdecEncAPI::GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc)
{
    TInt ret = 0;
    TUint i = 0;
    iav_udec_transcoder_bs_info_t bits;

    memset(&bits, 0x0, sizeof(iav_udec_transcoder_bs_info_t));
    bits.id = enc_id;

    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_READ_BITS, &bits);
    if (DUnlikely(ret < 0)) {
        if ((-EBUSY) == ret) {
            LOGM_NOTICE("[flow cmd]: IAV_IOC_UDEC_TRANSCODER_READ_BITS, read last packet done.\n");
            return EECode_OK;
        }
        if (mpConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: IAV_IOC_UDEC_TRANSCODER_READ_BITS ret %d.\n", ret);
        } else {
            perror("IAV_IOC_UDEC_TRANSCODER_READ_BITS");
            LOGM_ERROR("!!!!!IAV_IOC_UDEC_TRANSCODER_READ_BITS error, ret %d.\n", ret);
        }
        return EECode_OSError;
    }
    //LOGM_INFO("[flow cmd]: IAV_IOC_UDEC_TRANSCODER_READ_BITS done\n");

    p_desc->tot_desc_number = bits.count;
    DASSERT(IAV_UDEC_TRANSCODER_NUM_USER_DESC == DMaxDescNumber);
    for (i = 0; i < bits.count; i++) {
        p_desc->desc[i].pic_type = bits.desc[i].pic_type;
        p_desc->desc[i].pts = bits.desc[i].PTS;
        p_desc->desc[i].pstart = (TU8 *)bits.desc[i].start_addr;
        p_desc->desc[i].size = bits.desc[i].pic_size;

        p_desc->desc[i].frame_number = bits.desc[i].frame_num;
    }

    return EECode_OK;
}

EECode CIOneMUdecEncAPI::Start(TUint enc_id)
{
    TInt ret;
    iav_udec_start_transcoder_t start;

    memset(&start, 0x0, sizeof(start));

    start.transcoder_id = enc_id;

    LOGM_INFO("[flow cmd]: before IAV_IOC_UDEC_START_TRANSCODER.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_START_TRANSCODER, &start);
    if (ret < 0) {
        perror("IAV_IOC_UDEC_START_TRANSCODER");
        LOGM_ERROR("!!!!!IAV_IOC_UDEC_START_TRANSCODER error, ret %d.\n", ret);
        return EECode_OSError;
    }
    LOGM_INFO("[flow cmd]: IAV_IOC_UDEC_START_TRANSCODER done\n");

    return EECode_OK;
}

EECode CIOneMUdecEncAPI::Stop(TUint enc_id, TUint stop_flag)
{
    TInt ret;
    iav_udec_stop_transcoder_t stop;

    memset(&stop, 0x0, sizeof(stop));

    stop.transcoder_id = enc_id;
    stop.stop_flag = stop_flag;

    LOGM_INFO("[flow cmd]: before IAV_IOC_UDEC_STOP_TRANSCODER.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_STOP_TRANSCODER, &stop);
    if (ret < 0) {
        perror("IAV_IOC_UDEC_STOP_TRANSCODER");
        LOGM_ERROR("!!!!!IAV_IOC_UDEC_STOP_TRANSCODER error, ret %d.\n", ret);
        return EECode_OSError;
    }
    LOGM_INFO("[flow cmd]: IAV_IOC_UDEC_STOP_TRANSCODER done\n");

    return EECode_OK;
}

CIOneDuplexEncAPI::CIOneDuplexEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneDuplexEncAPI")
    , mIavFd(fd)
    , mDSPMode(dsp_mode)
    , mMainStreamEnabled(1)
    , mSecondStreamEnabled(0)
    , mpConfig(p_config)
{
    DASSERT(fd >= 0);
    DASSERT(EDSPOperationMode_FullDuplex == dsp_mode);
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPEnc);

    mDSPEncType = 0;
}

CIOneDuplexEncAPI::~CIOneDuplexEncAPI()
{
}

EECode CIOneDuplexEncAPI::InitEncoder(TUint &enc_id, SEncoderParam *param)
{
    iav_duplex_init_encoder_t encoder_t;
    memset((void *)&encoder_t, 0x0, sizeof(encoder_t));

    DASSERT(0 == enc_id);
    enc_id = 0; //hard code here
    encoder_t.enc_id = enc_id;
    mMainStreamEnabled = 1;

    encoder_t.profile_idc = param->profile;
    encoder_t.level_idc = param->level;
    encoder_t.num_mbrows_per_bitspart = 0;//default value

    encoder_t.encode_w_sz = param->enc_width;
    encoder_t.encode_h_sz = param->enc_height;
    encoder_t.encode_w_ofs = param->enc_offset_x;
    encoder_t.encode_h_ofs = param->enc_offset_y;

    encoder_t.second_stream_enabled = param->second_stream_enabled;
    encoder_t.second_encode_w_sz = param->second_enc_width;
    encoder_t.second_encode_h_sz = param->second_enc_height;
    encoder_t.second_encode_w_ofs = param->second_enc_offset_x;
    encoder_t.second_encode_h_ofs = param->second_enc_offset_y;
    encoder_t.second_average_bitrate = param->second_bitrate;
    if (encoder_t.second_stream_enabled) {
        mSecondStreamEnabled = 1;
    }

    encoder_t.M = param->M;
    encoder_t.N = param->N;
    encoder_t.idr_interval = param->idr_interval;

    encoder_t.gop_structure = param->gop_structure;
    encoder_t.numRef_P = param->numRef_P;
    encoder_t.numRef_B = param->numRef_B;

    encoder_t.use_cabac = param->use_cabac;
    encoder_t.quality_level = param->quality_level;

    encoder_t.average_bitrate = param->bitrate;
    encoder_t.vbr_setting = param->vbr_setting;

    //all zero for other fields

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_INIT_ENCODER, &encoder_t) < 0) {
        perror("IAV_IOC_DUPLEX_INIT_ENCODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_INIT_ENCODER fail.\n");
        return EECode_Error;
    }

    param->bits_fifo_start = encoder_t.bits_fifo_start;
    param->bits_fifo_size = encoder_t.bits_fifo_size;

    return EECode_OK;
}

EECode CIOneDuplexEncAPI::ReleaseEncoder(TUint enc_id)
{
    DASSERT(0 == enc_id);
    if (ioctl(mIavFd, IAV_IOC_DUPLEX_RELEASE_ENCODER, 0) < 0) {
        perror("IAV_IOC_DUPLEX_RELEASE_ENCODER");
        LOGM_ERROR("IAV_IOC_DUPLEX_RELEASE_ENCODER fail.\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIOneDuplexEncAPI::GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc)
{
    TInt ret = 0;
    TUint i = 0;
    iav_duplex_bs_info_t read_t;
    memset((void *)&read_t, 0x0, sizeof(read_t));
    read_t.enc_id = 0;
    p_desc->status = 0;

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_READ_BITS, &read_t)) < 0) {
        if ((-EBUSY) == ret) {
            LOGM_ERROR("read last frame, eos comes.\n");
            //eos
            p_desc->status |= DFlagLastFrame;
            p_desc->tot_desc_number = 0;
            return EECode_OK;
        }
        perror("IOC_DUPLEX_READ_BITS");
        LOGM_ERROR("IOC_DUPLEX_READ_BITS fail.\n");
        return EECode_OSError;
    }

    p_desc->tot_desc_number = read_t.count;
    DASSERT(DUPLEX_NUM_USER_DESC == DMaxDescNumber);
    for (i = 0; i < read_t.count; i++) {
        p_desc->desc[i].pic_type = read_t.desc[i].pic_type;
        p_desc->desc[i].pts = read_t.desc[i].PTS;
        p_desc->desc[i].pstart = (TU8 *)read_t.desc[i].start_addr;
        p_desc->desc[i].size = read_t.desc[i].pic_size;

        p_desc->desc[i].frame_number = read_t.desc[i].frame_num;

        //ugly convert, fake enc_id
        if (DSP_ENC_STREAM_TYPE_FULL_RESOLUTION == read_t.desc[i].stream_id) {
            p_desc->desc[i].enc_id = 0;
        } else if (DSP_ENC_STREAM_TYPE_PIP_RESOLUTION == read_t.desc[i].stream_id) {
            p_desc->desc[i].enc_id = 1;
        } else {
            LOGM_ERROR("BAD stream_id %d.\n", read_t.desc[i].stream_id);
        }

        p_desc->desc[i].fb_id = INVALID_FD_ID;//avoid release, ugly here
    }

    return EECode_OK;
}

EECode CIOneDuplexEncAPI::Start(TUint enc_id)
{
    DASSERT(0 == enc_id);
    iav_duplex_start_encoder_t start;
    start.enc_id = enc_id;

    if (mMainStreamEnabled) {
        start.stream_type = DSP_ENC_STREAM_TYPE_FULL_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_START_ENCODER, &start) < 0) {
            perror("IAV_IOC_DUPLEX_START_ENCODER");
            LOGM_ERROR("IAV_IOC_DUPLEX_START_ENCODER main stream failed.\n");
            return EECode_Error;
        }
    }

    if (mSecondStreamEnabled) {
        start.stream_type = DSP_ENC_STREAM_TYPE_PIP_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_START_ENCODER, &start) < 0) {
            perror("IAV_IOC_DUPLEX_START_ENCODER");
            LOGM_ERROR("IAV_IOC_DUPLEX_START_ENCODER second stream failed.\n");
            return EECode_Error;
        }
    }

    return EECode_OK;
}

EECode CIOneDuplexEncAPI::Stop(TUint enc_id, TUint stop_flag)
{
    DASSERT(0 == enc_id);
    iav_duplex_start_encoder_t stop;
    stop.enc_id = enc_id;

    if (mMainStreamEnabled) {
        stop.stream_type = DSP_ENC_STREAM_TYPE_FULL_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_ENCODER, &stop) < 0) {
            perror("IAV_IOC_DUPLEX_STOP_ENCODER");
            LOGM_ERROR("IAV_IOC_DUPLEX_STOP_ENCODER main stream failed.\n");
            return EECode_Error;
        }
    }

    if (mSecondStreamEnabled) {
        stop.stream_type = DSP_ENC_STREAM_TYPE_PIP_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_ENCODER, &stop) < 0) {
            perror("IAV_IOC_DUPLEX_STOP_ENCODER");
            LOGM_ERROR("IAV_IOC_DUPLEX_STOP_ENCODER second stream failed.\n");
            return EECode_Error;
        }
    }

    return EECode_OK;
}

CIOneRecordEncAPI::CIOneRecordEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneRecordEncAPI")
    , mIavFd(fd)
    , mDSPMode(dsp_mode)
    , mTotalStreamNumber(1)
    , mStreamMask(0x1)
    , mEosFlag(0)
    , mpConfig(p_config)
{
    DASSERT(fd >= 0);
    DASSERT(EDSPOperationMode_CameraRecording == dsp_mode);
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPEnc);
    mDSPEncType = 0;
}

CIOneRecordEncAPI::~CIOneRecordEncAPI()
{
}

EECode CIOneRecordEncAPI::InitEncoder(TUint &enc_id, SEncoderParam *param)
{
    mStreamMask = 0;
    mTotalStreamNumber = 0;
    //mEosFlag = 0;

    iav_enc_config_t mEncConfig[3];

    mEncConfig[0].flags = 0;
    mEncConfig[0].enc_id = enc_id;
    mEncConfig[0].encode_type = IAV_ENCODE_H264;
    mEncConfig[0].u.h264.M = param->M;
    mEncConfig[0].u.h264.N = param->N;
    mEncConfig[0].u.h264.idr_interval = param->idr_interval;
    mEncConfig[0].u.h264.gop_model = param->gop_structure;
    mEncConfig[0].u.h264.bitrate_control = param->vbr_setting;
    mEncConfig[0].u.h264.calibration = param->calibration;
    mEncConfig[0].u.h264.vbr_ness = param->vbr_ness;
    mEncConfig[0].u.h264.min_vbr_rate_factor = param->min_vbr_rate_factor;
    mEncConfig[0].u.h264.max_vbr_rate_factor = param->max_vbr_rate_factor;

    mEncConfig[0].u.h264.average_bitrate = param->bitrate;
    mEncConfig[0].u.h264.entropy_codec = param->use_cabac;

    if (param->second_stream_enabled) {
        mEncConfig[1].flags = 0;
        mEncConfig[1].enc_id = enc_id + 1;//hard code
        mEncConfig[1].encode_type = IAV_ENCODE_H264;
        mEncConfig[1].u.h264.M = param->M;
        mEncConfig[1].u.h264.N = param->N;
        mEncConfig[1].u.h264.idr_interval = param->idr_interval;
        mEncConfig[1].u.h264.gop_model = param->gop_structure;
        mEncConfig[1].u.h264.bitrate_control = param->vbr_setting;
        mEncConfig[1].u.h264.calibration = param->calibration;
        mEncConfig[1].u.h264.vbr_ness = param->vbr_ness;
        mEncConfig[1].u.h264.min_vbr_rate_factor = param->min_vbr_rate_factor;
        mEncConfig[1].u.h264.max_vbr_rate_factor = param->max_vbr_rate_factor;

        mEncConfig[1].u.h264.average_bitrate = param->second_bitrate;
        mEncConfig[1].u.h264.entropy_codec = param->use_cabac;
    }

    if (param->dsp_piv_enabled) {
        mEncConfig[2].flags = 0;
        mEncConfig[2].enc_id = enc_id + 2;//hard code
        mEncConfig[2].encode_type = IAV_ENCODE_MJPEG;

        mEncConfig[2].u.jpeg.chroma_format = 0;//hard code
        mEncConfig[2].u.jpeg.init_quality_level = 90;//hard code
        mEncConfig[2].u.jpeg.thumb_active_w = param->dsp_jpeg_active_win_w;
        mEncConfig[2].u.jpeg.thumb_active_h = param->dsp_jpeg_active_win_h;
        mEncConfig[2].u.jpeg.thumb_dram_w = param->dsp_jpeg_dram_w;
        mEncConfig[2].u.jpeg.thumb_dram_h = param->dsp_jpeg_dram_h;
    }

    if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[0]) < 0) {
        perror("IAV_IOC_ENCODE_SETUP\n");
        LOGM_ERROR("IAV_IOC_ENCODE_SETUP\n");
        return EECode_Error;
    }
    mStreamMask |= IAV_MAIN_STREAM;
    mTotalStreamNumber ++;

    LOGM_INFO("IAV_IOC_ENCODE_SETUP main done, bit rate %u.\n", mEncConfig[0].u.h264.average_bitrate);

    if (param->second_stream_enabled) {
        if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[1]) < 0) {
            perror("IAV_IOC_ENCODE_SETUP\n");
            LOGM_ERROR("IAV_IOC_ENCODE_SETUP\n");
            return EECode_Error;
        }
        LOGM_INFO("IAV_IOC_ENCODE_SETUP second done, bit rate %u.\n", mEncConfig[1].u.h264.average_bitrate);
        mStreamMask |= IAV_2ND_STREAM;
        mTotalStreamNumber ++;
    }

    if (param->dsp_piv_enabled) {
        LOGM_INFO("IAV_IOC_ENCODE_SETUP third(piv) start, format %d, quality level %d, active w %d, h %d, dram w %d, h %d.\n", mEncConfig[2].u.jpeg.chroma_format, mEncConfig[2].u.jpeg.init_quality_level, mEncConfig[2].u.jpeg.thumb_active_w, mEncConfig[2].u.jpeg.thumb_active_h, mEncConfig[2].u.jpeg.thumb_dram_w, mEncConfig[2].u.jpeg.thumb_dram_h);
        if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[2]) < 0) {
            perror("IAV_IOC_ENCODE_SETUP\n");
            LOGM_ERROR("IAV_IOC_ENCODE_SETUP\n");
            return EECode_Error;
        }

        mStreamMask |= IAV_3RD_STREAM;
        mTotalStreamNumber ++;
    }

    LOGM_INFO("CIOneRecordEncAPI::InitEncoder done.\n");
    return EECode_OK;
}

EECode CIOneRecordEncAPI::ReleaseEncoder(TUint enc_id)
{
    return EECode_OK;
}

EECode CIOneRecordEncAPI::GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc)
{
    TInt ret = 0;
    iav_frame_desc_t frame;

    p_desc->status = 0;
    p_desc->tot_desc_number = 0;
    memset(&frame, 0 , sizeof(iav_frame_desc_t));
    frame.enc_id = IAV_ENC_ID_MAIN;
    if (mTotalStreamNumber > 1)
    { frame.enc_id = IAV_ENC_ID_ALL; }

    ret = ::ioctl(mIavFd, IAV_IOC_GET_ENCODED_FRAME, &frame);
    if (ret < 0) {
        perror("IAV_IOC_GET_ENCODED_FRAME");
        LOGM_ERROR("IAV_IOC_GET_ENCODED_FRAME error, ret %d.\n", ret);
        return EECode_Error;
    }

    p_desc->tot_desc_number = 1;//hard code here
    p_desc->desc[0].pic_type = frame.pic_type;
    p_desc->desc[0].pts = frame.pts_64;
    p_desc->desc[0].pstart = (TU8 *)frame.usr_start_addr;
    p_desc->desc[0].size = frame.pic_size;

    p_desc->desc[0].enc_id = frame.enc_id;
    p_desc->desc[0].fb_id = frame.fd_id;
    p_desc->desc[0].frame_number = frame.frame_num;

    if (frame.is_eos) {
        //LOGM_WARN("enc_id %d, eos, mEosFlag 0x%x, mStreamMask 0x%x.\n", frame.enc_id, mEosFlag, mStreamMask);
        //mEosFlag |= 0x1 << frame.enc_id;
        //if (mEosFlag == mStreamMask) {
        //all eos
        LOGM_WARN("all enc stream, eos.\n");
        p_desc->status |= DFlagLastFrame;
        //}
        return EECode_OK;
    }

    p_desc->status |= DFlagNeedReleaseFrame;
#if 0
    //release it to iav driver
    if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
        LOGM_ERROR("IAV_IOC_RELEASE_ENCODED_FRAME error.\n");
    }
#endif
    return EECode_OK;
}

EECode CIOneRecordEncAPI::Start(TUint enc_id)
{
    TUint i = 0;
    TInt ret, error = 0;

    for (i = 0; i < mTotalStreamNumber; i++) {
        //skip jpeg, and mjpeg stream
        if (i >= 2) {
            continue;
        }
        LOGM_INFO("before IAV_IOC_START_ENCODE %d.\n", i);
        ret = ioctl(mIavFd, IAV_IOC_START_ENCODE, i);
        if (ret < 0) {
            LOGM_ERROR("IAV_IOC_START_ENCODE %d, error, ret %d.\n", i, ret);
            error = 1;
        } else {
            LOGM_INFO("IAV_IOC_START_ENCODE %d done.\n", i);
        }
    }

    if (!error) {
        return EECode_OK;
    }
    return EECode_Error;
}

EECode CIOneRecordEncAPI::Stop(TUint enc_id, TUint stop_flag)
{
    TUint i = 0;
    TInt ret, error = 0;

    for (i = 0; i < mTotalStreamNumber; i++) {
        //skip jpeg, and mjpeg stream
        if (i >= 2) {
            continue;
        }
        LOGM_INFO("before IAV_IOC_STOP_ENCODE %d.\n", i);
        ret = ioctl(mIavFd, IAV_IOC_STOP_ENCODE, i);
        if (ret < 0) {
            LOGM_ERROR("IAV_IOC_STOP_ENCODE %d, error, ret %d.\n", i, ret);
            error = 1;
        } else {
            LOGM_INFO("before IAV_IOC_STOP_ENCODE %d done.\n", i);
        }
    }

    if (!error) {
        return EECode_OK;
    }
    return EECode_Error;
}

CIOneDSPAPI::CIOneDSPAPI(const volatile SPersistMediaConfig *p_config)
    : inherited("CIOneDSPAPI")
    , mIavFd(-1)
    , mDSPMode(EDSPOperationMode_Invalid)
    , mpConfig(NULL)
    , mpDSPConfig(NULL)
{
    mpConfig = p_config;
    DASSERT(p_config);
    if (p_config) {
        mpDSPConfig = (SDSPConfig *)&p_config->dsp_config;
    }
    DSET_MODULE_LOG_CONFIG(ELogModuleDSPPlatform);
    //mConfigLogLevel = ELogLevel_Debug;

    mIavFd = -1;
}

CIOneDSPAPI::~CIOneDSPAPI()
{
}

EECode CIOneDSPAPI::OpenDevice(TInt &fd)
{
    DASSERT(mIavFd < 0);
    if (mIavFd < 0) {
        mIavFd = open("/dev/iav", O_RDWR, 0);
        if (mIavFd < 0) {
            LOGM_ERROR("open iav fail, ret %d.\n", mIavFd);
            return EECode_OSError;
        }
    } else {
        LOGM_ERROR("iav is already opened.\n");
    }
    fd = mIavFd;
    return EECode_OK;
}

EECode CIOneDSPAPI::CloseDevice()
{
    if (mIavFd >= 0) {
        close(mIavFd);
        mIavFd = -1;
        return EECode_OK;
    }

    LOGM_ERROR("iav is already closed.\n");
    return EECode_OK;
}

EECode CIOneDSPAPI::QueryVoutSettings(volatile SDSPConfig *config) const
{
    TInt i, num = 0, sink_id = -1, sink_type = -1;
    TInt vout_id = 0;
    struct amba_vout_sink_info  sink_info;

    if (::ioctl(mIavFd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
        perror("IAV_IOC_VOUT_GET_SINK_NUM");
        LOGM_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail!\n");
        return EECode_OSError;
    }

    if (num < 1) {
        LOGM_ERROR("Please load vout driver!\n");
        return EECode_OSError;
    }

    for (vout_id = 0; vout_id < EDisplayDevice_TotCount; vout_id++) {

        config->voutConfigs.voutConfig[vout_id].vout_id = vout_id;

        if (vout_id == 0) {
            sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
        } else {
            sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
        }

        for (i = num - 1; i >= 0; i--) {
            memset(&sink_info, 0x0, sizeof(sink_info));
            sink_info.id = i;
            if (::ioctl(mIavFd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
                perror("IAV_IOC_VOUT_GET_SINK_INFO");
                LOGM_ERROR("**IAV_IOC_VOUT_GET_SINK_INFO fail!\n");
                continue;
            }

            if (sink_info.source_id == vout_id &&
                    sink_info.sink_type == sink_type) {
                sink_id = sink_info.id;
                break;
            }
        }

        if (sink_info.state != AMBA_VOUT_SINK_STATE_RUNNING) {
            LOGM_CONFIGURATION("[VOUT]: the state of vout %d is not RUNNING, its state is %d\n", vout_id, sink_info.state);
            continue;
        }

        config->voutConfigs.voutConfig[vout_id].failed = 0;
        config->voutConfigs.voutConfig[vout_id].enable = sink_info.sink_mode.video_en;

        if (sink_info.sink_mode.fb_id < 0) {
            //osd disabled
            config->voutConfigs.voutConfig[vout_id].osd_disable = 1;
        } else {
            //osd enabled
            config->voutConfigs.voutConfig[vout_id].osd_disable = 0;
        }

        //config->voutConfigs.voutConfig[vout_id].enable = 1;
        config->voutConfigs.voutConfig[vout_id].sink_id = sink_id;
        config->voutConfigs.voutConfig[vout_id].width = sink_info.sink_mode.video_size.vout_width;
        config->voutConfigs.voutConfig[vout_id].height = sink_info.sink_mode.video_size.vout_height;
        config->voutConfigs.voutConfig[vout_id].size_x = sink_info.sink_mode.video_size.video_width;
        config->voutConfigs.voutConfig[vout_id].size_y = sink_info.sink_mode.video_size.video_height;
        config->voutConfigs.voutConfig[vout_id].pos_x = sink_info.sink_mode.video_offset.offset_x;
        config->voutConfigs.voutConfig[vout_id].pos_y = sink_info.sink_mode.video_offset.offset_y;

        config->voutConfigs.voutConfig[vout_id].flip = (TInt)sink_info.sink_mode.video_flip;
        config->voutConfigs.voutConfig[vout_id].rotate = (TInt)sink_info.sink_mode.video_rotate;

        LOGM_CONFIGURATION("[VOUT]: vout(%d, enable %d)'s original position/size, dimention: pos(%d,%d), size(%d,%d), dimention(%d,%d), flip %d, rotate %d.\n", vout_id, config->voutConfigs.voutConfig[vout_id].enable, \
                           config->voutConfigs.voutConfig[vout_id].pos_x, config->voutConfigs.voutConfig[vout_id].pos_y, \
                           config->voutConfigs.voutConfig[vout_id].size_x, config->voutConfigs.voutConfig[vout_id].size_y, \
                           config->voutConfigs.voutConfig[vout_id].width, config->voutConfigs.voutConfig[vout_id].height, \
                           config->voutConfigs.voutConfig[vout_id].flip, config->voutConfigs.voutConfig[vout_id].rotate);
    }

    return EECode_OK;
}

EECode CIOneDSPAPI::DSPModeConfig(const volatile SPersistMediaConfig *config)
{
    DASSERT(config);
    if (!config) {
        LOGM_ERROR("NULL pointer detected.\n");
        return EECode_BadParam;
    }

    mpConfig = config;
    mpDSPConfig = (const volatile SDSPConfig *)&config->dsp_config;
    return EECode_OK;
}

EECode CIOneDSPAPI::enterUdecMode()
{
    //enter udec mode config
    iav_udec_mode_config_t udec_mode;
    iav_udec_config_t *udec_configs;

    TUint i = 0;
    TUint voutmask = 0;
    TUint udec_num = 1;

    TInt ret;

    DASSERT(mpDSPConfig);
    if (!mpDSPConfig) {
        LOGM_ERROR("NULL mpDSPConfig, should invoke DSPModeConfig() first\n");
        return EECode_BadState;
    }

    voutmask = mpDSPConfig->modeConfig.vout_mask;
    DASSERT(voutmask);
    if (!voutmask) {
        LOGM_ERROR("NO vout requested, please specify voutmask in EnterDSPMode\n");
        return EECode_Error;
    }

    LOGM_DEBUG("[flow enterUdecMode]: voutmask %08x.\n", voutmask);

    DASSERT(udec_num == mpDSPConfig->modeConfig.num_udecs);

    if ((udec_configs = (iav_udec_config_t *)DDBG_MALLOC(udec_num * sizeof(iav_udec_config_t), "D0IC")) == NULL) {
        LOGM_ERROR(" no memory\n");
        return EECode_NoMemory;
    }

    LOGM_DEBUG("[flow enterUdecMode]: before init_udec_mode_config\n");

    memset(&udec_mode, 0, sizeof(udec_mode));
    udec_mode.postp_mode = mpDSPConfig->modeConfig.postp_mode;
    DASSERT(2 == udec_mode.postp_mode);

    udec_mode.enable_deint = mpDSPConfig->enable_deint;
    udec_mode.pp_chroma_fmt_max = mpDSPConfig->modeConfig.pp_chroma_fmt_max;
    DASSERT(2 == udec_mode.pp_chroma_fmt_max);

    udec_mode.pp_max_frm_width = mpDSPConfig->modeConfig.pp_max_frm_width;
    udec_mode.pp_max_frm_height = mpDSPConfig->modeConfig.pp_max_frm_height;
    udec_mode.pp_max_frm_num = mpDSPConfig->modeConfig.pp_max_frm_num;
    DASSERT(5 == udec_mode.pp_max_frm_num);

    udec_mode.pp_background_Y = mpDSPConfig->modeConfig.pp_background_Y;
    udec_mode.pp_background_Cb = mpDSPConfig->modeConfig.pp_background_Cb;
    udec_mode.pp_background_Cr = mpDSPConfig->modeConfig.pp_background_Cr;

    udec_mode.vout_mask = voutmask;

    udec_mode.num_udecs = udec_num;
    udec_mode.udec_config = udec_configs;

    //udec config
    memset(udec_configs, 0, sizeof(iav_udec_config_t) * udec_num);
    for (i = 0; i < udec_num; i++) {
        udec_configs[i].tiled_mode = mpDSPConfig->udecInstanceConfig[i].tiled_mode;
        udec_configs[i].frm_chroma_fmt_max = mpDSPConfig->udecInstanceConfig[i].frm_chroma_fmt_max;
        udec_configs[i].dec_types = mpDSPConfig->udecInstanceConfig[i].dec_types;
        udec_configs[i].max_frm_num = mpDSPConfig->udecInstanceConfig[i].max_frm_num;
        udec_configs[i].max_frm_width = mpDSPConfig->udecInstanceConfig[i].max_frm_width;
        udec_configs[i].max_frm_height = mpDSPConfig->udecInstanceConfig[i].max_frm_height;
        udec_configs[i].max_fifo_size = mpDSPConfig->udecInstanceConfig[i].max_fifo_size;
        LOGM_INFO("[%d] udec_configs[i].max_frm_width %d, udec_configs[i].max_frm_height %d\n", i, udec_configs[i].max_frm_width, udec_configs[i].max_frm_height);
    }

    LOGM_INFO("[flow]: before IAV_IOC_ENTER_UDEC_MODE, udec_num %d\n", udec_num);
    if ((ret = ioctl(mIavFd, IAV_IOC_ENTER_UDEC_MODE, &udec_mode)) < 0) {
        perror("IAV_IOC_ENTER_MDEC_MODE");
        LOGM_ERROR(" enter mdec mode fail\n");
        free(udec_configs);
        return EECode_OSError;
    }
    LOGM_INFO("[flow]: after IAV_IOC_ENTER_UDEC_MODE\n");

    free(udec_configs);
    return EECode_OK;
}

EECode CIOneDSPAPI::enterMudecMode()
{
    //enter mdec mode config
    iav_mdec_mode_config_t mdec_mode;
    iav_udec_mode_config_t *udec_mode = &mdec_mode.super;
    iav_udec_config_t *udec_configs;
    udec_window_t *windows;
    udec_render_t *renders;

    TUint udec_num = 5, win_num = 5, render_num = 5;
    TUint i = 0;

    TInt ret;

    DASSERT(mpDSPConfig);
    if (!mpDSPConfig) {
        LOGM_ERROR("NULL mpDSPConfig, should invoke DSPModeConfig() first\n");
        return EECode_BadState;
    }

    udec_num = mpDSPConfig->modeConfig.num_udecs;
    win_num = mpDSPConfig->modeConfigMudec.total_num_win_configs;
    render_num = mpDSPConfig->modeConfigMudec.total_num_render_configs;
    LOGM_INFO("[flow enterMudecMode]: %p, udec_num %u, win_num %u, render_num %u, voutmask 0x%02x.\n", &mpDSPConfig->modeConfigMudec, udec_num, win_num, render_num, mpDSPConfig->modeConfig.vout_mask);

    if ((udec_configs = (iav_udec_config_t *)DDBG_MALLOC(udec_num * sizeof(iav_udec_config_t), "D0UC")) == NULL) {
        LOGM_ERROR(" no memory\n");
        return EECode_NoMemory;
    }

    if ((windows = (udec_window_t *)DDBG_MALLOC(win_num * sizeof(udec_window_t), "D0WT")) == NULL) {
        LOGM_ERROR(" no memory\n");
        DDBG_FREE(udec_configs, "D0UC");
        return EECode_NoMemory;
    }

    if ((renders = (udec_render_t *)DDBG_MALLOC(render_num * sizeof(udec_render_t), "D0RT")) == NULL) {
        LOGM_ERROR(" no memory\n");
        DDBG_FREE(udec_configs, "D0UC");
        DDBG_FREE(windows, "D0WT");
        return EECode_NoMemory;
    }

    LOGM_DEBUG("[flow enterMudecMode]: before init_udec_mode_config\n");

    memset(&mdec_mode, 0, sizeof(mdec_mode));
    udec_mode->postp_mode = mpDSPConfig->modeConfig.postp_mode;
    DASSERT(3 == udec_mode->postp_mode);

    udec_mode->enable_deint = mpDSPConfig->enable_deint;
    udec_mode->pp_chroma_fmt_max = mpDSPConfig->modeConfig.pp_chroma_fmt_max;
    DASSERT(2 == udec_mode->pp_chroma_fmt_max);

    udec_mode->pp_max_frm_width = mpDSPConfig->modeConfig.pp_max_frm_width;
    udec_mode->pp_max_frm_height = mpDSPConfig->modeConfig.pp_max_frm_height;
    udec_mode->pp_max_frm_num = mpDSPConfig->modeConfig.pp_max_frm_num;
    DASSERT(5 == udec_mode->pp_max_frm_num);

    udec_mode->pp_background_Y = mpDSPConfig->modeConfig.pp_background_Y;
    udec_mode->pp_background_Cb = mpDSPConfig->modeConfig.pp_background_Cb;
    udec_mode->pp_background_Cr = mpDSPConfig->modeConfig.pp_background_Cr;

    udec_mode->vout_mask = mpDSPConfig->modeConfig.vout_mask;
    udec_mode->primary_vout = mpDSPConfig->modeConfig.primary_vout;

    udec_mode->num_udecs = mpDSPConfig->modeConfig.num_udecs;
    udec_mode->udec_config = udec_configs;
    if (mpDSPConfig->modeConfig.enable_transcode) {
        udec_mode->enable_transcode = 1;
        udec_mode->udec_transcoder_config.total_channel_number = 1;
        udec_mode->udec_transcoder_config.transcoder_config[0].main_type = DSP_ENCRM_TYPE_H264;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_type = DSP_ENCRM_TYPE_NONE;
        udec_mode->udec_transcoder_config.transcoder_config[0].piv_type = DSP_ENCRM_TYPE_NONE;
        udec_mode->udec_transcoder_config.transcoder_config[0].mctf_flag = 1;
        udec_mode->udec_transcoder_config.transcoder_config[0].encoding_width = 1280;
        udec_mode->udec_transcoder_config.transcoder_config[0].encoding_height = 720;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_encoding_width = 0;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_encoding_height = 0;
    }

    mdec_mode.total_num_win_configs = win_num;
    mdec_mode.total_num_render_configs = render_num;
    mdec_mode.windows_config = windows;
    mdec_mode.render_config = renders;

    mdec_mode.pre_buffer_len = mpDSPConfig->modeConfigMudec.pre_buffer_len;
    mdec_mode.enable_buffering_ctrl = mpDSPConfig->modeConfigMudec.enable_buffering_ctrl;
    mdec_mode.av_sync_enabled = mpDSPConfig->modeConfigMudec.av_sync_enabled;
    mdec_mode.voutA_enabled = mpDSPConfig->modeConfigMudec.voutA_enabled;
    mdec_mode.voutB_enabled = mpDSPConfig->modeConfigMudec.voutB_enabled;

    mdec_mode.video_win_width = mpDSPConfig->modeConfigMudec.video_win_width;
    mdec_mode.video_win_height = mpDSPConfig->modeConfigMudec.video_win_height;
    if (!mpDSPConfig->modeConfigMudec.frame_rate_in_tick) {
        LOGM_WARN("not set transcode framerate, use default\n");
        mdec_mode.frame_rate_in_ticks = DDefaultVideoFramerateDen;
    } else {
        LOGM_PRINTF("set transcode framerate %f, tick %d\n", (float)DDefaultTimeScale / (float)mpDSPConfig->modeConfigMudec.frame_rate_in_tick, mpDSPConfig->modeConfigMudec.frame_rate_in_tick);
        mdec_mode.frame_rate_in_ticks = mpDSPConfig->modeConfigMudec.frame_rate_in_tick;
    }

    //udec config
    memset(udec_configs, 0, sizeof(iav_udec_config_t) * udec_num);
    for (i = 0; i < udec_num; i++) {
        udec_configs[i].tiled_mode = mpDSPConfig->udecInstanceConfig[i].tiled_mode;
        udec_configs[i].frm_chroma_fmt_max = mpDSPConfig->udecInstanceConfig[i].frm_chroma_fmt_max;
        udec_configs[i].dec_types = mpDSPConfig->udecInstanceConfig[i].dec_types;
        udec_configs[i].max_frm_num = mpDSPConfig->udecInstanceConfig[i].max_frm_num;
        udec_configs[i].max_frm_width = mpDSPConfig->udecInstanceConfig[i].max_frm_width;
        udec_configs[i].max_frm_height = mpDSPConfig->udecInstanceConfig[i].max_frm_height;
        udec_configs[i].max_fifo_size = mpDSPConfig->udecInstanceConfig[i].max_fifo_size;
    }

    //window config
    memset(windows, 0, sizeof(udec_window_t) * win_num);
    for (i = 0; i < win_num; i++) {
        windows[i].win_config_id = mpDSPConfig->modeConfigMudec.windows_config[i].win_config_id;
        windows[i].input_offset_x = mpDSPConfig->modeConfigMudec.windows_config[i].input_offset_x;
        windows[i].input_offset_y = mpDSPConfig->modeConfigMudec.windows_config[i].input_offset_y;
        windows[i].input_width = mpDSPConfig->modeConfigMudec.windows_config[i].input_width;
        windows[i].input_height = mpDSPConfig->modeConfigMudec.windows_config[i].input_height;
        windows[i].target_win_offset_x = mpDSPConfig->modeConfigMudec.windows_config[i].target_win_offset_x;
        windows[i].target_win_offset_y = mpDSPConfig->modeConfigMudec.windows_config[i].target_win_offset_y;
        windows[i].target_win_width = mpDSPConfig->modeConfigMudec.windows_config[i].target_win_width;
        windows[i].target_win_height = mpDSPConfig->modeConfigMudec.windows_config[i].target_win_height;
    }

    //render config
    memset(renders, 0, sizeof(udec_render_t) * render_num);
    for (i = 0; i < render_num; i++) {
        renders[i].render_id = mpDSPConfig->modeConfigMudec.render_config[i].render_id;
        renders[i].win_config_id = mpDSPConfig->modeConfigMudec.render_config[i].win_config_id;
        renders[i].win_config_id_2nd = mpDSPConfig->modeConfigMudec.render_config[i].win_config_id_2nd;
        renders[i].udec_id = mpDSPConfig->modeConfigMudec.render_config[i].udec_id;
        renders[i].first_pts_low = mpDSPConfig->modeConfigMudec.render_config[i].first_pts_low;
        renders[i].first_pts_high = mpDSPConfig->modeConfigMudec.render_config[i].first_pts_high;
        renders[i].input_source_type = mpDSPConfig->modeConfigMudec.render_config[i].input_source_type;
    }

    mdec_mode.max_num_windows = win_num + 1;

    LOGM_INFO("[flow]: before IAV_IOC_ENTER_MDEC_MODE, num render configs %d, num win configs %d, max num windows %d\n", mdec_mode.total_num_render_configs, mdec_mode.total_num_win_configs, mdec_mode.max_num_windows);
    if ((ret = ioctl(mIavFd, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode)) < 0) {
        perror("IAV_IOC_ENTER_MDEC_MODE");
        LOGM_ERROR(" enter mdec mode fail\n");
        free(udec_configs);
        free(windows);
        free(renders);
        return EECode_OSError;
    }

    free(udec_configs);
    free(windows);
    free(renders);
    return EECode_OK;
}

void CIOneDSPAPI::updateCurrentDSPMode()
{
    TInt state;
    TInt ret;

    DASSERT(0 <= mIavFd);
    ret = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);

    DASSERT(0 == ret);

    switch (state) {

        case IAV_STATE_IDLE:
            mDSPMode = EDSPOperationMode_IDLE;
            break;

        case IAV_STATE_DECODING:
            DASSERT((EDSPOperationMode_UDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC_Transcode == mDSPMode));
            if ((EDSPOperationMode_UDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC == mDSPMode) || (EDSPOperationMode_MultipleWindowUDEC_Transcode == mDSPMode)) {
                //do nothing
            } else {
                LOGM_WARN("should not comes here, guess dsp mode is UDEC\n");
                mDSPMode = EDSPOperationMode_UDEC;
            }
            break;

        case IAV_STATE_PREVIEW:
        case IAV_STATE_ENCODING:
        case IAV_STATE_STILL_CAPTURE:
            mDSPMode = EDSPOperationMode_CameraRecording;
            break;

        case IAV_STATE_TRANSCODING:
            mDSPMode = EDSPOperationMode_Transcode;
            break;

        case IAV_STATE_DUPLEX:
            mDSPMode = EDSPOperationMode_FullDuplex;
            break;

        case IAV_STATE_INIT:
            LOGM_FATAL("dsp not initialized?\n");
            mDSPMode = EDSPOperationMode_Invalid;
            break;

        default:
            LOGM_FATAL("BAD state %d, iav driver have problem?\n", state);
            break;
    }

    return;
}

EECode CIOneDSPAPI::EnterDSPMode(TUint mode)
{
    TInt ret;
    EECode err;

    if (mIavFd < 0) {
        LOGM_ERROR("BAD iav fd %d, need invoke OpenDevice() first?\n", mIavFd);
        return EECode_BadState;
    }

    if (EDSPOperationMode_Invalid == mDSPMode) {
        updateCurrentDSPMode();
    }

    if (mode == mDSPMode) {
        DASSERT(mode != EDSPOperationMode_FullDuplex);
        LOGM_WARN("already in mode %d\n", mDSPMode);
        return EECode_OK;
    }

    switch (mode) {
        case EDSPOperationMode_IDLE:
            ret = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
            if (ret != 0) {
                LOGM_ERROR("DSP enter IDLE mode fail, ret %d.\n", ret);
                return EECode_OSError;
            }
            mDSPMode = EDSPOperationMode_IDLE;
            break;

        case EDSPOperationMode_UDEC:
            if (EDSPOperationMode_IDLE != mDSPMode) {
                LOGM_ERROR("dsp mode can only switch from IDLE to UDEC, current mode(%d) is not IDLE\n", mDSPMode);
                return EECode_OSError;
            }
            err = enterUdecMode();
            if (EECode_OK == err) {
                mDSPMode = EDSPOperationMode_UDEC;
            } else {
                LOGM_INFO("DSP enter UDEC mode(%d) fail, try again, enter idle first\n", mode);

                //try again, enter idle first
                ret = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
                if (ret != 0) {
                    LOGM_ERROR("DSP enter IDLE mode fail, ret %d.\n", ret);
                    return EECode_OSError;
                }
                mDSPMode = EDSPOperationMode_IDLE;

                LOGM_INFO("DSP enter IDLE mode done, then try enter mode %d...\n", mode);

                //then enther UDEC mode
                err = enterUdecMode();
                if (EECode_OK == err) {
                    mDSPMode = EDSPOperationMode_UDEC;
                    LOGM_INFO("DSP enter UDEC mode done\n");
                    return EECode_OK;
                }

                LOGM_ERROR("DSP enter UDEC mode again, fail again, ret %d.\n", err);
                return EECode_OSError;
            }
            break;

        case EDSPOperationMode_MultipleWindowUDEC:
        case EDSPOperationMode_MultipleWindowUDEC_Transcode:
            if (EDSPOperationMode_IDLE != mDSPMode) {
                LOGM_ERROR("dsp mode can only switch from IDLE to MUDEC, current mode(%d) is not IDLE\n", mDSPMode);
                return EECode_OSError;
            }
            err = enterMudecMode();
            if (EECode_OK == err) {
                mDSPMode = (EDSPOperationMode)mode;//EDSPOperationMode_MultipleWindowUDEC;
            } else {
                LOGM_ERROR("DSP enter MUDEC mode(%d) fail\n", mode);
                return EECode_Error;
            }
            break;

        case EDSPOperationMode_CameraRecording:
            LOGM_ERROR("EDSPOperationMode_CameraRecording TO DO\n");
            break;

        case EDSPOperationMode_FullDuplex:
            LOGM_ERROR("EDSPOperationMode_FullDuplex TO DO\n");
            break;

        default:
            LOGM_ERROR("unknown dsp mode %d\n", mode);
            return EECode_BadParam;
            break;
    }

    return EECode_OK;
}

EECode CIOneDSPAPI::DSPControl(EDSPControlType type, void *p_params)
{
    if (EDSPOperationMode_MultipleWindowUDEC == mDSPMode
            || EDSPOperationMode_MultipleWindowUDEC_Transcode == mDSPMode) {

        switch (type) {

            case EDSPControlType_UDEC_pb_still_capture:
                if (p_params) {
                    SPlaybackCapture *params = (SPlaybackCapture *) p_params;
                    int ret;
                    TUint i = 0;
                    iav_udec_capture_t capture;
                    TU16 cap_max_width = 0, cap_max_height = 0;
                    TInt file_index = params->capture_file_index;

                    FILE *pfile = NULL;
                    char dump_file_name[320] = {0};

                    memset(&capture, 0x0, sizeof(capture));

                    capture.dec_id = params->capture_id;

                    if (params->capture_coded) {
                        capture.capture_coded = 1;
                    }

                    if (params->capture_screennail) {
                        capture.capture_screennail = 1;
                    }

                    if (params->capture_thumbnail) {
                        capture.capture_thumbnail = 1;
                    }

                    for (i = 0; i < CAPTURE_TOT_NUM; i ++) {
                        capture.capture[i].quality = params->jpeg_quality;
                    }

                    capture.capture[CAPTURE_CODED].target_pic_width = params->coded_width;
                    capture.capture[CAPTURE_CODED].target_pic_height = params->coded_height;

                    if (cap_max_width < capture.capture[CAPTURE_CODED].target_pic_width) {
                        cap_max_width = capture.capture[CAPTURE_CODED].target_pic_width;
                    }
                    if (cap_max_height < capture.capture[CAPTURE_CODED].target_pic_height) {
                        cap_max_height = capture.capture[CAPTURE_CODED].target_pic_height;
                    }

                    capture.capture[CAPTURE_THUMBNAIL].target_pic_width = params->thumbnail_width;
                    capture.capture[CAPTURE_THUMBNAIL].target_pic_height = params->thumbnail_height;

                    if (cap_max_width < capture.capture[CAPTURE_THUMBNAIL].target_pic_width) {
                        cap_max_width = capture.capture[CAPTURE_THUMBNAIL].target_pic_width;
                    }
                    if (cap_max_height < capture.capture[CAPTURE_THUMBNAIL].target_pic_height) {
                        cap_max_height = capture.capture[CAPTURE_THUMBNAIL].target_pic_height;
                    }

                    capture.capture[CAPTURE_SCREENNAIL].target_pic_width = params->screennail_width;
                    capture.capture[CAPTURE_SCREENNAIL].target_pic_height = params->screennail_height;

                    if (cap_max_width < capture.capture[CAPTURE_SCREENNAIL].target_pic_width) {
                        cap_max_width = capture.capture[CAPTURE_SCREENNAIL].target_pic_width;
                    }
                    if (cap_max_height < capture.capture[CAPTURE_SCREENNAIL].target_pic_height) {
                        cap_max_height = capture.capture[CAPTURE_SCREENNAIL].target_pic_height;
                    }

                    DASSERT(capture.capture[CAPTURE_CODED].target_pic_width <= 1920);
                    DASSERT(capture.capture[CAPTURE_CODED].target_pic_height <= 1080);//or 1088?

                    if (capture.capture[CAPTURE_CODED].target_pic_width > 1920) {
                        capture.capture[CAPTURE_CODED].target_pic_width = 1920;//cap_max_width;
                    }
                    if (capture.capture[CAPTURE_CODED].target_pic_height > 1080) {
                        capture.capture[CAPTURE_CODED].target_pic_height = 1080;//cap_max_height;
                    }

                    LOGM_INFO("[cmd flow]: before IAV_IOC_UDEC_CAPTURE, decoder_id %02x, coded %dx%d, thumbnail %dx%d, screennail %dx%d, max %dx%d.\n", capture.dec_id, \
                              capture.capture[CAPTURE_CODED].target_pic_width, capture.capture[CAPTURE_CODED].target_pic_height, \
                              capture.capture[CAPTURE_THUMBNAIL].target_pic_width, capture.capture[CAPTURE_THUMBNAIL].target_pic_height, \
                              capture.capture[CAPTURE_SCREENNAIL].target_pic_width, capture.capture[CAPTURE_SCREENNAIL].target_pic_height, \
                              cap_max_width, cap_max_height \
                             );

                    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_CAPTURE, &capture)) < 0) {
                        perror("IAV_IOC_UDEC_CAPTURE");
                        return EECode_OSError;
                    }

                    LOGM_INFO("[cmd flow]: after IAV_IOC_UDEC_CAPTURE.\n");

                    //save bit-stream
                    if (capture.capture_coded) {
                        sprintf(dump_file_name, "coded_%02x_%02x.jpeg", params->source_id, file_index);
                        pfile = fopen(dump_file_name, "wb");
                        if (pfile) {
                            DASSERT(capture.capture[CAPTURE_CODED].buffer_base);
                            DASSERT(capture.capture[CAPTURE_CODED].buffer_limit);
                            DASSERT(capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit);
                            LOGM_INFO("[pb capture]: coded buffer start %p, end %p, size %d\n", (void *)capture.capture[CAPTURE_CODED].buffer_base, (void *)capture.capture[CAPTURE_CODED].buffer_limit, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base);
                            if (capture.capture[CAPTURE_CODED].buffer_base < capture.capture[CAPTURE_CODED].buffer_limit) {
                                fwrite((void *)capture.capture[CAPTURE_CODED].buffer_base, 1, capture.capture[CAPTURE_CODED].buffer_limit - capture.capture[CAPTURE_CODED].buffer_base, pfile);
                            }
                            fclose(pfile);
                        }
                    } else {
                        LOGM_NOTICE("EDSPControlType_UDEC_pb_still_capture, capture_coded %u, coded capture file will not be saved.\n", capture.capture_coded);
                    }

                    if (capture.capture_thumbnail) {
                        sprintf(dump_file_name, "thumb_%02x_%02x.jpeg", params->source_id, file_index);
                        pfile = fopen(dump_file_name, "wb");
                        if (pfile) {
                            DASSERT(capture.capture[CAPTURE_THUMBNAIL].buffer_base);
                            DASSERT(capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
                            DASSERT(capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit);
                            LOGM_INFO("[pb capture]: coded buffer start %p, end %p, size %d\n", (void *)capture.capture[CAPTURE_THUMBNAIL].buffer_base, (void *)capture.capture[CAPTURE_THUMBNAIL].buffer_limit, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base);
                            if (capture.capture[CAPTURE_THUMBNAIL].buffer_base < capture.capture[CAPTURE_THUMBNAIL].buffer_limit) {
                                fwrite((void *)capture.capture[CAPTURE_THUMBNAIL].buffer_base, 1, capture.capture[CAPTURE_THUMBNAIL].buffer_limit - capture.capture[CAPTURE_THUMBNAIL].buffer_base, pfile);
                            }
                            fclose(pfile);
                        }
                    } else {
                        LOGM_NOTICE("EDSPControlType_UDEC_pb_still_capture, capture_thumbnail %u, thumbnail capture file will not be saved.\n", capture.capture_thumbnail);
                    }

                    if (capture.capture_screennail) {
                        sprintf(dump_file_name, "screen_%02x_%02x.jpeg", params->source_id, file_index);
                        pfile = fopen(dump_file_name, "wb");
                        if (pfile) {
                            DASSERT(capture.capture[CAPTURE_SCREENNAIL].buffer_base);
                            DASSERT(capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
                            DASSERT(capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit);
                            LOGM_INFO("[pb capture]: coded buffer start %p, end %p, size %d\n", (void *)capture.capture[CAPTURE_SCREENNAIL].buffer_base, (void *)capture.capture[CAPTURE_SCREENNAIL].buffer_limit, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base);
                            if (capture.capture[CAPTURE_SCREENNAIL].buffer_base < capture.capture[CAPTURE_SCREENNAIL].buffer_limit) {
                                fwrite((void *)capture.capture[CAPTURE_SCREENNAIL].buffer_base, 1, capture.capture[CAPTURE_SCREENNAIL].buffer_limit - capture.capture[CAPTURE_SCREENNAIL].buffer_base, pfile);
                            }
                            fclose(pfile);
                        }
                    } else {
                        LOGM_NOTICE("EDSPControlType_UDEC_pb_still_capture, capture_screennail %u, screennail capture file will not be saved.\n", capture.capture_screennail);
                    }
                } else {
                    LOGM_ERROR("NULL params\n");
                }
                break;

            case EDSPControlType_UDEC_zoom:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    int ret;
                    iav_udec_zoom_t zoom;
                    memset(&zoom, 0x0, sizeof(zoom));

                    if (!params->u8_param[1]) {
                        //mode 2
                        zoom.render_id = params->u8_param[0];
                        zoom.input_center_x = params->u16_param[0];
                        zoom.input_center_y = params->u16_param[1];
                        zoom.input_width = params->u16_param[2];
                        zoom.input_height = params->u16_param[3];
                        LOGM_INFO("[cmd flow]: before IAV_IOC_UDEC_ZOOM(mode 2), render_id %d, input width %d, height %d, center x %d, center y %d.\n", zoom.render_id, zoom.input_width, zoom.input_height, zoom.input_center_x, zoom.input_center_y);
                    } else {
                        zoom.render_id = params->u8_param[0];
                        zoom.zoom_factor_x = params->u32_param[0];
                        zoom.zoom_factor_y = params->u32_param[1];
                        LOGM_INFO("[cmd flow]: before IAV_IOC_UDEC_ZOOM(mode 1), render_id %d, zoom factor x 0x%08x, zoom factor y 0x%08x.\n", zoom.render_id, zoom.zoom_factor_x, zoom.zoom_factor_y);
                    }

                    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_ZOOM, &zoom)) < 0) {
                        perror("IAV_IOC_UDEC_ZOOM");
                        return EECode_OSError;
                    }
                    LOGM_INFO("[cmd flow]: after IAV_IOC_UDEC_ZOOM.\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                }
                break;

            case EDSPControlType_UDEC_stream_switch:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    int ret;
                    iav_postp_stream_switch_t stream_switch;
                    memset(&stream_switch, 0x0, sizeof(stream_switch));

                    stream_switch.num_config = 1;
                    stream_switch.switch_config[0].render_id = params->u8_param[0];
                    stream_switch.switch_config[0].new_udec_id = params->u8_param[1];

                    LOGM_INFO("[cmd flow]: before IAV_IOC_POSTP_STREAM_SWITCH, render_id %d, new_udec_index %d.\n", stream_switch.switch_config[0].render_id, stream_switch.switch_config[0].new_udec_id);
                    if ((ret = ioctl(mIavFd, IAV_IOC_POSTP_STREAM_SWITCH, &stream_switch)) < 0) {
                        perror("IAV_IOC_POSTP_STREAM_SWITCH");
                        return EECode_OSError;
                    }
                    LOGM_INFO("[cmd flow]: after IAV_IOC_POSTP_STREAM_SWITCH.\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                }
                break;

            case EDSPControlType_UDEC_render_config:
                if (p_params) {
                    int ret;
                    TInt offset = 0;
                    iav_postp_render_config_t render;
                    SVideoPostPConfiguration *p_postp = (SVideoPostPConfiguration *) p_params;

                    memset(&render, 0x0, sizeof(render));

                    render.total_num_windows_to_render = p_postp->cur_render_number;
                    render.num_configs = p_postp->cur_render_number;
                    DASSERT(render.num_configs < 10);
                    for (ret = 0; ret < render.num_configs; ret ++) {
                        offset = p_postp->cur_render_start_index + ret;
                        render.configs[ret].render_id = p_postp->render[offset].render_id;
                        render.configs[ret].win_config_id = p_postp->render[offset].win_config_id;
                        render.configs[ret].win_config_id_2nd = p_postp->render[offset].win_config_id_2nd;
                        render.configs[ret].udec_id = p_postp->render[offset].udec_id;
                        render.configs[ret].first_pts_high = p_postp->render[offset].first_pts_high;
                        render.configs[ret].first_pts_low = p_postp->render[offset].first_pts_low;
                        render.configs[ret].input_source_type = p_postp->render[offset].input_source_type;
                        LOGM_INFO("    [render %d]: win %d, win_2rd %d, udec_id %d.\n", render.configs[ret].render_id, render.configs[ret].win_config_id, render.configs[ret].win_config_id_2nd, render.configs[ret].udec_id);
                    }

                    LOGM_INFO("[cmd flow]: before IAV_IOC_POSTP_RENDER_CONFIG.\n");
                    if ((ret = ioctl(mIavFd, IAV_IOC_POSTP_RENDER_CONFIG, &render)) < 0) {
                        perror("IAV_IOC_POSTP_RENDER_CONFIG");
                        return EECode_OSError;
                    }
                    LOGM_INFO("[cmd flow]: after IAV_IOC_POSTP_RENDER_CONFIG.\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                }
                break;

            case EDSPControlType_UDEC_update_multiple_window_display:

                if (p_params) {
                    int ret;
                    TUint i = 0, j = 0;
                    iav_postp_update_mw_display_t display;
                    SVideoPostPConfiguration *p_postp = (SVideoPostPConfiguration *) p_params;
                    udec_window_t window[DMaxPostPWindowNumber];
                    udec_render_t render[DMaxPostPRenderNumber];

                    memset(&display, 0x0, sizeof(display));
                    memset(&window, 0x0, sizeof(window));
                    memset(&render, 0x0, sizeof(render));

                    if (!p_postp->single_view_mode) {
                        display.max_num_windows = p_postp->cur_window_number + 1;
                        display.total_num_win_configs = p_postp->cur_window_number;
                        display.total_num_render_configs = p_postp->cur_render_number;

                        LOGM_NOTICE("display.max_num_windows %d, display.total_num_win_configs %d, display.total_num_render_configs %d\n", display.max_num_windows, display.total_num_win_configs, display.total_num_render_configs);

                        display.audio_on_win_id = 0;
                        display.pre_buffer_len = 0;
                        display.enable_buffering_ctrl = 0;

                        display.av_sync_enabled = 0;

                        if (p_postp->voutA_enabled) {
                            display.voutA_enabled = 1;
                        } else {
                            display.voutA_enabled = 0;
                        }

                        if (p_postp->voutB_enabled) {
                            display.voutB_enabled = 1;
                        } else {
                            display.voutB_enabled = 0;
                        }

                        LOGM_NOTICE("p_postp->voutA_enabled %d, p_postp->voutB_enabled %d, display.voutA_enabled %d, display.voutB_enabled %d\n", p_postp->voutA_enabled, p_postp->voutB_enabled, display.voutA_enabled, display.voutB_enabled);

                        display.video_win_width = p_postp->display_width;
                        display.video_win_height = p_postp->display_height;

                        for (i = p_postp->cur_render_start_index, j = 0; i < p_postp->cur_render_number; i ++, j ++) {
                            render[j].render_id = p_postp->render[i].render_id;
                            render[j].win_config_id = p_postp->render[i].win_config_id;
                            render[j].win_config_id_2nd = p_postp->render[i].win_config_id_2nd;
                            render[j].udec_id = p_postp->render[i].udec_id;
                            render[j].first_pts_low = p_postp->render[i].first_pts_low;
                            render[j].first_pts_high = p_postp->render[i].first_pts_high;
                            render[j].input_source_type = p_postp->render[i].input_source_type;
                        }

                        for (i = p_postp->cur_window_start_index, j = 0; i < p_postp->cur_window_number; i ++, j ++) {
                            window[j].win_config_id = p_postp->window[i].win_config_id;
                            window[j].input_offset_x = p_postp->window[i].input_offset_x;
                            window[j].input_offset_y = p_postp->window[i].input_offset_y;
                            window[j].input_width = p_postp->window[i].input_width;
                            window[j].input_height = p_postp->window[i].input_height;
                            window[j].target_win_offset_x = p_postp->window[i].target_win_offset_x;
                            window[j].target_win_offset_y = p_postp->window[i].target_win_offset_y;
                            window[j].target_win_width = p_postp->window[i].target_win_width;
                            window[j].target_win_height = p_postp->window[i].target_win_height;
                        }

                        display.windows_config = &window[0];
                        display.render_config = &render[0];
                    } else {
                        display.max_num_windows = 2;
                        display.total_num_win_configs = 1;
                        display.total_num_render_configs = 1;

                        display.audio_on_win_id = 0;
                        display.pre_buffer_len = 0;
                        display.enable_buffering_ctrl = 0;

                        display.av_sync_enabled = 0;

                        if (p_postp->voutA_enabled) {
                            display.voutA_enabled = 1;
                        } else {
                            display.voutA_enabled = 0;
                        }

                        if (p_postp->voutB_enabled) {
                            display.voutB_enabled = 1;
                        } else {
                            display.voutB_enabled = 0;
                        }

                        LOGM_INFO("p_postp->voutA_enabled %d, p_postp->voutB_enabled %d, display.voutA_enabled %d, display.voutA_enabled %d\n", p_postp->voutA_enabled, p_postp->voutB_enabled, display.voutA_enabled, display.voutB_enabled);

                        display.video_win_width = p_postp->display_width;
                        display.video_win_height = p_postp->display_height;

                        render[0].render_id = p_postp->single_view_render.render_id;
                        render[0].win_config_id = p_postp->single_view_render.win_config_id;
                        render[0].win_config_id_2nd = p_postp->single_view_render.win_config_id_2nd;
                        render[0].udec_id = p_postp->single_view_render.udec_id;
                        render[0].first_pts_low = p_postp->single_view_render.first_pts_low;
                        render[0].first_pts_high = p_postp->single_view_render.first_pts_high;
                        render[0].input_source_type = p_postp->single_view_render.input_source_type;

                        window[0].win_config_id = p_postp->single_view_window.win_config_id;
                        window[0].input_offset_x = p_postp->single_view_window.input_offset_x;
                        window[0].input_offset_y = p_postp->single_view_window.input_offset_y;
                        window[0].input_width = p_postp->single_view_window.input_width;
                        window[0].input_height = p_postp->single_view_window.input_height;
                        window[0].target_win_offset_x = p_postp->single_view_window.target_win_offset_x;
                        window[0].target_win_offset_y = p_postp->single_view_window.target_win_offset_y;
                        window[0].target_win_width = p_postp->single_view_window.target_win_width;
                        window[0].target_win_height = p_postp->single_view_window.target_win_height;

                        display.windows_config = &window[0];
                        display.render_config = &render[0];
                    }

                    LOGM_INFO("before IAV_IOC_POSTP_UPDATE_MW_DISPLAY\n");
                    ret = ioctl(mIavFd, IAV_IOC_POSTP_UPDATE_MW_DISPLAY, &display);
                    if (ret < 0) {
                        perror("IAV_IOC_POSTP_UPDATE_MW_DISPLAY");
                        LOGM_ERROR("IAV_IOC_POSTP_UPDATE_MW_DISPLAY fail, ret %d\n", ret);
                    }
                    LOGM_INFO("IAV_IOC_POSTP_UPDATE_MW_DISPLAY done\n");

                }

                break;

            case EDSPControlType_UDEC_window_config:
                if (p_params) {
                    int ret;
                    TUint i = 0, j = 0;
                    iav_postp_window_config_t windows;
                    SVideoPostPConfiguration *p_postp = (SVideoPostPConfiguration *) p_params;

                    if (DUnlikely(p_postp->cur_window_number > 12)) {
                        LOGM_FATAL("BAD p_postp->cur_window_number %d\n", p_postp->cur_window_number);
                        return EECode_BadParam;
                    }
                    memset(&windows, 0x0, sizeof(iav_postp_window_config_t));
                    for (i = p_postp->cur_window_start_index, j = 0; i < p_postp->cur_window_number; i ++, j ++) {
                        windows.configs[j].win_config_id = p_postp->window[i].win_config_id;
                        windows.configs[j].input_offset_x = p_postp->window[i].input_offset_x;
                        windows.configs[j].input_offset_y = p_postp->window[i].input_offset_y;
                        windows.configs[j].input_width = p_postp->window[i].input_width;
                        windows.configs[j].input_height = p_postp->window[i].input_height;
                        windows.configs[j].target_win_offset_x = p_postp->window[i].target_win_offset_x;
                        windows.configs[j].target_win_offset_y = p_postp->window[i].target_win_offset_y;
                        windows.configs[j].target_win_width = p_postp->window[i].target_win_width;
                        windows.configs[j].target_win_height = p_postp->window[i].target_win_height;
                    }
                    windows.num_configs = p_postp->cur_window_number - p_postp->cur_window_start_index;

                    LOGM_INFO("before IAV_IOC_POSTP_WINDOW_CONFIG\n");
                    ret = ioctl(mIavFd, IAV_IOC_POSTP_WINDOW_CONFIG, &windows);
                    if (ret < 0) {
                        perror("IAV_IOC_POSTP_WINDOW_CONFIG");
                        LOGM_ERROR("IAV_IOC_POSTP_WINDOW_CONFIG fail, ret %d\n", ret);
                    }
                    LOGM_INFO("IAV_IOC_POSTP_WINDOW_CONFIG done\n");

                }
                break;

            case EDSPControlType_UDEC_one_window_config:
                if (p_params) {
                    int ret;
                    iav_postp_window_config_t windows;
                    SVideoPostPConfiguration *p_postp = (SVideoPostPConfiguration *) p_params;

                    if (DUnlikely(p_postp->cur_window_number < p_postp->input_param_update_window_index)) {
                        LOGM_FATAL("BAD p_postp->input_param_update_window_index %d, p_postp->cur_window_number %d\n", p_postp->input_param_update_window_index, p_postp->cur_window_number);
                        return EECode_BadParam;
                    }
                    int window_index = p_postp->input_param_update_window_index;

                    memset(&windows, 0x0, sizeof(iav_postp_window_config_t));
                    windows.configs[0].win_config_id = p_postp->window[window_index].win_config_id;
                    windows.configs[0].input_offset_x = p_postp->window[window_index].input_offset_x;
                    windows.configs[0].input_offset_y = p_postp->window[window_index].input_offset_y;
                    windows.configs[0].input_width = p_postp->window[window_index].input_width;
                    windows.configs[0].input_height = p_postp->window[window_index].input_height;
                    windows.configs[0].target_win_offset_x = p_postp->window[window_index].target_win_offset_x;
                    windows.configs[0].target_win_offset_y = p_postp->window[window_index].target_win_offset_y;
                    windows.configs[0].target_win_width = p_postp->window[window_index].target_win_width;
                    windows.configs[0].target_win_height = p_postp->window[window_index].target_win_height;
                    windows.num_configs = 1;

                    LOGM_INFO("before IAV_IOC_POSTP_WINDOW_CONFIG\n");
                    ret = ioctl(mIavFd, IAV_IOC_POSTP_WINDOW_CONFIG, &windows);
                    if (ret < 0) {
                        perror("IAV_IOC_POSTP_WINDOW_CONFIG");
                        LOGM_ERROR("IAV_IOC_POSTP_WINDOW_CONFIG fail, ret %d\n", ret);
                    }
                    LOGM_INFO("IAV_IOC_POSTP_WINDOW_CONFIG done\n");

                }
                break;

            case EDSPControlType_UDEC_wait_switch_done:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    int ret;
                    iav_wait_stream_switch_msg_t wait_stream_switch;
                    memset(&wait_stream_switch, 0x0, sizeof(wait_stream_switch));

                    wait_stream_switch.render_id = params->u8_param[0];
                    wait_stream_switch.wait_flags = 0;//blocked

                    LOGM_INFO("[cmd flow]: before IAV_IOC_WAIT_STREAM_SWITCH_MSG, render_id %d.\n", wait_stream_switch.render_id);
                    if ((ret = ioctl(mIavFd, IAV_IOC_WAIT_STREAM_SWITCH_MSG, &wait_stream_switch)) < 0) {
                        perror("IAV_IOC_WAIT_STREAM_SWITCH_MSG");
                        LOGM_ERROR("IAV_IOC_WAIT_STREAM_SWITCH_MSG fail, ret %d\n", ret);
                        return EECode_OSError;
                    }
                    LOGM_INFO("[cmd flow]: after IAV_IOC_WAIT_STREAM_SWITCH_MSG, status %d.\n", wait_stream_switch.switch_status);
                } else {
                    LOGM_ERROR("NULL params\n");
                }
                break;

            case EDSPControlType_UDEC_trickplay:
                if (p_params) {
                    SDSPTrickPlay *params = (SDSPTrickPlay *) p_params;
                    iav_udec_trickplay_t trickplay;
                    int ret;

                    trickplay.decoder_id = params->udec_id;
                    if (EDSPTrickPlay_Pause == params->trickplay_type) {
                        trickplay.mode = 0;
                    } else if (EDSPTrickPlay_Resume == params->trickplay_type) {
                        trickplay.mode = 1;
                    } else if (EDSPTrickPlay_Step == params->trickplay_type) {
                        trickplay.mode = 2;
                    } else {
                        LOGM_FATAL("BAD trickplay type %d\n", params->trickplay_type);
                        return EECode_BadParam;
                    }

                    LOGM_INFO("[flow cmd]: before IAV_IOC_UDEC_TRICKPLAY, udec(%d), trickplay_mode(%d).\n", trickplay.decoder_id, trickplay.mode);
                    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
                    if (ret < 0) {
                        perror("IAV_IOC_UDEC_TRICKPLAY");
                        LOGM_ERROR("!!!!!IAV_IOC_UDEC_TRICKPLAY error, ret %d.\n", ret);
                        if ((-EPERM) == ret) {
                            //to do
                        } else {
                            //to do
                        }
                        LOGM_ERROR("IAV_IOC_UDEC_TRICKPLAY fail, ret %d\n", ret);
                        return EECode_OSError;
                    }
                    LOGM_INFO("[flow cmd]: IAV_IOC_UDEC_TRICKPLAY done, udec(%d), trickplay_mode(%d).\n", trickplay.decoder_id, trickplay.mode);
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            case  EDSPControlType_UDEC_get_status:
                if (p_params) {
                    TInt ret = 0;
                    iav_udec_state_t state;
                    SDSPControlStatus *p_status = (SDSPControlStatus *)p_params;

                    memset(&state, 0, sizeof(state));
                    state.decoder_id = (TU8)p_status->decoder_id;
                    state.flags = IAV_UDEC_STATE_DSP_READ_POINTER | IAV_UDEC_STATE_BTIS_FIFO_ROOM | IAV_UDEC_STATE_ARM_WRITE_POINTER;

                    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
                    if (ret) {
                        perror("IAV_IOC_GET_UDEC_STATE");
                        LOGM_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
                        return EECode_Error;
                    }

                    p_status->error_code = state.error_code;
                    p_status->last_pts = ((TTime)state.current_pts_low) | (((TTime)state.current_pts_high) << 32);
                    p_status->dec_state = state.udec_state;
                    p_status->vout_state = state.vout_state;

                    p_status->dsp_current_read_bitsfifo_addr = state.dsp_current_read_bitsfifo_addr;
                    p_status->arm_last_write_bitsfifo_addr = state.arm_last_write_bitsfifo_addr;

                    p_status->dsp_current_read_bitsfifo_addr_phys = state.dsp_current_read_bitsfifo_addr_phys;
                    p_status->arm_last_write_bitsfifo_addr_phys = state.arm_last_write_bitsfifo_addr_phys;

                    p_status->bits_fifo_total_size = state.bits_fifo_total_size;
                    p_status->bits_fifo_free_size = state.bits_fifo_free_size;
                    p_status->bits_fifo_phys_start = state.bits_fifo_phys_start;

                    p_status->tot_decode_cmd_cnt = state.tot_decode_cmd_cnt;
                    p_status->tot_decode_frame_cnt = state.tot_decode_frame_cnt;
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            case EDSPControlType_UDEC_clear:
                if (p_params) {
                    TInt ret = 0;
                    SDSPControlClear *p_clear = (SDSPControlClear *)p_params;
                    TUint stop_code = (((p_clear->flag & 0xff) << 24) | p_clear->decoder_id);

                    LOGM_INFO("EDSPControlType_UDEC_clear start, stop_code 0x%x.\n", stop_code);
                    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_STOP error %d.\n", ret);
                        return EECode_OSError;
                    }
                    LOGM_INFO("EDSPControlType_UDEC_clear done.\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            case EDSPControlType_UDEC_TRANSCODER_update_params:
                if (p_params) {
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_params;
                    //update bitrate
                    iav_udec_transcoder_update_bitrate_t bitrate_t;
                    memset((void *)&bitrate_t, 0x0, sizeof(bitrate_t));

                    bitrate_t.id = p_encoder_params->index;
                    bitrate_t.average_bitrate = p_encoder_params->bitrate;
                    bitrate_t.pts_to_change_bitrate = p_encoder_params->bitrate_pts;

                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE start, index=%u bitrate %d pts=%llu\n", bitrate_t.id, bitrate_t.average_bitrate, bitrate_t.pts_to_change_bitrate);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE, &bitrate_t) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE done\n");
                    //update framerate
                    iav_udec_transcoder_update_framerate_t framerate_t;
                    memset((void *)&framerate_t, 0x0, sizeof(framerate_t));

                    framerate_t.id = p_encoder_params->index;
                    framerate_t.framerate_reduction_factor = p_encoder_params->framerate_reduce_factor;
                    framerate_t.framerate_code = p_encoder_params->framerate;

                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE index=%u framerate_factor %d, framerate_code %d\n", framerate_t.id, framerate_t.framerate_reduction_factor, framerate_t.framerate_code);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE, &framerate_t) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE done\n");
                    //update gop
                    //TODO
                    //demand IDR
                    //TODO
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;
            case EDSPControlType_UDEC_TRANSCODER_update_bitrate:
                if (p_params) {
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_params;
                    iav_udec_transcoder_update_bitrate_t bitrate_t;
                    memset((void *)&bitrate_t, 0x0, sizeof(bitrate_t));

                    bitrate_t.id = p_encoder_params->index;
                    bitrate_t.average_bitrate = p_encoder_params->bitrate;
                    bitrate_t.pts_to_change_bitrate = p_encoder_params->bitrate_pts;

                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE start, index=%u bitrate %d pts=%llu\n", bitrate_t.id, bitrate_t.average_bitrate, bitrate_t.pts_to_change_bitrate);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE, &bitrate_t) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE done\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;
            case EDSPControlType_UDEC_TRANSCODER_update_framerate:
                if (p_params) {
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_params;
                    iav_udec_transcoder_update_framerate_t framerate_t;
                    memset((void *)&framerate_t, 0x0, sizeof(framerate_t));

                    framerate_t.id = p_encoder_params->index;
                    framerate_t.framerate_reduction_factor = p_encoder_params->framerate_reduce_factor;
                    framerate_t.framerate_code = p_encoder_params->framerate_integer;

                    LOGM_NOTICE("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE index=%u framerate_factor %d, framerate_code %d\n", framerate_t.id, framerate_t.framerate_reduction_factor, framerate_t.framerate_code);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE, &framerate_t) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE done\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;
            case EDSPControlType_UDEC_TRANSCODER_update_gop:
                if (p_params) {
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_params;
                    iav_udec_transcoder_update_gop_t gop;
                    memset((void *)&gop, 0x0, sizeof(gop));

                    gop.id = p_encoder_params->index;
                    gop.change_gop_option = 2;//hard code, next I/P
                    gop.follow_gop = 0;
                    gop.fgop_max_N = 0;
                    gop.fgop_min_N = 0;
                    gop.M = p_encoder_params->gop_M;
                    gop.N = p_encoder_params->gop_N;
                    gop.idr_interval = p_encoder_params->gop_idr_interval;
                    gop.gop_structure = p_encoder_params->gop_structure;
                    gop.pts_to_change = 0xffffffffffffffffLL;//hard code

                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE index=%u, M %d, N %d, idr_interval %d, gop_structure %d\n", gop.id, gop.M, gop.N, gop.idr_interval, gop.gop_structure);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE, &gop) < 0) {
                        LOGM_ERROR("IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE done\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;
            case EDSPControlType_UDEC_TRANSCODER_demand_IDR:
                if (p_params) {
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_params;
                    iav_udec_transcoder_demand_idr_t demand_t;
                    memset((void *)&demand_t, 0x0, sizeof(demand_t));

                    demand_t.id = p_encoder_params->index;
                    demand_t.on_demand_idr = p_encoder_params->demand_idr_strategy;
                    demand_t.pts_to_change = 0;

                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR index=%u, on_demand_idr %d, pts_to_change %llu\n", demand_t.id, demand_t.on_demand_idr, demand_t.pts_to_change);
                    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR, &demand_t) < 0) {
                        LOGM_ERROR("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR");
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR done\n");
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            default:
                LOGM_ERROR("unknown cmd type %d\n", type);
                break;
        }
    } else if (EDSPOperationMode_UDEC == mDSPMode) {
        switch (type) {

            case EDSPControlType_UDEC_trickplay:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    iav_udec_trickplay_t trickplay;
                    int ret;

                    trickplay.decoder_id = params->u8_param[0];
                    trickplay.mode = params->u8_param[1];
                    LOGM_INFO("[flow cmd]: before IAV_IOC_UDEC_TRICKPLAY, udec(%d), trickplay_mode(%d).\n", trickplay.decoder_id, trickplay.mode);
                    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
                    if (ret < 0) {
                        perror("IAV_IOC_UDEC_TRICKPLAY");
                        LOGM_ERROR("!!!!!IAV_IOC_UDEC_TRICKPLAY error, ret %d.\n", ret);
                        if ((-EPERM) == ret) {
                            //to do
                        } else {
                            //to do
                        }
                        LOGM_ERROR("IAV_IOC_UDEC_TRICKPLAY fail, ret %d\n", ret);
                        return EECode_OSError;
                    }
                    LOGM_INFO("[flow cmd]: IAV_IOC_UDEC_TRICKPLAY done, udec(%d), trickplay_mode(%d).\n", trickplay.decoder_id, trickplay.mode);
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            case EDSPControlType_UDEC_wait_vout_dormant:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    TInt ret = 0;
                    iav_udec_state_t state;
                    memset(&state, 0, sizeof(state));
                    state.decoder_id = params->u8_param[0];
                    state.flags = 0;

                    LOGM_INFO("start in EDSPControlType_UDEC_wait_vout_dormant.\n");

                    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
                    DASSERT(ret == 0);
                    if (ret) {
                        perror("IAV_IOC_GET_UDEC_STATE");
                        LOGM_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
                        return EECode_OSError;
                    }

                    DASSERT(IAV_UDEC_STATE_RUN == state.udec_state || IAV_UDEC_STATE_READY == state.udec_state);
                    DASSERT(IAV_VOUT_STATE_PRE_RUN == state.vout_state || IAV_VOUT_STATE_DORMANT == state.vout_state);

                    //check vout_state
                    if (IAV_VOUT_STATE_DORMANT == state.vout_state) {
                        LOGM_INFO("[udec %d]: vout already in dormant state done, udec_state %d, vout state %d\n", params->u8_param[0], state.udec_state, state.vout_state);
                        return EECode_OK;
                    }

                    iav_wait_decoder_t wait;
                    memset(&wait, 0x0, sizeof(wait));
                    wait.decoder_id = params->u8_param[0];
                    wait.flags = IAV_WAIT_VOUT_STATE | IAV_WAIT_UDEC_EOS | IAV_WAIT_VDSP_INTERRUPT;
                    ret = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait);
                    if (ret < 0) {
                        LOGM_WARN("IAV_IOC_WAIT_DECODER fail, ret %d\n", ret);
                        return EECode_TryAgain;
                    } else {
                        if (IAV_WAIT_VOUT_STATE == wait.flags) {
                            memset(&state, 0, sizeof(state));
                            state.decoder_id = params->u8_param[0];
                            state.flags = 0;
                            ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
                            DASSERT(ret == 0);
                            if (ret) {
                                perror("IAV_IOC_GET_UDEC_STATE");
                                LOGM_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
                                return EECode_OSError;
                            }

                            if (IAV_VOUT_STATE_DORMANT == state.vout_state) {
                                DASSERT(IAV_UDEC_STATE_RUN == state.udec_state);
                                LOGM_INFO("vout state is already dormant? state.vout_state %d, state.udec_state %d\n", state.vout_state, state.udec_state);
                                return EECode_OK;
                            } else {
                                return EECode_TryAgain;
                            }
                        } else if (IAV_WAIT_UDEC_EOS == wait.flags) {
                            return EECode_Error;
                        } else if (IAV_WAIT_UDEC_ERROR == wait.flags) {
                            return EECode_Error;
                        } else if (IAV_WAIT_VDSP_INTERRUPT == wait.flags) {
                            return EECode_TryAgain;
                        } else {
                            LOGM_FATAL("[DEBUG FLOW]: why comes here, ioctl should have problem\n");
                            return EECode_InternalLogicalBug;
                        }
                    }
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            case EDSPControlType_UDEC_wake_vout:
                if (p_params) {
                    SDSPControlParams *params = (SDSPControlParams *) p_params;
                    iav_wake_vout_t wake_vout;
                    memset(&wake_vout, 0, sizeof(wake_vout));
                    wake_vout.decoder_id = params->u8_param[0];

                    LOGM_INFO("before IAV_IOC_WAKE_VOUT, dec_id %d\n", wake_vout.decoder_id);
                    if (ioctl(mIavFd, IAV_IOC_WAKE_VOUT, &wake_vout) < 0) {
                        perror("IAV_IOC_WAKE_VOUT");
                        LOGM_ERROR("IAV_IOC_WAKE_VOUT fail, dec_id %d\n", wake_vout.decoder_id);
                        return EECode_OSError;
                    }
                    LOGM_INFO("IAV_IOC_WAKE_VOUT done, dec_id %d\n", wake_vout.decoder_id);
                } else {
                    LOGM_ERROR("NULL params\n");
                    return EECode_BadParam;
                }
                break;

            default:
                LOGM_ERROR("unknown cmd type %d\n", type);
                break;
        }
    } else {
        LOGM_ERROR("need add support in dsp mode %d, cmd type %d\n", mDSPMode, type);
        return EECode_NoImplement;
    }

    LOGM_INFO("EDSPControlType %d, return EECode_OK\n", type);
    return EECode_OK;
}

EDSPPlatform CIOneDSPAPI::QueryDSPPlatform() const
{
    return EDSPPlatform_AmbaI1;
}

EDSPOperationMode CIOneDSPAPI::QueryCurrentDSPMode() const
{
    return mDSPMode;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

