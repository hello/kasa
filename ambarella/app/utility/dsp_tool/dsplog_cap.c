/*
 * dsplog_cap.c
 *
 * History:
 *	2012/05/05 - [Jian Tang] created file
 * 	2013/09/26 - [Louis Sun] improve it to prevent data loss
 *
 * Copyright (c) 2016 Ambarella, Inc.
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


#include "dsplog_cap.h"

static dsplog_memory_block dsplog_mem;
static dsplog_debug_obj dsplog_debug = {
	.output = "/tmp/dsplog.dat",
	.parse_in = "/tmp/dsplog.dat",
	.parse_out = "",
	.verify = "",
	.ucode = "/lib/firmware",
	.input_file_flag = 0,
	.output_file_flag = 0,
	.verify_flag = 0,
	.capture_current_flag = 0,
	.work_mode = DSPLOG_CAPTURE_MODE,

	.debug_level_flag = 0,
	.debug_level = DSPLOG_DEBUG,
	.module_id = 0,
	.module_id_last = 0,
	.modules = {
		-1, -1, -1, -1, -1, -1, -1, -1,
	},

	.thread_bitmask_flag = 0,
	.thread_bitmask = 0,
	.debug_bitmask = {
		0, 0, 0, 0, 0, 0, 0, 0,
	},
	.operation_add = {
		0, 0, 0, 0, 0, 0, 0, 0,
	},

	.show_version_flag = 0,
	.show_ucode_flag = 0,
	.pinpong_flag = 0,
	.transfer_tcp_flag = 0,

	.transfer_fd = -1,
	.pinpong_filesize_limit = DEFAULT_PINPONG_FILE_SIZE,
};

static sem_t exit_main_sem;

static struct option long_options[] = {
	{"module",	HAS_ARG,	0,	'm'},     //must be string.
	{"level",		HAS_ARG,	0,	'l'},       //debug level 0: OFF 1: NORMAL 2: VERBOSE 3: DEBUG
	{"thread",	HAS_ARG,	0,	'r'},       //coding orc thread
	{"bin-out",	HAS_ARG,	0,	'o'},      //output file (for log capture)
	{"parse-in",	HAS_ARG,	0,	'i'},       //parse_in file (for parse log)
	{"tcp",		NO_ARG,	       0,	't'},       //use tcp to transfer dsp log
	{"bitmask",	HAS_ARG,       0 ,   'b'},  //debug bitmask
	{"ucode-dir",	HAS_ARG,       0 ,   'd'},  //use different ucode dir to parse log
	{"current",	NO_ARG,          0 ,   'c'},   // only capture log in dsp buf only (small log)
	{"logtext",	HAS_ARG,      0,     'f'},  //  log text file (after parse)
	{"pinpong",	HAS_ARG,      0,    'p'},  //  write log to two files, by pinpong mode, each file is limited to size specified by this option, for example, each file is 100MB,
	{"ucode-ver",	NO_ARG,        0,   'v'},  // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"version",	NO_ARG,        0,   'w'},  // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"verify",		HAS_ARG,          0,   'y'},  // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"add",		NO_ARG,         0,    'a'},     //"Add or Set" operation to debug mask.  when specified, it's Add.   else it's set
	{"caponly",		NO_ARG,         0,    's'},     //not set level and bitmask
	{0, 0, 0, 0}
};

static const char *short_options = "b:m:l:r:o:i:td:cf:p:vwy:as";

static const struct hint_s hint[] = {
	{"string", "\tcommon, vcap, vout, encoder, decoder,  idsp, memd, api, perf, all"},
	{"0~4", "\tdebug level 0: OFF, 1: FORCE, 2: NORMAL, 3: VERBOSE, 4: DEBUG."},
	{"0~255", "\tDSP thread bitmask, decimal or hex (start with 0x), set bit to 0 to enable"},
	{"filename", "\tfilename of dsplog binary to capture(output)"},
	{"filename", "filename of dsplog binary to parse(parse_in)"},
	{"", "\t\tuse TCP to transfer dsp log (work with ps.exe)"},
	{"32-bit", "\tdebug bitmask, decimal or hex (start with 0x), set bit to 1 to enable, ONLY for S2/iOne"},
	{"path", "\tuse another ucode path to parse the dsplog,  like /tmp/mmcblk0p1/latest"},
	{"",  "\t\tcapture current dsp log in LOGMEM buffer, which is 128KB"},
	{"filename", "\tfilename of dsplog text file after parse"},
	{"filesize", "\tspecify the dsplog file size limit in pinpong mode"},
	{"", "\t\tappend ucode version to ucode binary filename, show ucode version when used alone"},
	{"", "\t\tshow dsplog_cap tool version"},
	{"filename", "\tuse dsp log mem buffer to verify captured log file (DSP should have halted)"},
	{"",  "\t\tif specified, current bitmask is added to, but not set.  (else it's set by default)"},
	{"",  "\t\tif specified, capture only, do not set debug level and bitmask"},
};

static void show_quit(void)
{
	fprintf(stderr, "\nPlease press 'Ctrl+C' to quit capture, or use"
		" 'killall dsplog_cap' to stop capture. Don't use 'kill -9' to force quit.\n");
}

static void usage(void)
{
	int i;
	char *itself = "dsplog_cap";
	printf("\n\n############################################################\n");
	printf("%s ver %s,  it supports A5s / A7L / iOne / S2 / S2L / S3L.  usage:\n\n",
		itself, DSPLOG_VERSION_STRING);
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val)) {
			printf("-%c ", long_options[i].val);
		} else {
			printf("   ");
		}
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0) {
			printf(" [%s]", hint[i].arg);
		}
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
	extra_usage(itself);
}

static int get_ucode_version(char * buffer, int max_len)
{
	int ucode_fd = -1;
	int ret = 0;
	ucode_version_t ucode_version;

	do {
		if ((ucode_fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
			perror("/dev/ucode");
			ret = -1;
			break;
		}
		if (ioctl(ucode_fd, IAV_IOC_GET_UCODE_VERSION, &ucode_version) < 0) {
			perror("IAV_IOC_GET_UCODE_VERSION");
			ret = -1;
			break;
		}
		snprintf(buffer, max_len - 1, "%d-%d-%d.num%d-ver%d",
			ucode_version.year, ucode_version.month, ucode_version.day,
			ucode_version.edition_num, ucode_version.edition_ver);
		buffer[max_len - 1] = '\0';
	} while (0);

	if (ucode_fd >=0)
		close(ucode_fd);
	return ret;
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;
	int module_id;
	int bitmask;
	int thread_mask;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options,
					long_options, &option_index)) != -1) {
		switch (ch) {
		case 'm':
			if ((module_id = get_dsp_module_id(optarg)) < 0) {
				fprintf(stderr,"Invalid dsp module id.\n");
				return -1;
			} else {
				if (dsplog_debug.module_id >= DSP_MODULE_TOTAL_NUM) {
					fprintf(stderr, "Too many modules to debug.\n");
					return -1;
				}
				dsplog_debug.modules[dsplog_debug.module_id] = module_id;
				dsplog_debug.module_id_last = dsplog_debug.module_id;
				dsplog_debug.module_id++;
			}
			break;

		case 'l':
			value = atoi(optarg);
			if ((value >= DSPLOG_LEVEL_FIRST) && (value < DSPLOG_LEVEL_LAST)) {
				dsplog_debug.debug_level = value;
				dsplog_debug.debug_level_flag = 1;
			} else {
				fprintf(stderr, "Invalid debug level.\n");
				return -1;
			}
			break;

		case 'r':
			if ((thread_mask = get_thread_bitmask(optarg)) < 0) {
				fprintf(stderr, "Invalid thread bit mask.\n");
				return -1;
			} else {
				dsplog_debug.thread_bitmask = thread_mask;
				dsplog_debug.thread_bitmask_flag = 1;
			}
			break;

		case 'o':
			if (strlen(optarg) > 0) {
				strncpy(dsplog_debug.output, optarg, STR_LEN);
				dsplog_debug.output[STR_LEN - 1] = '\0';
				dsplog_debug.output_file_flag =1;
			} else {
				fprintf(stderr, "Invalid parse_out DSP log file name.\n");
				return -1;
			}
			break;

		case 'i':
			if (strlen(optarg) > 0) {
				strncpy(dsplog_debug.parse_in, optarg, STR_LEN);
				dsplog_debug.parse_in[STR_LEN - 1] = '\0';
				dsplog_debug.input_file_flag =1;
			} else {
				fprintf(stderr, "Invalid parse_in DSP log file name.\n");
				return -1;
			}
			break;

		case 't':
			dsplog_debug.transfer_tcp_flag = 1;
			break;

		case  'b':
			//bitmask
			bitmask = get_dsp_bitmask(optarg);
			dsplog_debug.debug_bitmask[dsplog_debug.module_id_last] = bitmask;
			break;

		case 'd':
			strncpy(dsplog_debug.ucode, optarg, STR_LEN);
			dsplog_debug.ucode[STR_LEN - 1] = '\0';
			break;

		case 'c':
			dsplog_debug.capture_current_flag = 1;
			break;

		case 'f':
			strncpy(dsplog_debug.parse_out, optarg, STR_LEN);
			dsplog_debug.parse_out[STR_LEN - 1] = '\0';
			break;

		case 'p':
			dsplog_debug.pinpong_flag = 1;
			dsplog_debug.pinpong_filesize_limit = atoi(optarg);
			if (dsplog_debug.pinpong_filesize_limit < (2 << 20)) {
				dsplog_debug.pinpong_filesize_limit = DEFAULT_PINPONG_FILE_SIZE;
				fprintf(stderr, "Use pinpong buffer with default size %d MB.\n",
					(DEFAULT_PINPONG_FILE_SIZE >> 20));
			}
			break;

		case 'v':
			dsplog_debug.show_ucode_flag = 1;
			break;

		case 'w':
			dsplog_debug.show_version_flag = 1;
			break;

		case 'y':
			strcpy(dsplog_debug.verify, optarg);
			dsplog_debug.verify_flag = 1;
			break;

		case 'a':
			dsplog_debug.operation_add[dsplog_debug.module_id_last] = 1;
			break;

		case 's':
			dsplog_debug.capture_only_flag = 1;
			break;

		default:
			break;
		}
	}

	//check the options
	if (dsplog_debug.capture_current_flag ||
		dsplog_debug.show_ucode_flag ||
		dsplog_debug.show_version_flag ||
		dsplog_debug.verify_flag) {
		return 0;
	}

	if ((dsplog_debug.input_file_flag) && (dsplog_debug.output_file_flag)) {
		fprintf(stderr, "Cannot enable dsplog capture (output) and dsplog "
			"parse (parse_in) at same time!\n");
		return -1;
	} else if ((!dsplog_debug.input_file_flag) &&
			(!dsplog_debug.output_file_flag)) {
		fprintf(stderr,"Please specify cmd to either capture or parse!\n");
		return -1;
	}

	if (dsplog_debug.input_file_flag) {
		dsplog_debug.work_mode = DSPLOG_PARSE_MODE;
	} else {
		dsplog_debug.work_mode = DSPLOG_CAPTURE_MODE;
	}

	//check filename, do not have dir path in filename when transfer is tcp mode
	if (dsplog_debug.transfer_tcp_flag) {
		if (strchr(dsplog_debug.output, '/')) {
			fprintf(stderr, "When tcp transfer mode specified, please do not "
				"use '/' in outputfilename, current name is [%s].\n",
				dsplog_debug.output);
			return -1;
		}
		if (dsplog_debug.output_file_flag) {
			fprintf(stderr, "Do not support parse dsp log by -t option now.\n");
			return -1;
		}
	}

	return 0;
}


/**********************************************************************
 *
 *   CAPTURE DSP LOG START
 *
 *********************************************************************/
