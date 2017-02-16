/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __FB_H__
#define __FB_H__

enum {
	FB_COLOR_TRANSPARENT	= 0,
	FB_COLOR_SEMI_TRANSPARENT,
	FB_COLOR_RED,
	FB_COLOR_GREEN,
	FB_COLOR_BLUE,
	FB_COLOR_WHITE,
	FB_COLOR_BLACK,
};

#ifdef __cplusplus
extern "C" {
#endif

int open_fb(unsigned int w, unsigned int h);
int blank_fb(void);
int render_frame(char *d);
int close_fb(void);

#ifdef __cplusplus
}
#endif

#endif
