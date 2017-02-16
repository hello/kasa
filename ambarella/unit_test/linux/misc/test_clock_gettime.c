/*
 * test_clock_gettime.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

struct timespec time_diff(struct timespec start, struct timespec end)
{
	struct timespec				temp;

	if ((end.tv_nsec - start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000LL + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return temp;
}

void clock_diff(clockid_t clock_id, int loop, const char *test_name)
{
	struct timespec				start;
	struct timespec				end;
	struct timespec				diff;

	clock_gettime(clock_id, &start);
	while (loop--) {
		__asm__ __volatile__ ("nop");
	}
	clock_gettime(clock_id, &end);
	diff = time_diff(start, end);

	printf("%s: start %lld\n", test_name,
		start.tv_sec * 1000000000LL + start.tv_nsec);
	printf("%s: end %lld\n", test_name,
		end.tv_sec * 1000000000LL + end.tv_nsec);
	printf("%s: diff %lld\n", test_name,
		diff.tv_sec * 1000000000LL + diff.tv_nsec);
}

int main(int argc, char **argv)
{
	sleep(1);
	clock_diff(CLOCK_THREAD_CPUTIME_ID, 1000000000, "CLOCK_THREAD_CPUTIME");
	sleep(1);
	clock_diff(CLOCK_PROCESS_CPUTIME_ID, 1000000000, "CLOCK_PROCESS_CPUTIME");
	sleep(1);
	clock_diff(CLOCK_REALTIME, 1000000000, "CLOCK_REALTIME");
	sleep(1);
	clock_diff(CLOCK_MONOTONIC, 1000000000, "CLOCK_MONOTONIC");

	return 0;
}

