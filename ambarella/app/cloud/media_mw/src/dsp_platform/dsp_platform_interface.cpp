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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "mw_internal.h"

#include "dsp_platform_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

typedef struct _SBitsWriter {
    TU8 *pStart, *pCur;
    TUint size;
    TUint leftBits;
    TUint leftBytes;
    TUint full;
} SBitsWriter;

enum {
    UDEC_VFormat_H264 = 1,
    UDEC_VFormat_MPEG4 = 2,
    UDEC_VFormat_VC1 = 3,
    UDEC_VFormat_WMV3 = 4,
    UDEC_VFormat_MPEG12 = 5,
};

#if BUILD_DSP_AMBA_I1
extern IDSPAPI *gfCreateiOneDSPAPI(const volatile SPersistMediaConfig *p_config);
extern IDSPDecAPI *gfCreateiOneDSPDecAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
extern IDSPEncAPI *gfCreateiOneDSPEncAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
#endif

#if BUILD_DSP_AMBA_S2
extern IDSPAPI *gfCreateS2DSPAPI(const volatile SPersistMediaConfig *p_config);
extern IDSPDecAPI *gfCreateS2DSPDecAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
extern IDSPEncAPI *gfCreateS2DSPEncAPI(TInt iav_fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
#endif

const TChar GUdecStringDecoderType[EH_DecoderType_Cnt][20] = {
    "Common ",
    "H264 ",
    "MPEG12 ",
    "MPEG4 HW ",
    "MPEG4 Hybird ",
    "VC-1 ",
    "RV40 Hybird ",
    "JPEG ",
    "SW "
};

const TChar GUdecStringErrorLevel[EH_ErrorLevel_Last][20] = {
    "NoError ",
    "Warning ",
    "Recoverable ",
    "Fatal "
};

const TChar GUdecStringCommonErrorType[EH_CommonErrorType_Last][20] = {
    "NoError ",
    "HardwareHang ",
    "BufferLeak "
};

IDSPAPI *gfDSPAPIFactory(const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform)
{
    IDSPAPI *api = NULL;

    switch (dsp_platform) {
        case EDSPPlatform_AutoSelect:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPAPI(p_config);
#elif BUILD_DSP_AMBA_S2
            api = gfCreateS2DSPAPI(p_config);
#else
            LOG_FATAL("no dsp platform compiled\n");
#endif
            break;

        case EDSPPlatform_AmbaI1:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPAPI(p_config);
#else
            LOG_FATAL("dsp amba i1 is not compiled\n");
#endif
            break;

        case EDSPPlatform_AmbaS2:
#if BUILD_DSP_AMBA_S2
            api = gfCreateS2DSPAPI(p_config);
#else
            LOG_FATAL("dsp amba s2 is not compiled\n");
#endif
            break;

        case EDSPPlatform_SimpleSW:
            LOG_ERROR("please implement EDSPPlatform_SimpleSW\n");
            break;

        case EDSPPlatform_CPUARMv7AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUARMv7AccelSIMD\n");
            break;

        case EDSPPlatform_CPUX86AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUX86AccelSIMD\n");
            break;

        case EDSPPlatform_GPUAccelNv:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelNv\n");
            break;

        case EDSPPlatform_GPUAccelAMD:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelAMD\n");
            break;

        case EDSPPlatform_GPUAccelImg:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelImg\n");
            break;

        default:
            LOG_FATAL("BAD EDSPPlatform %d\n", dsp_platform);
            break;
    }

    return api;
}

IDSPDecAPI *gfDSPDecAPIFactory(TInt iav_fd, EDSPOperationMode dsp_mode, StreamFormat codec_type, const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform)
{
    IDSPDecAPI *api = NULL;

    switch (dsp_platform) {
        case EDSPPlatform_AutoSelect:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPDecAPI(iav_fd, dsp_mode, p_config);
#elif BUILD_DSP_AMBA_S2
            api = gfCreateS2DSPDecAPI(iav_fd, dsp_mode, p_config);
#else
            LOG_FATAL("no dsp is compiled\n");
#endif
            break;

        case EDSPPlatform_AmbaI1:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPDecAPI(iav_fd, dsp_mode, p_config);
#else
            LOG_FATAL("dsp amba i1 is not compiled\n");
#endif
            break;

        case EDSPPlatform_AmbaS2:
#if BUILD_DSP_AMBA_S2
            api = gfCreateS2DSPDecAPI(iav_fd, dsp_mode, p_config);
#else
            LOG_FATAL("dsp amba s2 is not compiled\n");
#endif
            break;

        case EDSPPlatform_SimpleSW:
            LOG_ERROR("please implement EDSPPlatform_SimpleSW\n");
            break;

        case EDSPPlatform_CPUARMv7AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUARMv7AccelSIMD\n");
            break;

        case EDSPPlatform_CPUX86AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUX86AccelSIMD\n");
            break;

        case EDSPPlatform_GPUAccelNv:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelNv\n");
            break;

        case EDSPPlatform_GPUAccelAMD:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelAMD\n");
            break;

        case EDSPPlatform_GPUAccelImg:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelImg\n");
            break;

        default:
            LOG_FATAL("BAD EDSPPlatform %d\n", dsp_platform);
            break;
    }

    return api;
}

IDSPEncAPI *gfDSPEncAPIFactory(TInt iav_fd, EDSPOperationMode dsp_mode, StreamFormat codec_type, const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform)
{
    IDSPEncAPI *api = NULL;

    switch (dsp_platform) {
        case EDSPPlatform_AutoSelect:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPEncAPI(iav_fd, dsp_mode, p_config);
#else
            LOG_FATAL("dsp amba i1 is not compiled\n");
#endif
            break;

        case EDSPPlatform_AmbaI1:
#if BUILD_DSP_AMBA_I1
            api = gfCreateiOneDSPEncAPI(iav_fd, dsp_mode, p_config);
#else
            LOG_FATAL("dsp amba i1 is not compiled\n");
#endif
            break;

        case EDSPPlatform_SimpleSW:
            LOG_ERROR("please implement EDSPPlatform_SimpleSW\n");
            break;

        case EDSPPlatform_CPUARMv7AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUARMv7AccelSIMD\n");
            break;

        case EDSPPlatform_CPUX86AccelSIMD:
            LOG_ERROR("please implement EDSPPlatform_CPUX86AccelSIMD\n");
            break;

        case EDSPPlatform_GPUAccelNv:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelNv\n");
            break;

        case EDSPPlatform_GPUAccelAMD:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelAMD\n");
            break;

        case EDSPPlatform_GPUAccelImg:
            LOG_ERROR("please implement EDSPPlatform_GPUAccelImg\n");
            break;

        default:
            LOG_FATAL("BAD EDSPPlatform %d\n", dsp_platform);
            break;
    }

    return api;
}

const TChar *gfGetDSPPlatformString(EDSPPlatform platform)
{
    switch (platform) {

        case EDSPPlatform_Invalid:
            return "EDSPPlatform_Invalid";
            break;

        case EDSPPlatform_AutoSelect:
            return "EDSPPlatform_AutoSelect";
            break;

        case EDSPPlatform_SimpleSW:
            return "EDSPPlatform_SimpleSW";
            break;

        case EDSPPlatform_CPUARMv7AccelSIMD:
            return "EDSPPlatform_CPUARMv7AccelSIMD";
            break;

        case EDSPPlatform_CPUX86AccelSIMD:
            return "EDSPPlatform_CPUX86AccelSIMD";
            break;

        case EDSPPlatform_GPUAccelAMD:
            return "EDSPPlatform_GPUAccelAMD";
            break;

        case EDSPPlatform_AmbaA5s:
            return "EDSPPlatform_AmbaA5s";
            break;

        case EDSPPlatform_AmbaI1:
            return "EDSPPlatform_AmbaI1";
            break;

        case EDSPPlatform_AmbaS2:
            return "EDSPPlatform_AmbaS2";
            break;

        default:
            LOG_FATAL("BAD EDSPPlatform %d\n", platform);
            break;
    }

    return "unkown EDSPPlatform";
}

static TUint __getUDECFormat(StreamFormat format)
{
    switch (format) {
        case StreamFormat_H264:
            return UDEC_VFormat_H264;
            break;
        case StreamFormat_VC1:
            return UDEC_VFormat_VC1;
            break;
        case StreamFormat_MPEG4:
            return UDEC_VFormat_MPEG4;
            break;
        case StreamFormat_WMV3:
            return UDEC_VFormat_WMV3;
            break;
        case StreamFormat_MPEG12:
            return UDEC_VFormat_MPEG12;
            break;
        default:
            LOG_FATAL("BAD format %d\n", format);
            break;
    }

    return 0;
}

static SBitsWriter *createBitsWriter(TU8 *pStart, TUint size)
{
    SBitsWriter *pWriter = (SBitsWriter *) DDBG_MALLOC(sizeof(SBitsWriter), "D0BW");

    //memset(pStart, 0x0, size);
    pWriter->pStart = pStart;
    pWriter->pCur = pStart;
    pWriter->size = size;
    pWriter->leftBits = 8;
    pWriter->leftBytes = size;
    pWriter->full = 0;

    return pWriter;
}

static void deleteBitsWriter(SBitsWriter *pWriter)
{
    DDBG_FREE(pWriter, "D0BW");
}

static TInt writeBits(SBitsWriter *pWriter, TUint value, TUint bits)
{
    //    TUint shift = 0;

    DASSERT(!pWriter->full);
    DASSERT(pWriter->size == (pWriter->pCur - pWriter->pStart + pWriter->leftBytes));
    DASSERT(pWriter->leftBits <= 8);

    if (pWriter->full) {
        DASSERT(0);
        return -1;
    }

    while (bits > 0 && !pWriter->full) {
        if (bits <= pWriter->leftBits) {
            DASSERT(pWriter->leftBits <= 8);
            *pWriter->pCur |= (value << (32 - bits)) >> (32 - pWriter->leftBits);
            pWriter->leftBits -= bits;
            //value >>= bits;

            if (pWriter->leftBits == 0) {
                if (pWriter->leftBytes == 0) {
                    pWriter->full = 1;
                    return 0;
                }
                pWriter->leftBits = 8;
                pWriter->leftBytes --;
                pWriter->pCur ++;
            }
            return 0;
        } else {
            DASSERT(pWriter->leftBits <= 8);
            *pWriter->pCur |= (value << (32 - bits)) >> (32 - pWriter->leftBits);
            value <<= 32 - bits + pWriter->leftBits;
            value >>= 32 - bits + pWriter->leftBits;
            bits -= pWriter->leftBits;

            if (pWriter->leftBytes == 0) {
                pWriter->full = 1;
                return 0;
            }
            pWriter->leftBits = 8;
            pWriter->leftBytes --;
            pWriter->pCur ++;
        }

    }
    return -2;
}

TUint gfFillUSEQHeader(TU8 *pHeader, StreamFormat vFormat, TU32 timeScale, TU32 tickNum, TU32 is_mp4s_flag, TS32 vid_container_width, TS32 vid_container_height)
{
    if (StreamFormat_MPEG12 == vFormat)
    { return 0; }

    DASSERT(pHeader);
    TInt ret = 0;

    SBitsWriter *pWriter = createBitsWriter(pHeader, is_mp4s_flag ? DUDEC_SEQ_HEADER_EX_LENGTH : DUDEC_SEQ_HEADER_LENGTH);
    DASSERT(pWriter);
    memset(pHeader, 0x0, is_mp4s_flag ? DUDEC_SEQ_HEADER_EX_LENGTH : DUDEC_SEQ_HEADER_LENGTH);

    //start code prefix
    ret |= writeBits(pWriter, 0, 16);
    ret |= writeBits(pWriter, 1, 8);

    //start code
    if (vFormat == StreamFormat_H264) {
        ret |= writeBits(pWriter, DUDEC_SEQ_STARTCODE_H264, 8);
    } else if (vFormat == StreamFormat_MPEG4) {
        ret |= writeBits(pWriter, DUDEC_SEQ_STARTCODE_MPEG4, 8);
    } else if (vFormat == StreamFormat_VC1 || vFormat == StreamFormat_WMV3) {
        ret |= writeBits(pWriter, DUDEC_SEQ_STARTCODE_VC1WMV3, 8);
    }

    //add Signature Code 0x2449504F "$IPO"
    ret |= writeBits(pWriter, 0x2449504F, 32);

    //video format
    ret |= writeBits(pWriter, __getUDECFormat(vFormat), 8);

    //Time Scale low
    ret |= writeBits(pWriter, (timeScale & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, timeScale & 0xff, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //Time Scale high
    ret |= writeBits(pWriter, (timeScale & 0xff000000) >> 24, 8);
    ret |= writeBits(pWriter, (timeScale & 0x00ff0000) >> 16, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //num units tick low
    ret |= writeBits(pWriter, (tickNum & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, tickNum & 0xff, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //num units tick high
    ret |= writeBits(pWriter, (tickNum & 0xff000000) >> 24, 8);
    ret |= writeBits(pWriter, (tickNum & 0x00ff0000) >> 16, 8);

    //resolution info flag(0:no res info, 1:has res info)
    ret |= writeBits(pWriter, (is_mp4s_flag & 0x1), 1);

    if (is_mp4s_flag) {
        //pic width
        ret |= writeBits(pWriter, (vid_container_width & 0x7f00) >> 8, 7);
        ret |= writeBits(pWriter, vid_container_width & 0xff, 8);

        //markbit
        ret |= writeBits(pWriter, 3, 2);

        //pic height
        ret |= writeBits(pWriter, (vid_container_height & 0x7f00) >> 8, 7);
        ret |= writeBits(pWriter, vid_container_height & 0xff, 8);
    }

    //padding
    ret |= writeBits(pWriter, 0xffffffff, 20);

    DASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    DASSERT(pWriter->leftBytes == 0);
    DASSERT(!pWriter->full);
    DASSERT(!ret);

    deleteBitsWriter(pWriter);

    return is_mp4s_flag ? DUDEC_SEQ_HEADER_EX_LENGTH : DUDEC_SEQ_HEADER_LENGTH;
}

void gfInitUPESHeader(TU8 *pHeader, StreamFormat vFormat)
{
    DASSERT(pHeader);
    //DASSERT(vFormat>=StreamFormat_H264);
    //DASSERT(vFormat<=UDEC_VFormat_MPEG12);

    memset(pHeader, 0x0, DUDEC_PES_HEADER_LENGTH);

    //start code prefix
    pHeader[2] = 0x1;

    //start code
    if (vFormat == StreamFormat_H264) {
        pHeader[3] = DUDEC_PES_STARTCODE_H264;
    } else if (vFormat == StreamFormat_MPEG4) {
        pHeader[3] = DUDEC_PES_STARTCODE_MPEG4;
    } else if (vFormat == StreamFormat_VC1 || vFormat == StreamFormat_WMV3) {
        pHeader[3] = DUDEC_PES_STARTCODE_VC1WMV3;
    } else if (vFormat == StreamFormat_MPEG12) {
        pHeader[3] = DUDEC_PES_STARTCODE_MPEG12;
    }

    //add Signature Code 0x2449504F "$IPO"
    pHeader[4] = 0x24;
    pHeader[5] = 0x49;
    pHeader[6] = 0x50;
    pHeader[7] = 0x4F;
}

TUint gfFillPESHeader(TU8 *pHeader, TU32 ptsLow, TU32 ptsHigh, TU32 auDatalen, TUint hasPTS, TUint isPTSJump)
{
    DASSERT(pHeader);

    TInt ret = 0;

    SBitsWriter *pWriter = createBitsWriter(pHeader + 8, DUDEC_PES_HEADER_LENGTH - 8);
    DASSERT(pWriter);
    memset(pHeader + 8, 0x0, DUDEC_PES_HEADER_LENGTH - 8);

    ret |= writeBits(pWriter, hasPTS, 1);

    if (hasPTS) {
        //pts 0
        ret |= writeBits(pWriter, (ptsLow & 0xff00) >> 8, 8);
        ret |= writeBits(pWriter, ptsLow & 0xff, 8);
        //marker bit
        ret |= writeBits(pWriter, 1, 1);
        //pts 1
        ret |= writeBits(pWriter, (ptsLow & 0xff000000) >> 24, 8);
        ret |= writeBits(pWriter, (ptsLow & 0xff0000) >> 16, 8);
        //marker bit
        ret |= writeBits(pWriter, 1, 1);
        //pts 2
        ret |= writeBits(pWriter, (ptsHigh & 0xff00) >> 8, 8);
        ret |= writeBits(pWriter, ptsHigh & 0xff, 8);

        //marker bit
        ret |= writeBits(pWriter, 1, 1);
        //pts 3
        ret |= writeBits(pWriter, (ptsHigh & 0xff000000) >> 24, 8);
        ret |= writeBits(pWriter, (ptsHigh & 0xff0000) >> 16, 8);

        //marker bit
        ret |= writeBits(pWriter, 1, 1);

        //PTS_JUMP
        ret |= writeBits(pWriter, isPTSJump, 1);

        //padding
        ret |= writeBits(pWriter, 0xffffffff, 27);
    }

    //au_data_length low
    //AM_PRINTF("auDatalen %d, 0x%x.\n", auDatalen, auDatalen);

    ret |= writeBits(pWriter, (auDatalen & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, (auDatalen & 0xff), 8);
    //marker bit
    ret |= writeBits(pWriter, 1, 1);
    //au_data_length high
    ret |= writeBits(pWriter, (auDatalen & 0x07000000) >> 24, 3);
    ret |= writeBits(pWriter, (auDatalen & 0x00ff0000) >> 16, 8);
    //pading
    ret |= writeBits(pWriter, 0xf, 3);

    DASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    DASSERT(!ret);

    ret = pWriter->size - pWriter->leftBytes + 8;

    deleteBitsWriter(pWriter);
    return ret;

}

TUint gfFillPrivateGopNalHeader(TU8 *pHeader, TU32 timeScale, TU32 tickNum, TUint m, TUint n, TU32 pts_high, TU32 pts_low)
{
    TInt ret = 0;
    DASSERT(pHeader);

    SBitsWriter *pWriter = createBitsWriter(pHeader, DPRIVATE_GOP_NAL_HEADER_LENGTH);
    DASSERT(pWriter);
    memset(pHeader, 0x0, DPRIVATE_GOP_NAL_HEADER_LENGTH);

    LOG_WARN("print GOP header: time scale %d, tick %d, m %d, n %d, pts_high 0x%08x, pts_low 0x%08x\n", timeScale, tickNum, m, n, pts_high, pts_low);

    //start code prefix
    ret |= writeBits(pWriter, 0, 24);
    ret |= writeBits(pWriter, 1, 8);

    //start code
    ret |= writeBits(pWriter, 0x7a, 8);

    //version main
    ret |= writeBits(pWriter, 0x01, 8);
    //version main
    ret |= writeBits(pWriter, 0x01, 8);

    ret |= writeBits(pWriter, 0x00, 2);

    //num units tick high
    ret |= writeBits(pWriter, (tickNum & 0xff000000) >> 24, 8);
    ret |= writeBits(pWriter, (tickNum & 0x00ff0000) >> 16, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //num units tick low
    ret |= writeBits(pWriter, (tickNum & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, tickNum & 0xff, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //Time Scale high
    ret |= writeBits(pWriter, (timeScale & 0xff000000) >> 24, 8);
    ret |= writeBits(pWriter, (timeScale & 0x00ff0000) >> 16, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //Time Scale low
    ret |= writeBits(pWriter, (timeScale & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, timeScale & 0xff, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //Time PTS high
    ret |= writeBits(pWriter, (pts_low & 0xff000000) >> 24, 8);
    ret |= writeBits(pWriter, (pts_low & 0x00ff0000) >> 16, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //Time PTS low
    ret |= writeBits(pWriter, (pts_low & 0xff00) >> 8, 8);
    ret |= writeBits(pWriter, pts_low & 0xff, 8);

    //markbit
    ret |= writeBits(pWriter, 1, 1);

    //n
    ret |= writeBits(pWriter, n, 8);

    //m
    ret |= writeBits(pWriter, m, 4);

    //padding
    ret |= writeBits(pWriter, 0xff, 4);

    DASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    DASSERT(pWriter->leftBytes == 0);
    DASSERT(!ret);

    deleteBitsWriter(pWriter);

    return DPRIVATE_GOP_NAL_HEADER_LENGTH;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

