/*
 * arm_chide.c
 *****************************************************************************
 * Author: Anthony Ginger <mapfly@gmail.com>
 *
 * Copyright (C) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <linux/types.h>

#include <basetypes.h>
#include <amba_debug.h>

/*===========================================================================*/
#define PAGE_SIZE				(1 << 12)
#define APP_LOG_NAME				"chide"
#define ARRAY_SIZE(x)				(sizeof(x) / sizeof(x[0]))
#define CHIDE_MAXARGS				(512)

/*===========================================================================*/
struct chide_hint_info {
	const char *arg;
	const char *str;
};

struct chide_command_info {
	const char *command;
	int (*func)(int argc, const char *const *argv);
	int min_argc;
};

/*===========================================================================*/
static const char *chide_short_options = "hf:a:v:";
static struct option chide_long_options[] = {
	{"help",		no_argument,		NULL,	'h'},
	{"file",		required_argument,	NULL,	'f'},
	{"chip",		required_argument,	NULL,	'a'},
	{"verbose",		required_argument,	NULL,	'v'},
	{NULL,			0,			NULL,	0}
};
static const struct chide_hint_info chide_hint_list[] = {
	{"", "Display this help"},
	{"", "Chide Diag File(default: tester.chide)"},
	{"", "Chide Address fix(default: 0)"},
	{"", "Verbose(default: 0)"},
	{NULL, NULL}
};

static bool chide_show_help = false;
static char chide_diag_file_name[512] = "tester.chide";
static u32 chide_address_fix = 0;
static u32 chide_address_verbose = 0;

typedef enum chide_mode {
	AMBA_UNKNOWN,
	AMBA_LOAD_BINARY,
	AMBA_LOAD_IMAGE,
	AMBA_STORE_BINARY,
	AMBA_STORE_IMAGE,
	AMBA_POKE,
} CHIDE_MODE;

/*===========================================================================*/
static void chide_printf(const char *fmt, ...)
{
	va_list ap;

	if (chide_address_verbose) {
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
	}
}

static void chide_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*===========================================================================*/
static void chide_usage(void)
{
	uint32_t i;

	printf("chide usage:\n");
	for (i = 0; i < (ARRAY_SIZE(chide_long_options) - 1); i++) {
		printf("\t-%c/--%s\t%s\t[%s]\n",
			chide_long_options[i].val,
			chide_long_options[i].name,
			(chide_hint_list[i].arg == NULL) ? "\t" :
			chide_hint_list[i].arg, chide_hint_list[i].str);
	}
}

static int chide_init_param(int argc, const char *const *argv)
{
	int ret_val = 0;
	int opt_val;
	int option_index = 0;

	while ((opt_val = getopt_long(argc, (char *const *)argv,
		chide_short_options, chide_long_options,
		&option_index)) != -1) {
		switch (opt_val) {
		case 'h':
			chide_show_help = true;
			break;
		case 'f':
			snprintf(chide_diag_file_name,
				sizeof(chide_diag_file_name),
				"%s", optarg);
			break;
		case 'a':
			chide_address_fix = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			chide_address_verbose = strtoul(optarg, NULL, 0);
			break;
		default:
			chide_show_help = true;
			ret_val = -1;
			break;
		}
	}

	return ret_val;
}

/*===========================================================================*/
static char *chide_read_file(const char *fn, uint32_t *pfsz)
{
	int fd;
	char *data = NULL;
	int fsz = 0;
	int ret_val = 0;

	fd = open(fn, O_RDONLY);
	if (fd < 0) {
		goto aginit_read_file_exit;
	}

	fsz = lseek(fd, 0, SEEK_END);
	if (fsz < 0) {
		goto aginit_read_file_close;
	}
	ret_val = lseek(fd, 0, SEEK_SET);
	if (ret_val != 0) {
		ret_val = -1;
		goto aginit_read_file_close;
	}

	data = (char *)malloc(fsz + 2);
	if (data == NULL) {
		goto aginit_read_file_close;
	}
	ret_val = read(fd, data, fsz);
	if (ret_val != fsz) {
		ret_val = -1;
		goto aginit_read_file_close;
	}

aginit_read_file_close:
	close(fd);
	if (data != NULL) {
		if (ret_val < 0) {
			free(data);
			data = NULL;
		} else {
			data[fsz] = '\n';
			data[fsz + 1] = 0;
			if (pfsz) {
				*pfsz = fsz;
			}
		}
	}

aginit_read_file_exit:
	return data;
}

