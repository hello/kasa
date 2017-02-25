/*******************************************************************************
 * streaming_if.cpp
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
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "codec_misc.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
SRTSPHeader g_RTSPHeaders[ERTSPHeaderName_COUNT] = {
    {"Accept", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                         ERTSPHeaderName_Accept},
    {"Accept-Encoding", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                         ERTSPHeaderName_Accept_Encoding},
    {"Accept-Language", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                         ERTSPHeaderName_Accept_Language},
    {"Allow", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                         ERTSPHeaderName_Allow},
    {"Authorization", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                         ERTSPHeaderName_Authorization},
    {"Bandwidth", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                          ERTSPHeaderName_Bandwidth},
    {"Blocksize", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ~(ESessionMethod_RTSP_OPTIONS | ESessionMethod_RTSP_TEARDOWN),                         ERTSPHeaderName_Blocksize},
    {"Cache-Control", (TU16)ERTSPHeaderType_general, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_SETUP,                                                                                    ERTSPHeaderName_Cache_Control},
    {"Conference", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_SETUP,                                                                                   ERTSPHeaderName_Conference},
    {"Connection", (TU16)ERTSPHeaderType_general, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                            ERTSPHeaderName_Connection},
    {"Content-Base", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                             ERTSPHeaderName_Content_Base},
    {"Content-Encoding", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_SET_PARAMETER,                                                                       ERTSPHeaderName_Content_Encoding_1},
    {"Content-Encoding", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE,                               ERTSPHeaderName_Content_Encoding_2},
    {"Content-Language", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE,                              ERTSPHeaderName_Content_Language},
    {"Content-Length", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_SET_PARAMETER | ESessionMethod_RTSP_ANNOUNCE,                    ERTSPHeaderName_Content_Length_1},
    {"Content-Length", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                           ERTSPHeaderName_Content_Length_2},
    {"Content-Location", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                            ERTSPHeaderName_Content_Location},
    {"Content-Type", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_SET_PARAMETER | ESessionMethod_RTSP_ANNOUNCE,                    ERTSPHeaderName_Content_Type_1},
    {"Content-Type", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                            ERTSPHeaderName_Content_Type_1},
    {"CSeq", (TU16)ERTSPHeaderType_general, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                            ERTSPHeaderName_CSeq},
    {"Date", (TU16)ERTSPHeaderType_general, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                            ERTSPHeaderName_Date},
    {"Expires", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_ANNOUNCE,                              ERTSPHeaderName_Expires},
    {"From", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                            ERTSPHeaderName_From},
    {"If-Modified-Since", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_DESCRIBE | ESessionMethod_RTSP_SETUP,                                    ERTSPHeaderName_If_Modified_Since},
    {"Last-Modified", (TU16)ERTSPHeaderType_entity, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                           ERTSPHeaderName_Last_Modified},
    {"Proxy-Authenticate", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                           ERTSPHeaderName_Proxy_Authenticate},
    {"Proxy-Require", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                           ERTSPHeaderName_Proxy_Require},
    {"Public", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                        ERTSPHeaderName_Public},
    {"Range", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_PLAY | ESessionMethod_RTSP_PAUSE | ESessionMethod_RTSP_RECORD,    ERTSPHeaderName_Range_1},
    {"Range", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_PLAY | ESessionMethod_RTSP_PAUSE | ESessionMethod_RTSP_RECORD,    ERTSPHeaderName_Range_2},
    {"Referer", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                          ERTSPHeaderName_Referer},
    {"Require", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_required,        0xffffffff,                                                                                                          ERTSPHeaderName_Require},
    {"Retry-After", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                          ERTSPHeaderName_Retry_After},
    {"RTP-Info", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_PLAY,                                                                                    ERTSPHeaderName_RTP_Info},
    {"Scale", (TU16)ERTSPHeaderType_response | (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_PLAY | ESessionMethod_RTSP_RECORD,  ERTSPHeaderName_Scale},
    {"Session", (TU16)ERTSPHeaderType_response | (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_required,        ~(ESessionMethod_RTSP_SETUP | ESessionMethod_RTSP_OPTIONS),  ERTSPHeaderName_Session},
    {"Server", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,        0xffffffff,                                                                                                           ERTSPHeaderName_Server},
    {"Speed", (TU16)ERTSPHeaderType_response | (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,        ESessionMethod_RTSP_PLAY,                                 ERTSPHeaderName_Speed},
    {"Transport", (TU16)ERTSPHeaderType_response | (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_required,        ESessionMethod_RTSP_SETUP,                               ERTSPHeaderName_Transport},
    {"Unsupported", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_required,         0xffffffff,                                                                                                          ERTSPHeaderName_Unsupported},
    {"User-Agent", (TU16)ERTSPHeaderType_request, (TU16)ERTSPHeaderSupport_optional,         0xffffffff,                                                                                                          ERTSPHeaderName_User_Agent},
    {"Via", (TU16)ERTSPHeaderType_general, (TU16)ERTSPHeaderSupport_optional,         0xffffffff,                                                                                                          ERTSPHeaderName_Via},
    {"WWW-Authenticate", (TU16)ERTSPHeaderType_response, (TU16)ERTSPHeaderSupport_optional,         0xffffffff,                                                                                                          ERTSPHeaderName_WWW_Authenticate},
};
#endif

void apend_string(TChar *&ori, TUint &tot_size, TChar *append_str)
{
    TUint append_len = 0;
    TUint ori_len = 0;
    TChar *tobe_freed = NULL;
    if (!append_str) {
        LOG_ERROR("NULL pointer here.\n");
        return;
    }

    append_len = strlen(append_str);
    if (ori) {
        ori_len = strlen(ori);
    }

    if (!ori || (append_len + ori_len + 1) > tot_size) {
        tobe_freed = ori;
        ori = (TChar *)DDBG_MALLOC(append_len + ori_len + 16, "APST");
        if (ori) {
            tot_size = append_len + ori_len + 16;
        }
        if (tobe_freed) {
            snprintf(ori, tot_size, "%s%s", tobe_freed, append_str);
            free(tobe_freed);
        } else {
            strncpy(ori, append_str, tot_size);
        }
    } else {
        strncat(ori, append_str, append_len);
    }
}

TChar *generate_sdp_description_h264(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }

    const TChar *const h264_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                      "c=IN IP4 0.0.0.0\r\n"
                                      "b=AS:2000\r\n"
                                      "a=rtpmap:%d H264/90000\r\n"
                                      "a=fmtp:%d packetization-mode=1;profile-level-id=42A01E\r\n"
                                      "a=control:trackID=0\r\n"
                                      "a=cliprect:0,0,%d,%d\r\n"
                                      "a=framerate:%f\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_dsp_fmt, payload_type, payload_type, payload_type, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_aac(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint video_payload_type, TUint width, TUint height, float framerate, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *p_aac_config, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }

    const TChar *const h264_aac_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                          "c=IN IP4 0.0.0.0\r\n"
                                          "b=AS:2000\r\n"
                                          "a=rtpmap:%d H264/90000\r\n"
                                          "a=fmtp:%d packetization-mode=1;profile-level-id=42A01E\r\n"
                                          "a=control:trackID=0\r\n"
                                          "a=cliprect:0,0,%d,%d\r\n"
                                          "a=framerate:%f\r\n"
                                          "m=audio 0 RTP/AVP %d\r\n"
                                          "c=IN IP4 0.0.0.0\r\n"
                                          "b=RR:0\r\n"
                                          "a=rtpmap:%d mpeg4-generic/%d/%d\r\n"
                                          "a=fmtp:%d StreamType=5; profile-level-id=15; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\n"
                                          "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    if ((!p_aac_config) || (0x0 == p_aac_config[0])) {
        TChar aac_config_base16[32] = {0};
        TUint generated_config_size = 0;
        TU8 *generated_aac_config = gfGenerateAudioExtraData(StreamFormat_AAC, samplerate, channel_number, generated_config_size);

        if (DLikely(generated_aac_config && (16 > generated_config_size))) {
            gfEncodingBase16(aac_config_base16, generated_aac_config, generated_config_size);
            aac_config_base16[31] = 0x0;
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, aac_config_base16);
            free(generated_aac_config);
        } else {
            LOG_ERROR("gfGenerateAudioExtraData(aac) fail\n");
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, "1188");
        }
    } else {
        snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, p_aac_config);
    }

    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_pcm_g711_mu(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const h264_pcmmu_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                            "c=IN IP4 0.0.0.0\r\n"
                                            "b=AS:2000\r\n"
                                            "a=rtpmap:%d H264/90000\r\n"
                                            "a=fmtp:%d packetization-mode=1;profile-level-id=42A01E\r\n"
                                            "a=control:trackID=0\r\n"
                                            "a=cliprect:0,0,%d,%d\r\n"
                                            "a=framerate:%f\r\n"
                                            "m=audio 0 RTP/AVP 0\r\n"
                                            "c=IN IP4 0.0.0.0\r\n"
                                            "b=RR:0\r\n"
                                            "a=rtpmap:0 PCMU/8000/1\r\n"
                                            "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_pcmmu_dsp_fmt, payload_type, payload_type, payload_type, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_pcm_g711_a(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const h264_pcma_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                           "c=IN IP4 0.0.0.0\r\n"
                                           "b=AS:2000\r\n"
                                           "a=rtpmap:%d H264/90000\r\n"
                                           "a=fmtp:%d packetization-mode=1;profile-level-id=42A01E\r\n"
                                           "a=control:trackID=0\r\n"
                                           "a=cliprect:0,0,%d,%d\r\n"
                                           "a=framerate:%f\r\n"
                                           "m=audio 0 RTP/AVP 8\r\n"
                                           "c=IN IP4 0.0.0.0\r\n"
                                           "b=RR:0\r\n"
                                           "a=rtpmap:8 PCMA/8000/1\r\n"
                                           "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_pcma_dsp_fmt, payload_type, payload_type, payload_type, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_aac(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *p_aac_config, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }

    const TChar *const h264_aac_dsp_fmt = "m=audio 0 RTP/AVP %d\r\n"
                                          "c=IN IP4 0.0.0.0\r\n"
                                          "b=RR:0\r\n"
                                          "a=rtpmap:%d mpeg4-generic/%d/%d\r\n"
                                          "a=fmtp:%d StreamType=5; profile-level-id=15; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\n"
                                          "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    if ((!p_aac_config) || (0x0 == p_aac_config[0])) {
        TChar aac_config_base16[32] = {0};
        TUint generated_config_size = 0;
        TU8 *generated_aac_config = gfGenerateAudioExtraData(StreamFormat_AAC, samplerate, channel_number, generated_config_size);

        if (DLikely(generated_aac_config && (16 > generated_config_size))) {
            gfEncodingBase16(aac_config_base16, generated_aac_config, generated_config_size);
            aac_config_base16[31] = 0x0;
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, aac_config_base16);
            free(generated_aac_config);
        } else {
            LOG_ERROR("gfGenerateAudioExtraData(aac) fail\n");
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, "1188");
        }
    } else {
        snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, p_aac_config);
    }
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_pcm_g711_mu(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const fMiscSDPLines = "m=audio 0 RTP/AVP 0\r\n"
                                       "c=IN IP4 0.0.0.0\r\n"
                                       "b=RR:0\r\n"
                                       "a=rtpmap:0 PCMU/8000/1\r\n"
                                       "a=control:trackID=1\r\n";

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_pcm_g711_a(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const fMiscSDPLines = "m=audio 0 RTP/AVP 8\r\n"
                                       "c=IN IP4 0.0.0.0\r\n"
                                       "b=RR:0\r\n"
                                       "a=rtpmap:8 PCMA/8000/1\r\n"
                                       "a=control:trackID=1\r\n";

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length)
{
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const h264_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                      "c=IN IP4 0.0.0.0\r\n"
                                      "b=AS:2000\r\n"
                                      "a=rtpmap:%d H264/90000\r\n"
                                      "a=fmtp:%d packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s,%s;\r\n"
                                      "a=control:trackID=0\r\n"
                                      "a=cliprect:0,0,%d,%d\r\n"
                                      "a=framerate:%f\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_dsp_fmt, payload_type, payload_type, payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_aac_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint video_payload_type, TUint width, TUint height, float framerate, TUint audio_payload_type, TUint samplerate, TUint channel_number, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TChar *p_aac_config, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }

    const TChar *const h264_aac_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                          "c=IN IP4 0.0.0.0\r\n"
                                          "b=AS:2000\r\n"
                                          "a=rtpmap:%d H264/90000\r\n"
                                          "a=fmtp:%d packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s,%s;\r\n"
                                          "a=control:trackID=0\r\n"
                                          "a=cliprect:0,0,%d,%d\r\n"
                                          "a=framerate:%f\r\n"
                                          "m=audio 0 RTP/AVP %d\r\n"
                                          "c=IN IP4 0.0.0.0\r\n"
                                          "b=RR:0\r\n"
                                          "a=rtpmap:%d mpeg4-generic/%d/%d\r\n"
                                          "a=fmtp:%d StreamType=5; profile-level-id=15; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\n"
                                          "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    if ((!p_aac_config) || (0x0 == p_aac_config[0])) {
        TChar aac_config_base16[32] = {0};
        TUint generated_config_size = 0;
        TU8 *generated_aac_config = gfGenerateAudioExtraData(StreamFormat_AAC, samplerate, channel_number, generated_config_size);

        if (DLikely(generated_aac_config && (16 > generated_config_size))) {
            gfEncodingBase16(aac_config_base16, generated_aac_config, generated_config_size);
            aac_config_base16[31] = 0x0;
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, aac_config_base16);
            free(generated_aac_config);
        } else {
            LOG_ERROR("gfGenerateAudioExtraData(aac) fail\n");
            snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, "1188");
        }
    } else {
        snprintf(fMiscSDPLines, 1023, h264_aac_dsp_fmt, video_payload_type, video_payload_type, video_payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate, audio_payload_type, audio_payload_type, samplerate, channel_number, audio_payload_type, p_aac_config);
    }

    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_pcm_g711_mu_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const h264_pcmmu_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                            "c=IN IP4 0.0.0.0\r\n"
                                            "b=AS:2000\r\n"
                                            "a=rtpmap:%d H264/90000\r\n"
                                            "a=fmtp:%d packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s,%s;\r\n"
                                            "a=control:trackID=0\r\n"
                                            "a=cliprect:0,0,%d,%d\r\n"
                                            "a=framerate:%f\r\n"
                                            "m=audio 0 RTP/AVP 0\r\n"
                                            "c=IN IP4 0.0.0.0\r\n"
                                            "b=RR:0\r\n"
                                            "a=rtpmap:0 PCMU/8000/1\r\n"
                                            "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_pcmmu_dsp_fmt, payload_type, payload_type, payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

TChar *generate_sdp_description_h264_pcm_g711_a_sps_params(TChar const *pStreamName, TChar const *pInfo, TChar const *pDescription, TChar const *ipAddressStr, TChar *rtspURL, TUint tv_sec, TUint tv_usec, TUint payload_type, TUint width, TUint height, float framerate, TChar *profile_level_id, TChar *sps_base64, TChar *pps_base64, TU64 length)
{
    //TU32 OurIPAddress;
    const TChar *const libNameStr = DSRTING_RTSP_SERVER_TAG;
    const TChar *const libVersionStr = "2013.12.14";
    const TChar *const sourceFilterLine = "";
    TChar rangeLine[64] = {0};
    if (length) {
        snprintf(rangeLine, 64, "a=range:npt=0-%lld\r\n", length);
    } else {
        strcpy(rangeLine, "a=range:npt=0-\r\n");
    }
    const TChar *const h264_pcma_dsp_fmt = "m=video 0 RTP/AVP %d\r\n"
                                           "c=IN IP4 0.0.0.0\r\n"
                                           "b=AS:2000\r\n"
                                           "a=rtpmap:%d H264/90000\r\n"
                                           "a=fmtp:%d packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s,%s;\r\n"
                                           "a=control:trackID=0\r\n"
                                           "a=cliprect:0,0,%d,%d\r\n"
                                           "a=framerate:%f\r\n"
                                           "m=audio 0 RTP/AVP 8\r\n"
                                           "c=IN IP4 0.0.0.0\r\n"
                                           "b=RR:0\r\n"
                                           "a=rtpmap:8 PCMA/8000/1\r\n"
                                           "a=control:trackID=1\r\n";

    TChar fMiscSDPLines[1024] = {0};

    snprintf(fMiscSDPLines, 1023, h264_pcma_dsp_fmt, payload_type, payload_type, payload_type, profile_level_id, sps_base64, pps_base64, height, width, framerate);
    fMiscSDPLines[1023] = 0x0;

    TChar const *const sdpPrefixFmt =
        "v=0\r\n"
        "o=- %u%u %d IN IP4 %s\r\n"
        "s=%s\r\n"
        "i=%s\r\n"
        "t=0 0\r\n"
        "a=tool:%s%s\r\n"
        "a=type:broadcast\r\n"
        "a=control:%s\r\n"
        "%s"
        "%s"
        "a=x-qt-text-nam:%s\r\n"
        "a=x-qt-text-inf:%s\r\n"
        "%s";

    TUint sdp_length = strlen(sdpPrefixFmt)
                       + 20 + 6 + 20 + 32 + strlen(ipAddressStr)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(libNameStr) + strlen(libVersionStr)
                       + strlen(rtspURL)
                       + strlen(sourceFilterLine)
                       + strlen(rangeLine)
                       + strlen(pDescription)
                       + strlen(pInfo)
                       + strlen(fMiscSDPLines);

    TChar *p_str = (TChar *)DDBG_MALLOC(sdp_length + 4, "GSST");
    DASSERT(p_str);

    // Generate the SDP prefix (session-level lines):
    snprintf(p_str, sdp_length, sdpPrefixFmt,
             tv_sec, tv_usec, // o= <session id>
             1, // o= <version> // (needs to change if params are modified)
             ipAddressStr, // o= <address>
             pDescription, // s= <description>
             pInfo, // i= <info>
             libNameStr, libVersionStr, // a=tool:
             rtspURL, //a=control
             sourceFilterLine, // a=source-filter: incl (if a SSM session)
             rangeLine, // a=range: line
             pDescription, // a=x-qt-text-nam: line
             pInfo, // a=x-qt-text-inf: line
             fMiscSDPLines); // miscellaneous session SDP lines (if any)

    return p_str;
}

static TChar *_strDupSize(TChar const *str)
{
    if (str == NULL) { return NULL; }
    size_t len = strlen(str) + 1;
    TChar *copy = new TChar[len];

    return copy;
}

TUint getTrackID(TChar *string, TChar *string_end)
{
    TChar *ptmp = string;
    TUint track_id = 0;
    TInt count = 4;

    while (count > 0) {
        ptmp = strchr(ptmp, 't');
        if ((!ptmp) || (ptmp >= string_end)) {
            LOG_ERROR("cannot find 'trackID='.\n");
            return 0;
        }

        if (!strncmp(ptmp, "trackID=", 8)) {
            sscanf(ptmp, "trackID=%d", &track_id);
            return track_id;
        }

        count --;
    }

    LOG_ERROR("cannot get 'trackID=', should not comes here.\n");
    return 0;
}

bool parseTransportHeader(TChar const *buf,
                          ProtocolType &streamingMode,
                          TChar *streamingModeString,
                          TChar *destinationAddressStr,
                          TChar *destinationTTL,
                          TU16 *clientRTPPortNum, // if UDP
                          TU16 *clientRTCPPortNum, // if UDP
                          TChar *rtpChannelId, // if TCP
                          TChar *rtcpChannelId // if TCP
                         )
{
    // Initialize the result parameters to default values:
    streamingMode = ProtocolType_UDP;
    *destinationTTL = 255;
    *clientRTPPortNum = 0;
    *clientRTCPPortNum = 1;
    *rtpChannelId = *rtcpChannelId = 0xFF;

    TU16 p1, p2;
    TU32 ttl, rtpCid, rtcpCid;

    // First, find "Transport:"
    while (1) {
        if (*buf == '\0') { return false; } // not found
        if (strncmp(buf, "Transport: ", 11) == 0) { break; }
        ++buf;
    }

    // Then, run through each of the fields, looking for ones we handle:
    TChar const *fields = buf + 11;
    TChar *field = _strDupSize(fields);

    while (sscanf(fields, "%[^;]", field) == 1) {
        if (strcmp(field, "RTP/AVP/TCP") == 0) {
            streamingMode = ProtocolType_TCP;
        } else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
                   strcmp(field, "MP2T/H2221/UDP") == 0) {
            streamingMode = ProtocolType_UDP;
            strncpy(streamingModeString, field, strlen(streamingModeString));
        } else if (strncmp(field, "destination=", 12) == 0) {
            delete[] destinationAddressStr;
            strncpy(destinationAddressStr, field + 12, strlen(destinationAddressStr));
        } else if (sscanf(field, "ttl%u", &ttl) == 1) {
            *destinationTTL = (TChar)ttl;
        } else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2) {
            *clientRTPPortNum = p1;
            *clientRTCPPortNum = p2;
        } else if (sscanf(field, "client_port=%hu", &p1) == 1) {
            *clientRTPPortNum = p1;
            *clientRTCPPortNum = (streamingMode == ProtocolType_UDP) ? 0 : p1 + 1;
        } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
            *rtpChannelId = (TChar)rtpCid;
            *rtcpChannelId = (TChar)rtcpCid;
        }

        fields += strlen(field);
        while (*fields == ';') { ++fields; } // skip over separating ';' chars
        if (*fields == '\0' || *fields == '\r' || *fields == '\n') { break; }
    }

    delete[] field;
    return true;
}

bool parseSessionHeader(TChar const *buf, TU16 &sessionId)
{
    while (1) {
        if (*buf == '\0') { return false; }
        if (strncmp(buf, "Session: ", 9) == 0) { break; }
        ++buf;
    }
    unsigned int session;
    if (sscanf(buf, "Session: %X\r\n", &session) == 1) {
        sessionId = (TU16)session;
        return true;
    }
    return false;
}

EECode gfSDPProcessVideoExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size)
{
    TInt in_size = 0, out_size = 0;
    TChar *p_out_buffer = NULL;
    TU8 *p_in_buffer = NULL, *p_next = NULL;

    if (DUnlikely((!subsession) || (!p_extra_data) || (!size))) {
        LOG_FATAL("gfSDPProcessVideoExtraData: NULL %p, %p or zero %d\n", subsession, p_extra_data, size);
        return EECode_BadParam;
    }

    //LOG_NOTICE("%p, size %d\n", p_extra_data, size);
    //gfPrintMemory(p_extra_data, size);

    if ((0x0 == p_extra_data[0]) && (0x0 == p_extra_data[1]) && (0x0 == p_extra_data[2]) && (0x01 == p_extra_data[3]) && (0x09 == p_extra_data[4])) {
        p_extra_data += 6;
        size -= 6;
        while (0x0 != p_extra_data[0]) {
            if (size > 20) {
                p_extra_data ++;
                size --;
            } else {
                LOG_FATAL("BAD case\n");
                break;
            }
        }
    }

    //LOG_NOTICE("%p, size %d\n", p_extra_data, size);
    //gfPrintMemory(p_extra_data, size);

    p_in_buffer = p_extra_data + 5;
    p_out_buffer = &subsession->profile_level_id_base16[0];
    in_size = 3;
    out_size = 6;
    gfEncodingBase16(p_out_buffer, p_in_buffer, in_size);
    p_out_buffer[out_size] = 0x0;

    p_next = gfNALUFindNextStartCode(p_extra_data + 4, size - 4);
    if (DUnlikely(!p_next)) {
        LOG_FATAL("do not find pps?\n");
        return EECode_InternalLogicalBug;
    }

    //LOG_NOTICE("%p, size %d, p_next %p, gap %d\n", p_extra_data, size, p_next, (TU32)(p_next - p_extra_data));
    //gfPrintMemory(p_extra_data, size);
    if (0x0 == p_extra_data[2]) {
        DASSERT(0x0 == p_extra_data[0]);
        DASSERT(0x0 == p_extra_data[1]);
        DASSERT(0x1 == p_extra_data[3]);
        p_in_buffer = p_extra_data + 4;
    } else {
        DASSERT(0x0 == p_extra_data[0]);
        DASSERT(0x0 == p_extra_data[1]);
        DASSERT(0x1 == p_extra_data[2]);
        p_in_buffer = p_extra_data + 3;
    }

    DASSERT(ENalType_SPS == (p_in_buffer[0] & 0x1f));
    p_out_buffer = &subsession->sps_base64[0];
    in_size = (TULong)p_next - 4 - (TULong)p_in_buffer;
    DASSERT(in_size > 20);
    DASSERT(in_size < 128);
    out_size = 4 * ((in_size + 2) / 3);
    gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
    p_out_buffer[out_size] = 0x0;

    //LOG_NOTICE("%p, size %d, p_next %p, gap %d\n", p_extra_data, size, p_next, (TU32)(p_next - p_extra_data));
    //gfPrintMemory(p_extra_data, size);

    p_in_buffer = p_next;
    DASSERT(ENalType_PPS == (p_in_buffer[0] & 0x1f));
    p_out_buffer = &subsession->pps_base64[0];
    in_size = ((TULong)(p_extra_data) + size) - (TULong)p_in_buffer;

    if (DUnlikely(in_size > 20)) {
        LOG_FATAL("extra data corruption, in_size %d\n", in_size);
        return EECode_DataCorruption;
    }

    DASSERT(in_size < 20);
    DASSERT(in_size > 3);
    out_size = 4 * ((in_size + 2) / 3);
    gfEncodingBase64(p_out_buffer, out_size, p_in_buffer, in_size);
    p_out_buffer[out_size] = 0x0;

    LOG_INFO("[h264 streaming]: profile level id %s\n", &subsession->profile_level_id_base16[0]);
    LOG_INFO("[h264 streaming]: sps %s\n", &subsession->sps_base64[0]);
    LOG_INFO("[h264 streaming]: pps %s\n", &subsession->pps_base64[0]);

    subsession->data_comes = 1;
    subsession->parameters_setup = 1;

    return EECode_OK;
}

EECode gfSDPProcessAudioExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size)
{
    if (DUnlikely((!subsession) || (!p_extra_data) || (!size))) {
        LOG_FATAL("gfSDPProcessAudioExtraData: NULL %p, %p or zero %d\n", subsession, p_extra_data, size);
        return EECode_BadParam;
    }

    if (p_extra_data && size) {
        DASSERT(size < 32);
        if (DLikely(32 > size)) {
            gfEncodingBase16(subsession->aac_config_base16, p_extra_data, size);
            subsession->aac_config_base16[31] = 0x0;
        }
    }

    subsession->data_comes = 1;
    subsession->parameters_setup = 1;

    return EECode_OK;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

