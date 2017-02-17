/**
 * bld/cmd_fdt.c
 *
 * History:
 *    2013/08/15 - [Cao Rongrong] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#include <bldfunc.h>
#include <libfdt.h>

/*===========================================================================*/
#define MAX_LEVEL		32
#define MAX_DATA_LENGTH		1024

/*===========================================================================*/
static int cmd_fdt_print_prop(const void *fdt, int offset)
{
	const struct fdt_property *fdt_prop;
	const char *pathp;
	int len, i, p;
	u32 *data;

	fdt_prop = fdt_offset_ptr(fdt, offset, sizeof(*fdt_prop));
	pathp = fdt_string(fdt, fdt32_to_cpu(fdt_prop->nameoff));
	len = fdt32_to_cpu(fdt_prop->len);

	if (len == 0) {
		/* the property has no value */
		putstr(pathp);
		putstr("\r\n");
	} else {
		putstr(pathp);
		putstr(" = ");

		p = 1;
		for (i = 0; i < len - 1; i++) {
			if (!isprint(fdt_prop->data[i])) {
				p = 0;
				break;
			}
		}

		if (p) {
			putstr(fdt_prop->data);
		} else {
			data = (u32 *)fdt_prop->data;
			for (i = 0; i < len / 4; i++) {
				putstr("0x");
				puthex(fdt32_to_cpu(data[i]));
				putstr(" ");
			}
		}

		putstr("\r\n");
	}

	return 0;
}

static void cmd_fdt_show_header(const void *fdt)
{
	putstr("magic:\t\t\t0x");
	puthex(fdt_magic(fdt));
	putstr("\r\n");
	putstr("totalsize:\t\t");
	putdec(fdt_totalsize(fdt));
	putstr("\r\n");
	putstr("off_dt_struct:\t\t0x");
	putdec(fdt_off_dt_struct(fdt));
	putstr("\r\n");
	putstr("off_dt_strings:\t\t0x");
	putdec(fdt_off_dt_strings(fdt));
	putstr("\r\n");
	putstr("off_mem_rsvmap:\t\t0x");
	putdec(fdt_off_mem_rsvmap(fdt));
	putstr("\r\n");
	putstr("version:\t\t");
	putdec(fdt_version(fdt));
	putstr("\r\n");
	putstr("last_comp_version:\t");
	putdec(fdt_last_comp_version(fdt));
	putstr("\r\n");
	putstr("size_dt_struct:\t\t0x");
	putdec(fdt_size_dt_struct(fdt));
	putstr("\r\n");
	putstr("number mem_rsv:\t\t0x");
	putdec(fdt_num_mem_rsv(fdt));
	putstr("\r\n\r\n");
}

static int cmd_fdt_getnode(const void *fdt, const char *path, int print_prop)
{
	int offset, nextoffset, level;
	u32 tag;

	offset = fdt_path_offset(fdt, path);
	if (offset < 0) {
		putstr("libfdt fdt_path_offset() error: ");
		putstr(fdt_strerror(offset));
		putstr("\r\n");
		return -1;
	}

	level = 0;
	while(level >= 0) {
		char buf[64];

		tag = fdt_next_tag(fdt, offset, &nextoffset);

		switch(tag) {
		case FDT_BEGIN_NODE:
			if (print_prop)
				putstr("\r\n");
			fdt_get_path(fdt, offset, buf, sizeof(buf));
			putstr(buf);
			putstr("\r\n");

			level++;
			if (level >= MAX_LEVEL) {
				putstr("Man, too deep!\r\n");
				return -1;
			}
			break;

		case FDT_END_NODE:
			level--;
			if (level == 0)
				level = -1;	/* exit the loop */
			break;

		case FDT_PROP:
			if (print_prop)
				cmd_fdt_print_prop(fdt, offset);
			break;

		case FDT_NOP:
			putstr("NOP tag\r\n");
			break;

		case FDT_END:
			return 0;

		default:
			putstr("Unknown tag 0x");
			puthex(tag);
			putstr("\r\n");
			return -1;
		}
		offset = nextoffset;
	}

	putstr("\r\n");

	return 0;
}

