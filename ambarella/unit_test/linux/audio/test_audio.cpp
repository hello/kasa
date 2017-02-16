/*
 * test_audio.c
 *
 * History:
 *	2008/06/27 - [Cao Rongrong] created file
 *	2008/11/14 - [Cao Rongrong] support PAUSE and RESUME
 *	2009/02/24 - [Cao Rongrong] add duplex, playback support more format
 *	2009/03/02 - [Cao Rongrong] add xrun
 *
 *	Copyright (C) 2007-2008, Ambarella, Inc.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program will record the sound data to a file. It will run almost infinite,
 *  you can type "CTRL + C" to terminate it.
 *
 *  When the file is recorded, you can play it by "aplay". But you should pay
 *  attention to the option of "aplay", if the option is incorrect, you may hear
 *  noise rather than the sound you record just now.
 */


#include <alsa/asoundlib.h>
#include <assert.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <basetypes.h>
#include <string.h>
#include <getopt.h>

#include "formats.h"
#include "audio_encode.h"

/* The file record/provide the sound data */
#define FILE_NAME	"/mnt/test_audio.dat"

/* The sound format you want to record */
#define DEFAULT_FORMAT		SND_PCM_FORMAT_A_LAW//SND_PCM_FORMAT_S16_LE //  SND_PCM_FORMAT_MU_LAW//
#define DEFAULT_RATE		8000
#define DEFAULT_CHANNELS	1

#ifndef LLONG_MAX
#define LLONG_MAX	9223372036854775807LL
#endif

/* global data */
static int pausing = 0;
static int resuming = 0;

char pcm_name[256];
static snd_pcm_t *handle = NULL;

static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams;

typedef struct
{
		unsigned int	fmtMagic;
		unsigned int	fmtSize;
		WaveFmtBody		fmtBody;
}WaveFmt;

typedef struct
{
	WaveHeader 		wavHeader;
	WaveFmt			waveFmt;
	WaveChunkHeader	ChunkHeader;
} WaveContainer;

typedef struct
{
	WaveHeader 		wavHeader;
	WaveFmt			waveFmt;
	unsigned short	extra_bytes;
	WaveChunkHeader	ChunkHeader;
} WaveContainer2;

typedef struct
{
	char* 	samples;
	int		ratio;
	int		offset;
	int 	size;
} AudioConverter;

/*
 * When DEFAULT_CHANNELS is 1 ("mono"), channel_id is used to
 * choose which channel to record (Channel 0 or Channel 1 ?)
 */
static int channel_id = -1;

static size_t bits_per_sample, bits_per_frame;
static int duplex = 0;

static int fd = -1;
char file_name[256];

static int test_debug = 0;

static int transform = 0;

static AudioConverter converter[3];

static const char *short_options = "hPCDd:c:f:r:s:T";
static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"playback", 0, 0, 'P'},
	{"capture", 0, 0, 'C'},
	{"duplex", 0, 0, 'D'},
	{"transform", 1, 0, 'T'},
	{"device", 1, 0, 'd'},
	{"channels", 1, 0, 'c'},
	{"format", 1, 0, 'f'},
	{"rate", 1, 0, 'r'},
	{"selch", 1, 0, 's'},
	{"test", 0, 0, 't'},
	{0, 0, 0, 0}
};

static void usage(void)
{
	printf("\nusage: test_audio [OPTION]...[FILE]...\n");
	printf("\t-h, --help		help\n"
		 "\t-P, --playback		playback mode\n"
		 "\t-C, --capture		capture mode\n"
		 "\t-D, --duplex		duplex mode\n"
		 "\t-T, --transform		transform mode\n"
		 "\t-d, --device		pcm device\n"
		 "\t-f, --format=FORMAT	sample format (case insensitive)\n"
		 "\t-c, --channels=#	channels\n"
		 "\t-r, --rate=#		sample rate\n"
		 "\t-s, --selch=#		select channel\n"
		 "\t-t, --test		frequently stop playback test\n");
	printf("Example:\n");
	printf(" --- Default: playback /mnt/test_audio.dat in A_LAW/Mono/8KHz format\n"
		"\ttest_audio \n"
		" --- Playback: S16_LE/stereo/48KHz\n"
		"\ttest_audio -P -fdat file_name.dat\n"
		" --- Capture: MU_LAW/Mono/8KHz\n"
		"\ttest_audio -C -fmu_law -c1 -s0 -r8000 file_name.dat\n"
		" --- Playback file_name.dat, Capture to file_name_cp.dat\n"
		"\ttest_audio -D -fdat file_name.dat\n"
		" --- Transform: U8/Mono/8KHz\n"
		"\ttest_audio -T -d input.wav -c1 -r8000 -fU8 -s0 ouput.wav\n");
	printf("\n");

	printf("The program support PAUSE and RESUME when it is in playback mode.\n"
		"You can press 'CTRL + Z' to PAUSE, and press 'CTRL + Z' again to RESUME.\n");
	printf("\n");

	printf("\nNOTE:   you should pay attention to the option of \"aplay\","
		"if the option\n\tis incorrect, you may hear noise rather than "
		"the sound you\n\trecord just now.\n");
	printf("Following are some examples of options of \"aplay\"\n");
	printf("\tSND_PCM_FORMAT_S16_LE,48K,Stereo:aplay -fdat /mnt/file.dat\n");
	printf("\tSND_PCM_FORMAT_MU_LAW,8K, Stereo:aplay -f MU_LAW -r 8000 -c 2 /mnt/file.dat\n");
	printf("\tSND_PCM_FORMAT_A_LAW, 8K, Mono:  aplay -f A_LAW  -r 8000 -c 1 /mnt/file.dat\n");
	printf("\n");
}

