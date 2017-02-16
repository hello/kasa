/*******************************************************************************
 * lbr_api.h
 *
 * History:
 *   2014-02-17 - [Louis Sun] created file
 *
 * Copyright (C) 2014-2018, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef __LBR_API_H__
#define __LBR_API_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "lbr_common.h"

//LBR library does not have thread inside. it uses the caller 's thread

int lbr_open();
int lbr_init(lbr_init_t *rc_init);   //init lbr library with iav file handle and etc., must call init after open
int lbr_close();                     //close will also deinit it

/* All lbr APIs can be called on the fly
  If IAV state is not right, then the IAV call will fail. but no harm
*/

int lbr_set_style(LBR_STYLE style, int stream_id);           //let LBR to setup stream size and etc.
int lbr_get_style(LBR_STYLE *style, int stream_id);
int lbr_set_motion_level(LBR_MOTION_LEVEL motion_level, int stream_id);  //update lbr about motion level
int lbr_get_motion_level(LBR_MOTION_LEVEL *motion_level, int stream_id);
int lbr_set_noise_level(LBR_NOISE_LEVEL noise_level, int stream_id);    //update lbr about noise level
int lbr_get_noise_level(LBR_NOISE_LEVEL *noise_level, int stream_id);
int lbr_set_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id);  //set max bitrate to get to
int lbr_get_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id);
int lbr_get_log_level(int *pLog);
int lbr_set_log_level(int log);
u32 lbr_get_version();
int lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile, u32 numerator,
                                     u32 denominator);
#ifdef __cplusplus
}
#endif

#endif /* __LBR_API_H__ */