int dsplog_enable(void)
{
	int fd = -1;
	int debug_flag = 0;
	int ret = 0;

	do {
		if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
			perror("/dev/ambad");
			ret = -1;
			break;
		}
		if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
			ret = -1;
			break;
		}
		debug_flag |= AMBA_DEBUG_DSP;
		if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
			ret = -1;
			break;
		}
	} while (0);

	if (fd >= 0) {
		close(fd);
	}
	return ret;
}

int dsplog_get_transfer_fd(int method, char * output_filename,  int dsplog_port)
{
	int transfer_fd = -1;
	char filename[512];

	if (!dsplog_debug.pinpong_flag) {
		if (dsplog_debug.transfer_fd >=0) {
			return dsplog_debug.transfer_fd;
		}
		if ((transfer_fd = amba_transfer_open(output_filename,
						method, dsplog_port)) < 0) {
			fprintf(stderr, "Create file for transfer failed %s.\n",
				output_filename);
			return -1;
		}
		dsplog_debug.transfer_fd = transfer_fd;
		dsplog_debug.transfer_method = method;
	} else {
		if (dsplog_debug.transfer_fd >=0) {
			//now it's pinpong mode.
			if (dsplog_debug.pinpong_filesize_now >=
				dsplog_debug.pinpong_filesize_limit) {
				//oversize file, close current fd and open new
				amba_transfer_close(dsplog_debug.transfer_fd, method);
				//switch pinpong state
				dsplog_debug.pinpong_state = !dsplog_debug.pinpong_state;
			} else {
				return dsplog_debug.transfer_fd;
			}
		}
		//now create new file
		if (!dsplog_debug.pinpong_state) {
			sprintf(filename, "%s.1", output_filename);
		} else {
			sprintf(filename, "%s.2", output_filename);
		}

		if ((transfer_fd = amba_transfer_open(filename,
							method, dsplog_port)) < 0) {
			fprintf(stderr, "create file for transfer failed %s \n", filename);
			return -1;
		} else {
			dsplog_debug.transfer_fd = transfer_fd;
			dsplog_debug.transfer_method = method;
		}

		//reset to new file with size
		dsplog_debug.pinpong_filesize_now = 0;
	}
	return transfer_fd;
}


