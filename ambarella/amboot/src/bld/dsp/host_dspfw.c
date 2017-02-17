/**
 * prebuild/sys_data/preload_idsp/host_dspfw.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <dsp/dspfw.h>

#if 0
#define DSP_HOST_DBG(format, arg...)	fprintf(stdout, format , ## arg)
#else
#define DSP_HOST_DBG(format, arg...)
#endif
#define DSP_HOST_ERR(format, arg...)	fprintf(stderr, format , ## arg)

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define	BIN_ALIAN(x)		(((x) + 3) & ~3);

struct bmp_file_header
{
	unsigned short bfType;
	unsigned int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int bfOffBits;
} __attribute__((packed));

struct bmp_info_header
{
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMerer;
	unsigned int biYPelsPerMerer;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
}__attribute__((packed));

/* ==========================================================================*/
static int parse_file_type(const char *line, char *filetype)
{
	int n = 0;

	*filetype = '\0';

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Check for comment */
	if (*line == '#' || *line == '\0')
		return 0;

	if (*line++ != '[')
		return -1;

	/* Get 'binname' */
	while (*line != '\0' && !isspace(*line) && *line != '#' && *line != ']') {
		*filetype++ = *line++;
	}

	if (*line++ != ']')
		return -1;

	*filetype = '\0';

	return 1;
}

static int parse_line(const char *line, char *binname,
		int bin_argc, char bin_argv[][512])
{
	int i, j;

	*binname = '\0';

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Check for comment */
	if (*line == '#' || *line == '\0')
		return 0;

	/* Get 'binname' */
	while (*line != '\0' && !isspace(*line) && *line != '#') {
		*binname++ = *line++;
	}
	*binname = '\0';

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Check for comment */
	if (*line == '#' || *line == '\0')
		return 1;

	/* Get 'argument' */
	i = 0;
	while (i < bin_argc) {
		j = 0;
		while (*line != '\0' && !isspace(*line) && *line != '#') {
			bin_argv[i][j++] = *line++;
		}
		bin_argv[i++][j] = '\0';

		/* Skip white spaces */
		while (*line != '\0' && isspace(*line)) {
			line++;
		}

		if ((*line == '#' || *line == '\0') && i < bin_argc)
			return -1;
	}

	/* Consume rest of line */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	if (*line != '\0' && *line != '#') {
		return -1;	/* Line has more than allowed # of args */
	}

	return 2;
}

static int fill_bin_file(FILE *foutput, FILE *fbin,
		struct ucode_file_info *ucode_info, char *binname)
{
	int rval = 0;
	size_t tmp_size;
	unsigned char buf[1024];

	snprintf(ucode_info->name, sizeof(ucode_info->name), "%s", binname);
	ucode_info->size = 0;
	ucode_info->offset = ftell(foutput);
	ucode_info->offset = BIN_ALIAN(ucode_info->offset);
	rval = fseek(foutput, ucode_info->offset, SEEK_SET);
	if (rval < 0) {
		goto fill_bin_file_exit;
	}

	do {
		tmp_size = fread(buf, 1, sizeof(buf), fbin);
		if (tmp_size <= 0) {
			rval = tmp_size;
			break;
		}
		rval = fwrite(buf, 1, tmp_size, foutput);
		if (rval == tmp_size)
			ucode_info->size += tmp_size;
		else
			rval = -1;
	} while (rval > 0);

	DSP_HOST_DBG("%s[%d @ 0x%08X]\n", ucode_info->name,
		ucode_info->size, ucode_info->offset);

fill_bin_file_exit:
	return rval;
}

static unsigned int rgb2yuv(unsigned char *rgb_clut)
{
	unsigned int table;
	unsigned char y, cb, cr;
	int R, G, B, Alpha;

	B = rgb_clut[0];
	G = rgb_clut[1];
	R = rgb_clut[2];
	Alpha = 255;

	y = (77 * R + 150 * G + 29 * B) / 256;
	cb = 128 - (44 * R + 87 * G - 131 * B) / 256;
	cr = 128 + (131 * R - 110 * G - 21 * B) / 256;

	table = (Alpha << 24) + (y << 16) + (cb << 8) + cr;

	return table;
}

static int fill_bmp_file(FILE *foutput, FILE *fbin,
		struct splash_file_info *splash_info, char *binname)
{
	struct bmp_file_header filehdr;
	struct bmp_info_header infohdr;
	unsigned int yuv_clut[SPLASH_CLUT_SIZE];
	unsigned char *buf, *out_buf;
	int size, i, flip, rval = 0;

	splash_info->size = 0;
	splash_info->offset = ftell(foutput);
	splash_info->offset = BIN_ALIAN(splash_info->offset);
	rval = fseek(foutput, splash_info->offset, SEEK_SET);
	if (rval < 0) {
		goto bmp_exit0;
	}

