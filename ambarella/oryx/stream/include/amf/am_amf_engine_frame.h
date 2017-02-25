/*******************************************************************************
 * am_amf_engine_frame.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
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
    inline void set_output_pins_enabled(AMIPacketFilter *filter, bool enabled);

  protected:
    Filter     *m_filters;
    Connection *m_connections;
    uint32_t    m_filter_num;
    uint32_t    m_filter_num_max;
    uint32_t    m_connection_num;
    uint32_t    m_connection_num_max;
    bool        m_is_filter_running;
};

#endif /* AM_AMF_ENGINE_FRAME_H_ */