static snd_pcm_uframes_t set_params(snd_pcm_t *handle, snd_pcm_stream_t stream)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t chunk_size = 0;
	snd_pcm_uframes_t start_threshold;
	unsigned period_time = 0;
	unsigned buffer_time = 0;
	size_t chunk_bytes = 0;
	int err;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);

	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("Broken configuration for this PCM: no configurations available");
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("Access type not available");
		exit(EXIT_FAILURE);
	}

	printf("%s: %s\n", stream == SND_PCM_STREAM_PLAYBACK ? "Playback" : "Capture", file_name);
	printf("format = %s, channels = %d, rate = %d\n",
		snd_pcm_format_name(hwparams.format),
		hwparams.channels, hwparams.rate);

	//err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (hwparams.format == SND_PCM_FORMAT_U8) {
		err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
	} else {
		err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	}
	if (err < 0) {
		printf("Sample format non available");
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	//err = snd_pcm_hw_params_set_channels(handle, params, 2);
	if (err < 0) {
		printf("Channels count non available");
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_hw_params_set_rate(handle, params, hwparams.rate, 0);
	if (err < 0) {
		printf("Rate non available");
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_hw_params_get_buffer_time_max(params,
		&buffer_time, 0);
	assert(err >= 0);
	if (buffer_time > 500000)
		buffer_time = 500000;

	period_time = buffer_time / 4;

	err = snd_pcm_hw_params_set_period_time_near(handle, params,
		&period_time, 0);
	assert(err >= 0);

	err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
		&buffer_time, 0);

	assert(err >= 0);

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to install hw params:");
		exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		printf("Can't use period equal to buffer size (%lu == %lu)",
			chunk_size, buffer_size);
		exit(EXIT_FAILURE);
	}

	snd_pcm_sw_params_current(handle, swparams);

	err = snd_pcm_sw_params_set_avail_min(handle, swparams, chunk_size);

	if(stream == SND_PCM_STREAM_PLAYBACK)
		start_threshold = (buffer_size / chunk_size) * chunk_size;
	else
		start_threshold = 1;
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
	assert(err >= 0);

	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, buffer_size);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		printf("unable to install sw params:");
		exit(EXIT_FAILURE);
	}
	if (hwparams.format == SND_PCM_FORMAT_U8) {
		bits_per_sample = snd_pcm_format_physical_width(SND_PCM_FORMAT_U8);
	} else {
		bits_per_sample = snd_pcm_format_physical_width(SND_PCM_FORMAT_S16_LE);
	}

	bits_per_frame = bits_per_sample * hwparams.channels;

	//bits_per_frame = bits_per_sample * 2;
	chunk_bytes = chunk_size * bits_per_frame / 8;

	printf("chunk_size = %d,chunk_bytes = %d,buffer_size = %d\n\n",
		(int)chunk_size,chunk_bytes,(int)buffer_size);

	return chunk_size;
}