	rval = fread(&filehdr, 1, sizeof(struct bmp_file_header), fbin);
	if (rval < 0)
		goto bmp_exit0;

	if (filehdr.bfType != 0x4d42) {
		DSP_HOST_ERR("%s is not bmp file [0x%04X]!\n",
				binname, filehdr.bfType);
		rval = -1;
		goto bmp_exit0;
	}

	DSP_HOST_DBG("%s: file_size = %d\n", binname, filehdr.bfSize);
	DSP_HOST_DBG("%s: offset = %d\n", binname, filehdr.bfOffBits);
	if (filehdr.bfSize <= filehdr.bfOffBits) {
		DSP_HOST_ERR("%s: Invalid bfSize/bfOffbits: %d, %d!\n",
				binname, filehdr.bfSize, filehdr.bfOffBits);
		rval = -1;
		goto bmp_exit0;
	}

	buf = malloc(filehdr.bfSize - filehdr.bfOffBits);
	out_buf = malloc(filehdr.bfSize - filehdr.bfOffBits);
	if (buf == NULL || out_buf == NULL) {
		rval = -1;
		goto bmp_exit0;
	}

	rval = fread(&infohdr, 1, sizeof(struct bmp_info_header), fbin);
	if (rval < 0)
		goto bmp_exit1;

	DSP_HOST_DBG("%s: bmp_infoheader_size = %d\n", binname, infohdr.biSize);
	if (infohdr.biSize != 40) {
		DSP_HOST_ERR("%s: this bmp file is not supported: %d!\n",
				binname, infohdr.biSize);
		rval = -1;
		goto bmp_exit1;
	}

	DSP_HOST_DBG("%s: width = %d\n", binname, infohdr.biWidth);
	splash_info->width = infohdr.biWidth;
	if (splash_info->width <= 0 || (splash_info->width % 32) != 0) {
		DSP_HOST_ERR("%s: BMP width must be 32x: %d\n",
				binname, splash_info->width);
		rval = -1;
		goto bmp_exit1;
	}

	DSP_HOST_DBG("%s: height = %d\n", binname, infohdr.biHeight);
	flip = infohdr.biHeight < 0 ? 0 : 1;
	splash_info->height = infohdr.biHeight < 0 ?  -infohdr.biHeight : infohdr.biHeight;
	if (splash_info->height <= 0 || (splash_info->height % 4) != 0) {
		DSP_HOST_ERR("%s: BMP height must be 4x: %d\n",
				binname, splash_info->height);
		rval = -1;
		goto bmp_exit1;
	}

	DSP_HOST_DBG("%s: bits_per_pixel = %d\n", binname, infohdr.biBitCount);
	if (infohdr.biBitCount != 8) {
		DSP_HOST_ERR("%s: Only BMP 256 is supported: %d!\n",
			binname, infohdr.biBitCount);
		rval = -1;
		goto bmp_exit1;
	}

	DSP_HOST_DBG("%s: compression = %d\n", binname, infohdr.biCompression);
	if (infohdr.biCompression != 0) {
		DSP_HOST_ERR("%s: Compression is not supported: %d!\n",
			binname, infohdr.biCompression);
		rval = -1;
		goto bmp_exit1;
	}

	DSP_HOST_DBG("%s: used_colors = %d\n", binname, infohdr.biClrUsed);
	DSP_HOST_DBG("%s: important_colors = %d\n", binname, infohdr.biClrImportant);

	size = filehdr.bfOffBits - sizeof(filehdr) - sizeof(infohdr);
	rval = fread(buf, 1, size, fbin);
	if (rval <= 0)
		goto bmp_exit1;

	/* convert the rgb_clut to yuv_clut, then store it */
	for (i = 0; i < SPLASH_CLUT_SIZE; i++)
		yuv_clut[i] = rgb2yuv(buf + i * 4);

	rval = fwrite(yuv_clut, 1, sizeof(yuv_clut), foutput);
	if (rval != sizeof(yuv_clut))
		goto bmp_exit1;

	splash_info->size += sizeof(yuv_clut);

	rval = fread(buf, 1, filehdr.bfSize - filehdr.bfOffBits, fbin);
	if (rval <= 0)
		goto bmp_exit1;

	for (i = 0; i < splash_info->height; i++) {
		unsigned char *ptr;
		if (flip)
			ptr = out_buf + splash_info->width * (splash_info->height - i - 1);
		else
			ptr = out_buf + splash_info->width * i;

		memcpy(ptr, buf + splash_info->width * i, splash_info->width);
	}

	rval = fwrite(out_buf, 1, filehdr.bfSize - filehdr.bfOffBits, foutput);
	if (rval != (filehdr.bfSize - filehdr.bfOffBits))
		goto bmp_exit1;

