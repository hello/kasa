/*============================================================================
  bandwidth 0.24, a benchmark to estimate memory transfer bandwidth.
  Copyright (C) 2005-2010 by Zack T Smith.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The author may be reached at fbui@comcast.net.
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>
#include <math.h>
#include <pthread.h>

#include <netdb.h> // gethostbyname
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defs.h"
#include "BMP.h"
#include "config.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef __linux__
#include <linux/fb.h>
#include <sys/mman.h>
#endif

#ifdef CONFIG_ARCH_S2L
#if  defined(CONFIG_BSP_BOARD_S2LM_KIWI) || defined(CONFIG_BSP_BOARD_STRAWBERRY)
#define DRAM_SIZE_SMALL
#endif
#endif

//----------------------------------------
// Graphing data.
//
static char graph_title [500];
#define TITLE "Results from bandwidth " VERSION " by Zack Smith, http://caladan.tk"
static BMP *graph;	// Graph of results.
static int graph_width = 1280;
static int graph_height = 720;
static int graph_left_margin = 100;
static int graph_margin = 50; // top/bottom/right
static int graph_x_span = 1;
static int graph_y_span = 1;
static int graph_last_x = -1;
static int graph_last_y = -1;
static unsigned long graph_fg = RGB_BLACK;
static int legend_y;
#define MAX_GRAPH_DATA 5000
static long graph_data [MAX_GRAPH_DATA];
static int graph_data_index = 0;
enum {
	DATUM_SIZE=0,
	DATUM_AMOUNT=1,
	DATUM_COLOR=2,
};
static int max_bandwidth = 0;	// Always 10 times the # of megabyte/sec.

static bool use_sse2 = true;
static bool use_sse4 = true;

static int goon_flag = 1;
static int thread_num = 4;
static int chunk_index = 0;
static int cpu_num = 0;

struct thread_params {
	int id;
	unsigned long size;
	bool random;
	unsigned long **chunk_ptrs;
	unsigned char *chunk;
	unsigned long loops;
};

//----------------------------------------
// Parameters for the tests.
//
static long usec_per_test = 5000000;	// 5 seconds per test.

static int chunk_sizes[] = {
	256,
	512,
	768,
	1024,
	2048,
	3072,
	4096,
	6144,
	8192,	// Some processors' L1 data caches are only 8kB.
	12288,
	16384,
	20480,
	24576,
	28672,
	32768,	// Common L1 data cache size.
	40960,
	49152,
	65536,
	131072,	// Old L2 cache size.
	192 * 1024,
	256 * 1024,	// Old L2 cache size.
	384 * 1024,
	512 * 1024,	// Old L2 cache size.
	768 * 1024,
	1 << 20,	// 1 MB = common L2 cache size.
	(1024 + 256) * 1024,	// 1.25
	(1024 + 512) * 1024,	// 1.5
	(1024 + 768) * 1024,	// 1.75
	1 << 21,	// 2 MB = common L2 cache size.
	(2048 + 256) * 1024,	// 2.25
	(2048 + 512) * 1024,	// 2.5
	(2048 + 768) * 1024,	// 2.75
	3072 * 1024,	// 3 MB = common L2 cache sized.
	1 << 22,	// 4 MB
	5242880,	// 5 megs
	6291456,	// 6 megs (std L2 cache size)
	16 * 1024 * 1024,
	64 * 1024 * 1024,
	0
};

//----------------------------------------
// Under CeGCC, the math.h log2() function
// turned out to be very inaccurate e.g.
// log2(8)=1.44, so I have here hard-coded
// the logarithms.
//
static double chunk_sizes_log2[] =
{
	8,
	9,
	9.585,
	10,
	11,
	11.585,
	12,
	12.585,
	13,		// 8 kB
	13.585,
	14,		// 16 kB
	14.3219,	// 20 kB
	14.585,		// 24 kB
	14.8074,	// 28 kB
	15,		// 32 kB
	15.3219,	// 40 kB
	15.585,		// 48 kB
	16,		// 64 kB
	17,		// 128 kB
	17.585,		// 192 kB
	18,		// 256 kB
	18.585,		// 385 kB
	19,		// 512 kB
	19.585,		// 768 kB
	20,		// 1 MB
	20.3219,	// 1.25
	20.585,		// 1.5
	20.8074,	// 1.75
	21,		// 2 MB
	21.1699,	// 2.25 MB
	21.3219,	// 2.5 MB
	21.4594,	// 2.75 MB
	21.585,		// 3 MB
	22,		// 4 MB
	22.3219,
	22.585,
	24,
	26,
	0
};

static int min_chunk_size = 1;	// These are determined in graph_draw_labels().
static int max_chunk_size = 1;

//----------------------------------------------------------------------------
// Name:	error
// Purpose:	Complain and exit.
//----------------------------------------------------------------------------
void error (char *s)
{
#ifndef __WIN32__
	fprintf (stderr, "Error: %s\n", s);
	exit (1);
#else
	wchar_t tmp [200];
	int i;
	for (i = 0; s[i]; i++)
		tmp[i] = s[i];
	tmp[i] = 0;
	MessageBoxW (0, tmp, L"Error", 0);
	ExitProcess (0);
#endif
}

void
dump_hex64 (unsigned long long value)
{
	unsigned long long v2 = value;
	int i = 16;
	while (i--) {
		unsigned long long tmp = v2 >> 60;
		unsigned int tmp2 = (unsigned int) tmp;
		printf ("%1x", tmp2);
		v2 <<= 4;
	}
}

//============================================================================
// Graphing logic.
//============================================================================

//----------------------------------------------------------------------------
// Name:	graph_draw_labels
// Purpose:	Draw the labels and ticks.
//----------------------------------------------------------------------------
void
graph_draw_labels ()
{
	int i;

	//----------------------------------------
	// Horizontal
	//
	//--------------------
	// Establish min & max.
	//
	min_chunk_size = 1000;
	max_chunk_size = 0;
	i = 0;
	int j;
	while ((j = chunk_sizes_log2 [i])) {
		if (j < min_chunk_size)
			min_chunk_size = j;
		if (j > max_chunk_size)
			max_chunk_size = j;
		i++;
	}

	for (i = min_chunk_size; i <= max_chunk_size; i++) {
		char str[20];
		int x = graph_left_margin +
			((i-min_chunk_size) * graph_x_span) /
			(max_chunk_size - min_chunk_size);
		int y = graph_height - graph_margin + 10;

		unsigned long amt = 1 << i;
		if (amt < 1024)
			sprintf (str, "%ld B", amt);
		else if (amt < (1<<20)) {
			sprintf (str, "%ld kB", amt >> 10);
		}
		else {
			j = amt >> 20;
			switch ((amt >> 18) & 3) {
			case 0: sprintf (str, "%d MB", j); break;
			case 1: sprintf (str, "%d.25 MB", j); break;
			case 2: sprintf (str, "%d.5 MB", j); break;
			case 3: sprintf (str, "%d.75 MB", j); break;
			}
		}

		BMP_vline (graph, x, y, y-10, RGB_BLACK);
		BMP_draw_mini_string (graph, str, x - 10, y+8, RGB_BLACK);
	}

	//----------------------------------------
	// Vertical
	//
	for (i = 0; i <= (max_bandwidth/10000); i++) {
		char str[20];
		int x = graph_left_margin - 10;
		int y = graph_height - graph_margin -
			(i * graph_y_span) / (max_bandwidth/10000);

		BMP_hline (graph, x, x+10, y, RGB_BLACK);

		sprintf (str, "%d GB/s", i);
		BMP_draw_mini_string (graph, str,
			x - 40, y - MINIFONT_HEIGHT/2, RGB_BLACK);
	}
}

void
graph_init ()
{
	if (!graph)
		return;

	BMP_clear (graph, RGB_WHITE);

	BMP_hline (graph, graph_left_margin, graph_width - graph_margin,
			graph_height - graph_margin, RGB_BLACK);
	BMP_vline (graph, graph_left_margin, graph_margin,
			graph_height - graph_margin, RGB_BLACK);

	graph_x_span = graph_width - (graph_margin + graph_left_margin);
	graph_y_span = graph_height - 2 * graph_margin;

	BMP_draw_mini_string (graph, graph_title,
		graph_left_margin, graph_margin/2, RGB_BLACK);

	legend_y = graph_margin;
}

void
graph_new_line (char *str, unsigned long color)
{
	BMP_draw_mini_string (graph, str,
		graph_width - graph_margin - 200, legend_y, color);

	legend_y += 10;

	graph_fg = color;
	graph_last_x = graph_last_y = -1;

	if (graph_data_index >= MAX_GRAPH_DATA-2)
		error ("Too many graph data.");

	graph_data [graph_data_index++] = DATUM_COLOR;
	graph_data [graph_data_index++] = (long) color;
}

//----------------------------------------------------------------------------
// Name:	graph_add_point
// Purpose:	Adds a point to this list to be drawn.
//----------------------------------------------------------------------------
void
graph_add_point (int size, int amount)
{
	if (graph_data_index >= MAX_GRAPH_DATA-4)
		error ("Too many graph data.");

	graph_data [graph_data_index++] = DATUM_SIZE;
	graph_data [graph_data_index++] = size;
	graph_data [graph_data_index++] = DATUM_AMOUNT;
	graph_data [graph_data_index++] = amount;
}

//----------------------------------------------------------------------------
// Name:	graph_plot
// Purpose:	Plots a point on the current graph.
//----------------------------------------------------------------------------
void
graph_plot (int size, int amount)
{
	//----------------------------------------
	// Get the log2 of the chunk size.
	// We cannot rely on the libm math.h log2
	// function, because under CeGCC,
	// log2(8) = 1.44.
	//
	int i = chunk_index;
	while (chunk_sizes [i] && chunk_sizes [i] != size)
		i++;
	if (!chunk_sizes [i])
		error ("Lookup of chunk size failed.");
	double tmp = chunk_sizes_log2 [i];

	//----------------------------------------
	// Plot the point. The x axis is
	// logarithmic, base 2.
	//
	tmp -= (double) min_chunk_size;
	tmp *= (double) graph_x_span;
	tmp /= (double) (max_chunk_size - min_chunk_size);

	int x = graph_left_margin + (int) tmp;
	int y = graph_height - graph_margin -
		(amount * graph_y_span) / max_bandwidth;

// Really I ought to save all data points, take max of everything, then plot.

	if (graph_last_x != -1 && graph_last_y != -1) {
		BMP_line (graph, graph_last_x, graph_last_y, x, y, graph_fg);
	}

	graph_last_x = x;
	graph_last_y = y;
}

//----------------------------------------------------------------------------
// Name:	graph_make
// Purpose:	Plots all lines.
//----------------------------------------------------------------------------
void
graph_make ()
{
	int i;

	//----------------------------------------
	// Get the maximum bandwidth in order to
	// properly scale the graph vertically.
	//
	max_bandwidth = 0;
	for (i = 0; i < graph_data_index; i += 2) {
		if (graph_data[i] == DATUM_AMOUNT) {
			int amt = graph_data[i+1];
			if (amt > max_bandwidth)
				max_bandwidth = amt;
		}
	}
	max_bandwidth /= 10000;
	max_bandwidth *= 10000;
	max_bandwidth += 10000;

	graph_draw_labels ();

	//----------------------------------------
	// OK, now draw the lines.
	//
	int size = -1, amt = -1;
	for (i = 0; i < graph_data_index; i += 2)
	{
		int type = graph_data[i];
		long value = graph_data[i+1];

		switch (type) {
		case DATUM_AMOUNT:	amt = value; break;
		case DATUM_SIZE:	size = value; break;
		case DATUM_COLOR:
			graph_fg = (unsigned long) value;
			graph_last_x = -1;
			graph_last_y = -1;
			break;
		}

		if (amt != -1 && size != -1) {
			graph_plot (size, amt);
			amt = size = -1;
		}
	}
}

//============================================================================
// Output buffer logic.
//============================================================================

#define MSGLEN 10000
static wchar_t msg [MSGLEN];

void print (wchar_t *s)
{
	wcscat (msg, s);
}

void newline ()
{
	wcscat (msg, L"\n");
}

void println (wchar_t *s)
{
	wcscat (msg, s);
	newline ();
}

void print_int (int d)
{
#if defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
	swprintf (msg + wcslen (msg), L"%d", d);
#else
	swprintf (msg + wcslen (msg), MSGLEN, L"%d", d);
#endif
}

void println_int (int d)
{
	print_int (d);
	newline ();
}

void print_result (long double result)
{
#if defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
	swprintf (msg + wcslen (msg), L"%.1Lf MB/s", result);
#else
	swprintf (msg + wcslen (msg), MSGLEN, L"%.1Lf MB/s", result);
#endif
}

void dump (FILE *f)
{
	if (!f)
		f = stdout;

	int i = 0;
	while (msg[i]) {
		char ch = (char) msg[i];
		fputc (ch, f);
		i++;
	}

	msg [0] = 0;
}

void flush ()
{
#if defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
	MessageBeep (MB_OK);
#else
	dump (NULL);
	fflush (stdout);
#endif
}

void print_size (unsigned long size)
{
	if (size < 1024) {
		print_int (size);
		print (L" B");
	}
	else if (size < (1<<20)) {
		print_int (size >> 10);
		print (L" kB");
	} else {
		print_int (size >> 20);
		switch ((size >> 18) & 3) {
		case 1: print (L".25"); break;
		case 2: print (L".5"); break;
		case 3: print (L".75"); break;
		}
		print (L" MB");
	}
}

//============================================================================
// Timing logic.
//============================================================================

//----------------------------------------------------------------------------
// Name:	mytime
// Purpose:	Reports time in microseconds.
//----------------------------------------------------------------------------
unsigned long mytime ()
{
#ifndef __WIN32__
	struct timeval tv;
	struct timezone tz;
	memset (&tz, 0, sizeof(struct timezone));
	gettimeofday (&tv, &tz);
	return 1000000 * tv.tv_sec + tv.tv_usec;
#else
	return 1000 * GetTickCount ();	// accurate enough.
#endif
}

//----------------------------------------------------------------------------
// Name:	calculate_result
// Purpose:	Calculates and prints a result.
// Returns:	10 times the number of megabytes per second.
//----------------------------------------------------------------------------
int
calculate_result (unsigned long chunk_size, long long total_count, long diff)
{
	if (!diff)
		error ("Zero time difference.");

// printf ("\nIn calculate_result, chunk_size=%ld, total_count=%lld, diff=%ld\n", chunk_size, total_count, diff);
	long double result = (long double) chunk_size;
	result *= (long double) total_count;
	result *= 1000000.;
	result /= 1048576.;
	result /= (long double) diff;

	print_result (result);

	return (long) (10.0 * result);
}

//============================================================================
// Tests.
//============================================================================

//----------------------------------------------------------------------------
// Name:	do_write
// Purpose:	Performs write on chunk of memory of specified size.
//----------------------------------------------------------------------------
enum {
	NO_SSE2,
	SSE2,
	SSE2_BYPASS,
};

static void *do_thread_write(void *arg)
{
	struct thread_params *params = (struct thread_params *)arg;
	unsigned long total_count = 0;
#if defined(__x86_64__) || defined(__aarch64__)
	unsigned long value = 0x1234567689abcdef;
#else
	unsigned long value = 0x12345678;
#endif
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(params->id % cpu_num, &mask);

	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
		fprintf(stderr, "set thread %d affinity failed\n", params->id);

	while(goon_flag) {
		total_count += params->loops;

		if (params->random)
			RandomWriter (params->chunk_ptrs, params->size/256, params->loops, value);
		else
			Writer (params->chunk, params->size, params->loops, value);
	}

	params->loops = total_count;

	 pthread_exit(NULL);

         return NULL;
}


int
do_write (unsigned long size, int mode, bool random)
{
	unsigned char *chunk;
	unsigned char *chunk0;
	unsigned long loops;
	unsigned long long total_count=0;
	unsigned long diff=0, t0;
	unsigned long tmp;
	unsigned long **chunk_ptrs = NULL;
	struct thread_params *params;
	pthread_t *tid;
	int i, rval;

	if (size & 255)
		error ("do_write(): chunk size is not multiple of 256.");

	params = malloc(sizeof(struct thread_params) * thread_num);
	if (!params)
		error ("Out of memory");

	tid = malloc(sizeof(pthread_t) * thread_num);
	if (!tid)
		error ("Out of memory");

	//-------------------------------------------------
	chunk0 = malloc (size+32);
	chunk = chunk0;
	if (!chunk)
		error ("Out of memory");

	tmp = (unsigned long) chunk;
	if (tmp & 15) {
		tmp -= (tmp & 15);
		tmp += 16;
		chunk = (unsigned char*) tmp;
	}

	//----------------------------------------
	// Set up random pointers to chunks.
	//
	if (random) {
		tmp = size/256;
		chunk_ptrs = (unsigned long**) malloc (sizeof (unsigned long*) * tmp);
		if (!chunk_ptrs)
			error ("Out of memory.");

		//----------------------------------------
		// Store pointers to all chunks into array.
		//
		int i;
		for (i = 0; i < tmp; i++) {
			chunk_ptrs [i] = (unsigned long*) (chunk + 256 * i);
		}

		//----------------------------------------
		// Randomize the array of chunk pointers.
		//
		int k = 100;
		while (k--) {
			for (i = 0; i < tmp; i++) {
				int j = rand() % tmp;
				if (i != j) {
					unsigned long *ptr = chunk_ptrs [i];
					chunk_ptrs [i] = chunk_ptrs [j];
					chunk_ptrs [j] = ptr;
				}
			}
		}
	}

	//-------------------------------------------------
	if (random)
		print (L"Random write ");
	else
		print (L"Sequential write ");

	if (mode == SSE2) {
		print (L"(128-bit), size = ");
	}
	else
	if (mode == SSE2_BYPASS) {
		print (L"bypassing cache (128-bit), size = ");
	} else {
#if defined(__x86_64__) || defined(__aarch64__)
		print (L"(64-bit), size = ");
#else
		print (L"(32-bit), size = ");
#endif
	}

	print_size (size);
	print (L", ");

	loops = (1 << 26) / size;// XX need to adjust for CPU MHz

	tmp = size / thread_num;

	for (i = 0; i < thread_num; i++) {
		params[i].id = i;
		params[i].random = random;
		params[i].size = tmp < 1024 ? size : tmp;
		if (random)
			params[i].chunk_ptrs = tmp < 1024 ? chunk_ptrs : chunk_ptrs + i * (tmp / 256);
		else
			params[i].chunk = tmp < 1024 ? chunk : chunk + i * tmp;
		params[i].loops = loops;
	}

	t0 = mytime ();

	goon_flag = 1;

	for (i = 0; i < thread_num; i++) {
		rval = pthread_create(&tid[i] ,NULL, do_thread_write, &params[i]);
		if (rval < 0) {
			perror("can't create pthread\n");
			return rval;
		}
	}

	usleep(usec_per_test);

	goon_flag = 0;

	for (i = 0; i < thread_num; i++) {
		pthread_join(tid[i], NULL);
		total_count += params[i].loops;
	}

	diff = mytime () - t0;

	total_count /= thread_num;

	print (L"loops = ");
	print_int (total_count);
	print (L", ");

	flush ();

	int result = calculate_result (size, total_count, diff);
	newline ();

	flush ();

	free ((void*)chunk0);

	if (chunk_ptrs)
		free (chunk_ptrs);

	return result;
}

static void *do_thread_read(void *arg)
{
	struct thread_params *params = (struct thread_params *)arg;
	unsigned long total_count = 0;
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(params->id % cpu_num, &mask);

	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
		fprintf(stderr, "set thread %d affinity failed\n", params->id);

	while(goon_flag) {
		total_count += params->loops;

		if (params->random)
			RandomReader (params->chunk_ptrs, params->size/256, params->loops);
		else
			Reader (params->chunk, params->size, params->loops);
	}

	params->loops = total_count;

	 pthread_exit(NULL);

         return NULL;
}

//----------------------------------------------------------------------------
// Name:	do_read
// Purpose:	Performs sequential read on chunk of memory of specified size.
//----------------------------------------------------------------------------
int
do_read (unsigned long size, bool use_sse2, bool random)
{
	unsigned long diff=0;
	unsigned long long total_count = 0;
	unsigned char *chunk;
	unsigned char *chunk0;
	unsigned long tmp;
	unsigned long **chunk_ptrs = NULL;
	unsigned long t0, loops = (1 << 26) / size;	// XX need to adjust for CPU MHz
	struct thread_params *params;
	pthread_t *tid;
	int i, rval;

	if (size & 255)
		error ("do_read(): chunk size is not multiple of 256.");

	params = malloc(sizeof(struct thread_params) * thread_num);
	if (!params)
		error ("Out of memory");

	tid = malloc(sizeof(pthread_t) * thread_num);
	if (!tid)
		error ("Out of memory");

	//-------------------------------------------------
	if (random)
		print (L"Random read ");
	else
		print (L"Sequential read ");

	if (use_sse2) {
		print (L"(128-bit), size = ");
	} else {
#if defined(__x86_64__) || defined(__aarch64__)
		print (L"(64-bit), size = ");
#else
		print (L"(32-bit), size = ");
#endif
	}

	print_size (size);
	print (L", ");

	flush ();

	//-------------------------------------------------
	chunk0 = chunk = malloc (size+32);
	if (!chunk)
		error ("Out of memory");

	memset (chunk, 0, size);

	tmp = (unsigned long) chunk;
	if (tmp & 15) {
		tmp -= (tmp & 15);
		tmp += 16;
		chunk = (unsigned char*) tmp;
	}

	//----------------------------------------
	// Set up random pointers to chunks.
	//
	if (random) {
		int tmp = size/256;
		chunk_ptrs = (unsigned long**) malloc (sizeof (unsigned long*) * tmp);
		if (!chunk_ptrs)
			error ("Out of memory.");

		//----------------------------------------
		// Store pointers to all chunks into array.
		//
		int i;
		for (i = 0; i < tmp; i++) {
			chunk_ptrs [i] = (unsigned long*) (chunk + 256 * i);
		}

		//----------------------------------------
		// Randomize the array of chunk pointers.
		//
		int k = 100;
		while (k--) {
			for (i = 0; i < tmp; i++) {
				int j = rand() % tmp;
				if (i != j) {
					unsigned long *ptr = chunk_ptrs [i];
					chunk_ptrs [i] = chunk_ptrs [j];
					chunk_ptrs [j] = ptr;
				}
			}
		}
	}

	tmp = size / thread_num;

	for (i = 0; i < thread_num; i++) {
		params[i].id = i;
		params[i].random = random;
		params[i].size = tmp < 1024 ? size : tmp;
		if (random)
			params[i].chunk_ptrs = tmp < 1024 ? chunk_ptrs : chunk_ptrs + i * (tmp / 256);
		else
			params[i].chunk = tmp < 1024 ? chunk : chunk + i * tmp;
		params[i].loops = loops;
	}

	t0 = mytime ();

	goon_flag = 1;

	for (i = 0; i < thread_num; i++) {
		rval = pthread_create(&tid[i] ,NULL, do_thread_read, &params[i]);
		if (rval < 0) {
			perror("can't create pthread\n");
			return rval;
		}
	}

	usleep(usec_per_test);

	goon_flag = 0;

	for (i = 0; i < thread_num; i++) {
		pthread_join(tid[i], NULL);
		total_count += params[i].loops;
	}

	diff = mytime () - t0;

	total_count /= thread_num;

	print (L"loops = ");
	print_int (total_count);
	print (L", ");

	int result = calculate_result (size, total_count, diff);
	newline ();

	flush ();

	free (chunk0);

	if (chunk_ptrs)
		free (chunk_ptrs);

	free(params);
	free(tid);

	return result;
}



//----------------------------------------------------------------------------
// Name:	do_copy
// Purpose:	Performs sequential memory copy.
//----------------------------------------------------------------------------
int
do_copy (unsigned long size, int mode)
{
	unsigned long loops;
	unsigned long long total_count = 0;
	unsigned long t0, diff=0;
	unsigned char *chunk_src;
	unsigned char *chunk_dest;
	unsigned char *chunk_src0;
	unsigned char *chunk_dest0;
	unsigned long tmp;

	if (size & 255)
		error ("do_copy(): chunk size is not multiple of 256.");

	//-------------------------------------------------
	chunk_src0 = chunk_src = malloc (size+32);
	if (!chunk_src)
		error ("Out of memory");
	chunk_dest0 = chunk_dest = malloc (size+32);
	if (!chunk_dest)
		error ("Out of memory");

	memset (chunk_src, 100, size);
	memset (chunk_dest, 200, size);

	tmp = (unsigned long) chunk_src;
	if (tmp & 15) {
		tmp -= (tmp & 15);
		tmp += 16;
		chunk_src = (unsigned char*) tmp;
	}
	tmp = (unsigned long) chunk_dest;
	if (tmp & 15) {
		tmp -= (tmp & 15);
		tmp += 16;
		chunk_dest = (unsigned char*) tmp;
	}

	//-------------------------------------------------
	print (L"Sequential copy ");

	if (mode == SSE2) {
		print (L"(128-bit), size = ");
	}
	else {
#if defined(__x86_64__) || defined(__aarch64__)
		print (L"(64-bit), size = ");
#else
		print (L"(32-bit), size = ");
#endif
	}

	print_size (size);
	print (L", ");

	flush ();

	loops = (1 << 26) / size;	// XX need to adjust for CPU MHz

	t0 = mytime ();

	while (diff < usec_per_test) {
		total_count += loops;

#if !defined(__arm__) && !defined(__aarch64__)
		if (mode == SSE2)
			CopySSE (chunk_dest, chunk_src, size, loops);
#if 0
		else
			Copy (chunk_dest, chunk_src, size, loops);
#endif
#endif

		diff = mytime () - t0;
	}

	print (L"loops = ");
	print_int (total_count);
	print (L", ");

	int result = calculate_result (size, total_count, diff);
	newline ();

	flush ();

	free (chunk_src0);
	free (chunk_dest0);

	return result;
}


//----------------------------------------------------------------------------
// Name:	fb_readwrite
// Purpose:	Performs sequential read & write tests on framebuffer memory.
//----------------------------------------------------------------------------
#if defined(__linux__) && defined(FBIOGET_FSCREENINFO)
void
fb_readwrite (bool use_sse2)
{
	//unsigned long counter, total_count;
  unsigned long total_count;
	unsigned long length;
	unsigned long diff, t0;
	static struct fb_fix_screeninfo fi;
	static struct fb_var_screeninfo vi;
	unsigned long *fb = NULL;
	//unsigned long datum;
	int fd;
	//register unsigned long foo;
#if defined(__x86_64__) || defined(__aarch64__)
	unsigned long value = 0x1234567689abcdef;
#else
	unsigned long value = 0x12345678;
#endif

	//-------------------------------------------------

	fd = open ("/dev/fb0", O_RDWR);
	if (fd < 0)
		fd = open ("/dev/fb/0", O_RDWR);
	if (fd < 0) {
		println (L"Cannot open framebuffer device.");
		return;
	}

	if (ioctl (fd, FBIOGET_FSCREENINFO, &fi)) {
		close (fd);
		println (L"Cannot get framebuffer info");
		return;
	}
	else
	if (ioctl (fd, FBIOGET_VSCREENINFO, &vi)) {
		close (fd);
		println (L"Cannot get framebuffer info");
		return;
	}
	else
	{
		if (fi.visual != FB_VISUAL_TRUECOLOR &&
		    fi.visual != FB_VISUAL_DIRECTCOLOR ) {
			close (fd);
			println (L"Need direct/truecolor framebuffer device.");
			return;
		} else {
			unsigned long fblen;

			print (L"Framebuffer resolution: ");
			print_int (vi.xres);
			print (L"x");
			print_int (vi.yres);
			print (L", ");
			print_int (vi.bits_per_pixel);
			println (L" bpp\n");

			fb = (unsigned long*) fi.smem_start;
			fblen = fi.smem_len;

			fb = mmap (fb, fblen,
				PROT_WRITE | PROT_READ,
				MAP_SHARED, fd, 0);
			if (fb == MAP_FAILED) {
				close (fd);
				println (L"Cannot access framebuffer memory.");
				return;
			}
		}
	}

	//-------------------
	// Use only the upper half of the display.
	//
	length = FB_SIZE;

	//-------------------
	// READ
	//
	print (L"Framebuffer memory sequential read ");
	flush ();

	t0 = mytime ();

	total_count = FBLOOPS_R;

#if !defined(__arm__) && !defined(__aarch64__)
	if (use_sse2)
		ReaderSSE2 (fb, length, FBLOOPS_R);
	else
#endif
		Reader (fb, length, FBLOOPS_R);

	diff = mytime () - t0;

	calculate_result (length, total_count, diff);
	newline ();

	//-------------------
	// WRITE
	//
	print (L"Framebuffer memory sequential write ");
	flush ();

	t0 = mytime ();

	total_count = FBLOOPS_W;

#if !defined(__arm__) && !defined(__aarch64__)
	if (use_sse2)
		WriterSSE2_bypass (fb, length, FBLOOPS_W, value);
	else
#endif
		Writer (fb, length, FBLOOPS_W, value);

	diff = mytime () - t0;

	calculate_result (length, total_count, diff);
	newline ();
}
#endif

//----------------------------------------------------------------------------
// Name:	register_test
// Purpose:	Determines bandwidth of register-to-register transfers.
//----------------------------------------------------------------------------
void
register_test ()
{
	long long total_count = 0;
	unsigned long t0;
	unsigned long diff = 0;

	//--------------------------------------
#if defined(__x86_64__) || defined(__aarch64__)
	print (L"Main register to main register transfers (64-bit) ");
#else
	print (L"Main register to main register transfers (32-bit) ");
#endif
	flush ();
#define REGISTER_COUNT 10000

	t0 = mytime ();
	while (diff < usec_per_test)
	{
		RegisterToRegister (REGISTER_COUNT);
		total_count += REGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

#if !defined(__arm__) && !defined(__aarch64__)
	//--------------------------------------
#ifdef __x86_64__
	print (L"Main register to vector register transfers (64-bit) ");
#else
	print (L"Main register to vector register transfers (32-bit) ");
#endif
	flush ();
#define VREGISTER_COUNT 3333

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		RegisterToVector (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

	//--------------------------------------
#ifdef __x86_64__
	print (L"Vector register to main register transfers (64-bit) ");
#else
	print (L"Vector register to main register transfers (32-bit) ");
#endif
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		VectorToRegister (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

	//--------------------------------------
	print (L"Vector register to vector register transfers (128-bit) ");
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		VectorToVector (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

	//--------------------------------------
	if (use_sse4) {
		print (L"Vector 8-bit datum to main register transfers ");
		flush ();

		t0 = mytime ();
		diff = 0;
		total_count = 0;
		while (diff < usec_per_test)
		{
			Vector8ToRegister (VREGISTER_COUNT);
			total_count += VREGISTER_COUNT;

			diff = mytime () - t0;
		}

		calculate_result (256, total_count, diff);
		newline ();
		flush ();
	}

	//--------------------------------------
	print (L"Vector 16-bit datum to main register transfers ");
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		Vector16ToRegister (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

	//--------------------------------------
	if (use_sse4) {
		print (L"Vector 32-bit datum to main register transfers ");
		flush ();

		t0 = mytime ();
		diff = 0;
		total_count = 0;
		while (diff < usec_per_test)
		{
			Vector32ToRegister (VREGISTER_COUNT);
			total_count += VREGISTER_COUNT;

			diff = mytime () - t0;
		}

		calculate_result (256, total_count, diff);
		newline ();
		flush ();
	}

#ifdef __x86_64__
	//--------------------------------------
	print (L"Vector 64-bit datum to main register transfers ");
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		Vector64ToRegister (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();
#endif

	//--------------------------------------
	if (use_sse4) {
		print (L"Main register 8-bit datum to vector register transfers ");
		flush ();

		t0 = mytime ();
		diff = 0;
		total_count = 0;
		while (diff < usec_per_test)
		{
			Register8ToVector (VREGISTER_COUNT);
			total_count += VREGISTER_COUNT;

			diff = mytime () - t0;
		}

		calculate_result (256, total_count, diff);
		newline ();
		flush ();
	}

	//--------------------------------------
	print (L"Main register 16-bit datum to vector register transfers ");
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		Register16ToVector (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

	//--------------------------------------
	if (use_sse4) {
		print (L"Main register 32-bit datum to vector register transfers ");
		flush ();

		t0 = mytime ();
		diff = 0;
		total_count = 0;
		while (diff < usec_per_test)
		{
			Register32ToVector (VREGISTER_COUNT);
			total_count += VREGISTER_COUNT;

			diff = mytime () - t0;
		}

		calculate_result (256, total_count, diff);
		newline ();
		flush ();
	}

#ifdef __x86_64__
	//--------------------------------------
	print (L"Main register 64-bit datum to vector register transfers ");
	flush ();

	t0 = mytime ();
	diff = 0;
	total_count = 0;
	while (diff < usec_per_test)
	{
		Register64ToVector (VREGISTER_COUNT);
		total_count += VREGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();
#endif
#endif
}

//----------------------------------------------------------------------------
// Name:	stack_test
// Purpose:	Determines bandwidth of stack-to/from-register transfers.
//----------------------------------------------------------------------------
void
stack_test ()
{
	long long total_count = 0;
	unsigned long t0;
	unsigned long diff = 0;

#if defined(__x86_64__) || defined(__aarch64__)
	print (L"Stack-to-register transfers (64-bit) ");
#else
	print (L"Stack-to-register transfers (32-bit) ");
#endif
	flush ();

	//--------------------------------------
	diff = 0;
	total_count = 0;
	t0 = mytime ();
	while (diff < usec_per_test)
	{
		StackReader (REGISTER_COUNT);
		total_count += REGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();

#if defined(__x86_64__) || defined(__aarch64__)
	print (L"Register-to-stack transfers (64-bit) ");
#else
	print (L"Register-to-stack transfers (32-bit) ");
#endif
	flush ();

	//--------------------------------------
	diff = 0;
	total_count = 0;
	t0 = mytime ();
	while (diff < usec_per_test)
	{
		StackWriter (REGISTER_COUNT);
		total_count += REGISTER_COUNT;

		diff = mytime () - t0;
	}

	calculate_result (256, total_count, diff);
	newline ();
	flush ();
}

//----------------------------------------------------------------------------
// Name:	library_test
// Purpose:	Performs C library tests (memset, memcpy).
//----------------------------------------------------------------------------
void
library_test ()
{
	char *a1, *a2;
	unsigned long t, t0;
	int i;


#if defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
	#define NT_SIZE (1024*1024)
	#define NT_SIZE2 (50)
#elif !defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
#if defined(DRAM_SIZE_SMALL)
	#define NT_SIZE (16*1024*1024)
#else
	#define NT_SIZE (32*1024*1024)
#endif
	#define NT_SIZE2 (50)
#else
	#define NT_SIZE (64*1024*1024)
	#define NT_SIZE2 (100)
#endif

	a1 = malloc (NT_SIZE);
	if (!a1)
		error ("Out of memory");

	a2 = malloc (NT_SIZE);
	if (!a2)
		error ("Out of memory");

	//--------------------------------------
	t0 = mytime ();
	for (i=0; i<NT_SIZE2; i++) {
		memset (a1, i, NT_SIZE);
	}
	t = mytime ();

	print (L"Library: memset ");
	calculate_result (NT_SIZE, NT_SIZE2, t-t0);
	newline ();

	flush ();

	//--------------------------------------
	t0 = mytime ();
	for (i=0; i<NT_SIZE2; i++) {
		memcpy (a2, a1, NT_SIZE);
	}
	t = mytime ();

	print (L"Library: memcpy ");
	calculate_result (NT_SIZE, NT_SIZE2, t-t0);
	newline ();

	flush ();

	free (a1);
	free (a2);
}

//----------------------------------------------------------------------------
// Name:	network_test_core
// Purpose:	Performs the network test, talking to and receiving data
//		back from a transponder node.
// Note:	Port number specified using server:# notation.
// Returns:	-1 on error, else the network duration in microseconds.
//----------------------------------------------------------------------------
long
network_test_core (const char *net_path, char *chunk,
			unsigned long chunk_size,
			unsigned long count)
{
	char hostname [PATH_MAX];
	char *s;
	int port = NETWORK_DEFAULT_PORTNUM ;
	strcpy (hostname, net_path);
	if ((s = strchr (hostname, ':'))) {
		*s++ = 0;
		port = atoi (s);
	}

	struct hostent* host = gethostbyname (hostname);
	if (!host)
		return -1;

	char *host_ip = inet_ntoa (*(struct in_addr *)*host->h_addr_list);
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host_ip);
	addr.sin_port = htons(port);

	if (connect (sock, (struct sockaddr*) &addr, sizeof (struct sockaddr)))
	{
		// perror ("connect");
		close (sock);
		return -1;
	}

	//------------------------------------
	// Send all of our data.
	//
	unsigned long t0 = mytime ();
	int i;
	for (i = 0; i < count; i++)
		send (sock, chunk, chunk_size, 0);

#if 0
	//------------------------------------
	// Set nonblocking mode.
	//
	int opt = 1;
	ioctl (sock, FIONBIO, &opt);
#endif

	//------------------------------------
	// Read the response.
	//
	char *buffer = malloc (chunk_size);
	if (!buffer) {
		close (sock);
		// perror ("malloc");
		return -1;
	}
	int amount = recv (sock, buffer, chunk_size, 0);
	if (amount <= 0) {
		close (sock);
		//perror ("recv");
		return -1;
	}

	long t = mytime () - t0;
	close (sock);
	free (buffer);
	return t;
}

//----------------------------------------------------------------------------
// Name:	ip_to_str
//----------------------------------------------------------------------------
void
ip_to_str (unsigned long addr, char *str)
{
	if (!str)
		return;

	unsigned short a = 0xff & addr;
	unsigned short b = 0xff & (addr >> 8);
	unsigned short c = 0xff & (addr >> 16);
	unsigned short d = 0xff & (addr >> 24);
	sprintf (str, "%u.%u.%u.%u", a,b,c,d);
}

//----------------------------------------------------------------------------
// Name:	network_transponder
// Purpose:	Act as a transponder, receiving chunks of data and sending
//		back an acknowledgement once the enture chunk is read.
// Returns:	False if a problem occurs setting up the network socket.
//----------------------------------------------------------------------------
bool
network_transponder ()
{
	struct sockaddr_in sin, from;

	//------------------------------
	// Get listening socket for port.
	// Then listen on given port#.
	//
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(NETWORK_DEFAULT_PORTNUM);
	int listensock;
	if ((listensock = socket (AF_INET, SOCK_STREAM, 0)) < 0)  {
		perror ("socket");
		return false;
	}
	if (bind (listensock, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		perror ("bind");
		close (listensock);
		return false;
	}
	if (listen (listensock, 500) < 0) {
		perror ("listen");
		close (listensock);
		return false;
	}

	bool done = false;
	while (!done) {
		//----------------------------------------
		// Wait for a client to contact us.
		//
		socklen_t len = sizeof (struct sockaddr);
		int sock = accept (listensock, (struct sockaddr*) &from, &len);
		if (sock < 0) {
			perror ("accept");
			close (listensock);
			return false;
		}

		if (len != sizeof (struct sockaddr_in)) {
			close (sock);
			close (listensock);
			return false;
		}

#if 0
		unsigned long ipaddr = from.sin_addr.s_addr;
		char ipstring[30];
		ip_to_str (ipaddr, ipstring);
		fprintf (stderr, "Incoming connection from %s\n", ipstring);
#endif

		char chunk [NETWORK_CHUNK_SIZE+1];
		long n_chunks = 0;
		int amount_read = read (sock, chunk, NETWORK_CHUNK_SIZE);
		chunk [amount_read] = 0;
		if (1 != sscanf (chunk, "%ld", &n_chunks)) {
			close (sock);
			close (listensock);
			return false;
		}

		//----------------------------------------
		// If the leader sends us a chunk count of
		// -99, this indicates that we should exit.
		//
		if (n_chunks == -99) {
			close (sock);
			close (listensock);
			return true;
		}

//		printf ("Reading %lu chunks of %d bytes...\n", n_chunks, NETWORK_CHUNK_SIZE);

		unsigned long long remaining = n_chunks;
		remaining *= NETWORK_CHUNK_SIZE;

//		printf ("remaining="); dump_hex64(remaining); puts("");

		remaining -= amount_read;
		while (remaining > 0) {
			amount_read = read (sock, chunk, NETWORK_CHUNK_SIZE);
			remaining -= amount_read;

			if (amount_read < 0) {
				perror ("read");
				break;
			} else
			if (!amount_read)
				break;
		}

		char *foo = "OK.\n\n";
		write (sock, foo, 4);
		close (sock);
	}

	return true;
}

//----------------------------------------------------------------------------
// Name:	network_test
//----------------------------------------------------------------------------
bool
network_test (char **destinations, int n_destinations)
{
	int i;

	//----------------------------------------
	// The memory chunk starts with a 12-byte
	// length of the overall send size.
	// The memory chunk will have a list of
	// the destinations in it.
	// In future, there will be a mechanism
	// for testing bandwidth between all nodes,
	// not just the leader & each of the
	// transponders.
	//
	char chunk [NETWORK_CHUNK_SIZE];
	memset (chunk, 0, NETWORK_CHUNK_SIZE);
	sprintf (chunk, "000000000000\n%d\n", n_destinations);
	for (i = 0; i < n_destinations; i++) {
		char *s = destinations [i];
		int chunk_len = strlen (chunk);
		int len = strlen (s);
		if (len + chunk_len < NETWORK_CHUNK_SIZE-1) {
			//----------------------------------------
			// "transp" indicates that the given node
			// has not yet been a leader.
			// In future, "done" will indicate it has.
			//
			sprintf (chunk + chunk_len, "%s %s\n", s, "transp");
		}
	}

	//----------------------------------------
	// For each destination, run the test.
	//
	for (i = 0; i < n_destinations; i++) {
		int j = 0;
		bool problem = false;

		char *hostname = destinations[i];
		printf ("Bandwidth sending to %s:\n", hostname);

		//----------------------------------------
		// Send from 8kB up to 32 MB of data.
		//
		while (!problem && j < 13) {
			unsigned long chunk_count = 1 << j;
			unsigned long long amt_to_send = chunk_count;
			amt_to_send *= NETWORK_CHUNK_SIZE;

			if (!amt_to_send) // unlikely
				break;

			//----------------------------------------
			// Write the overall send size into the
			// 1st line of the chunk so that the
			// transponder knows how large the send
			// is without guessing.
			//
			sprintf (chunk, "%11lu", chunk_count);
			chunk[11] = ' ';

			//--------------------
			// Send the data.
			//
			long duration = network_test_core (hostname,
				chunk, NETWORK_CHUNK_SIZE, chunk_count);
			if (duration == -1) {
				problem = true;
				fprintf (stderr, "\nCan't connect to %s\n", hostname);
			} else {
				unsigned long amt_in_kb = amt_to_send / 1024;
				unsigned long amt_in_mb = amt_to_send / 1048576;
				if (!amt_in_mb) {
					printf ("\tSent %lu kB...", amt_in_kb);
				} else {
					printf ("\tSent %lu MB...", amt_in_mb);
				}

				//------------------------------
				// Calculate rate in MB/sec.
				//
				// Get total # bytes.
				unsigned long long tmp = NETWORK_CHUNK_SIZE;
				tmp *= chunk_count;

				// Get total bytes per second.
				tmp *= 1000000;
				tmp /= duration;

				// Bytes to megabytes.
				tmp /= 1000;
				tmp /= 10;
				unsigned long whole = tmp / 100;
				unsigned long frac = tmp % 100;
				printf ("%lu.%02lu MB/second\n", whole, frac);
			}
			j++;
		}

		puts ("");
	}

	return true;
}

//----------------------------------------------------------------------------
// Name:	usage
//----------------------------------------------------------------------------
void
usage ()
{
	printf ("Usage for memory tests: bandwidth [--quick] [--thread N] [--chunk-size N]\n");
	printf ("Usage for starting network tests: bandwidth --network <ipaddr1> [<ipaddr2...]\n");
	printf ("Usage for receiving network tests: bandwidth --transponder\n");
	exit (0);
}

//----------------------------------------------------------------------------
// Name:	main
//----------------------------------------------------------------------------
int
main (int argc, char **argv)
{
	int i, j, chunk_size;

	--argc;
	++argv;

	strcpy (graph_title, TITLE);

	bool network_mode = false;
	bool network_leader = false; // false => transponder
	int network_destinations_size = 0;
	int n_network_destinations = 0;
	char **network_destinations = NULL;

	i = 0;
	while (i < argc) {
		char *s = argv [i++];
		if (!strcmp ("--network", s)) {
			network_mode = true;
			network_leader = true;
			network_destinations_size = 20;
			network_destinations = (char**) malloc (network_destinations_size * sizeof (char*));
		}
		else
		if (!strcmp ("--transponder", s)) {
			network_mode = true;
		}
		else
		if (!strcmp ("--slow", s)) {
			usec_per_test=20000000;	// 20 seconds per test.
		}
		else
		if (!strcmp ("--quick", s)) {
			usec_per_test = 250000;	// 0.25 seconds per test.
		}
		else
		if (!strcmp ("--nosse2", s)) {
			use_sse2 = false;
			use_sse4 = false;
		}
		else
		if (!strcmp ("--nosse4", s)) {
			use_sse4 = false;
		}
		else
		if (!strcmp ("--help", s)) {
			usage ();
		}
		else
		if (!strcmp ("--title", s) && i != argc) {
			sprintf (graph_title, "%s -- %s", TITLE, argv[i++]);
		}
		else
		if (!strcmp ("--thread", s)) {
			int n = 0;
			thread_num = atoi(argv[i++]);
			for (j = 0; j < 32; j++)
				n += (thread_num >> j) & 0x1;
			if (n > 1)
				error("thread_num must be power of 2\n");
		}
		else
		if (!strcmp ("--chunk-size", s)) {
			chunk_size = strtoul(argv[i++], NULL, 0);
			for (j = 0; j < sizeof(chunk_sizes) / sizeof(chunk_sizes[0]); j++) {
				if (chunk_size <= chunk_sizes[j])
					break;
			}
			chunk_index = j;
		}
		else {
			if (!network_mode || !network_leader)
				usage ();

			if ('-' == *s)
				usage ();

			if (n_network_destinations >= network_destinations_size) {
				network_destinations_size *= 2;
				int newsize = sizeof(char*) * network_destinations_size;
				network_destinations = realloc (network_destinations,
					newsize);
			}

			network_destinations [n_network_destinations++] = strdup (s);
		}
	}

	msg[0] = 0;

#if !(defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__)))
	printf ("This is bandwidth version %s.\n", VERSION);
	printf ("Copyright (C) 2005-2010 by Zack T Smith.\n\n");
	printf ("This software is covered by the GNU Public License.\n");
	printf ("It is provided AS-IS, use at your own risk.\n");
	printf ("See the file COPYING for more information.\n\n");
	fflush (stdout);
#else
	println (L"(C) 2010 by Zack Smith");
	println (L"Under GNU Public License");
	println (L"Use at your own risk.");
#endif

	//----------------------------------------
	// If network mode selected, enter it now.
	// Currently cannot combine memory tests
	// & network tests.
	//
	if (network_mode) {
		if (network_leader) {
			network_test (network_destinations, n_network_destinations);
		} else {
			network_transponder ();
		}

		puts ("Done.");
		return 0;
	}

#if !defined(__arm__) && !defined(__aarch64__)
	if (!has_sse2 ()) {
		puts ("Processor does not have SSE2.");
		use_sse2 = false;
		use_sse4 = false;
	}

#ifdef __x86_64__
	if (use_sse2)
		println (L"Using 128-bit and 64-bit data transfers.");
	else
		println (L"Using 64-bit data transfers.");
#else
	if (use_sse2)
		println (L"Using 128-bit and 32-bit data transfers.");
	else
		println (L"Using 32-bit data transfers.");
#endif

#else

#if defined(__aarch64__)
	println (L"Using 64-bit transfers.");
#else
	println (L"Using 32-bit transfers.");
#endif

	use_sse2 = false;
#endif

	println (L"Notation: kB = 1024 B, MB = 1048576 B.");

	flush ();

	//------------------------------------------------------------
	// Attempt to obtain information about the CPU.
	//
#ifdef __linux__
	struct stat st;
	if (!stat ("/proc/cpuinfo", &st)) {
#define TMPFILE "/tmp/bandw_tmp"
		unlink (TMPFILE);
		if (-1 == system ("grep MHz /proc/cpuinfo | uniq | sed \"s/[\\t\\n: a-zA-Z]//g\" > "TMPFILE))
			perror ("system");

		FILE *f = fopen (TMPFILE, "r");
		if (f) {
			float cpu_speed = 0.0;

			if (1 == fscanf (f, "%g", &cpu_speed)) {
				puts ("");
				printf ("CPU speed is %g MHz.\n", cpu_speed);
			}
			fclose (f);
		}

#if !defined(__arm__) && !defined(__aarch64__)
		unlink (TMPFILE);
		if (-1 == system ("grep -i sse4 /proc/cpuinfo > "TMPFILE))
			perror ("system");

		if (!stat (TMPFILE, &st)) {
			if (st.st_size < 2) {
				use_sse4 = false;
				puts ("Processor lacks SSE4.");
			}
		}

		if (!use_sse2) {
			unlink (TMPFILE);
			if (-1 == system ("grep -i sse2 /proc/cpuinfo > "TMPFILE))
				perror ("system");

			if (!stat (TMPFILE, &st)) {
				if (st.st_size < 2) {
					use_sse2 = false;
					puts ("Processor lacks SSE2.");
				}
			}
		}
#endif
	} else {
		printf ("CPU information is not available (/proc/cpuinfo).\n");
	}

	cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	printf("System has %d processor(s)\n", cpu_num);

	fflush (stdout);
#endif

	graph = BMP_new (graph_width, graph_height);
	graph_init ();

#if !defined(__arm__) && !defined(__aarch64__)
	//------------------------------------------------------------
	// SSE2 sequential reads.
	//
	if (use_sse2) {
		graph_new_line ("Sequential 128-bit reads", RGB_RED);

		newline ();

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_read (chunk_size, true, false);

			graph_add_point (chunk_size, amount);
		}
	}

	//------------------------------------------------------------
	// SSE2 random reads.
	//
	if (use_sse2) {
		graph_new_line ("Random 128-bit reads", RGB_MAROON);

		newline ();
		srand (time (NULL));

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_read (chunk_size, true, true);

			graph_add_point (chunk_size, amount);
		}
	}

	//------------------------------------------------------------
	// SSE2 sequential writes that do not bypass the caches.
	//
	if (use_sse2) {
		graph_new_line ("Sequential 128-bit cache writes", RGB_PURPLE);

		newline ();

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_write (chunk_size, SSE2, false);

			graph_add_point (chunk_size, amount);
		}
	}

	//------------------------------------------------------------
	// SSE2 random writes that do not bypass the caches.
	//
	if (use_sse2) {
		graph_new_line ("Random 128-bit cache writes", RGB_NAVYBLUE);

		newline ();
		srand (time (NULL));

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_write (chunk_size, SSE2, true);

			graph_add_point (chunk_size, amount);
		}
	}

	//------------------------------------------------------------
	// SSE2 sequential writes that do bypass the caches.
	//
	if (use_sse2) {
		graph_new_line ("Sequential 128-bit bypassing writes", RGB_DARKORANGE);

		newline ();

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_write (chunk_size, SSE2_BYPASS, false);

			graph_add_point (chunk_size, amount);
		}
	}

	//------------------------------------------------------------
	// SSE2 random writes that bypass the caches.
	//
	if (use_sse2) {
		graph_new_line ("Random 128-bit bypassing writes", RGB_LEMONYELLOW);

		newline ();
		srand (time (NULL));

		i = chunk_index;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_write (chunk_size, SSE2_BYPASS, true);

			graph_add_point (chunk_size, amount);
		}
	}
#endif

	//------------------------------------------------------------
	// Sequential non-SSE2 reads.
	//
	newline ();
#if defined(__x86_64__) || defined(__aarch64__)
	graph_new_line ("Sequential 64-bit reads", RGB_BLUE);
#else
	graph_new_line ("Sequential 32-bit reads", RGB_BLUE);
#endif

	i = chunk_index;
	while ((chunk_size = chunk_sizes [i++])) {
		int amount = do_read (chunk_size, false, false);

		graph_add_point (chunk_size, amount);
	}

	//------------------------------------------------------------
	// Random non-SSE2 reads.
	//
	newline ();
#if defined(__x86_64__) || defined(__aarch64__)
	graph_new_line ("Random 64-bit reads", RGB_CYAN);
#else
	graph_new_line ("Random 32-bit reads", RGB_CYAN);
#endif
	srand (time (NULL));

	i = chunk_index;
	while ((chunk_size = chunk_sizes [i++])) {
		int amount = do_read (chunk_size, false, true);

		graph_add_point (chunk_size, amount);
	}

	//------------------------------------------------------------
	// Sequential non-SSE2 writes.
	//
#if defined(__x86_64__) || defined(__aarch64__)
	graph_new_line ("Sequential 64-bit writes", RGB_DARKGREEN);
#else
	graph_new_line ("Sequential 32-bit writes", RGB_DARKGREEN);
#endif

	newline ();

	i = chunk_index;
	while ((chunk_size = chunk_sizes [i++])) {
		int amount = do_write (chunk_size, NO_SSE2, false);

		graph_add_point (chunk_size, amount);
	}

	//------------------------------------------------------------
	// Random non-SSE2 writes.
	//
#if defined(__x86_64__) || defined(__aarch64__)
	graph_new_line ("Random 64-bit writes", RGB_GREEN);
#else
	graph_new_line ("Random 32-bit writes", RGB_GREEN);
#endif

	newline ();
	srand (time (NULL));

	i = chunk_index;
	while ((chunk_size = chunk_sizes [i++])) {
		int amount = do_write (chunk_size, NO_SSE2, true);

		graph_add_point (chunk_size, amount);
	}

#if !defined(__arm__) && !defined(__aarch64__)
	//------------------------------------------------------------
	// SSE2 sequential copy.
	//
	if (use_sse2) {
		graph_new_line ("Sequential 128-bit copy", 0x8f8844);

		newline ();

		i = 0;
		while ((chunk_size = chunk_sizes [i++])) {
			int amount = do_copy (chunk_size, SSE2);

			graph_add_point (chunk_size, amount);
		}
	}
#endif

	//------------------------------------------------------------
	// Register to register.
	//
	newline ();
	register_test ();

	//------------------------------------------------------------
	// Stack to/from register.
	//
	newline ();
	stack_test ();

	//------------------------------------------------------------
	// C library performance.
	//
	newline ();
	library_test ();

	//------------------------------------------------------------
	// Framebuffer read & write.
	//
#if defined(__linux__) && defined(FBIOGET_FSCREENINFO)
	newline ();
	fb_readwrite (true);
#endif

#if defined(__WIN32__) && (defined(__arm__) || defined(__aarch64__))
	MessageBoxW (0, msg, APPNAME, 0);

	FILE *of = fopen ("bandwidth.log", "w");
	if (of) {
		dump (of);
		fclose (of);
	}
#else
	flush ();
#endif

	graph_make ();

	BMP_write (graph, "bandwidth.bmp");
	BMP_delete (graph);
#if defined(__linux__) || defined(__CYGWIN__) || defined(__APPLE__)
	puts ("\nWrote graph to bandwidth.bmp.");
	puts ("");
	puts ("Done.");
#endif

	return 0;
}