static int cmd_fdt_parse_prop(int argc, char *argv[], char *data, int *len)
{
	u32 i, val;

	if (!strcmp(argv[0], "bool")) {
		if (!strcmp(argv[1], "0"))
			*len = -1; /* -1 means to remove property */
		else if (!strcmp(argv[1], "1"))
			*len = 0;
		else
			return -1;
	} else if (!strcmp(argv[0], "hex")) {
		*len = 0;
		for (i = 0; i < argc - 1; i++) {
			if (strtou32(argv[1 + i], &val) < 0)
				return -1;

			*(u32 *)data = cpu_to_fdt32(val);
			data += 4;
			*len += 4;

			BUG_ON(*len > MAX_DATA_LENGTH);
		}
	} else if (!strcmp(argv[0], "byte")) {
		*len = 0;
		for (i = 0; i < argc - 1; i++) {
			if (strtou32(argv[1 + i], &val) < 0)
				return -1;

			*(u8 *)data = val & 0xff;
			data += 1;
			*len += 1;

			BUG_ON(*len > MAX_DATA_LENGTH);
		}
	} else if (!strcmp(argv[0], "str")) {
		strfromargv(data, MAX_DATA_LENGTH, argc - 1, &argv[1]);
		*len = strlen(data) + 1;
	} else {
		putstr("Invalid data type: bool, hex, byte, str\r\n\r\n");
		return -1;
	}

	return 0;
}

static int cmd_fdt_setprop(void *fdt, int argc, char *argv[])
{
	const char *path = argv[0];
	const char *prop = argv[1];
	char data[MAX_DATA_LENGTH];
	int rval, offset, len = 0;

	rval = cmd_fdt_parse_prop(argc - 2, &argv[2], data, &len);
	if (rval < 0)
		return rval;

	offset = fdt_path_offset(fdt, path);
	if (offset < 0) {
		putstr("libfdt fdt_path_offset() error: ");
		putstr(fdt_strerror(offset));
		putstr("\r\n");
		return -1;
	}

	if (len == -1) {
		rval = fdt_delprop(fdt, offset, prop);
		if (rval < 0) {
			putstr("libfdt fdt_delprop() error: ");
			putstr(fdt_strerror(rval));
			putstr("\r\n");
			return -1;
		}
	} else {
		rval = fdt_setprop(fdt, offset, prop, data, len);
		if (rval < 0) {
			putstr("libfdt fdt_setprop() error: ");
			putstr(fdt_strerror(rval));
			putstr("\r\n");
			return -1;
		}
	}

	return 0;
}

static int cmd_fdt(int argc, char *argv[])
{
	void *fdt;
	const char *path;
	int rval = 0, update = 0;

	fdt = (void *)bld_hugebuf_addr;

	/*read DTB from the address to fdt*/
	rval = flprog_get_dtb((void *)fdt);
	if (rval < 0) {
		putstr("Get dtb failed\r\n");
		return rval;
	}

	if (strcmp(argv[1], "header") == 0) {
		cmd_fdt_show_header(fdt);
	} else if (strcmp(argv[1], "getnode") == 0) {
		if (argc > 2)
			path = argv[2];
		else
			path = "/";
		cmd_fdt_getnode(fdt, path, 0);
	} else if (strcmp(argv[1], "getprop") == 0) {
		if (argc > 2)
			path = argv[2];
		else
			path = "/";
		cmd_fdt_getnode(fdt, path, 1);
	} else if (strcmp(argv[1], "setprop") == 0) {
		if (argc < 5)
			return -1;

		rval = cmd_fdt_setprop(fdt, argc - 2, &argv[2]);
		if (rval < 0)
			return rval;
		update = 1;
	} else if (strcmp(argv[1], "cmdline") == 0) {
		char cmdline[MAX_CMDLINE_LEN];
		if (argc > 2) {
			strfromargv(cmdline, sizeof(cmdline), argc - 2, &argv[2]);
			rval = fdt_update_cmdline(fdt, cmdline);
			if (rval < 0)
				return rval;
			update = 1;
		} else {
			cmd_fdt_getnode(fdt, "/chosen", 1);
		}
	} else {
		return -3;
	}

	if (update)
		rval = flprog_set_dtb(fdt, fdt_totalsize(fdt), 1);

	return rval;
}

/*===========================================================================*/
static char help_fdt[] =
	"fdt utility commands\r\n"
	"fdt header                                - show fdt header\r\n"
	"fdt cmdline [cmdline]                     - set cmdline\r\n"
	"fdt getnode [path]                        - get node path\r\n"
	"fdt getprop [path]                        - get node path and property\r\n"
	"fdt setprop [path] [prop] [TYPE] [value]  - set property value\r\n"
	"e.g.:\r\n"
	"     fdt setprop [path] [prop] bool 0|1\r\n"
	"     fdt setprop [path] [prop] hex  0x12345678\r\n"
	"     fdt setprop [path] [prop] byte 02 11 22 0xab\r\n"
	"     fdt setprop [path] [prop] str  string\r\n";
__CMDLIST(cmd_fdt, "fdt", help_fdt);

