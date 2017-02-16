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

#ifndef __IAV_H__
#define __IAV_H__

typedef enum {
	IAV_BUF_MAIN = 0,
	IAV_BUF_SECOND = 1,
	IAV_BUF_THIRD = 2,
	IAV_BUF_FOURTH = 3,
	IAV_BUF_ID_MAX,
} iav_buf_id_t;

typedef enum {
	IAV_BUF_YUV = 0,
	IAV_BUF_ME0 = 1,
	IAV_BUF_ME1 = 2,
	IAV_BUF_TYPE_MAX,
} iav_buf_type_t;

#ifdef __cplusplus
extern "C" {
#endif

int init_iav(iav_buf_id_t buf_id, iav_buf_type_t buf_type);
int get_iav_buf_size(unsigned int *p, unsigned int *w, unsigned int *h);
char * get_iav_buf(void);
int exit_iav(void);

#ifdef __cplusplus
}
#endif

#endif
