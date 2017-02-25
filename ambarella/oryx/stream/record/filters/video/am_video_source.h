/*******************************************************************************
 * am_video_source.h
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
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
#ifndef AM_VIDEO_SOURCE_H_
#define AM_VIDEO_SOURCE_H_

#include "am_mutex.h"
#include <atomic>
#include <vector>

struct AMMemMapInfo;
struct AMVideoPtsInfo;
struct AMVideoFrameNumInfo;
struct VideoSourceConfig;
struct AMQueryDataFrameDesc;

class AMEvent;
class AMFixedPacketPool;
class AMVideoSourceConfig;
class AMVideoSourceOutput;
class AMVideoSource: public AMPacketActiveFilter, public AMIVideoSource
{
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AM_VIDEO_INFO*> AMVideoInfoList;
    typedef std::vector<AMVideoPtsInfo*> AMVideoPtsList;
    typedef std::vector<AMPacketOutputPin*> AMOutputPinList;
    typedef std::vector<AMVideoFrameNumInfo*> AMFrameNumberList;

    enum AM_VIDEO_SRC_STATE {
      AM_VIDEO_SRC_CREATED,
      AM_VIDEO_SRC_INITTED,
      AM_VIDEO_SRC_STOPPED,
      AM_VIDEO_SRC_WAITING,
      AM_VIDEO_SRC_RUNNING,
    };
  public:
    static AMIVideoSource* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);

  public:
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

  private:
    AMVideoSource(AMIEngine *engine);
    virtual ~AMVideoSource();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    AM_VIDEO_INFO* find_video_info(uint32_t streamid);
    AMVideoPtsInfo* find_video_last_pts(uint32_t streamid);
    AMVideoFrameNumInfo* find_video_last_frame_num(uint32_t streamid);
    bool process_video(AMQueryFrameDesc &data, AMPacket *&packet);
    void send_packet(AMPacket *packet);
    void check_video_pts_and_frame_num(AMQueryFrameDesc &data);

  private:
    VideoSourceConfig    *m_vconfig; /* No need to delete */
    AMVideoSourceConfig  *m_config;
    AMFixedPacketPool    *m_packet_pool;
    AMEvent              *m_event;
    AMQueryFrameDesc     *m_frame_desc;
    AMIVideoReaderPtr     m_video_reader;
    AMIVideoAddressPtr    m_video_address;
    uint32_t              m_input_num;
    uint32_t              m_output_num;
    AM_VIDEO_SRC_STATE    m_filter_state;
    std::atomic_bool      m_run;
    AMOutputPinList       m_outputs;
    AMVideoInfoList       m_video_info;
    AMVideoPtsList        m_last_pts;
    AMFrameNumberList     m_last_frame_num;
    AMMemLock             m_lock;
    AMMemLock             m_lock_state;
};

class AMVideoSourceOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend class AMVideoSource;

  public:
    static AMVideoSourceOutput* create(AMPacketFilter *filter)
    {
      return (new AMVideoSourceOutput(filter));
    }

  private:
    AMVideoSourceOutput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMVideoSourceOutput(){}
};

#endif /* AM_VIDEO_SOURCE_H_ */
