/*******************************************************************************
 * am_muxer_if.h
 *
 * History:
 *   2014-12-23 - [Chengcai Jing] created file
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
#ifndef AM_MUXER_CODEC_IF_H_
#define AM_MUXER_CODEC_IF_H_
#include "am_muxer_codec_info.h"
#include "stream/am_record_msg.h"
#include "am_iid_type.h"

extern const AM_IID IID_AMIMuxerCodec;

class AMPacket;

struct AMMuxerCodecConfig;

enum AM_MUXER_CODEC_STATE {
  AM_MUXER_CODEC_ERROR = -1, //Error, can not be restarted, should be destroyed
  AM_MUXER_CODEC_INIT = 1,   //Finished initialing, not started
  AM_MUXER_CODEC_STOPPED,    //stopped, can be restarted
  AM_MUXER_CODEC_RUNNING,    //Have been started, is running.
};

class AMIMuxerCodec
{
  public:
    virtual AM_STATE start()                                                = 0;
    virtual AM_STATE stop()                                                 = 0;
    virtual const char* name()                                              = 0;
    virtual bool is_running()                                               = 0;
    virtual void* get_muxer_codec_interface(AM_REFIID refiid)               = 0;
    virtual AM_STATE set_config(AMMuxerCodecConfig *config)                 = 0;
    virtual AM_STATE get_config(AMMuxerCodecConfig *config)                 = 0;
    virtual AM_MUXER_CODEC_STATE get_state()                                = 0;
    virtual AM_MUXER_ATTR get_muxer_attr()                                  = 0;
    virtual uint8_t get_muxer_codec_stream_id()                             = 0;
    virtual uint32_t get_muxer_id()                                         = 0;
    virtual void feed_data(AMPacket* packet)                                = 0;
    virtual ~AMIMuxerCodec(){}
};

#ifdef __cplusplus
extern "C"
{
#endif
AMIMuxerCodec* get_muxer_codec(const char* config);
#ifdef __cplusplus
}
#endif

typedef AMIMuxerCodec* (*MuxerCodecNew)(const char* config);

#define MUXER_CODEC_NEW ((const char*)"get_muxer_codec")

#endif /* AM_MUXER_CODEC_IF_H_ */