/*===========================================================================*/
static void chide_command_process_arg_str(char *arg_str,
	int *pargc, char **pargv, int argv_size)
{
	const char delimiters[] = " \r\n";
	char *pargs;

	*pargc = 0;
	do {
		pargs = strsep(&arg_str, delimiters);
		if (pargs && (strlen(pargs) != 0)) {
			pargv[*pargc] = pargs;
			(*pargc)++;
		}
	} while (pargs && (*pargc < argv_size));
}

static int chide_command_handle_dsp_diag(char const *filename, u32 address,
	u32 size, u32 width, u32 height, u32 pitch, u32 data, CHIDE_MODE mode)
{
	int ret_val = 0;
	int fd, fp;
	char *mem_ptr = NULL;
	u32 mmap_start = 0, mmap_offset;
	u32 i = 0;

	fd = open("/dev/ambad", O_RDWR, 0);
	if (fd < 0) {
		perror("/dev/ambad");
		return fd;
	}

	mmap_start = address & (~(PAGE_SIZE - 1));
	mmap_offset = address - mmap_start;
	mem_ptr = (char *)mmap(NULL, size + mmap_offset,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_start);
	if (mem_ptr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	switch (mode) {
	case AMBA_LOAD_BINARY:
		fp = open(filename, O_RDONLY, 0777);
		if (fp < 0) {
			perror(filename);
			ret_val = fp;
			goto chide_command_handle_exit;
		}
		ret_val = read(fp,
			(void *)((char *)(mem_ptr + mmap_offset)), size);
		if (ret_val < 0) {
			perror("read");
			close(fp);
			goto chide_command_handle_exit;
		}
		close(fp);
		ret_val = 0;
		break;

	case AMBA_LOAD_IMAGE:
		fp = open(filename, O_RDONLY, 0777);
		if (fp < 0) {
			perror(filename);
			ret_val = fp;
			goto chide_command_handle_exit;
		}
		if (data) {
			ret_val = lseek(fp, data, SEEK_SET);
			if (ret_val != data) {
				close(fp);
				ret_val = -1;
				goto chide_command_handle_exit;
			}
		}
		for (i = 0; i < height; i++) {
			ret_val = read(fp, (char *)(mem_ptr + mmap_offset) +
				i * pitch, width);
			if (ret_val != width) {
				perror("read");
				close(fp);
				ret_val = -1;
				goto chide_command_handle_exit;
			}
			/* padding with 0 */
			memset((mem_ptr + mmap_offset + i * pitch + width),
				0, (pitch - width));
		}
		close(fp);
		ret_val = 0;
		break;

	case AMBA_STORE_BINARY:
		fp = open(filename, (O_CREAT | O_TRUNC | O_WRONLY), 0777);
		if (fp < 0) {
			perror(filename);
			ret_val = fp;
			goto chide_command_handle_exit;
		}
		ret_val = write(fp,
			(void *)((char *)(mem_ptr + mmap_offset)), size);
		if (ret_val != size) {
			perror("write");
			close(fp);
			ret_val = -1;
			goto chide_command_handle_exit;
		}
		close(fp);
		ret_val = 0;
		break;

	case AMBA_STORE_IMAGE:
		fp = open(filename, (O_CREAT | O_TRUNC | O_WRONLY), 0777);
		if (fp < 0) {
			perror(filename);
			ret_val = fp;
			goto chide_command_handle_exit;
		}
		if (width) {
			/* if storeimage with pitch/pad_width, we need to remove the
			 * pad_width - width bytes for each line when store image */
			for (i = 0; i < size / pitch; i++) {
				ret_val = write(fp,
				(char *)(mem_ptr + mmap_offset) + i * pitch,
				width);
				if (ret_val != width) {
					perror("write");
					close(fp);
					ret_val = -1;
					goto chide_command_handle_exit;
				}
			}
		} else {
			ret_val = write(fp,
				(void *)((char *)(mem_ptr + mmap_offset)),
				size);
			if (ret_val != size) {
				perror("write");
				close(fp);
				ret_val = -1;
				goto chide_command_handle_exit;
			}
		}
		close(fp);
		ret_val = 0;
		break;

	case AMBA_POKE:
		for (i = 0; i < size; i += 4) {
			*(u32 *)(mem_ptr + mmap_offset + i) = data;
		}
		ret_val = 0;
		break;

	default:
		ret_val = -1;
		break;
	}

chide_command_handle_exit:
	if (mem_ptr) {
		munmap(mem_ptr, size);
	}
	close(fd);

	return ret_val;
}

static int chide_command_connect(int argc, const char *const *argv)
{
	return 0;
}

static int chide_command_init_jtag(int argc, const char *const *argv)
{
	return 0;
}

static int chide_command_arm_status(int argc, const char *const *argv)
{
	return 0;
}

static int chide_command_arm_stop(int argc, const char *const *argv)
{
	return 0;
}

static int chide_command_cmdfile(int argc, const char *const *argv)
{
	if (argc) {
		chide_printf("Ignore [cmdfile %s]\n", argv[0]);
	}

	return 0;
}

static int chide_command_loadelf(int argc, const char *const *argv)
{
	if (argc) {
		chide_error("Ignore [loadelf %s]\n", argv[0]);
	}

	return 0;
}

static int chide_command_loadbinary(int argc, const char *const *argv)
{
	int ret_val = 0;
	char const *pfile_name;
	uint32_t address;
	uint32_t length = 0;
	int i;
	int file_arg = -1;
	int address_arg = -1;
	char *pfile_buffer;
	uint32_t file_size;

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(&argv[i][1], "length")) {
				// TBD: check argc > (i + 1)
				length = strtoul(argv[i + 1], NULL, 0);
			} else {
				chide_error("unknown option '%s'\n",
					&argv[i][1]);
				ret_val = -1;
				goto chide_command_loadbinary_exit;
			}
			i++;
			continue;
		} else {
			if (file_arg < 0) {
				file_arg = i;
			} else if (address_arg < 0) {
				address_arg = i;
			}
		}
	}

	pfile_name = argv[file_arg];
	address = strtoul(argv[address_arg], NULL, 0);
	pfile_buffer = chide_read_file(pfile_name, &file_size);
	if (pfile_buffer == NULL) {
		chide_error("Can't load '%s'\n", pfile_name);
		ret_val = -1;
		goto chide_command_loadbinary_exit;
	}
	if (length == 0) {
		length = file_size;
	}

	if (chide_address_fix) {
		address -= chide_address_fix;
	}
	chide_printf("loadbinary %s 0x%08x %d\n", pfile_name, address, length);
	ret_val = chide_command_handle_dsp_diag(pfile_name, address, length,
		0, 0, 0, 0, AMBA_LOAD_BINARY);

	free(pfile_buffer);

