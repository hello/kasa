/*******************************************************************************
 * am_player.h
 *
 * History:
 *   2014-9-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYER_H_
#define AM_PLAYER_H_

struct PlayerConfig;
class AMEvent;
class AMPlayerInput;
class AMPlayerAudio;
class AMPlayerConfig;
class AMPlayer: public AMPacketActiveFilter, public AMIPlayer
{
    typedef AMPacketActiveFilter inherited;
    friend class AMPlayerInput;

  public:
    static AMIPlayer* create(AMIEngine *engine, const std::string& config,
                             uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t id);
    virtual AMIPacketPin* get_output_pin(uint32_t id);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual AM_STATE pause(bool enabled);
    virtual uint32_t version();
    virtual void on_run();

  private:
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_eof(AMPacket *packet);
    AM_STATE process_packet(AMPacket *packet);

  private:
    AMPlayer(AMIEngine *engine);
    virtual ~AMPlayer();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    AMSpinLock     *m_lock;
    AMPlayerAudio  *m_audio_player;
    AMPlayerConfig *m_config;
    AMEvent        *m_event;
    PlayerConfig   *m_player_config; /* No need to delete */
    AMPlayerInput **m_input_pins;
    uint32_t        m_eos_map;
    uint32_t        m_input_num;
    uint32_t        m_output_num;
    bool            m_run;
    bool            m_is_paused;
};

class AMPlayerInput: public AMPacketActiveInputPin
{
    typedef AMPacketActiveInputPin inherited;
    friend class AMPlayer;

  public:
    static AMPlayerInput* create(AMPacketFilter *filter, const char *name)
    {
      AMPlayerInput *result = new AMPlayerInput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name)))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMPlayerInput(AMPacketFilter *filter) :
      inherited(filter){}
    virtual ~AMPlayerInput(){}
    AM_STATE init(const char *name)
    {
      return inherited::init(name);
    }
    virtual AM_STATE process_packet(AMPacket *packet)
    {
      return ((AMPlayer*)m_filter)->process_packet(packet);
    }
};

#endif /* AM_PLAYER_H_ */
