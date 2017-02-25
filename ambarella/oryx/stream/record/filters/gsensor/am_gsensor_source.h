/*******************************************************************************
 * am_gsensor_source.h
 *
 * History:
 *   2016-11-28 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#ifndef AM_GSENSOR_SOURCE_H_
#define AM_GSENSOR_SOURCE_H_
#include "am_mutex.h"
#include <atomic>
#include <vector>

#include "am_sei_define.h"
#include "am_gsensor_source_if.h"
struct GsensorSourceConfig;
class AMGsensorSourceConfig;
class AMFixedPacketPool;
class AMGsensorSourceOutput;
class AMGsensorReader;
class AMEvent;
class AMGsensorSource : public AMPacketActiveFilter, public AMIGsensorSource
{
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AMPacketOutputPin*> AMOutputPinList;

    enum AM_GSENSOR_SRC_STATE {
      AM_GSENSOR_SRC_CREATED,
      AM_GSENSOR_SRC_INITTED,
      AM_GSENSOR_SRC_STOPPED,
      AM_GSENSOR_SRC_WAITING,
      AM_GSENSOR_SRC_RUNNING,
    };
  public:
    static AMIGsensorSource* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);
  public :
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual uint32_t version();
  private:
      virtual void on_run();
  private :
      AMGsensorSource(AMIEngine *engine);
      virtual ~AMGsensorSource();
      AM_STATE init(const std::string& config,
                    uint32_t input_num,
                    uint32_t output_num);
      void send_packet(AMPacket* packet);
      int64_t get_current_pts();
      bool create_resource();
      void release_resource();
  private :
      int64_t                    m_last_pts        = 0;
      GsensorSourceConfig       *m_config_struct   = nullptr;/*No need to delete*/
      AMGsensorSourceConfig     *m_config          = nullptr;
      AMFixedPacketPool         *m_packet_pool     = nullptr;
      AMGsensorReader           *m_gsensor_reader  = nullptr;
      AMEvent                   *m_event           = nullptr;
      uint32_t                   m_input_num       = 0;
      uint32_t                   m_output_num      = 0;
      int32_t                    m_hw_timer_fd     = -1;
      AM_GSENSOR_SRC_STATE       m_filter_state    = AM_GSENSOR_SRC_CREATED;
      int                        m_ctrl_gsensor[2] = { -1 };
      bool                       m_info_send       = false;
      std::atomic_bool           m_run             = {false};
      AMOutputPinList            m_outputs;
      AMMemLock                  m_lock;
      AMMemLock                  m_state_lock;
      AM_GSENSOR_INFO            m_info;
};

class AMGsensorSourceOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend class AMGsensorSource;

  public:
    static AMGsensorSourceOutput* create(AMPacketFilter *filter)
    {
      return (new AMGsensorSourceOutput(filter));
    }

  private:
    AMGsensorSourceOutput(AMPacketFilter *filter) :
      inherited(filter)
  {}
    virtual ~AMGsensorSourceOutput(){}
};

#endif /* AM_GPS_SOURCE_H_ */