// dsplog log-filename [ 1000 (max log size) ]
// dsplog d module debuglevel coding_thread_printf_disable_mask

// for iOne / S2:
// dsplog level [module] [0:add or 1:set] [debug mask]
// dsplog thread [thread disable bit mask]

int dsplog_capture(int dsplog_drv_fd, char * log_buffer,
	int method, char * output_filename,  int dsplog_port)
{
	int transfer_fd = -1;

	amba_transfer_init(method);

	//start capture
	if (ioctl(dsplog_drv_fd, AMBA_IOC_DSPLOG_START_CAPUTRE) < 0) {
		perror("AMBA_IOC_DSPLOG_START_CAPUTRE");
		return -1;
	}

	while (1) {
		transfer_fd = dsplog_get_transfer_fd(method, output_filename, dsplog_port);
		if (!transfer_fd) {
			fprintf(stderr, "dsplog_get_transfer_fd failed for %s \n ", output_filename);
			return -1;
		}

		ssize_t size = read(dsplog_drv_fd, log_buffer, MAX_LOG_READ_SIZE);
		if (size < 0) {
			if (size == -EAGAIN) {
				//do it again, the previous read fails because of EAGIN (interupted by user)
				fprintf(stderr, "read it again \n");
				size = read(dsplog_drv_fd, log_buffer, MAX_LOG_READ_SIZE);
				if (size < 0) {
					fprintf(stderr, "read fail again \n");
					return -1;
				}
			} else {
				perror("dsplog_cap: read\n");
				return -1;
			}
		} else if (size == 0) {
			// fprintf(stderr, "dsplog read finished. stop amba_transfer.\n");
			amba_transfer_close(dsplog_debug.transfer_fd,  dsplog_debug.transfer_method);
			amba_transfer_deinit(dsplog_debug.transfer_method);
			break;
		}

		if (size > 0) {
			if (amba_transfer_write(transfer_fd, log_buffer, size, method) < 0) {
				fprintf(stderr,"dsplog_cap: write failed \n");
				return -1;
			} else {
				//accumulate file size
				if (dsplog_debug.pinpong_flag) {
					dsplog_debug.pinpong_filesize_now += size;
				}
			}
		}
	}

	return 0;
}