chide_command_loadbinary_exit:
	return ret_val;
}

static int chide_command_storebinary(int argc, const char *const *argv)
{
	int ret_val = 0;
	char const *pfile_name;
	uint32_t address;
	uint32_t length = 0;
	int i;
	int file_arg = -1;
	int address_arg = -1;
	int length_arg = -1;

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(&argv[i][1], "length")) {
				// TBD: check argc > (i + 1)
				length = strtoul(argv[i + 1], NULL, 0);
			} else {
				chide_error("unknown option '%s'\n",
					&argv[i][1]);
				ret_val = -1;
				goto chide_command_storebinary_exit;
			}
			i++;
			continue;
		} else {
			if (file_arg < 0) {
				file_arg = i;
			} else if (address_arg < 0) {
				address_arg = i;
			} else if (length_arg < 0) {
				length_arg = i;
			}
		}
	}

	pfile_name = argv[file_arg];
	address = strtoul(argv[address_arg], NULL, 0);
	if (length == 0) {
		length = strtoul(argv[length_arg], NULL, 0);
	}

	if (chide_address_fix) {
		address -= chide_address_fix;
	}
	chide_printf("storebinary %s 0x%08x %d\n", pfile_name, address, length);
	ret_val = chide_command_handle_dsp_diag(pfile_name, address, length,
		0, 0, 0, 0, AMBA_STORE_BINARY);

chide_command_storebinary_exit:
	return ret_val;
}