	splash_info->size += (filehdr.bfSize - filehdr.bfOffBits);

	DSP_HOST_DBG("%s[%d @ 0x%08X]:[%d x %d]\n", binname,
		splash_info->size, splash_info->offset,
		splash_info->width, splash_info->height);

bmp_exit1:
	free(buf);
	free(out_buf);
bmp_exit0:
	if (rval < 0)
		DSP_HOST_ERR("%s: Error!!! (%d)\n", binname, rval);

	return rval;
}

static int process_bin_file(FILE *fp, FILE *foutput, struct dspfw_header *hdr,
		char *path, int lineno)
{
	FILE *fp_bin;
	char buf[512], binfile[512], binname[512];
	int rval;

	for (; fgets(buf, sizeof(buf), fp) != NULL; lineno++) {
		rval = parse_line(buf, binname, 0, NULL);
		if (rval < 0) {
			DSP_HOST_ERR("line %d looks suspicious! ignored...\n",
				lineno);
			continue;
		}
		if (rval == 0) {
			/* Line is pure comment */
			continue;
		}
		if (hdr->total_dsp >= ARRAY_SIZE(hdr->ucode)) {
			DSP_HOST_ERR("Too many ucode files! ignore[%s]\n", binname);
			continue;
		}

		sprintf(binfile, "%s/%s", path, binname);
		fp_bin = fopen(binfile, "rb");
		if (fp_bin == NULL) {
			DSP_HOST_ERR("Error: unable to open '%s'\n", binfile);
			return -ENOENT;
		}

		DSP_HOST_DBG("UCODE: process[%s]\n", binfile);
		rval = fill_bin_file(foutput, fp_bin, &hdr->ucode[hdr->total_dsp], binname);
		if (rval < 0) {
			DSP_HOST_ERR("Error: fill_bin_file(%s) = %d\n",
				binfile, rval);
			return -1;
		}

		hdr->total_dsp++;
		fclose(fp_bin);
	}

	return 0;
}

static int process_idspcfg_file(FILE *fp, FILE *foutput, struct dspfw_header *hdr,
		char *path, int lineno)
{
	FILE *fp_bin;
	char buf[512], binfile[512], binname[512], binargv[2][512];
	int tmp_size, idspcfg_header, idspcfg_offset;
	int n ,rval;

	for (n = 0; n < hdr->total_dsp; n++) {
		if (!strcmp(hdr->ucode[n].name, "default_binary.bin"))
			break;
	}

	if (n >= hdr->total_dsp) {
		DSP_HOST_ERR("Cannot find default_binary.bin!\n");
		return -ENOENT;
	}

	for (; fgets(buf, sizeof(buf), fp) != NULL; lineno++) {
		rval = parse_line(buf, binname, 2, binargv);
		if (rval < 0) {
			DSP_HOST_ERR("Error on IDSPCFG line %d!\n", lineno);
			rval = -EINVAL;
			goto idspcfg_exit;
		}

		if (rval > 0)
			break;
	}

	idspcfg_header = strtoul(binargv[0], NULL, 0);
	if (idspcfg_header < 0) {
		DSP_HOST_ERR("Error: invalid idspcfg_header: %s\n", binargv[0]);
		rval = -EINVAL;
		goto idspcfg_exit;
	}

	idspcfg_offset = strtoul(binargv[1], NULL, 0);
	if (idspcfg_offset < 0) {
		DSP_HOST_ERR("Error: invalid idspcfg_offset: %s\n", binargv[1]);
		rval = -EINVAL;
		goto idspcfg_exit;
	}

	rval = fseek(foutput, hdr->ucode[n].offset + idspcfg_offset, SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("%d: Error: fseek = %d\n", __LINE__, rval);
		goto idspcfg_exit;
	}

	sprintf(binfile, "%s/%s", path, binname);
	DSP_HOST_DBG("IDSPCFG: process[%s]\n", binfile);

	fp_bin = fopen(binfile, "rb");
	if (fp_bin == NULL) {
		DSP_HOST_ERR("Error: unable to open '%s'\n", binfile);
		rval = -ENOENT;
		goto idspcfg_exit;
	}

	rval = fseek(fp_bin, idspcfg_header, SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("%d: Error: fseek = %d\n", __LINE__, rval);
		goto idspcfg_exit;
	}

	do {
		tmp_size = fread(buf, 1, sizeof(buf), fp_bin);
		if (tmp_size <= 0) {
			rval = tmp_size;
			goto idspcfg_exit;
		}

		rval = fwrite(buf, 1, tmp_size , foutput);
		if (rval != tmp_size) {
			DSP_HOST_ERR("%d: Error: fwrite = %d\n", __LINE__, rval);
			rval = -EIO;
			goto idspcfg_exit;
		}
	} while (rval > 0);

idspcfg_exit:
	fseek(foutput, 0, SEEK_END);
	fclose(fp_bin);
	return rval;
}