int dsplog_capture_stop(int dsplog_drv_fd)
{
	//start capture
	if (ioctl(dsplog_drv_fd, AMBA_IOC_DSPLOG_STOP_CAPTURE) < 0) {
		perror("AMBA_IOC_DSPLOG_STOP_CAPUTRE");
		return -1;
	} else {
		return 0;
	}
}

/**********************************************************************
 *
 *  CAPTURE DSP LOG END
 *
 *********************************************************************/


/**********************************************************************
 *
 *  PARSE DSP LOG START
 *
 *********************************************************************/
u8 *mmap_ucode_bin(char *dir, char *name)
{
	u8 *mem = NULL;
	FILE *fp = NULL;
	int fsize;
	char filename[512];
	do {
		if ((!dir) || (!name)) {
			mem = NULL;
			break;
		}
		sprintf(filename, "%s/%s", dir, name);
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "Cannot open ucode bin file [%s].\n", filename);
			perror(filename);
			mem = NULL;
			break;
		}

		if (fseek(fp, 0, SEEK_END) < 0) {
			perror("fseek");
			mem = NULL;
			break;
		}
		fsize = ftell(fp);

		mem = (u8 *)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
		if ((intptr_t)mem == -1) {
			perror("mmap");
			mem = NULL;
			break;
		}
	}while(0);

	return mem;
}