static int chide_command_loadimage(int argc, const char *const *argv)
{
	int ret_val = 0;
	char const *pfile_name;
	uint32_t address;
	uint32_t pitch = 0;
	uint32_t head_width = 0;
	uint32_t head_height = 0;
	int i;
	int file_arg = -1;
	int address_arg = -1;
	char *pfile_buffer;
	uint32_t file_size;
	u32 size = 0;
	uint32_t file_offset = 0;

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(&argv[i][1], "pitch")) {
				pitch = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "pad_width")) {
				pitch = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "width")) {
				head_width = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "height")) {
				head_height = strtoul(argv[i + 1], NULL, 0);
			} else {
				chide_error("unknown option '%s'\n",
					&argv[i][1]);
				ret_val = -1;
				goto chide_command_loadimage_exit;
			}
			i++;
			continue;
		} else {
			if (file_arg < 0) {
				file_arg = i;
			} else if (address_arg < 0) {
				address_arg = i;
			}
		}
	}

	pfile_name = argv[file_arg];
	address = strtoul(argv[address_arg], NULL, 0);
	pfile_buffer = chide_read_file(pfile_name, &file_size);
	if (pfile_buffer == NULL) {
		chide_error("Can't load '%s'\n", pfile_name);
		ret_val = -1;
		goto chide_command_loadimage_exit;
	}
	if ((head_width == 0) && (head_height == 0)) {
		head_width = pfile_buffer[1];
		head_width <<= 8;
		head_width |= pfile_buffer[0];
		head_height = pfile_buffer[3];
		head_height <<= 8;
		head_height |= pfile_buffer[2];
		file_offset = 4;
	}
	if (pitch == 0) {
		pitch = ((head_width + 31) & 0xFFFFFFE0);
	}

	if (chide_address_fix) {
		address -= chide_address_fix;
	}
	chide_printf("loadimage %s 0x%08x %d %d %d\n",
		pfile_name, address, pitch, head_width, head_height);

	/* image size use pitch * height */
	size = pitch * head_height;
	ret_val = chide_command_handle_dsp_diag(pfile_name, address, size,
		head_width, head_height, pitch, file_offset, AMBA_LOAD_IMAGE);

	free(pfile_buffer);

chide_command_loadimage_exit:
	return ret_val;
}

static int chide_command_storeimage(int argc, const char *const *argv)
{
	int ret_val = 0;
	char const *pfile_name;
	uint32_t address;
	uint32_t length = 0;
	uint32_t pitch = 0;
	uint32_t head_width = 0;
	uint32_t head_height = 1;
	int i;
	int file_arg = -1;
	int address_arg = -1;
	int length_arg = -1;
	int width_arg = -1;
	u32 size;

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(&argv[i][1], "width")) {
				head_width = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "height")) {
				head_height = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "padwidth")) {
				pitch = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "pitch")) {
				pitch = strtoul(argv[i + 1], NULL, 0);
			} else if (!strcmp(&argv[i][1], "length")) {
				length = strtoul(argv[i + 1], NULL, 0);
			} else {
				chide_error("unknown option '%s'\n",
					&argv[i][1]);
				ret_val = -1;
				goto chide_command_storeimage_exit;
			}
			i++;
			continue;
		} else {
			if (file_arg < 0) {
				file_arg = i;
			} else if (address_arg < 0) {
				address_arg = i;
			} else if (length_arg < 0) {
				length_arg = i;
			} else if (width_arg < 0) {
				width_arg = i;
			}
		}
	}

	pfile_name = argv[file_arg];
	address = strtoul(argv[address_arg], NULL, 0);
	if (head_width == 0) {
		if (width_arg > 0) {
			head_width = strtoul(argv[width_arg], NULL, 0);
		} else {
			chide_error("storeimage: unknown width\n");
			ret_val = -1;
			goto chide_command_storeimage_exit;
		}
	}
	if (length == 0) {
		if (length_arg > 0) {
			length = strtoul(argv[length_arg], NULL, 0);
		} else {
			length = head_width * head_height;
		}
	}
	if (pitch == 0) {
		pitch = ((head_width + 31) & 0xFFFFFFE0);
	}

	if (chide_address_fix) {
		address -= chide_address_fix;
	}
	chide_printf("storeimage %s 0x%08x %d %d %d\n",
		pfile_name, address, length, head_width, pitch);

	size = length / head_width * pitch;
	ret_val = chide_command_handle_dsp_diag(pfile_name, address, size,
		head_width, 0, pitch, 0, AMBA_STORE_IMAGE);

chide_command_storeimage_exit:
	return ret_val;
}

static int chide_command_poke(int argc, const char *const *argv)
{
	int ret_val = 0;
	uint32_t address;
	uint32_t data = 0;
	uint32_t size;
	uint32_t i;

	address = strtoul(argv[0], NULL, 0);
	chide_printf("poke 0x%08x", address);
	for (i = 1; i < argc; i++) {
		data = strtoul(argv[i], NULL, 0);
		chide_printf(" 0x%08lx", strtoul(argv[i], NULL, 0));
	}
	chide_printf("\n");

	/* data is 32-bit */
	i--;
	size = 4*i;
	ret_val = chide_command_handle_dsp_diag(NULL, address, size, 0,
		0, 0, data, AMBA_POKE);
	return ret_val;
}