/* I/O error handler */
static void xrun(snd_pcm_stream_t stream)
{
	snd_pcm_status_t *status;
	int res;

	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		printf("status error: %s\n", snd_strerror(res));
		exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		struct timeval now, diff, tstamp;
		gettimeofday(&now, 0);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
		fprintf(stderr, "%s!!! (at least %.3f ms long)\n",
			stream == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",
			diff.tv_sec * 1000 + diff.tv_usec / 1000.0);

		if ((res = snd_pcm_prepare(handle))<0) {
			printf("xrun: prepare error: %s\n", snd_strerror(res));
			exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
		printf("draining!!!\n");
		if (stream == SND_PCM_STREAM_CAPTURE) {
			fprintf(stderr, "capture stream format change? attempting recover...\n");
			if ((res = snd_pcm_prepare(handle))<0) {
				printf("xrun(DRAINING): prepare error: %s", snd_strerror(res));
				exit(EXIT_FAILURE);
			}
			return;
		}
	}

	exit(EXIT_FAILURE);
}

static snd_pcm_uframes_t handle_data_pb(ssize_t count, u_char *data, u_char *tmp)
{
	ssize_t tmp_cnt, i;
	size_t bfcount;

	if ((hwparams.channels == 2) && (channel_id != -1)) {
		if (hwparams.format == SND_PCM_FORMAT_S16_LE) {
			for (i = 0;i < count; i++)
				if(channel_id)
					*((u_short *)tmp + i) = *((u_short *)data + i / 2 + 1);
				else
					*((u_short *)tmp + i) = *((u_short *)data + i / 2);
		} else {
			for (i = 0; i < count << 1; i++)
				if(channel_id)
					*(tmp + i) = *(data + i / 2 + 1);
				else
					*(tmp + i) = *(data + i / 2);
		}
		tmp_cnt = count << 1;
	} else if (hwparams.channels == 2) {
		if ((hwparams.format == SND_PCM_FORMAT_A_LAW) ||
			(hwparams.format == SND_PCM_FORMAT_MU_LAW)) {
			printf("MU_LAW and A_LAW only support mono\n");
			exit(EXIT_FAILURE);
		}
		tmp_cnt = count;
		memcpy(tmp, data, tmp_cnt);
	} else {
		tmp_cnt = count;
		memcpy(tmp, data, tmp_cnt);
	}

	switch (hwparams.format) {
	case SND_PCM_FORMAT_A_LAW:
		bfcount = G711::ALawDecode((s16 *)data, tmp, tmp_cnt);
		break;
	case SND_PCM_FORMAT_MU_LAW:
		bfcount = G711::ULawDecode((s16 *)data, tmp, tmp_cnt);
		break;
	case SND_PCM_FORMAT_S16_LE:
	case SND_PCM_FORMAT_U8:
		memcpy(data, tmp, tmp_cnt);
		bfcount = tmp_cnt;
		break;
	default:
		printf("Not supported format!\n");
		exit(EXIT_FAILURE);
	}

	return bfcount * 8 / bits_per_frame;
}

static size_t pcm_read(snd_pcm_uframes_t chunk_size, u_char *data, size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;

	if (count != chunk_size) {
		count = chunk_size;
	}

	while (count > 0) {
		r = snd_pcm_readi(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 1000);
		} else if (r == -EPIPE) {
			xrun(SND_PCM_STREAM_CAPTURE);
		} else if (r < 0) {
			printf("read error: %s", snd_strerror(r));
			exit(EXIT_FAILURE);
		}

		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}

	return result;
}

static size_t handle_data_cp(snd_pcm_uframes_t chunk_size, u_char *data, u_char *tmp)
{
	size_t tmp_cnt, i, bfcount, chunk_bytes;

	chunk_bytes = chunk_size * bits_per_frame / 8;

	if ((hwparams.channels == 2) && (channel_id != -1)) {
		tmp_cnt = chunk_bytes >> 1;
		if (hwparams.format == SND_PCM_FORMAT_U8) {
			for (i = 0;i < tmp_cnt; i++) {
				if(channel_id)
					*(tmp + i) = *(data + i * 2 + 1);
				else
					*(tmp + i) = *(data + i * 2);
			}
		} else {
			for (i = 0;i < tmp_cnt; i++) {
				if(channel_id)
					*((u_short *)tmp + i) = *((u_short *)data + i * 2 + 1);
				else
					*((u_short *)tmp + i) = *((u_short *)data + i * 2);
			}
		}
	} else if (hwparams.channels == 2){
		if ((hwparams.format == SND_PCM_FORMAT_A_LAW) ||
			(hwparams.format == SND_PCM_FORMAT_MU_LAW)) {
			printf("MU_LAW and A_LAW only support mono\n");
			exit(EXIT_FAILURE);
		}
		tmp_cnt = chunk_bytes;
		memcpy(tmp, data, tmp_cnt);
	} else {
		tmp_cnt = chunk_bytes;
		memcpy(tmp, data, tmp_cnt);
	}

	switch (hwparams.format) {
	case SND_PCM_FORMAT_A_LAW:
		bfcount = G711::ALawEncode(data, (s16 *)tmp, tmp_cnt);
		break;
	case SND_PCM_FORMAT_MU_LAW:
		bfcount = G711::ULawEncode(data, (s16 *)tmp, tmp_cnt);
		break;
	case SND_PCM_FORMAT_S16_LE:
	case SND_PCM_FORMAT_U8:
		memcpy(data, tmp, tmp_cnt);
		bfcount = tmp_cnt;
		break;
	default:
		printf("Not supported format!\n");
		exit(EXIT_FAILURE);
	}

	return bfcount;
}

