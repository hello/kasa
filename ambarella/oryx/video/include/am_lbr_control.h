/**
 * am_lbr_control.h
 *
 *  History:
 *		Nov 18, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AM_LBR_CONTROL_H_
#define AM_LBR_CONTROL_H_

#include "iav_ioctl.h"
#include "lbr_api.h"
#include "am_video_dsp.h"
#include "am_thread.h"
#define MAX_LBR_STREAM_NUM 4
#define LBR_CTRL_PROFILE_NUM 5
#define LBR_NOISE_NONE_DELAY 120
#define LBR_NOISE_LOW_DELAY 2
#define LBR_NOISE_HIGH_DELAY 0
#define VIN0_VSYNC "/proc/ambarella/vin0_idsp"

struct event_msg_t;

class AMLBRControl
{
public:
  AMLBRControl();
  virtual ~AMLBRControl();
  virtual AM_RESULT init(int32_t fd_iav, AMEncodeDevice *encode_device);
  virtual AM_RESULT lbr_ctrl_start();
  virtual AM_RESULT lbr_ctrl_stop();
  virtual AM_RESULT load_config(AMEncodeLBRConfig *lbr_config);
  virtual AM_RESULT set_config(AMEncodeLBRConfig *lbr_config);
  virtual AM_RESULT get_config(AMEncodeLBRConfig *lbr_config);
  virtual void receive_motion_info_from_ipc_lbr(void *msg_data);

private:
  AM_RESULT get_enable_lbr(uint32_t stream_id, uint32_t *enable_lbr);
  AM_RESULT set_enable_lbr(uint32_t stream_id, uint32_t enable_lbr);
  AM_RESULT get_lbr_bitrate_target(uint32_t stream_id,
                              lbr_bitrate_target_t *bitrate_target);
  AM_RESULT set_lbr_bitrate_target(uint32_t stream_id,
                              lbr_bitrate_target_t bitrate_target);
  AM_RESULT get_lbr_drop_frame(uint32_t stream_id, uint32_t *enable);
  AM_RESULT set_lbr_drop_frame(uint32_t stream_id, uint32_t enable);
  AM_RESULT get_lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile,
                                            AMScaleFactor *profile_bt_sf);
  AM_RESULT set_lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile,
                                            AMScaleFactor profile_bt_sf);
  AM_RESULT get_lbr_log_level(int32_t *log_level);
  AM_RESULT set_lbr_log_level(int32_t log_level);
  void print_lbr_config(void);
  static void lbr_main(void *data);
private:
  bool m_main_loop_exit;
  int32_t m_noise_low_threshold;
  int32_t m_noise_high_threshold;
  AMLBRStream m_stream_params[MAX_LBR_STREAM_NUM];
  uint8_t m_event_type; /*1: motion */
  uint32_t m_motion_value;
  int32_t m_noise_value;
  LBR_MOTION_LEVEL m_lbr_motion_level[MAX_LBR_STREAM_NUM];
  LBR_NOISE_LEVEL m_lbr_noise_level[MAX_LBR_STREAM_NUM];
  int32_t m_fd_iav;
  bool m_init_done;
  AMEncodeStreamFormat *m_lbr_stream_format[MAX_LBR_STREAM_NUM];
  uint32_t m_log_level;
  AMScaleFactor m_profile_bt_sf[LBR_CTRL_PROFILE_NUM];
  LBR_STYLE m_lbr_style[MAX_LBR_STREAM_NUM];
  AMThread *m_lbr_ctrl_thread;
  AMEncodeDevice *m_encode_device;
};

#endif /* AM_LBR_CONTROL_H_ */
