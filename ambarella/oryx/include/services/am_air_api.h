/*******************************************************************************
 * am_air_api.h
 *
 * History:
 *   2014-5-3 [lysun] created file
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

/*! @mainpage notitle
 *
 *  @section Introduction
 *  Oryx Air API is set of WEB interface APIs, which can be used to
 *  interact with Oryx services, detailed information, please refer to
 *  @ref oryx-service "Services".
 *
 *  @section License
 *  Copyright (c) 2016 Ambarella, Inc.
 *
 *  This file and its contents ("Software") are protected by intellectual
 *  property rights including, without limitation, U.S. and/or foreign
 *  copyrights. This Software is also the confidential and proprietary
 *  information of Ambarella, Inc. and its licensors. You may not use,
 *  reproduce, disclose, distribute, modify, or otherwise prepare derivative
 *  works of this Software or any portion thereof except pursuant to a signed
 *  license agreement or nondisclosure agreement with Ambarella, Inc. or its
 *  authorized affiliates. In the absence of such an agreement, you agree to
 *  promptly notify and return this Software to Ambarella, Inc.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 *  MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  @section oryx-service Services
 *  Oryx middleware consists of a set of services, each service provides
 *  certain functions and co-works with other services to accomplish the job.
 *  - <b>Video Service</b>
 *    - Process name is <b>video_svc</b>.\n
 *      source code is under \$(ORYX)/services/video_service \n
 *      video class source code is under \$(ORYX)/video \n
 *      This service controls VIN and DSP, which contains:
 *      1. Set and get VIN configuration:
 *         (vin.acs)
 *         - type  (rgb sensor or yuv sensor)
 *         - mode  (resolution, "auto" or 1080p or etc)
 *         - flip
 *         - FPS
 *
 *      2. Set and get each stream's format
 *         (stream_fmt.acs)
 *         - encode type (H.264 or MJPEG)
 *         - source buffer
 *         - frame factor (m/n)
 *         - resolution
 *         - offset
 *         - horizontal & vertical flip
 *         - rotate
 *         - enable
 *
 *      3. Set and get each stream's encode config
 *         (stream_cfg.acs)
 *         - gop model
 *         - bitrate control method
 *         - profile
 *         - au_type
 *         - chroma
 *         - M
 *         - N
 *         - IDR frame interval
 *         - target bit-rate
 *         - mv_threshold
 *         - enable & config multi-reference P frame
 *         - mjpeg quality
 *
 *      4. Start and stop encoding
 *
 *      5. Set and get each source buffer's format
 *         (source_buffer.acs)
 *         - buffer type
 *         - input window size
 *         - buffer size
 *
 *      6. Set and get source buffer allocation style
 *         (source_buffer.acs)
 *         - auto  (auto calculation from encode stream format)
 *         - manual
 *
 *      7. Set and get Low Bitrate Control(LBR/SmartAVC) configuration
 *         (lbr.acs)
 *
 *      See @ref airapi-commandid-video "Command ID of Video Service" for more
 *      information.
 *    .
 *  .
 *  - <b>Audio Service</b>
 *    - Process name is <b>audio_svc</b>.\n
 *      source code is under \$(ORYX)/services/audio_service \n
 *      audio class source code is under \$(ORYX)/audio \n
 *      This service provides interface to control system-wide audio devices and
 *      audio streams such as:\n
 *      - Audio Input/Output Device:
 *        1. Volume;
 *        2. Mute;
 *      .
 *      - Audio Recording/Playing Stream:
 *        1. Volume;
 *        2. Mute;
 *      .
 *      See @ref airapi-commandid-audio "Command ID of Audio Service" for more
 *      information.
 *    .
 *  .
 *  - <b>Media Service</b>
 *    - Process name is <b>media_svc</b>.\n
 *      source code is under \$(ORYX)/services/media_service \n
 *      media class source code is under \$(ORYX)/stream \n
 *      This service provides 2 functions :\n
 *      - File/Stream Recording:\n
 *        <b>Stream Record</b> reads Audio/Video raw data and writes the muxed
 *        data in MP4/TS file or sends the data through RTP packet.
 *      .
 *      - Audio File/Stream Playing:\n
 *        <b>Audio Playback</b> can play local audio files or audio data which
 *        is received through RTP protocol.
 *      .
 *      The <b>Media Service</b> contains an UNIX Domain Socket which can be
 *      connected by other services. Other services can send message to
 *      <b>Media Service</b> to control record and playback functions.
 *      For example, the <b>SIP Service</b> will interact with
 *      <b>Media Service</b> when a SIP connection is created.
 *      The <b>SIP Service</b> will send audio information and UDP port used by
 *      RTP protocol to media service and then, the <b>Media Service</b> will
 *      received the audio data sent by SIP client and play it.\n
 *      See @ref airapi-commandid-media "Command ID of Media Service" for more
 *      information.
 *    .
 *  .
 *  - <b>Image Service</b>
 *    - Process name is <b>img_svc</b>.\n
 *      source code is under \$(ORYX)/services/image_service \n
 *      image class source code is under \$(ORYX)/image_quality \n
 *      This service controls the following image related parameters:
 *        1. Auto Exposure (AE);  \n
 *          such as anti flicker setting (50Hz/60Hz), shutter limit,
 *          IR LED control, AE metering, Day/Night mode, backlight control, etc.
 *        2. Auto White Balance (AWB); \n
 *           WB mode
 *        3. Auto Noise Filter Adjustment (ADJ); \n
 *        4. image quality style as high level parameter:
 *           - Brightness;
 *           - Contrast;
 *           - Sharpness;
 *           - Saturation;
 *      .
 *      See @ref airapi-commandid-image "Command ID of Image Service" for more
 *      information.
 *    .
 *  .
 *  - <b>Event Service</b>
 *    - Process name is <b>event_svc</b>.\n
 *      source code is under \$(ORYX)/services/event \n
 *      event class source code is under \$(ORYX)/event \n
 *      This service provides the following functions:
 *      1. Audio Alert;
 *      2. Key Input Trigger;
 *
 *      This service is based on event monitor framework, which is a plug-in
 *      framework. All the functions listed above is a stand alone plug-in
 *      module, which can be loaded or unloaded according to your needs.
 *      When either of the events is triggered, <b>Event Service</b> can output
 *      @ref AM_EVENT_MESSAGE "unified event meta data".\n
 *      See @ref airapi-commandid-event "Command ID of Event Service" for more
 *      information.
 *    .
 *  .
 *  - <b>System Service</b>
 *    - Process name is <b>sys_svc</b>.\n
 *      source code is under \$(ORYX)/services/system \n
 *      This service supports 5 functions:
 *      - System Time Settings\n
 *        You can use get and set method to set or get time/NTP.
 *      .
 *      - NTP Settings\n
 *        Set system to sync time with NTP server.
 *      .
 *      - LED Settings\n
 *        You can set LED on, off or blink depending on your needs.
 *      .
 *      - System Upgrade Settings\n
 *        There are 3 upgrade modes for selecting:\n
 *        1. download firmware only;\n
 *           <b>System Service</b> will only download the upgrade firmware
 *           to the board, and does NOT upgrade the system using this firmware.
 *        2. upgrade only;\n
 *           <b>System Service</b> will upgrade process using the local upgrade
 *           firmware which can be specified by path.
 *        3. download and upgrade;\n
 *           <b>System Service</b> will download the upgrade firmware and
 *           upgrade the system automatically.
 *      .
 *      - Firmware Version Querying\n
 *        You can use this command to get kernel version and
 *        the firmware version.
 *      .
 *      Note:\n
 *      If you set system time and then enable NTP,
 *      the time would be reset by NTP synchronization.
 *      You’d better avoid using these two function at the same time.\n
 *      See @ref airapi-commandid-system "Command ID of System Service" for more
 *      information.
 *    .
 *  .
 *  - <b>SIP Service</b>
 *    - Process name is <b>sip_svc</b>.\n
 *      This service is based on <b>Session Initiation Protocol(SIP)</b> which
 *      is a communications protocol for signaling and controlling multi-media
 *      communication sessions. This service provides functions including
 *      registering to SIP server and bi-directional audio communication.\n
 *      You can register to your local SIP server but also to remote SIP
 *      server according to your register parameters settings.\n
 *      This service can support real-time bi-directional audio communication
 *      by accepting SIP-calls from SIP client.
 *      This service needs to communicate with both RTP muxer for sending
 *      local RTP data to remote SIP client and <b>Media Service</b> for
 *      receiving remote RTP data for playing back.\n
 *      See @ref airapi-commandid-sip "Command ID of SIP Service" for more
 *      information.
 *    .
 *  .
 *  - <b>RTSP Service</b>
 *    - Process name is <b>rtsp_svc</b>.\n
 *      <b>RTSP Service</b> handles RTSP protocol.\n
 *      When clients connect this server through URL like
 *      <b>rtsp://RTSP.Server.IP.Address/video=video0,audio=aac</b>,
 *      RTSP server communicates with RTP muxer in <b>Media Service</b> through
 *      UNIX Domain Socket and finds out the actual media type.
 *      After negotiating with <b>RTSP Service</b>, RTP muxer adds the client IP
 *      information into the RTP muxer's client-data-sending-queue and starts
 *      to send media data to the client.\n
 *      Currently, Ambarella RTSP Service can support the following media:
 *      - Video: Video type can be configured through <b>Video Service</b> APIs.
 *        1. H.264;
 *        2. MJPEG;
 *      .
 *      - Audio:
 *        1. AAC;
 *        2. OPUS;
 *        3. G.726;
 *        4. G.711
 *      .
 *      To specify media type, you can use the following string:\n
 *      1. <b>video=video0</b>\n
 *         <b>video0</b> stands for the first stream of the encoder output
 *      2. <b>audio=aac</b>
 *      3. <b>video=video0,audio=aac</b>
 *         or any combination of video stream and audio type.\n
 *      This service does not provide API currently.
 *    .
 *  .
 *  - <b>Network Service</b>
 *    - Process name is <b>net_svc</b>.\n
 *      Not implemented yet.
 *    .
 *  .
 *  - <b>API Proxy</b>
 *    - Library name is <b>libamapxy.so</b>, which runs in process <b>apps_launcher</b>\n
 *      source code can be found at \$(ORXY)/services/api_proxy \n
 *      <b>API Proxy</b> acts as Air API main entry point.\n
 *      Web CGI is supposed to connect to API Proxy by IPC, and web CGI sends
 *      Air API cmds to API Proxy, and API Proxy will route the cmds to real
 *      services that handles the cmd, and return the result of the cmd back to
 *      Web CGI when cmds finishes. It's like "agent/proxy" role. \n
 *      This design can let Web CGI only needs to talk to single process which
 *      runs API Proxy, and does not need to care who is handling the Air API.
 *      Web CGI is designed to call Air APIs by blocking call, which means it
 *      will pend there until the cmd is executed and returned. \n
 *      API Proxy can carry the Air API returned result in the function argument,
 *      because all Air API functions are void type.  \n
 *      API Proxy is designed to serialize the Web CGI cmd call to Air API, so
 *      if Web CGI tries to call API A and B and C in this order, then it will
 *      finish A, then B, then C, strictly following the order. \n
 *      API Proxy routes the Air API by cmd type, so if you add new Air API into
 *      the system, as long as you follow existing type definition, it will route
 *      the cmd into the same destination, without the need of changing the API
 *      proxy source code. In this sense, it works like a 'router'.  \n
 *   .
 *  .
 *  @section Features
 *  The Features configuration is for overall product feature\n
 *  Feature configuration is at \$(ORYX)/video/etc/features.acs. \n
 *
 *  The configuration items in feature.acs:
 *  - hdr: \n
 *      "none" : no HDR \n
 *      "2x"  : 2-exposure combination HDR (line by line)\n
 *      "3x"  : 3-exposure combination HDR (line by line)\n
 *      when HDR enabled by this option, Oryx will automatically choose HDR
 *      config for VIN and also for Image; and when this option is set to none
 *      HDR, then Oryx will also automatically choose linear config for VIN and
 *      for Image too.
 *    .
 *  - iso:\n
 *      "normal"  : normal ISO, good image quality in low light \n
 *      "plus"    : ISO+, better image quality in low light \n
 *      "advanced": Advanced ISO, best image quality in low light \n
 *    .
 *  - dewarp:\n
 *      "none" : No dewarp \n
 *      "ldc"  : lens distortion correction, single dewarp \n
 *      "full" : multi-region dewarp \n
 *      currently this feature is not enabled \n
 *
 *  Examples: \n
 *    If you want to enable HDR mode, you can change the "hdr = " line in features.acs from "none" to
 *    "2x", or "3x".  Please note that S2Lm only supports 2x HDR. And S2L can
 *    support 2x/3x HDR.
 *
 *    If you want to try "normal iso", you can change the "iso = " line to
 *    "iso = "normal"". But it's usually recommended to use iso = "plus" always
 *    for S2Lm for better image quality.
 *
 *    If you want to enable lens distortion correction, you can change the "dewarp ="
 *    line to "dewarp = "ldc"".
 *    Please note that S2Lm only supports ldc. And S2L can support multi-region dewarp.
 *
 *  Features Combination: \n
 *    All possible combinations of features list here: \n
 *    ('0' means not supported) \n
 *    {2xHDR, 3xHDR, normal ISO, ISO+, advanced ISO, LDC, multi-region dewarp} //All features \n
 *    {2xHDR, 0,     normal ISO, 0,    0,            LDC, 0                  }, //mode 0 \n
 *    {0,     0,     0,          0,    0,            0,   0                  }, //mode 1 (future) \n
 *    {0,     0,     0,          0,    0,            0,   0                  }, //mode 2 (reserved) \n
 *    {0,     0,     0,          0,    0,            0,   0                  }, //mode 3 (not used) \n
 *    {2xHDR, 0,     0,          ISO+, advanced ISO, LDC, 0                  }, //mode 4 \n
 *    {0,     3xHDR, 0,          0,    advanced ISO, 0,   0                  }, //mode 5 \n
 */

/*! @file am_air_api.h
 *  @brief This file is the unified header file of all services.
 *
 *  To call only one service's commands, just include the corresponding service
 *  header files.\n
 *  For example:
 *  To interact with Video Service, just write the following snippets:\n\n
 *  #include "am_api_video.h"\n
 *  #include "am_api_helper.h"\n
 */

#ifndef AM_AIR_API_H_
#define AM_AIR_API_H_

#include "am_api_video.h"
#include "am_api_audio.h"
#include "am_api_media.h"
#include "am_api_image.h"
#include "am_api_event.h"
#include "am_api_system.h"
#include "am_api_sip.h"
#include "commands/am_api_cmd_common.h"

#endif /* AM_AIR_API_H_ */
