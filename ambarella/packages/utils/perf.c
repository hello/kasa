/*******************************************************************************
 * perf.c
 *
 * History:
 *  Jun 5, 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <time.h>
#include "utils.h"

static struct timespec start_time;
static struct timespec stop_time;

void start_timing(void) {
	clock_gettime(CLOCK_REALTIME, &start_time);
}

void stop_timing(void) {
	clock_gettime(CLOCK_REALTIME, &stop_time);
}
void perf_report(void)
{
	long sec_gap = stop_time.tv_sec - start_time.tv_sec;
	long nsec_gap = stop_time.tv_nsec - start_time.tv_nsec;

	if (sec_gap == 0) {
		printf("cost %lu nano seconds = %lu mili seconds\n",
		    nsec_gap, nsec_gap / 1000000);
	} else if (sec_gap == 1 && nsec_gap < 0) {
		nsec_gap += 1000000000;
		printf("cost %lu nano seconds = %lu mili seconds\n",
		    nsec_gap, nsec_gap / 1000000);
	}
}
