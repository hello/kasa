/*******************************************************************************
 * am_encode_overlay.h
 *
 * History:
 *   2016-3-15 - [ypchang] created file
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
#ifndef AM_ENCODE_OVERLAY_H_
#define AM_ENCODE_OVERLAY_H_

#include "am_encode_plugin_if.h"
#include "am_encode_overlay_if.h"
#include "am_encode_overlay_config.h"

using std::recursive_mutex;
using std::atomic_int;
using std::vector;
using std::map;

class AMOverlayArea;
typedef map<AM_STREAM_ID, vector<AMOverlayArea*>>         AMOverlayAreaVecMap;
typedef map<AM_STREAM_ID, vector<pair<int32_t, int32_t>>> AMOverlayDataVecMap;
typedef map<AM_STREAM_ID, pair<int32_t, int32_t>>         AMOverlayBufStartMap;

class AMEncodeStream;
class AMEncodeOverlay: public AMIEncodePlugin, public AMIEncodeOverlay
{
  public:
    static AMEncodeOverlay* create(AMEncodeStream *stream);

  public:
    /* Implement interface of AMIEncodePlugin */
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();

  public:
    virtual int32_t init_area(AM_STREAM_ID stream_id,
                              AMOverlayAreaAttr &attr);
    //return value is the unique index in the area or error(<0)
    virtual int32_t add_data_to_area(AM_STREAM_ID stream_id,
                                     int32_t area_id,
                                     AMOverlayAreaData &data);
    virtual AM_RESULT destroy_overlay(AM_STREAM_ID id);
    virtual AM_RESULT update_area_data(AM_STREAM_ID stream_id,
                                       int32_t area_id,
                                       int32_t index,
                                       AMOverlayAreaData &data);
    virtual AM_RESULT delete_area_data(AM_STREAM_ID stream_id,
                                       int32_t area_id,
                                       int32_t index);
    virtual AM_RESULT change_state(AM_STREAM_ID stream_id,
                                   int32_t area_id,
                                   AM_OVERLAY_STATE state);
    virtual AM_RESULT get_area_param(AM_STREAM_ID stream_id,
                                     int32_t area_id,
                                     AMOverlayAreaParam &param);
    virtual AM_RESULT get_area_data_param(AM_STREAM_ID stream_id,
                                          int32_t area_id,
                                          int32_t index,
                                          AMOverlayAreaData &param);
    virtual void get_user_defined_limit_value(AMOverlayUserDefLimitVal &val);
    virtual AM_RESULT save_param_to_config();
    virtual uint32_t get_area_max_num();

  private:
    AM_RESULT load_config();
    AM_RESULT setup(AM_STREAM_ID id);
    AM_RESULT add(AMOverlayParams &params);
    AM_RESULT add_area(AM_STREAM_ID stream_id, AMOverlayAreaParam &param);
    AM_RESULT delete_overlay(AM_STREAM_ID id);
    AM_RESULT render_area(AM_STREAM_ID stream_id,
                          int32_t area_id,
                          int32_t state);
    AM_RESULT delete_area(AM_STREAM_ID stream_id, int32_t area_id);
    AM_RESULT render_stream(AM_STREAM_ID stream_id, int32_t state);
    AM_RESULT check_area_param(AM_STREAM_ID stream_id,
                               AMOverlayAreaAttr &attr,
                               AMOverlayAreaInsert  &overlay_area);
    AM_RESULT check_stream_id(AM_STREAM_ID stream_id);
    AM_RESULT check_area_id(int32_t area_id);
    int32_t get_available_area_id(AM_STREAM_ID stream_id);
    void area_config_to_param(const AMOverlayConfigAreaParam &src,
                              AMOverlayAreaParam &dst);
    void area_param_to_config(const AMOverlayAreaParam &src,
                              AMOverlayConfigAreaParam &dst);
    void  auto_update_overlay_data();
    static void static_update_thread(void* arg);

  private:
    AMEncodeOverlay();
    virtual ~AMEncodeOverlay();
    AM_RESULT init(AMEncodeStream *stream);
    AM_RESULT init_platform_param();

  private:
    static recursive_mutex  m_mtx;
    AMEvent                *m_event;
    AMThread               *m_thread_hand;
    AMEncodeStream         *m_stream;
    int32_t                 m_max_stream_num;
    int32_t                 m_max_area_num;
    std::string             m_name;
    std::atomic_bool        m_update_run;
    std::atomic_bool        m_exit;
    std::atomic_bool        m_mmap_flag;
    AMIPlatformPtr          m_platform;
    AMOverlayConfigPtr      m_config;
    AMOverlayParamMap       m_config_param;
    AMOverlayAreaVecMap     m_area;
    AMOverlayDataVecMap     m_update_area;
    AMOverlayBufStartMap    m_stream_buf_start;
    AMMemMapInfo            m_mmap_mem;
};

#endif /* AM_ENCODE_OVERLAY_H_ */
