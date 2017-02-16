/*
 * iav_vout.h
 *
 * History:
 *	2012/10/11- [Cao Rongrong] Created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_VOUT_H__
#define __IAV_VOUT_H__

#include "iav.h"

struct amba_iav_vout_info {
	u32				enabled;
	int				active_sink_id;
	struct amba_video_sink_mode	active_mode;
	struct amba_video_info		video_info;
};

extern struct amba_iav_vout_info G_voutinfo[];

void iav_config_vout(vout_src_t vout_src);
void iav_change_vout_src(vout_src_t vout_src);
int iav_vout_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg);

#endif

