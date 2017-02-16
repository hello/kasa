/*******************************************************************************
 * am_vin_trans.h
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

#ifndef AM_VIN_TRANS_H_
#define AM_VIN_TRANS_H_

#include <string>
#include "iav_ioctl.h"
std::string vin_hdr_enum_to_str(AM_HDR_EXPOSURE_TYPE hdr_type);
std::string vin_mode_enum_to_str(AM_VIN_MODE mode);
std::string vin_type_enum_to_str(AM_VIN_TYPE type);

AM_VIN_TYPE vin_type_str_to_enum(const char *type_str);
AM_VIN_MODE vin_mode_str_to_enum(const char *mode_str);
AM_HDR_EXPOSURE_TYPE vin_hdr_str_to_enum(const char *hdr_str);
int get_hdr_mode(AM_HDR_EXPOSURE_TYPE hdr_type);
int get_hdr_expose_num(AM_HDR_EXPOSURE_TYPE hdr_type);
int get_mirror_pattern(AM_VIDEO_FLIP flip);
AMResolution vin_mode_to_resolution(AM_VIN_MODE mode);
AM_VIN_MODE resolution_to_vin_mode(const AMResolution *size);
amba_video_mode get_amba_video_mode(AM_VIN_MODE mode);

#endif /* AM_VIN_TRANS_H_ */