static ssize_t pcm_write(snd_pcm_uframes_t chunk_size, u_char *data)
{
	ssize_t r;
	ssize_t result = 0;
	snd_pcm_uframes_t count;

	count = chunk_size;

	while (count > 0) {
		r = snd_pcm_writei(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 1000);
		} else if (r == -EPIPE) {
			xrun(SND_PCM_STREAM_PLAYBACK);
		} else if (r < 0) {
			printf("write error: %s", snd_strerror(r));

			if(r == -EIO)
				printf("-EIO error!\n");
			else if(r == -EINVAL)
				printf("-EINVAL error!\n");
			else
				printf("unknown error!\n");

			exit(EXIT_FAILURE);
		}
		if (r > 0) {
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return result;
}

static void playback(snd_pcm_uframes_t chunk_size)
{
	const char *name = file_name;
	u_char *buf_tx;
	u_char *buf_tmp;
	int rval = 0, rd = 0, cnt = 0;
	snd_pcm_state_t state;
	size_t chunk_bytes;

	chunk_bytes = chunk_size * bits_per_frame / 8;

	buf_tx = (u_char *)malloc(chunk_bytes * 2);
	buf_tmp = (u_char *)malloc(chunk_bytes * 2);
	if (buf_tmp == NULL || buf_tx == NULL) {
		printf("not enough memory");
		exit(EXIT_FAILURE);
	}

	if ((fd = open(name, O_RDONLY, 0)) == -1) {
		perror(name);
		exit(EXIT_FAILURE);
	}

	do {
		if (pausing == 0) {
			snd_pcm_uframes_t c;
			if (test_debug == 1 && cnt == -1) {
				snd_pcm_prepare(handle);
				snd_pcm_start(handle);
			}
			rd = read(fd, buf_tx, chunk_bytes);
			if (rd < 0) {
				perror(name);
				exit(EXIT_FAILURE);
			}
			c = handle_data_pb(rd, buf_tx, buf_tmp);
			pcm_write(c, buf_tx);
			/* Looped playback */
			if (rd == 0)
				lseek(fd, 0, SEEK_SET);
			if (test_debug == 1 && cnt++ == 100) {
				cnt = -1;
				snd_pcm_drop(handle);
			}
		} else {
			if (resuming == 1) {
				rval = snd_pcm_pause(handle, 0);
				if (rval < 0) {
					fprintf(stderr, "Resume error: %s\n", snd_strerror(rval));
					exit(EXIT_FAILURE);
				}
				fprintf(stderr, "Resumed...\n");
				resuming = 0;
				pausing = 0;
			} else {
				if ((state = snd_pcm_state(handle)) != SND_PCM_STATE_PAUSED) {
					rval = snd_pcm_pause(handle, 1);
					if (rval < 0) {
						fprintf(stderr, "Pause error: %s\n", snd_strerror(rval));
						exit(EXIT_FAILURE);
					}
					printf("Paused...\n");
				}
			}
		}
	}while (rd >= 0);

	snd_pcm_close(handle);
	close(fd);
	free(buf_tmp);
	free(buf_tx);
}

static void capture(snd_pcm_uframes_t chunk_size)
{
	const char *name = file_name;	/* current filename */
	unsigned long long rest;		/* number of bytes to capture */
	u_char *buf_rx;
	u_char *buf_tmp;
	size_t chunk_bytes;

	chunk_bytes = chunk_size * bits_per_frame / 8;

	buf_rx = (u_char *)malloc(chunk_bytes);
	buf_tmp = (u_char *)malloc(chunk_bytes);
	if (buf_tmp == NULL || buf_rx == NULL) {
		printf("not enough memory");
		exit(EXIT_FAILURE);
	}

	rest = LLONG_MAX;

	/* open a new file to write */
	remove(name);
	if ((fd = open(name, O_WRONLY | O_CREAT, 0644)) == -1) {
		perror(name);
		exit(EXIT_FAILURE);
	}

	/* capture */
	while (rest > 0) {
		ssize_t c = (rest <= (unsigned long long)chunk_bytes) ?
			(size_t)rest : chunk_bytes;
		size_t f = c * 8 / bits_per_frame;
		if (pcm_read(chunk_size, buf_rx, f) != f)
			break;
		c = handle_data_cp(chunk_size, buf_rx, buf_tmp);
		if (write(fd, buf_rx, c) != c) {
			perror(name);
			exit(EXIT_FAILURE);
		}
		rest -= c;
	}

	snd_pcm_close(handle);
	close(fd);
	free(buf_tmp);
	free(buf_rx);
}

static int run_duplex(snd_pcm_stream_t stream)
{
	int err;
	const char *delim = ".";
	char tmp[256];
	snd_pcm_uframes_t chunk_size;

	err = snd_pcm_open(&handle, pcm_name, stream, 0);
	if (err < 0) {
		printf("audio open error: %s", snd_strerror(err));
		return 1;
	}

	if (stream == SND_PCM_STREAM_CAPTURE) {
		strtok(file_name, delim);
		snprintf(tmp, sizeof(tmp), "%s_cp.dat", file_name);
		strcpy(file_name, tmp);
	}

	chunk_size = set_params(handle, stream);

	/* Make sure to start "playback" and "capture" after both "set_params" complete */
	//sleep(1);

	if (stream == SND_PCM_STREAM_PLAYBACK)
		playback(chunk_size);
	else
		capture(chunk_size);

	return 0;
}

static void signal_pause(int sig)
{
	if (test_debug == 1) {
		printf("Currently in test mode, PAUSE and RESUME functions are disabled\n");
	} else {
		if (pausing == 0) {
			pausing = 1;
		} else {
			resuming = 1;
		}
	}
}

static int check_wav_valid(WaveContainer *container)
{
	int ret = 0;

	if (container->wavHeader.magic != WAV_RIFF ||
		container->wavHeader.type != WAV_WAVE ||
		container->waveFmt.fmtMagic != WAV_FMT ||
		container->ChunkHeader.type != WAV_DATA) {
		printf("Not standard wave file!\n");
		return -1;
	}
	if (container->waveFmt.fmtBody.format != LE_SHORT(1) ||
		(container->waveFmt.fmtBody.bit_p_spl != LE_SHORT(16) &&
		container->waveFmt.fmtBody.bit_p_spl != LE_SHORT(8)) ||
		(container->waveFmt.fmtBody.sample_fq != LE_INT(48000) &&
		container->waveFmt.fmtBody.sample_fq != LE_INT(32000) &&
		container->waveFmt.fmtBody.sample_fq != LE_INT(16000) &&
		container->waveFmt.fmtBody.sample_fq != LE_INT(8000)) ||
		(container->waveFmt.fmtBody.channels != LE_SHORT(1) &&
		container->waveFmt.fmtBody.channels != LE_SHORT(2))) {

		printf("Unsupported wave file!\n");
		return -1;
	}

	return ret;
}

static int check_output_format(WaveContainer *inputContainer)
{
	int ret = 0;
	int bit_p_spl = 0;

	if (hwparams.channels < 1 || hwparams.channels > 2) {
		printf("Unsupported output channel count!\n");
		return -1;
	}
	if (hwparams.format != SND_PCM_FORMAT_S16_LE &&
		hwparams.format != SND_PCM_FORMAT_U8) {
		printf("Unsupported output wave format!\n");
		return -1;
	}
	if (hwparams.rate != 8000 && hwparams.rate != 16000 && hwparams.rate != 48000) {
		printf("Unsupported output sample rate!\n");
		return -1;
	}

	if (hwparams.format == SND_PCM_FORMAT_S16_LE) {
		bit_p_spl = 16;
	} else if (hwparams.format == SND_PCM_FORMAT_U8) {
		bit_p_spl = 8;
	}
	if (inputContainer->waveFmt.fmtBody.bit_p_spl < bit_p_spl) {
		printf("Not supporting upsample for bit per sample!\n");
		ret = -1;
	}
	if (inputContainer->waveFmt.fmtBody.channels < hwparams.channels) {
		printf("Not supporting upsample for channels!\n");
		ret = -1;
	}
	if ((inputContainer->waveFmt.fmtBody.channels == 2) && (hwparams.channels == 1)
		&& (channel_id == -1)) {
		printf("Please select which channel to keep from input 2 channels!\n");
		ret = -1;
	}
	if (inputContainer->waveFmt.fmtBody.sample_fq < hwparams.rate) {
		printf("Not supporting upsample for sample frequency!\n");
		ret = -1;
	}

	return ret;
}

static WaveContainer gen_wave_header(WaveContainer *inputContainer)
{
	WaveContainer outputContainer;
	int ratio;

	memcpy(&outputContainer, inputContainer, sizeof(WaveContainer));

	if (hwparams.format == SND_PCM_FORMAT_S16_LE) {
		outputContainer.waveFmt.fmtBody.bit_p_spl = 16;
	} else if (hwparams.format == SND_PCM_FORMAT_U8){
		outputContainer.waveFmt.fmtBody.bit_p_spl = 8;
	}
	outputContainer.waveFmt.fmtBody.sample_fq = hwparams.rate;
	outputContainer.waveFmt.fmtBody.channels = hwparams.channels;

	ratio = inputContainer->waveFmt.fmtBody.sample_fq /
		outputContainer.waveFmt.fmtBody.sample_fq;
	ratio *= inputContainer->waveFmt.fmtBody.bit_p_spl /
		outputContainer.waveFmt.fmtBody.bit_p_spl;
	ratio *= inputContainer->waveFmt.fmtBody.channels /
		outputContainer.waveFmt.fmtBody.channels;

	outputContainer.waveFmt.fmtBody.byte_p_spl =
		outputContainer.waveFmt.fmtBody.channels *
		outputContainer.waveFmt.fmtBody.bit_p_spl / 8;

	outputContainer.waveFmt.fmtBody.byte_p_sec = hwparams.rate *
		outputContainer.waveFmt.fmtBody.byte_p_spl;

	outputContainer.ChunkHeader.length =
		outputContainer.ChunkHeader.length / ratio;

	outputContainer.wavHeader.length = sizeof(WaveContainer) - 8 +
		outputContainer.ChunkHeader.length;

	return outputContainer;
}

static int refill_container(int fd, WaveContainer *container)
{
	WaveContainer2 container2;
	lseek(fd, 0L, SEEK_SET);
	if (read(fd, &container2, sizeof(container2)) != sizeof(container2)) {
		printf("error wave file size!\n");
	}
	container->wavHeader = container2.wavHeader;
	container->waveFmt = container2.waveFmt;
	container->ChunkHeader = container2.ChunkHeader;
	if (container->waveFmt.fmtSize == 18) {
		container->waveFmt.fmtSize = 16;
	} else {
		printf("wrong wave format size!\n");
		return -1;
	}

	return 0;
}

static int parse_wav(int fd, WaveContainer *container)
{
	int ret = 0;

	if (read(fd, container, sizeof(*container)) != sizeof(*container)) {
		printf("error wave file size!\n");
		return -1;
	}

	if (container->waveFmt.fmtSize != 16 &&
		container->waveFmt.fmtSize ==18) {
		printf("Alert!There are 2 extra bytes in wave format!\n");
		refill_container(fd, container);
	}

	if (check_wav_valid(container) < 0) {
		printf("Invalid input wave file!\n");
		return -1;
	}

	return ret;
}

static int init_converter(WaveContainer *inputContainer, WaveContainer *outputContainer,
	AudioConverter *converter, int levels, int count)
{
	if (levels != 3) {
		printf("error audio converter levels!\n");
		return -1;
	}

	// first convert bits per sample
	converter[0].ratio = inputContainer->waveFmt.fmtBody.bit_p_spl /
		outputContainer->waveFmt.fmtBody.bit_p_spl;

	converter[0].offset = converter[0].ratio - 1;
	converter[0].size = count * sizeof(char) / converter[0].ratio;
	converter[0].samples = (char*) malloc(converter[0].size);
	if (converter[0].samples == NULL) {
		printf("error when allocating buffer for mediate 0 samples!\n");
		return -1;
	}
	// second convert channels
	converter[1].ratio = inputContainer->waveFmt.fmtBody.channels /
		outputContainer->waveFmt.fmtBody.channels;
	// default keep channel 0
	converter[1].offset = 0;
	converter[1].size = converter[0].size / converter[1].ratio;
	converter[1].samples = (char*) malloc(converter[1].size);
	if (converter[1].samples == NULL) {
		printf("error when allocating buffer for mediate 1 samples!\n");
		return -1;
	}
	// finally convert sample frequency
	converter[2].ratio = inputContainer->waveFmt.fmtBody.sample_fq /
		outputContainer->waveFmt.fmtBody.sample_fq;
	// default keep sample 0
	converter[2].offset = 0;
	converter[2].size = converter[1].size / converter[2].ratio;
	converter[2].samples = (char*) malloc(converter[2].size);
	if (converter[2].samples == NULL) {
		printf("error when allocating buffer for mediate 1 samples!\n");
		return -1;
	}

	return 0;
}

static void convert_bitdepth(void* in, void* out, int in_dep, int out_dep, int bsize)
{
	short* in_ptr;
	unsigned char* out_ptr;
	int i;

	if (in_dep == out_dep) {
		memcpy(out, in, bsize);
	} else if (in_dep == 16 && out_dep == 8) {
		// convert from S16 to U8
		in_ptr = (short *)in;
		out_ptr = (unsigned char *)out;
		for (i = 0; i < bsize / 2; ++i, out_ptr++, in_ptr++) {
			*out_ptr = (*in_ptr >> 8) + 0x80;
		}
	} else {
		printf("not supported bit depth conversion!\n");
	}
}

static void convert_channels(void* in, void* out, int depth, int in_chs, int out_chs, int bsize)
{
	int i;

	if (in_chs == out_chs) {
		memcpy(out, in, bsize);
	} else if (in_chs == 2 && out_chs == 1) {
		if (depth == 8) {
			char* in_ptr = (char *)in;
			char* out_ptr = (char *)out;
			for (i = 0; i < bsize / 2; ++i) {
				out_ptr[i] = in_ptr[i * 2 + channel_id];
			}
		} else if (depth == 16) {
			short* in_ptr = (short *)in;
			short* out_ptr = (short *)out;
			for (i = 0; i < bsize / 4; ++i) {
				out_ptr[i] = in_ptr[i * 2 + channel_id];
			}
		}
	} else {
		printf("not supported channels conversion!\n");
	}
}

static void convert_rate(void* in, void* out, int spl_size, int ratio, int bsize)
{
	int i, j;
	char* in_ptr;
	char* out_ptr;
	int step;

	if (ratio == 1) {
		memcpy(out, in, bsize);
	} else {
		in_ptr = (char *)in;
		out_ptr = (char *)out;
		step = spl_size * ratio;
		for (i = 0; i < bsize; i += step) {
			for (j = 0; j < spl_size; ++j, out_ptr++) {
				*out_ptr = in_ptr[i + j];
			}
		}
	}
}

static void free_converter()
{
	unsigned int i;

	for(i = 0; i < sizeof(converter) / sizeof(AudioConverter); ++i) {
		if (converter[i].samples) {
			free(converter[i].samples);
		}
	}
}

static int write_content(int fd_in, WaveContainer *inputContainer,
	int fd_out, WaveContainer *outputContainer)
{
	int count = 2400;
	int read_size;
	int output_size;
	char* inputSamples;
	WaveFmtBody* in_fmt = &inputContainer->waveFmt.fmtBody;
	WaveFmtBody* out_fmt = &outputContainer->waveFmt.fmtBody;

	inputSamples = (char*) malloc(count * sizeof(char));
	if (inputSamples == NULL) {
		printf("error when allocating buffer for input samples!\n");
		return -1;
	}

	// 16 bits per sample by default
	if (in_fmt->bit_p_spl == 16 || in_fmt->bit_p_spl == 8) {
		// init converters
		init_converter(inputContainer, outputContainer, converter, 3, count);
		// convert data
		while((read_size = read(fd_in, inputSamples, count)) > 0) {
			// convert bit depth
			convert_bitdepth(inputSamples, converter[0].samples,
				in_fmt->bit_p_spl, out_fmt->bit_p_spl, read_size);
			output_size = read_size / converter[0].ratio;
			// convert channels
			convert_channels(converter[0].samples, converter[1].samples,
				out_fmt->bit_p_spl, in_fmt->channels, out_fmt->channels, output_size);
			output_size = output_size / converter[1].ratio;
			// convert sample rate
			convert_rate(converter[1].samples, converter[2].samples,
				out_fmt->byte_p_spl, converter[2].ratio, output_size);
			output_size = output_size / converter[2].ratio;
			// write output file
			if(write(fd_out, converter[2].samples, output_size) != output_size) {
				printf("error when writing output file!\n");
				return -1;
			}
		}
		free_converter();
	} else {
		printf("error input bits per sample!\n");
	}

	if (inputSamples) {
		free(inputSamples);
	}

	return 0;
}

static int transform_wav()
{
	int fd_in;
	int fd_out;
	WaveContainer input;
	WaveContainer output;

	if ((fd_in = open(pcm_name, O_RDONLY, 0)) < 0) {
		printf("wrong input wave file path %s!\n", pcm_name);
		return -1;
	}

	if ((fd_out = open(file_name, O_WRONLY | O_CREAT , 0644)) < 0) {
		printf("wrong output wave file path %s!\n", file_name);
		return -1;
	}

	if (parse_wav(fd_in, &input) != 0) {
		printf("error input wave file!\n");
		return -1;
	}

	if (check_output_format(&input) != 0) {
		printf("error output wave format!\n");
		return -1;
	}

	output = gen_wave_header(&input);

	// write wave header
	if(write(fd_out, &output, sizeof(output)) != sizeof(output)) {
		printf("error when writing output wave header!\n");
		return -1;
	}

	// write wave content
	if (write_content(fd_in, &input, fd_out, &output) < 0) {
		printf("error when writing output wave content!\n");
		return -1;
	}

	close(fd_in);
	close(fd_out);

	return 0;
}


int main(int argc, char *argv[])
{
	int err, i;
	int option_index, ch;
	snd_pcm_uframes_t chunk_size;
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

	signal(SIGTSTP, signal_pause);
	//signal(SIGTERM, signal_handler);
	//signal(SIGABRT, signal_handler);

	chunk_size = -1;
	hwparams.format = DEFAULT_FORMAT;
	hwparams.rate = DEFAULT_RATE;
	hwparams.channels = DEFAULT_CHANNELS;
	strcpy(file_name, FILE_NAME);
	strcpy(pcm_name, "default");

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;
		case 'P':
			stream = SND_PCM_STREAM_PLAYBACK;
			break;
		case 'C':
			stream = SND_PCM_STREAM_CAPTURE;
			break;
		case 'D':
			duplex = 1;
			break;
		case 'd':
			strcpy(pcm_name, optarg);
			break;
		case 'c':
			hwparams.channels = atoi(optarg);
			if(hwparams.channels < 1 || hwparams.channels > 2){
				printf("value %i for channels is invalid\n", hwparams.channels);
				return 1;
			}
			break;
		case 'f':
			if (strcasecmp(optarg, "cd") == 0){
				hwparams.format = SND_PCM_FORMAT_S16_LE;
				hwparams.rate = 44100;
				hwparams.channels = 2;
			} else if (strcasecmp(optarg, "dat") == 0) {
				hwparams.format = SND_PCM_FORMAT_S16_LE;
				hwparams.rate = 48000;
				hwparams.channels = 2;
			} else {
				hwparams.format = snd_pcm_format_value(optarg);
				if (hwparams.format == SND_PCM_FORMAT_UNKNOWN) {
					printf("wrong extended format '%s'\n", optarg);
					exit(EXIT_FAILURE);
				}
			}
			break;
		case 'r':
			int tmp;
			tmp = strtol(optarg, NULL, 0);
			if (tmp < 300)
				tmp *= 1000;
			hwparams.rate = tmp;
			if (tmp < 8000 || tmp > 48000) {
				printf("bad speed value %i\n", tmp);
				return 1;
			}
			break;
		case 's':
			channel_id = !!strtol(optarg, NULL, 0);
			break;
		case 'T':
			transform = 1;
			break;
		case 't':
			test_debug = 1;
			break;
		default:
			printf("Try `--help' for more information.\n");
			return 1;
		}
	}

	if(optind <= argc -1)
		strcpy(file_name, argv[optind]);

	chunk_size = 1024;

   	/* setup sound hardware */
	if (duplex == 1) {
		stream = SND_PCM_STREAM_PLAYBACK;
		for (i = 0; i < 2; i++) {
			int pid;

			pid = fork();
			if (pid < 0) {
				perror("fork");
				return -1;
			}

			if(pid == 0)
				run_duplex(stream);
			else
				stream = SND_PCM_STREAM_CAPTURE;
		}

		while (1)
			sleep(1000);

	} else if (transform == 1) {
		transform_wav();
	} else {
		err = snd_pcm_open(&handle, pcm_name, stream, 0);
		if (err < 0) {
			printf("audio open error: %s", snd_strerror(err));
			return 1;
		}

		chunk_size = set_params(handle, stream);
		if(stream == SND_PCM_STREAM_PLAYBACK)
			playback(chunk_size);
		else
			capture(chunk_size);
	}

//	if(handle)
//		snd_pcm_close(handle);
//	snd_config_update_free_global();
	return EXIT_SUCCESS;
}

