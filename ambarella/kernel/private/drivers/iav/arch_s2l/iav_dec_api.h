/*
 * iav_dec_api.h
 *
 * History:
 *	2015/01/21 - [Zhi He] created file
 *
 * Copyright (C) 2015 -2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_DEC_API_H__
#define __IAV_DEC_API_H__

//from dsp
typedef enum {
	HDEC_OPM_IDLE = 0,
	HDEC_OPM_VDEC_RUN,
	HDEC_OPM_VDEC_IDLE, // still have last picture left
	HDEC_OPM_VDEC_2_IDLE,
	HDEC_OPM_VDEC_2_VDEC_IDLE,
	HDEC_OPM_JDEC_SLIDE, // must be the first of JPEG stuff
	HDEC_OPM_JDEC_SLIDE_IDLE,
	HDEC_OPM_JDEC_SLIDE_2_IDLE,
	HDEC_OPM_JDEC_SLIDE_2_SLIDE_IDLE,
	HDEC_OPM_JDEC_MULTI_S,
	HDEC_OPM_JDEC_MULTI_S_2_IDLE,
	HDEC_OPM_RAW_DECODE,
	HDEC_OPM_RAW_DECODE_2_IDLE, // not used yet - jrc 12/01/2005
} HDEC_OPMODE;

void iav_clean_decode_stuff(struct ambarella_iav *iav);

int iav_decode_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg);

#endif	// __IAV_DEC_API_H__