static int dsplog_parse(char * log_filename, u8 * pcode,
	u8 * pmdxf, u8 * pmemd,  char * output_text_file)
{
	FILE * parse_in = NULL;
	FILE * parse_out = NULL;
	idsp_printf_base_t record;
	u8 is_first_record = 1;
	int first = 0;
	int last = 0;
	int total = 0;
	int loss;

	do {
		if ((parse_in = fopen(log_filename, "rb")) == NULL) {
			fprintf(stderr, "Cannot open dsplog file %s to parse, "
				"please use '-i' option to specify.\n", log_filename);
			break;
		}
		if ((parse_out = fopen(output_text_file, "wb")) == NULL) {
			fprintf(stderr, "Cannot open dsplog parse text file %s to write, "
				"please use '-f' option to specify.\n", output_text_file);
			break;
		}
	} while (0);

	if (!parse_in || !parse_out) {
		if (parse_in) {
			fclose(parse_in);
		}
		if (parse_out) {
			fclose(parse_out);
		}
		return -1;
	}
	fprintf(stderr, "You may press Ctrl+C to quit parsing.\n");
	while (1) {
		int rval;
		if ((rval = fread(&record, IDSP_LOG_SIZE, 1, parse_in)) != 1) {
			break;
		}
		if (is_first_record) {
			first = record.seq_num;
			is_first_record = 0;
		}
		print_log(&record, pcode, pmdxf, pmemd, parse_out, &dsplog_mem);

		total++;
		last = record.seq_num;
		if ((total % 1000) == 0) {
			fprintf(stderr, "\r%d", total);
			fflush(stderr);
		}
	}

	fprintf(stderr, "\r");
	fprintf(stderr, " first record : %d\n", first);
	fprintf(stderr, "  last record : %d\n", last);
	fprintf(stderr, "total records : %d\n", total);

	loss = (last - first + 1) - total;
	if ((loss < 0) && (loss + total == 0)) {
		fprintf(stderr, "this is a LOGMEM ring buf direct dump\n");
	} else {
		fprintf(stderr, " lost records : %d\n", loss);
	}

	if (parse_in) {
		fclose(parse_in);
	}
	if (parse_out) {
		fclose(parse_out);
	}

	return 0;
}

/**********************************************************************
 *
 *   PARSE DSP LOG ENDS
 *
 *********************************************************************/
