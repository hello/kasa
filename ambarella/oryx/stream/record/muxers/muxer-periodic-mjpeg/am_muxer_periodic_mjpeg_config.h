/*******************************************************************************
 * am_muxer_periodic_mjpeg_config.h
 *
 * History:
 *   2016-04-20 - [ccjing] created file
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
#ifndef AM_MUXER_PERIODIC_MJPEG_CONFIG_H_
#define AM_MUXER_PERIODIC_MJPEG_CONFIG_H_

struct AMMuxerCodecPeriodicMjpegConfig
{
    uint32_t       smallest_free_space        = 0;
    uint32_t       muxer_id                   = 0;
    uint32_t       video_id                   = 0;
    uint32_t       max_file_size              = 0;
    bool           auto_file_writing          = false;
    bool           file_location_auto_parse   = false;
    std::string    file_name_prefix;
    std::string    file_location;
    AMMuxerCodecPeriodicMjpegConfig() {}
    AMMuxerCodecPeriodicMjpegConfig& operator=(AMMuxerCodecPeriodicMjpegConfig& config)
    {
      smallest_free_space        = config.smallest_free_space;
      muxer_id                   = config.muxer_id;
      video_id                   = config.video_id;
      max_file_size              = config.max_file_size;
      auto_file_writing          = config.auto_file_writing;
      file_location_auto_parse   = config.file_location_auto_parse;
      file_name_prefix           = config.file_name_prefix;
      file_location              = config.file_location;
      return *this;
    }
};

class AMConfig;
class AMMuxerPeriodicMjpegConfig
{
  public:
    AMMuxerPeriodicMjpegConfig();
    virtual ~AMMuxerPeriodicMjpegConfig();
    AMMuxerCodecPeriodicMjpegConfig* get_config(const std::string& config_file);
    bool set_config(AMMuxerCodecPeriodicMjpegConfig* config);


  private:
    AMConfig                        *m_config                = nullptr;
    AMMuxerCodecPeriodicMjpegConfig *m_periodic_mjpeg_config = nullptr;
};

#endif /* AM_MUXER_PERIODIC_MJPEG_CONFIG_H_ */
