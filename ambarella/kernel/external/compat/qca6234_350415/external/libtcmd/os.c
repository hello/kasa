/*
* Copyright (c) 2010-2011 Atheros Communications Inc.
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "libtcmd.h"
#include  "os.h"

int tcmd_set_timer(struct tcmd_cfg *cfg)
{
	struct itimerspec exp_time;
	int err;

	//A_DBG("setting timer\n");
	bzero(&exp_time, sizeof(exp_time));
	exp_time.it_value.tv_sec = TCMD_TIMEOUT;
	err = timer_settime(cfg->timer, 0, &exp_time, NULL);
	cfg->timeout = false;
	if (err < 0)
		return errno;
	return 0;
}

int tcmd_reset_timer(struct tcmd_cfg *cfg)
{
	struct itimerspec curr_time;
	int err;

	err = timer_gettime(cfg->timer, &curr_time);
	if (err < 0)
		return errno;

	if (!curr_time.it_value.tv_sec && !curr_time.it_value.tv_nsec)
		return -ETIMEDOUT;

	//A_DBG("resetting timer\n");
	bzero(&curr_time, sizeof(curr_time));
	err = timer_settime(cfg->timer, 0, &curr_time, NULL);
	if (err < 0)
		return errno;
	return 0;
}