int block_signals()
{
	sigset_t   signal_mask;  // signals to block
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGINT);
	sigaddset (&signal_mask, SIGQUIT);
	sigaddset (&signal_mask, SIGTERM);
	pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	return 0;
}

static void * dsp_log_capture_func(void * data)
{
	dsplog_cap_t * p_cap = (dsplog_cap_t *) data;

//	fprintf(stderr,"dsplog_capture_thread, tid = %ld\n", syscall(SYS_gettid));

	if (!p_cap) {
		return NULL;
	}
	if (dsplog_capture(p_cap->logdrv_fd, p_cap->log_buffer,
		p_cap->datax_method, p_cap->output_filename, p_cap->log_port) < 0) {
		fprintf(stderr, "dsplog_cap: capture failed. \n");
		exit(1);
	}
	fprintf(stderr, "dsplog_cap: capture thread finished.\n");
	return NULL;
}

static void * dsp_log_capture_sig_thread_func(void * context)
{
	int sig_caught;    /* signal caught       */
	sigset_t signal_mask;  // signals to block
//	fprintf(stderr,"dsplog_capture_sig_thread, tid = %ld\n", syscall(SYS_gettid));
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGINT);
	sigaddset (&signal_mask, SIGQUIT);
	sigaddset (&signal_mask, SIGTERM);
	sigwait (&signal_mask, &sig_caught);
	switch (sig_caught) {
	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
		// send exit event to MainLoop
		if (dsplog_debug.work_mode == 0) {
//			fprintf(stderr, "signal captured in sig thread!tell main process to exit\n");
			sem_post(&exit_main_sem);
		} else {
			fprintf(stderr, "dsplog_cap: parse mode force quit. \n");
			exit(1);
		}
		break;
	default:
		/* should normally not happen */
		fprintf(stderr, "\nUnexpected signal caught in sig thread%d\n",
			sig_caught);
		break;
	}
	return NULL;
}

/* Verify dsplog binary file to see whether the log file is correct, missing,
 * and consistent with the current dsplog can only be used when platform
 * is not boot yet, so that amba_debug can dump the dsplog buffer to verify. */
static int verify_dsplog_file(char * bin_file)
{
	int ret = 0;

#ifdef CONFIG_DSP_LOG_START_IAVMEM

	fprintf(stderr, "Cannot run VERIFY function due to the non-fixed DSP log buffer start address.\n");
	ret = -1;

#else
	FILE * fp;
	int length;
	u32 last_record_in_logfile;
	idsp_printf_base_t record;
	char cmdstr[256];
	u32 log_entry_max;
	int log_lines_diff;

	do {
		fp = fopen(bin_file, "r");
		if (!fp) {
			fprintf(stderr, "dsplog verify cannot open [%s].\n", bin_file);
			ret = -1;
			break;
		}

		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		if (length % IDSP_LOG_SIZE) {
			fprintf(stderr, "dsplog verify found file %s has size not multiple"
				" of 32, should have data missing\n", bin_file);
			ret = -1;
			break;
		}
		fseek(fp, 0-IDSP_LOG_SIZE, SEEK_END);
		if (fread(&record, 1, IDSP_LOG_SIZE, fp) != IDSP_LOG_SIZE) {
			fprintf(stderr, "dsplog verify read file error.\n");
			ret = -1;
			break;
		}
		fclose(fp);
		fp = NULL;
		last_record_in_logfile = record.seq_num;

		fprintf(stderr, "last record in log file = %d \n", last_record_in_logfile);
		//now check dsp log mem buffer to find which is latest one

		//dump the dsplog mem to file

		sprintf(cmdstr, "amba_debug -r 0x%x -s 0x%x -f /tmp/.dsplogmem.bin",
		dsplog_mem.mem_phys_addr, dsplog_mem.mem_size);
		if (system(cmdstr) < 0) {
			ret = -1;
			perror("amba_debug");
			break;
		}

		fp = fopen("/tmp/.dsplogmem.bin", "r");
		if (!fp) {
			fprintf(stderr, "dsplog verify cannot open log mem file.\n");
			ret = -1;
			break;
		}

		log_entry_max = 0 ;
		while (fread(&record, 1, IDSP_LOG_SIZE, fp)) {
			if (record.seq_num > log_entry_max) {
				log_entry_max = record.seq_num;
			} else {
				break;
			}
		}
		fclose(fp);
		fp = NULL;
		unlink("/tmp/.dsplogmem.bin");

		fprintf(stderr, "log_entry_max = %d\n", log_entry_max);

		log_lines_diff =  log_entry_max -  last_record_in_logfile;
		if (last_record_in_logfile == log_entry_max) {
			fprintf(stderr, "dsplog verify PERFECT!\n");
		} else if (log_lines_diff < 10) {
			fprintf(stderr, "dsplog verify result GOOD, only a few lines (%d)"
				" of logs missing. \n", log_lines_diff );
		} else {
			fprintf(stderr, "dsplog verify result BAD, some lines (%d) of logs"
				" are missing. \n", log_lines_diff);
		}
	}while(0);

	if (fp) {
		fclose(fp);
	}

#endif

	return ret;
}

