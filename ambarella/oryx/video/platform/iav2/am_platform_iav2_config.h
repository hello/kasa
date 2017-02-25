/**
 * am_platform_iav2_config.h
 *
 *  History:
 *    Aug 25, 2015 - [Shupeng Ren] created file
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
 */
#ifndef ORYX_VIDEO_PLATFORM_IAV2_AM_PLATFORM_IAV2_CONFIG_H_
#define ORYX_VIDEO_PLATFORM_IAV2_AM_PLATFORM_IAV2_CONFIG_H_

#include <mutex>
#include <vector>
#include <atomic>
#include "am_video_types.h"
#include "am_pointer.h"

using std::pair;
using std::vector;
using std::string;
using std::atomic_int;
using std::recursive_mutex;

class AMFeatureConfig;
typedef AMPointer<AMFeatureConfig> AMFeatureConfigPtr;
class AMFeatureConfig
{
    friend AMFeatureConfigPtr;

  public:
    static AMFeatureConfigPtr get_instance();
    AM_RESULT get_config(AMFeatureParam &config);
    AM_RESULT set_config(const AMFeatureParam &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AMFeatureConfig();
    virtual ~AMFeatureConfig();
    void inc_ref();
    void release();

  private:
    static AMFeatureConfig  *m_instance;
    static recursive_mutex   m_mtx;
    atomic_int               m_ref_cnt;
    AMFeatureParam           m_config;
};

struct AMResourceParam
{
    pair<bool, uint32_t>                max_num_encode = {false, 0};
    pair<bool, uint32_t>                max_num_cap_sources = {false, 0};

    pair<bool, vector<AMResolution>>    buf_max_size = {false, {}};
    pair<bool, vector<uint32_t>>        buf_extra_dram = {false, {}};
    pair<bool, vector<AMResolution>>    stream_max_size = {false, {}};
    pair<bool, vector<uint32_t>>        stream_max_M = {false, {}};
    pair<bool, vector<uint32_t>>        stream_max_N = {false, {}};
    pair<bool, vector<uint32_t>>        stream_max_advanced_quality_model = {false, {}};
    pair<bool, vector<bool>>            stream_long_ref_possible = {false, {}};

    pair<bool, bool>                    rotate_possible = {false, false};
    pair<bool, bool>                    raw_capture_possible = {false, false};
    pair<bool, bool>                    vout_swap_possible = {false, false};
    pair<bool, bool>                    lens_warp_possible = {false, false};
    pair<bool, bool>                    enc_raw_rgb_possible = {false, false};
    pair<bool, bool>                    enc_raw_yuv_possible = {false, false};
    pair<bool, bool>                    enc_from_mem_possible = {false, false};
    pair<bool, bool>                    mixer_a_enable = {false, false};
    pair<bool, bool>                    mixer_b_enable = {false, false};
    pair<bool, uint32_t>                me0_scale = {false, 0};
    pair<bool, uint32_t>                efm_buf_num = {false, 0};
    pair<bool, uint32_t>                idsp_upsample_type = {false, 0};
    pair<bool, uint32_t>                dsp_partition_map = {false, 0};

    pair<bool, AMResolution>            raw_max_size = {false, {}};
    pair<bool, AMResolution>            v_warped_main_max_size = {false, {}};
    pair<bool, AMResolution>            max_warp_input_size = {false, {}};
    pair<bool, uint32_t>                max_warp_output_width = {false, 0};
    pair<bool, uint32_t>                max_padding_width = {false, 0};
    pair<bool, uint32_t>                enc_dummy_latency = {false, 0};
    pair<bool, uint32_t>                eis_delay_count = {false, 0};

    pair<bool, int32_t>                 debug_iso_type = {false, 0};
    pair<bool, int32_t>                 debug_stitched = {false, 0};
    pair<bool, int32_t>                 debug_chip_id = {false, 0};
};

class AMResourceConfig;
typedef AMPointer<AMResourceConfig> AMResourceConfigPtr;
class AMResourceConfig
{
    friend AMResourceConfigPtr;

  public:
    static AMResourceConfigPtr get_instance();
    AM_RESULT set_config_file(const string &file);
    AM_RESULT get_config(AMResourceParam &config);
    AM_RESULT set_config(const AMResourceParam &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AMResourceConfig();
    virtual ~AMResourceConfig();
    void inc_ref();
    void release();

  private:
    static AMResourceConfig  *m_instance;
    static recursive_mutex    m_mtx;
    atomic_int                m_ref_cnt;
    string                    m_config_file;
    AMResourceParam           m_config;
};

#endif /* ORYX_VIDEO_PLATFORM_IAV2_AM_PLATFORM_IAV2_CONFIG_H_ */