static int process_splash_file(FILE *fp, FILE *foutput, struct dspfw_header *hdr,
		char *path, int lineno)
{
	FILE *fp_bin;
	char buf[512], binfile[512], binname[512];
	int rval;

	for (; fgets(buf, sizeof(buf), fp) != NULL; lineno++) {
		rval = parse_line(buf, binname, 0, NULL);
		if (rval < 0) {
			DSP_HOST_ERR("line %d looks suspicious! ignored...\n",
				lineno);
			continue;
		}
		if (rval == 0) {
			/* Line is pure comment */
			continue;
		}
		if (hdr->total_logo >= ARRAY_SIZE(hdr->logo)) {
			DSP_HOST_ERR("Too many logo files! ignore[%s]\n", binname);
			continue;
		}

		sprintf(binfile, "%s/%s", path, binname);
		fp_bin = fopen(binfile, "rb");
		if (fp_bin == NULL) {
			DSP_HOST_ERR("Error: unable to open '%s'\n", binfile);
			return -ENOENT;
		}

		DSP_HOST_DBG("LOGO: process[%s]\n", binfile);
		rval = fill_bmp_file(foutput, fp_bin, &hdr->logo[hdr->total_logo], binname);
		if (rval < 0) {
			DSP_HOST_ERR("Error: fill_bmp_file(%s) = %d\n",
				binfile, rval);
			return -1;
		}

		hdr->total_logo++;
		fclose(fp_bin);
	}

	return 0;
}

/* ==========================================================================*/
int main(int argc, char **argv)
{
	struct dspfw_header hdr;
	FILE *fp, *foutput;
	char *p, path[512], buf[512], filetype[512];
	int lineno, n, rval = 0;

	if (argc < 3) {
		DSP_HOST_ERR("Usage: %s [dsp.bin] [file list ...]\n", argv[0]);
		rval = -EINVAL;
		goto main_exit;
	}

	memset(&hdr, 0, sizeof(struct dspfw_header));

	foutput = fopen(argv[1], "wb");
	if (foutput == NULL) {
		DSP_HOST_ERR("Unable to open '%s' for output!\n", argv[1]);
		rval = -EINVAL;
		goto main_exit;
	}

	rval = fseek(foutput, sizeof(struct dspfw_header), SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("fseek(%s) = %d\n", argv[1], rval);
		goto main_exit;
	}

	DSP_HOST_DBG("foutput[%ld:%lu:%lu]\n", ftell(foutput),
		sizeof(struct dspfw_header), sizeof(struct ucode_file_info));

	for (n = 2; n < argc; n++) {
		fp = fopen(argv[n], "r");
		if (fp == NULL) {
			DSP_HOST_ERR("Unable to open '%s' for input!\n", argv[n]);
			rval = -ENOENT;
			goto main_exit;
		}

		strcpy(path, argv[n]);
		p = strrchr(path, '/');
		*p = '\0';

		for (lineno = 1; fgets(buf, sizeof(buf), fp); lineno++) {
			rval = parse_file_type(buf, filetype);
			if (rval < 0) {
				DSP_HOST_ERR("%s: line %d: Cannot find file type!\n",
					argv[n], lineno);
				return rval;
			}

			/* find the file type successfully */
			if (rval > 0)
				break;
		}

		if (!strncmp(filetype, "bin", 3) || !strncmp(filetype, "ucode", 5)
			|| !strncmp(filetype, "fastosd", 7)) {
			rval = process_bin_file(fp, foutput, &hdr, path, lineno);
		} else if (!strncmp(filetype, "idspcfg", 7)) {
			rval = process_idspcfg_file(fp, foutput, &hdr, path, lineno);
		} else if (!strncmp(filetype, "splash", 6)) {
			rval = process_splash_file(fp, foutput, &hdr, path, lineno);
		}
		else{
			DSP_HOST_ERR("Unknown file type: %s!\n", filetype);
			rval = -EINVAL;
			goto main_exit;
		}
	}

	hdr.magic = DSPFW_IMG_MAGIC;
	fseek(foutput, 0, SEEK_END);
	hdr.size = ftell(foutput);

	rval = fseek(foutput, 0, SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("Error: HEAD fseek(%s) = %d\n", argv[1], rval);
		goto main_exit;
	}
	rval = fwrite(&hdr, 1, sizeof(struct dspfw_header), foutput);
	if (rval != sizeof(struct dspfw_header)) {
		DSP_HOST_ERR("Error: HEAD fwrite(%s) = %d\n", argv[1], rval);
		goto main_exit;
	}

	rval = 0;

main_exit:
	if (foutput != NULL)
		fclose(foutput);

	return rval;
}