int main(int argc, char **argv)
{
	int iav_fd = -1;
	int dsp_log_drv_fd = -1;
	char * dsp_log_buffer=NULL;
	int datax_method;
	u8 * pcode;
	u8 * pmemd;
	u8 * pmdxf;
	pthread_t  sig_thread;
	pthread_t cap_thread;
	dsplog_cap_t  log_cap;
	int ret = 0;
	int sem_created = 0;
//	fprintf(stderr, "dsplog main tid = %d \n", getpid());

#ifdef ENABLE_RT_SCHED
	struct sched_param param;
	param.sched_priority = 50;
	if (sched_setscheduler(0, SCHED_RR, &param) < 0)
		perror("sched_setscheduler");
#endif

	do {
		if (init_param(argc, argv) < 0) {
			usage();
			ret = -1;
			break;
		}
		preload_dsplog_mem(&dsplog_mem);

		if (dsplog_debug.show_ucode_flag) {
			char tmp_str[STR_LEN];
			get_ucode_version(tmp_str, STR_LEN);
			fprintf(stderr, "Current ucode version is %s.\n", tmp_str);
			ret = 0;
			break;
		}

		if (dsplog_debug.verify_flag) {
			ret = verify_dsplog_file(dsplog_debug.verify);
			break;
		}

		if (dsplog_debug.show_version_flag) {
			fprintf(stderr, "dsplog tool version is %s.\n", DSPLOG_VERSION_STRING);
			ret = 0;
			break;
		}

		if (dsplog_debug.capture_current_flag) {
			char cmdstr[STR_LEN];
			sprintf(cmdstr, "amba_debug -r 0x%x -s 0x%x -f /tmp/dspmem_dump.bin",
				dsplog_mem.mem_phys_addr, dsplog_mem.mem_size);
			if (system(cmdstr) < 0) {
				perror("amba_debug");
				ret = -1;
				break;
			} else {
				fprintf(stderr, "log got and save to /tmp/dspmem_dump.bin, "
					"please check it.\n");
				ret = 0;
				break;
			}
		}

		if (dsplog_debug.work_mode == DSPLOG_CAPTURE_MODE) {
			//capture log mode
			//block signals
			block_signals();

			//init sem
			if(sem_init(&exit_main_sem, 0, 0) < 0) {
				fprintf(stderr, "dsplog_cap: create sem for exit failed.\n");
				ret =  -1;
				break;
			} else {
				sem_created = 1;
			}

			//create sig thread
			if (pthread_create(&sig_thread, NULL,
					dsp_log_capture_sig_thread_func, &log_cap)!=0) {
				fprintf(stderr, "dsplog_cap: create sig thread failed.\n");
				ret = -1;
				break;
			}

			if (!dsplog_debug.capture_only_flag) {
				if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
					perror("/dev/iav");
					ret = -1;
					break;
				}

				if (dsplog_setup(iav_fd, &dsplog_debug) < 0 ) {
					fprintf(stderr, "dsplog_cap: dsplog_setup failed \n");
					ret = -1;
					break;
				} else {
					close(iav_fd);
				}
			} else {
				fprintf(stderr, "dsplog_cap: capture only mode, do not send 'set debug level and bitmask' \n");
			}

			//open dsplog driver
			if ((dsp_log_drv_fd = open("/dev/dsplog", O_RDONLY, 0)) < 0) {
				perror("cannot open /dev/dsplog");
				ret = -1;
				break;
			}

			//allocate mem for DSP log capture
			dsp_log_buffer = malloc(MAX_LOG_READ_SIZE);
			if (!dsp_log_buffer) {
				fprintf(stderr, "dsplog_cap: insufficient memory for cap buffer \n");
				ret = -1;
				break;
			}

			//enable dsplog  (TO remove to reduce complexity with Amba_debug ? )
			if (dsplog_enable() < 0) {
				ret = -1;
				break;
			}

			//start capture log
			if (!dsplog_debug.transfer_tcp_flag) {
				datax_method = TRANS_METHOD_NFS;
			} else {
				datax_method = TRANS_METHOD_TCP;
			}

			//prepare dsplog thread
			memset(&log_cap, 0, sizeof(log_cap));
			log_cap.datax_method = datax_method;
			log_cap.logdrv_fd = dsp_log_drv_fd;
			log_cap.log_buffer =  dsp_log_buffer;
			log_cap.log_port = DSP_LOG_PORT;
			log_cap.output_filename = dsplog_debug.output;
			if (pthread_create(&cap_thread, NULL, dsp_log_capture_func, &log_cap)!=0) {
				fprintf(stderr, "dsplog_cap: create cap thread failed \n");
				ret = -1;
				break;
			}

			show_quit();

			//wait for sig thread to give sem to stop capture & exit
			sem_wait(&exit_main_sem);

			//stop capture
			fprintf(stderr,"try to stop dsp log capture.\n");
			if (dsplog_capture_stop(log_cap.logdrv_fd) < 0) {
				fprintf(stderr, "dsplog cap stopped failed \n");
				ret = -1;
				break;
			}
			fprintf(stderr,"dsp log capture stopped OK.\n");
			if (pthread_join(sig_thread, NULL)!=0) {
				perror("pthread_join sig thread! \n");
			}
			if (pthread_join(cap_thread, NULL)!=0) {
				perror("pthread_join cap thread! \n");
			}
			fprintf(stderr, "dsplog_cap: capture done and main thread exit!\n");

		} else {
			/*********************************************************/
			//DSP log parse mode
			//mmap ucode binary to memory for parsing
			if (!(pcode = mmap_ucode_bin(dsplog_debug.ucode, dsplog_mem.core_file))) {
				fprintf(stderr,"mmap ucode bin %s failed \n", dsplog_mem.core_file);
				ret = -1;
				break;
			}
			if (!(pmemd =  mmap_ucode_bin(dsplog_debug.ucode, dsplog_mem.memd_file))) {
				fprintf(stderr,"mmap ucode bin %s failed \n", dsplog_mem.memd_file);
				ret = -1;
				break;
			}

			if  (!(pmdxf =  mmap_ucode_bin(dsplog_debug.ucode, dsplog_mem.mdxf_file))){
				fprintf(stderr,"mmap ucode bin %s failed \n", dsplog_mem.mdxf_file);
				ret = -1;
				break;
			}

			if (dsplog_parse(dsplog_debug.parse_in, pcode, pmdxf, pmemd,
				dsplog_debug.parse_out) <  0) {
				fprintf(stderr,"dsplog_cap: parse dsp log failed \n");
				ret = -1;
				break;
			}
		}

	}while(0);

	if (sem_created)
		sem_destroy(&exit_main_sem);

	if (dsp_log_buffer) {
		free (dsp_log_buffer);
		dsp_log_buffer = NULL;
	}

        return ret;
}

