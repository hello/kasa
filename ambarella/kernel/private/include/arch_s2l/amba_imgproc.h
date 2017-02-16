/*
 * kernel/private/include/arch_s2l/amba_imgproc.h
 *
 * History:
 *    2014/02/17 - [Jian Tang] Create
 *
 * Copyright (C) 2014 -2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_IMGPROC_H__
#define __AMBA_IMGPROC_H__

struct iav_imgproc_info {
	u32	iav_state;
	u32	hdr_mode : 2;
	u32	vin_num : 3;
	u32	hdr_expo_num : 3;
	u32	reserved : 24;
	u32	cap_width;
	u32	cap_height;
	u32	img_size;
	unsigned long img_virt;
	unsigned long img_phys;
	unsigned long img_config_offset;
	unsigned long dsp_virt;
	unsigned long dsp_phys;
	struct mutex		*iav_mutex;
};

int amba_imgproc_msg(encode_msg_t *msg, u64 mono_pts);
int amba_imgproc_cmd(struct iav_imgproc_info *info, unsigned int cmd, unsigned long arg);
int amba_imgproc_suspend(void);
int amba_imgproc_resume(int enc_mode, int expo_num);
#endif	// __AMBA_IMGPROC_H__

