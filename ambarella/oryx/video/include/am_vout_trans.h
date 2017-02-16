/*******************************************************************************
 * am_vout_trans.h
 *
 * Histroy:
 *   Sep 1, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VOUT_TRANS_H_
#define AM_VOUT_TRANS_H_

#include <string>
#include "iav_ioctl.h"

int get_amba_vout_fps(AM_VOUT_MODE mode);
int get_amba_vout_mode(AM_VOUT_MODE mode);
int get_vout_sink_type(AM_VOUT_TYPE  type);
int get_vout_source_id(AM_VOUT_ID am_vout_id);

std::string vout_type_enum_to_str(AM_VOUT_TYPE type);
std::string vout_mode_enum_to_str(AM_VOUT_MODE mode);
AM_VOUT_MODE vout_mode_str_to_enum(const char *mode_str);
AM_VOUT_TYPE vout_type_str_to_enum(const char *type_str);
AM_VOUT_MODE resolution_to_vout_mode(const AMResolution *size);
AMResolution vout_mode_to_resolution(AM_VOUT_MODE mode);

#endif /* AM_VOUT_TRANS_H_ */
