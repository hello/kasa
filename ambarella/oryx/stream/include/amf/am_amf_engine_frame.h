/*******************************************************************************
 * am_amf_engine_frame.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AMF_ENGINE_FRAME_H_
#define AM_AMF_ENGINE_FRAME_H_

/*
 * AMEngineFrame
 */
class AMPlugin;
class AMEngineFrame: public AMBaseEngine
{
    typedef AMBaseEngine inherited;

  public:
    void clear_graph();
    AM_STATE run_all_filters();
    void stop_all_filters();
    void purge_all_filters();
    void delete_all_connections();
    void remove_all_filters();
    void enable_output_pins(AMIPacketFilter *filter);
    void disable_output_pins(AMIPacketFilter *filter);
    AM_STATE add_filter(AMIPacketFilter *filter, AMPlugin *so);
    AM_STATE connect(AMIPacketFilter *upStreamFilter, uint32_t indexUp,
                     AMIPacketFilter *downStreamFilter, uint32_t indexDown);
    AM_STATE create_connection(AMIPacketPin *out, AMIPacketPin *in);
    const char* get_filter_name(AMIPacketFilter *filter);

  protected:
    struct Filter {
      AMPlugin        *so;
      AMIPacketFilter *filter;
      uint32_t         flags;
    };

    struct Connection {
        AMIPacketPin *pin_in;
        AMIPacketPin *pin_out;
    };

  protected:
    AMEngineFrame();
    virtual ~AMEngineFrame();
    AM_STATE init(uint32_t filter_num, uint32_t connection_num);

  private:
    void purge_filter(AMIPacketFilter *filter);
    inline void set_output_pins_enabled(AMIPacketFilter *filter, bool enabled);

  protected:
    bool        m_is_filter_running;
    uint32_t    m_filter_num;
    uint32_t    m_filter_num_max;
    uint32_t    m_connection_num;
    uint32_t    m_connection_num_max;
    Filter     *m_filters;
    Connection *m_connections;
};

#endif /* AM_AMF_ENGINE_FRAME_H_ */
