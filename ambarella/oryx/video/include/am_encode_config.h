/**
 * am_encode_config.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_CONFIG_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_CONFIG_H_

#include "am_encode_config_param.h"

#include <mutex>
#include <atomic>

class AMVinConfig;
class AMVoutConfig;
class AMBufferConfig;
class AMStreamConfig;
class AMResourceConfig;
class AMOverlayConfig;
class AMWarpConfig;

//For VIN
typedef AMPointer<AMVinConfig> AMVinConfigPtr;
class AMVinConfig
{
    friend AMVinConfigPtr;

  public:
    static AMVinConfigPtr get_instance();

    AM_RESULT get_config(AMVinParamMap &config);
    AM_RESULT set_config(const AMVinParamMap &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AMVinConfig();
    virtual ~AMVinConfig();
    void inc_ref();
    void release();

  private:
    static AMVinConfig         *m_instance;
    static std::recursive_mutex m_mtx;
    AMVinParamMap               m_config;
    std::atomic_int             m_ref_cnt;
};

//For VOUT
typedef AMPointer<AMVoutConfig> AMVoutConfigPtr;
class AMVoutConfig
{
    friend AMVoutConfigPtr;

  public:
    static AMVoutConfigPtr get_instance();

    AM_RESULT get_config(AMVoutParamMap &config);
    AM_RESULT set_config(const AMVoutParamMap &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AMVoutConfig();
    virtual ~AMVoutConfig();
    void inc_ref();
    void release();

  private:
    static AMVoutConfig        *m_instance;
    static std::recursive_mutex m_mtx;
    AMVoutParamMap              m_config;
    std::atomic_int             m_ref_cnt;
};

//For BUFFER
typedef AMPointer<AMBufferConfig> AMBufferConfigPtr;
class AMBufferConfig
{
    friend AMBufferConfigPtr;

  public:
    static AMBufferConfigPtr get_instance();
    AM_RESULT get_config(AMBufferParamMap &config);
    AM_RESULT set_config(const AMBufferParamMap &config);

  private:
    AMBufferConfig();
    virtual ~AMBufferConfig();
    void inc_ref();
    void release();

    AM_RESULT load_config();
    AM_RESULT save_config();

  private:
    static AMBufferConfig      *m_instance;
    static std::recursive_mutex m_mtx;
    AMBufferParamMap            m_config;
    std::atomic_int             m_ref_cnt;
};

//For STREAM
typedef AMPointer<AMStreamConfig> AMStreamConfigPtr;
class AMStreamConfig
{
    friend AMStreamConfigPtr;

  public:
    static AMStreamConfigPtr get_instance();

    AM_RESULT get_config(AMStreamParamMap &config);
    AM_RESULT set_config(const AMStreamParamMap &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AM_RESULT load_stream_format();
    AM_RESULT save_stream_format();

    AM_RESULT load_stream_config();
    AM_RESULT save_stream_config();

    AMStreamConfig();
    virtual ~AMStreamConfig();
    void inc_ref();
    void release();

  private:
    static AMStreamConfig      *m_instance;
    static std::recursive_mutex m_mtx;
    AMStreamParamMap            m_config;
    std::atomic_int             m_ref_cnt;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_CONFIG_H_ */
