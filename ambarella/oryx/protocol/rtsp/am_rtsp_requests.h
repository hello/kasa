/*******************************************************************************
 * am_rtsp_requests.h
 *
 * History:
 *   2015-1-4 - [Shiming Dong] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#ifndef AM_RTSP_REQUESTS_H_
#define AM_RTSP_REQUESTS_H_

#include "am_rtsp_server.h"
#include "am_rtp_msg.h"

const char* media_type_to_str(AM_RTP_MEDIA_TYPE type);

struct RtspTransHeader
{
    char *mode_string;
    char *client_dst_addr_string;
    AM_RTP_MODE mode;
    uint16_t client_rtp_port;
    uint16_t client_rtcp_port;
    uint8_t  rtp_channel_id;
    uint8_t  rtcp_channel_id;
    uint8_t  client_dst_ttl;
    uint8_t  reserved;
    RtspTransHeader() :
      mode_string(NULL),
      client_dst_addr_string(NULL),
      mode(AM_RTP_UDP),
      client_rtp_port(0),
      client_rtcp_port(0),
      rtp_channel_id(0),
      rtcp_channel_id(0),
      client_dst_ttl(0),
      reserved(0xff){}
    ~RtspTransHeader()
    {
      delete[] mode_string;
      delete[] client_dst_addr_string;
    }

    RtspTransHeader(RtspTransHeader& hdr) :
      mode(hdr.mode),
      client_rtp_port(hdr.client_rtp_port),
      client_rtcp_port(hdr.client_rtcp_port),
      rtp_channel_id(hdr.rtp_channel_id),
      rtcp_channel_id(hdr.rtcp_channel_id),
      client_dst_ttl(hdr.client_dst_ttl),
      reserved(0xff)
    {
      mode_string = amstrdup(hdr.mode_string);
      client_dst_addr_string = amstrdup(hdr.client_dst_addr_string);
    }

    void print_info()
    {
      INFO("         Mode: %s", mode_string);
      INFO("      RtpPort: %hu", client_rtp_port);
      INFO("     RtcpPort: %hu", client_rtcp_port);
      INFO(" RtpChannelID: %hhu", rtp_channel_id);
      INFO("RtcpChannelID: %hhu", rtcp_channel_id);
    }

    void set_streaming_mode_str(char* str)
    {
      delete[] mode_string;
      mode_string = amstrdup(str);
    }

    void set_client_dst_addr_str(char* str)
    {
      delete[] client_dst_addr_string;
      client_dst_addr_string = amstrdup(str);
    }
};

struct RtspAuthHeader
{
    char *realm;
    char *nonce;
    char *uri;
    char *username;
    char *response;

    RtspAuthHeader() :
      realm(NULL),
      nonce(NULL),
      uri(NULL),
      username(NULL),
      response(NULL){}
    ~RtspAuthHeader()
    {
      reset();
    }

    bool is_ok() {
      return (realm && nonce && uri && username && response);
    }

    void reset() {
      delete[] realm;
      realm = NULL;
      delete[] nonce;
      nonce = NULL;
      delete[] uri;
      uri = NULL;
      delete[] username;
      username = NULL;
      delete[] response;
      response = NULL;
    }

    void set_realm(char* rlm)
    {
      delete[] realm;
      realm = amstrdup(rlm);
    }

    void set_nonce(char* nce)
    {
      delete[] nonce;
      nonce = amstrdup(nce);
    }

    void set_uri(char* i)
    {
      delete[] uri;
      uri = amstrdup(i);
    }

    void set_username(char* name)
    {
      delete[] username;
      username = amstrdup(name);
    }

    void set_response(char* pass)
    {
      delete[] response;
      response = amstrdup(pass);
    }

    void print_info()
    {
      INFO("    Realm: %s", realm);
      INFO("    Nonce: %s", nonce);
      INFO("      Uri: %s", uri);
      INFO(" UserName: %s", username);
      INFO(" Response: %s", response);
    }
};

struct RtspRequest
{
    char *command;
    char *url_presuffix;
    char *url_suffix;
    char *cseq;
    char *media[AM_RTP_MEDIA_NUM];

    RtspRequest() :
      command(NULL),
      url_presuffix(NULL),
      url_suffix(NULL),
      cseq(NULL)
    {
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        media[i] = NULL;
      }
    }
    ~RtspRequest()
    {
      delete[] command;
      delete[] url_presuffix;
      delete[] url_suffix;
      delete[] cseq;
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        delete[] media[i];
      }
    }

    void set_command(char* cmd)
    {
      delete[] command;
      command = amstrdup(cmd);
    }

    void set_url_pre_suffix(char* presuffix)
    {
      delete[] url_presuffix;
      url_presuffix = amstrdup(presuffix);
    }

    void set_url_suffix(char* suffix)
    {
      delete[] url_suffix;
      url_suffix = amstrdup(suffix);
    }

    void set_cseq(char* str)
    {
      delete[] cseq;
      cseq = amstrdup(str);
    }

    void set_media(AM_RTP_MEDIA_TYPE type, char *str)
    {
      delete[] media[type];
      media[type] = amstrdup(str);
    }

    void print_info()
    {
      INFO("  Command: %s", command);
      INFO("PreSuffix: %s", url_presuffix);
      INFO("   Suffix: %s", url_suffix);
      INFO("     CSeq: %s", cseq);
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        INFO("    %s: %s", media_type_to_str(AM_RTP_MEDIA_TYPE(i)), media[i]);
      }
    }
};

#endif /* AM_RTSP_REQUESTS_H_ */
