/*******************************************************************************
 * am_video_source_new.h
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_VIDEO_SOURCE_NEW_H_
#define AM_VIDEO_SOURCE_NEW_H_

#include <vector>

struct AMMemMapInfo;
struct AMVideoPtsInfo;
struct VideoSourceConfig;
struct AMQueryDataFrameDesc;

class AMEvent;
class AMSpinLock;
class AMFixedPacketPool;
class AMVideoSourceConfig;
class AMVideoSourceOutput;
class AMVideoSource: public AMPacketActiveFilter, public AMIVideoSource
{
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AM_VIDEO_INFO*> AMVideoInfoList;
    typedef std::vector<AMVideoPtsInfo*> AMVideoPtsList;
    typedef std::vector<AMPacketOutputPin*> AMOutputPinList;

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
    bool process_video(AMQueryFrameDesc &data, AMPacket *&packet);
    void send_packet(AMPacket *packet);

  private:
    VideoSourceConfig    *m_vconfig; /* No need to delete */
    AMVideoSourceConfig  *m_config;
    AMFixedPacketPool    *m_packet_pool;
    AMSpinLock           *m_lock;
    AMSpinLock           *m_lock_state;
    AMEvent              *m_event;
    AMQueryFrameDesc     *m_frame_desc;
    AMOutputPinList       m_outputs;
    AMIVideoReaderPtr     m_video_reader;
    AMIVideoAddressPtr    m_video_address;
    AMVideoInfoList       m_video_info;
    AMVideoPtsList        m_last_pts;
    uint32_t              m_input_num;
    uint32_t              m_output_num;
    AM_VIDEO_SRC_STATE    m_filter_state;
    bool                  m_run;
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

#endif /* AM_VIDEO_SOURCE_NEW_H_ */