static int chide_command_sleep(int argc, const char *const *argv)
{
	int ret_val = 0;
	uint32_t sleep_tm;

	sleep_tm = strtoul(argv[0], NULL, 0);
	chide_printf("sleep 0x%08x\n", sleep_tm);
	sleep(sleep_tm);

	return ret_val;
}

static int chide_command_diff(int argc, const char *const *argv)
{
	int ret_val = 0;
	int i;
	char diff_cmd[512];
	uint32_t diff_cmd_offset = 0;

	diff_cmd_offset += snprintf((diff_cmd + diff_cmd_offset),
		(sizeof(diff_cmd) - diff_cmd_offset), "diff");
	for (i = 0; i < argc; i++) {
		diff_cmd_offset += snprintf((diff_cmd + diff_cmd_offset),
			(sizeof(diff_cmd) - diff_cmd_offset),
			" %s", argv[i]);
	}
	//chide_printf("%s\n", diff_cmd);
	ret_val = system(diff_cmd);
	if (ret_val) {
		chide_error("Error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		ret_val = 0;
	}

	return ret_val;
}

static const struct chide_command_info chide_command_list[] = {
	{"connect", chide_command_connect, 0},
	{"init_jtag", chide_command_init_jtag, 0},
	{"arm_status", chide_command_arm_status, 0},
	{"arm_stop", chide_command_arm_stop, 0},
	{"cmdfile", chide_command_cmdfile, 1},
	{"loadelf", chide_command_loadelf, 2},
	{"loadbinary", chide_command_loadbinary, 2},
	{"storebinary", chide_command_storebinary, 3},
	{"loadimage", chide_command_loadimage, 2},
	{"storeimage", chide_command_storeimage, 4},
	{"poke", chide_command_poke, 2},
	{"sleep", chide_command_sleep, 1},
	{"diff", chide_command_diff, 2},
	{NULL, NULL}
};

/*===========================================================================*/
static int chide_process_diag_line(char *pdiag_line)
{
	int ret_val = 0;
	const char delimiters[] = " \r\n";
	char *pcommand;
	uint32_t i;
	int cmd_argc;
	char *cmd_argv[CHIDE_MAXARGS];

	if (pdiag_line[0] == '#') {
		goto chide_process_diag_line_exit;
	}

	pcommand = strsep(&pdiag_line, delimiters);
	if (pcommand == NULL) {
		goto chide_process_diag_line_exit;
	}
	if (strlen(pcommand) == 0) {
		goto chide_process_diag_line_exit;
	}

	for (i = 0; i < ARRAY_SIZE(chide_command_list); i++) {
		if ((chide_command_list[i].command == NULL) ||
			(chide_command_list[i].func == NULL)) {
			break;
		}
		if (!strcmp(chide_command_list[i].command, pcommand)) {
			chide_command_process_arg_str(pdiag_line,
				&cmd_argc, cmd_argv, CHIDE_MAXARGS);
			if (cmd_argc < chide_command_list[i].min_argc) {
				chide_error("Too few args for command %s,"
					" [%d:%d]\n", pcommand, cmd_argc,
					chide_command_list[i].min_argc);
				ret_val = -1;
			} else {
				ret_val = chide_command_list[i].func(cmd_argc,
					(const char * const*)cmd_argv);
			}
			goto chide_process_diag_line_exit;
		}
	}
	chide_error("Can't process command[%s:%d]\n",
		pcommand, strlen(pcommand));
	ret_val = -1;

chide_process_diag_line_exit:
	return ret_val;
}

static int chide_process_diag_file(char *pdiag_file_buffer)
{
	int ret_val = 0;
	const char delimiters[] = "\n";
	char *pline;

	do {
		pline = strsep(&pdiag_file_buffer, delimiters);
		if (pline) {
			//chide_printf("New Line: %s\n", pline);
			ret_val = chide_process_diag_line(pline);
			if (ret_val) {
				break;
			}
		}
	} while(pline);

	return ret_val;
}

/*===========================================================================*/
int main(int argc, const char *const *argv)
{
	int ret_val = 0;
	char *pdiag_file_buffer;

	ret_val = chide_init_param(argc, argv);
	if (chide_show_help) {
		chide_usage();
		goto chide_exit;
	}
	if (ret_val) {
		goto chide_exit;
	}

	pdiag_file_buffer = chide_read_file(chide_diag_file_name, NULL);
	if (pdiag_file_buffer == NULL) {
		printf("Can't load %s.\n", chide_diag_file_name);
		ret_val = -1;
		goto chide_exit;
	}
	ret_val = chide_process_diag_file(pdiag_file_buffer);
	free(pdiag_file_buffer);

chide_exit:
	exit(ret_val);
}

