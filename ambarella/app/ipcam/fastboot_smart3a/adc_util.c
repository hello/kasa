/**
 * app/ipcam/fastboot_smart3a/adc_util.c
 *
 * Author: Caizhang Lin <czlin@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 */

#include <unistd.h>
#include <errno.h>
#include <linux/rtc.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>

#include "AmbaDataType.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "config.h"
#include "adc.h"
#include "upgrade_partition.h"
#include "upgrade_part_adc.h"

#include "iav_ioctl.h"
#include "adc_util.h"
#include "iav_fastboot.h"

#define UPDATED_ADC_IMAGE_PATH	"/tmp/updated_adc.bin"

static uint32_t idspcfg_dump_bin(int fd_iav, uint8_t *bin_buffer)
{
    idsp_config_info_t dump_idsp_info;
    int rval = -1;

    dump_idsp_info.id_section = 0;
    dump_idsp_info.addr = bin_buffer;

    rval = ioctl(fd_iav, IAV_IOC_IMG_DUMP_IDSP_SEC, &dump_idsp_info);
    if (rval < 0) {
        perror("IAV_IOC_IMG_DUMP_IDSP_SEC");
        return 0;
    }
    printf("idspcfg size=%u\n", dump_idsp_info.addr_long);
    return dump_idsp_info.addr_long;
}

static void rtc_read_tm(struct tm *ptm, int fd)
{
    memset(ptm, 0, sizeof(*ptm));
    ioctl(fd, RTC_RD_TIME, ptm);
    ptm->tm_isdst = -1; /* "not known" */
}

static int select_idsp_cfg_via_rtc_hour()
{
    struct tm tm_time;
    time_t timer;
    struct tm s_tblock = {0};
    struct tm *tblock = NULL;
    int fd = 0;
    char *oldtz = NULL;

    //get RTC time
    fd = open("/dev/rtc", O_RDONLY);
    if (fd < 0) {
        fd = open("/dev/rtc0", O_RDONLY);
        if (fd < 0) {
            fd = open("/dev/misc/rtc", O_RDONLY);
            if (fd < 0) {
                printf("Open RTC failed\n");
                return -1;
            }
        }
    }
    rtc_read_tm(&tm_time, fd);
    close(fd);

    //save old local time zone
    oldtz = getenv("TZ");

    //set local time zone to UTC
    putenv((char*)"TZ=UTC0");
    tzset();

    //get local time(now is UTC)
    timer = mktime(&tm_time);

    //parse local time(now is UTC)
    localtime_r(&timer, &s_tblock);

    //restore old local time zone
    unsetenv("TZ");
    if (oldtz) {
        putenv(oldtz - 3);
    }
    tzset();

    tblock = &s_tblock;
    if (tblock->tm_hour < 0 || tblock->tm_hour >= 24) {
        printf("RTC hour is invalid\n");
        return -1;
    } else {
        return tblock->tm_hour;
    }
}

// the first one is used for 3A parameters write back is required
static int select_idsp_cfg(enum idsp_cfg_select_policy policy)
{
    switch (policy) {
    case IDSP_CFG_SELECT_ONLY_ONE:
        return 0;
        break;

    case IDSP_CFG_SELECT_VIA_UTC_HOUR:
        return select_idsp_cfg_via_rtc_hour();
        break;

    case IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS:
        printf("Policy BRIGHTNESS not supported yet\n");
        return -1;
        break;

    default:
        printf("Invalid policy %d\n", policy);
        return -1;
        break;
    }
}

static int find_mtd_device_path(const char *dev_name, char *dev_info_buf,
                                int dev_info_bufsize)
{
    FILE *stream = 0;
    int device_index = -1;

    if (!dev_name || !dev_info_buf || 0 == dev_info_bufsize) {
        printf("Find mtd device path, NULL input\n");
        return -1;
    }
    memset(dev_info_buf, 0, dev_info_bufsize);

    //get mtd device index and path
    sprintf(dev_info_buf, "cat /proc/mtd | grep %s | cut -d':' -f1 | cut -d'd' -f2", dev_name);
    stream = popen(dev_info_buf , "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd  %s  failed.\n", dev_name);
        return -1;
    }
    fscanf(stream,"%d", &device_index);
    pclose(stream);

    if (device_index < 0) {
        printf("Not found %s partition on /proc/mtd\n", dev_name);
        return -1;
    }
    memset(dev_info_buf, 0, dev_info_bufsize);
    sprintf(dev_info_buf, "/dev/mtd%d", device_index);

    return 0;
}

static int load_adc_to_mem (uint32_t *adc_img_len,
                            uint32_t *adc_img_aligned_len, void **pp_adc_aligned_mem, int verbose)
{
    char dev_info_buf[128];
    int ret = 0, count = 0;
    int ptb_fd = 0, ptb_offset;
    uint8_t *ptb_buf = NULL;
    struct mtd_info_user ptb_meminfo;
    loff_t ptb_bad_offset;
    flpart_table_t *table;
    unsigned long long blockstart = 1;
    int fd = 0, bs, badblock = 0;
    struct mtd_info_user meminfo;
    unsigned long ofs;
    void *p_adc_mem_cur = NULL;

    if (!adc_img_len || !adc_img_aligned_len || !pp_adc_aligned_mem) {
        printf("Load ADC to mem, NULL input\n");
        ret = -1;
        goto closeall;
    }

    ret = find_mtd_device_path("ptb", dev_info_buf, sizeof(dev_info_buf));
    if (ret < 0) {
        printf("Find ptb partition failed\n");
        ret = -1;
        goto closeall;
    }

    /* Open the PTB device */
    if ((ptb_fd = open(dev_info_buf, O_RDONLY)) == -1) {
        perror("open PTB");
        ret = -1;
        goto closeall;
    }

    /* Fill in MTD device capability structure */
    if ((ret = ioctl(ptb_fd, MEMGETINFO, &ptb_meminfo)) != 0) {
        perror("PTB MEMGETINFO");
        ret = -1;
        goto closeall;
    }

    for (ptb_offset = 0; ptb_offset < ptb_meminfo.size; ptb_offset += ptb_meminfo.erasesize) {
        ptb_bad_offset = ptb_offset;
        if ((ret = ioctl(ptb_fd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
            perror("ioctl(MEMGETBADBLOCK)");
            goto closeall;
        }

        if (ret == 0) {
            break;
        }
    }
    if (ptb_offset >= ptb_meminfo.size) {
        printf("Can not find good block in PTB.\n");
        ret = -1;
        goto closeall;
    }

    ptb_buf = malloc(ptb_meminfo.erasesize);
    memset(ptb_buf, 0, ptb_meminfo.erasesize);

    /* Read partition table.
    * Note: we need to read and save the entire block data, because the
    * entire block will be erased when write partition table back to flash.
    * BTW, flpart_meta_t is located in the same block as flpart_table_t
    */
    count = ptb_meminfo.erasesize;
    if (pread(ptb_fd, ptb_buf, count, ptb_offset) != count) {
        perror("pread PTB");
        ret = -1;
        goto closeall;
    }

    table = PTB_TABLE(ptb_buf);
    *adc_img_len = table->part[PART_ADC].img_len;

    ret = find_mtd_device_path("adc", dev_info_buf, sizeof(dev_info_buf));
    if (ret < 0) {
        printf("Find ADC partition failed\n");
        ret = -1;
        goto closeall;
    }

    /* Open ADC device */
    if ((fd = open(dev_info_buf, O_RDONLY)) == -1) {
        perror("open mtd");
        ret = -1;
        goto closeall;
    }

    /* Fill in MTD device capability structure */
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
        perror("MEMGETINFO");
        ret = -1;
        goto closeall;
    }

    bs = meminfo.writesize;
    *adc_img_aligned_len = ((*adc_img_len)+(bs-1))&(~(bs-1));

    if (verbose) {
        printf("ADC img addr=%u, img len=%u, img aligned_len=%u\n",
            table->part[PART_ADC].mem_addr, *adc_img_len, *adc_img_aligned_len);
    }

    //will be freed when process return
    *pp_adc_aligned_mem = (void*)malloc(*adc_img_aligned_len);
    if (!(*pp_adc_aligned_mem)) {
        printf("Can not malloc memory for load ADC partiton!\n");
        ret = -1;
        goto closeall;
    }
    p_adc_mem_cur = *pp_adc_aligned_mem;

    if (verbose) {
        /* Print informative message */
        printf("Total size %u, Block size %u, page size %u, OOB size %u\n",
            meminfo.size,meminfo.erasesize, meminfo.writesize, meminfo.oobsize);
        printf("Loading data starting at 0x%08x and ending at 0x%08x\n",
            table->part[PART_ADC].mem_addr,
            table->part[PART_ADC].mem_addr+(*adc_img_aligned_len));
    }

    /* Load the flash contents */
    for (ofs = 0; ofs < *adc_img_aligned_len ; ofs+=bs) {
        // new eraseblock , check for bad block
        if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
            blockstart = ofs & (~meminfo.erasesize + 1);
            if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
                perror("ioctl(MEMGETBADBLOCK)");
                ret = -1;
                goto closeall;
            }
        }

        if (badblock) {
            //memset (p_adc_mem_cur, 0xff, bs);
            continue;
        } else {
            /* Read page data and exit on failure */
            if (pread(fd, p_adc_mem_cur, bs, ofs) != bs) {
                perror("pread");
                ret = -1;
                goto closeall;
            }
        }
        p_adc_mem_cur+=bs;
    }

    /* Exit happy */
    ret = 0;

closeall:
    if (ptb_buf) {
        free(ptb_buf);
        ptb_buf  = NULL;
    }

    if (ptb_fd) {
        close(ptb_fd);
        ptb_fd = 0;
    }
    if (fd) {
        close(fd);
        fd = 0;
    }

    return ret;
}

static int update_adc_partition(int verbose)
{
    char cmd[256];
    FILE   *stream = 0;
    int device_index = -1;

    memset(cmd, 0, sizeof(cmd));

    stream = popen( "cat /proc/mtd | grep adc | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd failed\n");
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found ADC partition on /proc/mtd\n");
        return -1;
    }
    //erase nand flash before re-write
    sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);

    if (verbose) {
        printf("CMD: %s\n", cmd);
    }
    system(cmd);
    if (upgrade_partition(device_index, UPDATED_ADC_IMAGE_PATH) != 0) {
        printf("upgrade_partition fail\n");
        return -1;
    }

    return 0;
}

static int get_reg_data(const uint16_t reg_addr, int fd_iav)
{
    struct vindev_reg reg;
    reg.vsrc_id = 0;
    reg.addr = reg_addr;
    int rval = ioctl(fd_iav, IAV_IOC_VIN_GET_REG, &reg);
    if (rval < 0) {
        printf("addr 0x%04x, read fail\n", reg_addr);
        return -1;
    }

    return reg.data;
}

int save_content_file(char *filename, char *content)
{
    FILE *fp;
    int ret = -1;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("fopen");
        return ret;
    }

    ret = fwrite(content, strlen(content), 1, fp);
    fclose(fp);
    fp = NULL;

    return ret;
}

static void read_awb_ae_config(struct smart3a_file_info *info, const char*  awb_filename, const char* ae_filename)
{
    FILE *fp_ae;
    FILE *fp_awb;
    char content[64];
    char *content_c = NULL;

    fp_awb = fopen(awb_filename, "r");
    if (fp_awb != NULL) {
        fseek(fp_awb, 0, SEEK_SET);
        memset(content, 0, sizeof(content));
        fread(content, sizeof(content) + 1, 1, fp_awb);
        fclose(fp_awb);
        fp_awb = NULL;

        content_c = strtok(content, ",");
        if (content_c) {
            info->r_gain = atoi(content_c);
        }
        content_c = strtok(NULL, ",");
        if (content_c) {
            info->b_gain = atoi(content_c);
        }
    }

    fp_ae = fopen(ae_filename, "r");
    if (fp_ae != NULL) {
        fseek(fp_ae, 0, SEEK_SET);
        memset(content, 0, sizeof(content));
        fread(content, sizeof(content) + 1, 1, fp_ae);
        fclose(fp_ae);
        fp_ae = NULL;

        content_c = strtok(content, ",");
        if (content_c) {
            info->d_gain= atoi(content_c);
        }
        content_c = strtok(NULL, ",");
        if (content_c) {
            info->shutter = atoi(content_c);
        }
        content_c = strtok(NULL, ",");
        if (content_c) {
            info->agc = atoi(content_c);
        }
    }

}


/************************************************
* external functions of adc_util
*************************************************/
static uint32_t adc_img_len_s = 0;   // Lengh of image in the partition
static uint32_t adc_img_aligned_len_s = 0;  //aligned to page size of adc parttiton
static void* p_adc_aligned_mem_s = NULL;   //aligned to page size of adc parttiton
struct adcfw_header * hdr_s = NULL;

// load adc to mem, return mem addr
int  adc_util_init(int verbose)
{
    int rval = 0;
    struct timeval time1 = {0, 0};
    struct timeval time2 = {0, 0};
    signed long long  time1_US = 0;
    signed long long  time2_US = 0;

    do {
        if (verbose) {
            gettimeofday(&time1, NULL);
        }
        //step 1: load whole adc partition to mem
        rval = load_adc_to_mem (&adc_img_len_s, &adc_img_aligned_len_s, &p_adc_aligned_mem_s, verbose);
        if (rval < 0) {
            printf("Load ADC to memory failed\n");
            break;
        }

        if (verbose) {
            gettimeofday(&time2, NULL);
            time1_US = ((signed long long)(time1.tv_sec) * 1000000 + (signed long long)(time1.tv_usec));
            time2_US = ((signed long long)(time2.tv_sec) * 1000000 + (signed long long)(time2.tv_usec));
            printf("[TIME]: <Load ADC partition to image> cost %lld US!\n", time2_US-time1_US);
        }

        hdr_s = (struct adcfw_header *)p_adc_aligned_mem_s;
        if (hdr_s->magic != ADCFW_IMG_MAGIC || hdr_s->fw_size != adc_img_len_s) {
            printf("Invalid ADC partition, magic=%u(should be %u), fw_size=%u(should be %u)\n",
                hdr_s->magic, ADCFW_IMG_MAGIC, hdr_s->fw_size, adc_img_len_s);
            rval = -1;
            break;
        }
    } while (0);

    return rval;
}

void  adc_util_finish()
{
    if (p_adc_aligned_mem_s) {
        free(p_adc_aligned_mem_s);
        p_adc_aligned_mem_s  = NULL;
    }
    hdr_s= NULL;
}

int adc_util_get_amboot_params(struct params_info* params)
{
    int rval = 0;
    do {
        if (NULL == hdr_s) {
            printf("adc_util not initialized!");
            rval =  -1;
            break;
        }
        *params = hdr_s->params_in_amboot;
    } while (0);
    return rval;
}

int adc_util_set_amboot_params(struct params_info* params)
{
    int rval = 0;
    do {
        if (NULL == hdr_s) {
            printf("adc_util not initialized!");
            rval =  -1;
            break;
        }
        hdr_s->params_in_amboot = *params;
    } while (0);
    return rval;
}

int adc_util_read_3a_params(int verbose)
{
    int rval = 0;
    int idsp_cfg_index = -1;
    enum idsp_cfg_select_policy policy = IDSP_CFG_SELECT_ONLY_ONE;
    uint32_t addr = 0, size = 0;
    char aaa_content[32];

    if (NULL == hdr_s) {
        printf("adc_util not initialized!");
        return -1;
    }
    do {
        //step 2: find the mem address of the selected section among adc image
        idsp_cfg_index = select_idsp_cfg(policy);
        if (idsp_cfg_index < 0 || idsp_cfg_index >= hdr_s->smart3a_num) {
            printf("Wrong idspcfg section via policy 0x%x, idspcfg index=%d, should in [0, %u)\n",
                policy, idsp_cfg_index, hdr_s->smart3a_num);
            rval = -1;
            break;
        }

        addr = (uint32_t)hdr_s + hdr_s->smart3a[idsp_cfg_index].offset;
        size = hdr_s->smart3a_size;

        if (size <= IDSPCFG_BINARY_HEAD_SIZE || 0 == addr) {
            printf("Invalid idspcfg section %d, addr=%u, size=%u!\n", idsp_cfg_index, addr, size);
            rval = -1;
            break;
        }

        printf("ADC 3A info: r_gain=%d, b_gain=%d. d_gain=%d, shutter=%d, agc=%d.\n",
             hdr_s->smart3a[idsp_cfg_index].r_gain, hdr_s->smart3a[idsp_cfg_index].b_gain,
             hdr_s->smart3a[idsp_cfg_index].d_gain, hdr_s->smart3a[idsp_cfg_index].shutter,
             hdr_s->smart3a[idsp_cfg_index].agc);

        memset(aaa_content, 0, sizeof(aaa_content));
        snprintf(aaa_content, sizeof(aaa_content), "%d,%d\n",
             hdr_s->smart3a[idsp_cfg_index].r_gain,
             hdr_s->smart3a[idsp_cfg_index].b_gain);

        save_content_file(PRELOAD_AWB_FILE, aaa_content);

        memset(aaa_content, 0, sizeof(aaa_content));
        snprintf(aaa_content, sizeof(aaa_content), "%d,%d,%d\n",
             hdr_s->smart3a[idsp_cfg_index].d_gain,
             hdr_s->smart3a[idsp_cfg_index].shutter,
             hdr_s->smart3a[idsp_cfg_index].agc);
        save_content_file(PRELOAD_AE_FILE, aaa_content);
    } while (0);

    return rval;
}


#define VIDEO_MODE_H264_1080P     0
#define VIDEO_MODE_H264_720P       1
#if defined(CONFIG_ISO_TYPE_ADVANCED)
struct idsp_cfg_file_info file_info[MAX_IMAGE_BINARY_NUM] = {
    {"01_cc_reg.bin",				2304},
    {"02_cc_3d_A.bin",				16384},
    {"03_cc_3d_C.bin",				0},
    {"04_cc_out.bin",				1024},
    {"05_thresh_dark.bin",			768},
    {"06_thresh_hot.bin",			768},
    {"07_local_exposure_gain.bin",		512},
    {"08_cmf_table.bin",			48},
    {"09_chroma_scale_gain.bin",		256},
    {"10_fir1_A.bin",				256},
    {"11_fir2_A.bin",				256},
    {"12_coring_A.bin",				256},
    {"13_fir1_B.bin",				256},
    {"14_fir2_B.bin",				256},
    {"15_coring_B.bin",				256},
    {"16_wide_chroma_mctf.bin",			528},
    {"17_fir1_motion_detection.bin",		256},
    {"18_fir1_motion_detection_map.bin",	256},
    {"19_coring_motion_detection_map.bin",	256},
    {"20_video_mctf.bin",			528},
    {"21_md_fir1.bin",				256},
    {"22_md_map_fir1.bin",			256},
    {"23_md_map_coring.bin",			256},
    {"24_cc_reg_combine.bin",			0},
    {"25_cc_3d_combine.bin",			0},
    {"26_thresh_dark_mo.bin",			0},
    {"27_thresh_hot_mo.bin",			0},
    {"28_cmf_table_mo.bin",			0},
    {"29_fir1_mo.bin",				0},
    {"30_fir2_mo.bin",				0},
    {"31_coring_mo.bin",			0},
};
#else
struct idsp_cfg_file_info file_info[MAX_IMAGE_BINARY_NUM] = {
    {"01_cc_reg.bin",				2304},
    {"02_cc_3d_A.bin",				16384},
    {"03_cc_3d_C.bin",				0},
    {"04_cc_out.bin",				1024},
    {"05_thresh_dark.bin",			768},
    {"06_thresh_hot.bin",			768},
    {"07_local_exposure_gain.bin",		512},
    {"08_cmf_table.bin",			48},
    {"09_chroma_scale_gain.bin",		256},
    {"10_fir1_A.bin",				256},
    {"11_fir2_A.bin",				256},
    {"12_coring_A.bin",				256},
    {"13_fir1_B.bin",				0},
    {"14_fir2_B.bin",				0},
    {"15_coring_B.bin",				0},
    {"16_wide_chroma_mctf.bin",			0},
    {"17_fir1_motion_detection.bin",		256},
    {"18_fir1_motion_detection_map.bin",	256},
    {"19_coring_motion_detection_map.bin",	256},
    {"20_video_mctf.bin",			528},
    {"21_md_fir1.bin",				256},
    {"22_md_map_fir1.bin",			256},
    {"23_md_map_coring.bin",			256},
    {"24_cc_reg_combine.bin",			0},
    {"25_cc_3d_combine.bin",			0},
    {"26_thresh_dark_mo.bin",			0},
    {"27_thresh_hot_mo.bin",			0},
    {"28_cmf_table_mo.bin",			0},
    {"29_fir1_mo.bin",				0},
    {"30_fir2_mo.bin",				0},
    {"31_coring_mo.bin",			0},
};
#endif

// in-memory update idsp config when in mode 4
static int adc_util_update_idsp_cfg_mode4(uint8_t *idspCfg_buffer, uint8_t section_index, int section_size, int base)
{
    uint32_t adc_idsp_mode4_addr = 0;
    int rval = -1;
    do {
        if (NULL == idspCfg_buffer) {
            break;
        }
        adc_idsp_mode4_addr = (uint32_t)hdr_s + hdr_s->smart3a[section_index].offset + base*IDSP_CFG_BIN_SIZE;
        memcpy((void *)adc_idsp_mode4_addr, (void *)idspCfg_buffer, section_size);
        rval = 0;
    } while (0);
    return rval;
}
static int adc_util_read_idspcfg_file(const char* dir,  uint8_t *dest_idspCfg_buffer, int idsp_cfg_index, int resolution_mode)
{
    FILE *f_idspCfg = NULL;
    int f = 0;
    char file_name_tmp[128];
    int rval = -1;
    uint8_t* tmp_buffer_p = dest_idspCfg_buffer;
    int base = 0;
    // if 720p, load the 2nd half of 3A config bin. 1080p and 720p idsp cfgs are packed in one bin
    if ( resolution_mode == VIDEO_MODE_H264_720P) {
        base = 1;
    }

    do {
        if (NULL == tmp_buffer_p) {
            break;
        }
        for (f = 0; f < MAX_IMAGE_BINARY_NUM; f++) {
            if (file_info[f].size == 0) {
                continue;
            }
            sprintf(file_name_tmp, "%s/%s", dir, file_info[f].name);
            f_idspCfg = fopen(file_name_tmp, "rb");
            if (f_idspCfg == NULL) {
                printf("Open %s failed\n", file_name_tmp);
                rval = -EINVAL;
                break;
            }

            int ret_size = fread(tmp_buffer_p, 1, file_info[f].size, f_idspCfg);
            if (ret_size != file_info[f].size) {
                printf("idspcfg dump bin size[%u] invalid, should be %u as specified\n", ret_size, file_info[f].size);
                rval = -EINVAL;
                break;
            }
            tmp_buffer_p += file_info[f].size;
            fclose(f_idspCfg);
        }
        if (rval == -EINVAL) {
            break;
        }
        rval= adc_util_update_idsp_cfg_mode4(dest_idspCfg_buffer, idsp_cfg_index, SIZE_IDSP_CFG_ADV_ISO, base);
        if (rval < 0) {
            printf("adc_util_update_idsp_cfg_mode4 idsp_params failed\n");
        }

        // read liso_cfg.bin, which contains paramters
        sprintf(file_name_tmp, "%s/%s", dir, IDSP_LISO_DUMP_FILE);
        f_idspCfg = fopen(file_name_tmp, "rb");
        if (f_idspCfg == NULL) {
            printf("Open %s failed\n", file_name_tmp);
            rval = -EINVAL;
            break;
        }

        // move tmp_buffer_p to point to the start of the buffer
        tmp_buffer_p = dest_idspCfg_buffer;
        int ret_size = fread(tmp_buffer_p, 1, SIZE_LISO_CFG_ADV_ISO, f_idspCfg);
        if (ret_size != SIZE_LISO_CFG_ADV_ISO) {
            printf("Low iso dump bin size[%u] invalid, should be %u as specified\n", ret_size, SIZE_LISO_CFG_ADV_ISO);
            rval = -EINVAL;
            break;
        }
        uint32_t adc_liso_cfg_addr = (uint32_t)hdr_s + hdr_s->smart3a[idsp_cfg_index].offset +  base*IDSP_CFG_BIN_SIZE + SIZE_IDSP_CFG_ADV_ISO;
        memcpy((void *)adc_liso_cfg_addr, (void *)tmp_buffer_p, SIZE_LISO_CFG_ADV_ISO);
    } while (0);

    if (f_idspCfg != NULL) {
        fclose(f_idspCfg);
        f_idspCfg = NULL;
    }

    return rval;
}

int adc_util_write(int verbose, int skip3a)
{
    int rval = -1;
    int fd_iav = -1;
    int value;
    int idsp_cfg_index = -1;
    uint32_t addr = 0, size = 0;
    uint32_t idsp_bin_size = 0;
    uint8_t *bin_buffer = NULL;
    FILE *f_updated_adc = NULL;
    enum idsp_cfg_select_policy policy = IDSP_CFG_SELECT_ONLY_ONE;
    struct iav_system_resource resource;
    struct vindev_devinfo vsrc_info;

    struct timeval time2 = {0, 0};
    struct timeval time3 = {0, 0};
    struct timeval time4 = {0, 0};
    struct timeval time5 = {0, 0};
    struct timeval time6 = {0, 0};
    struct timeval time7 = {0, 0};
    signed long long  time2_US = 0;
    signed long long  time3_US = 0;
    signed long long  time4_US = 0;
    signed long long  time5_US = 0;
    signed long long  time6_US = 0;
    signed long long  time7_US = 0;

    if (NULL == hdr_s) {
        printf("adc_util not initialized!");
        return -1;
    }
    do {
        if (verbose) {
            gettimeofday(&time2, NULL);
        }

        //step 2: find the mem address of the selected section among adc image
        if (!skip3a) {
            idsp_cfg_index = select_idsp_cfg(policy);
            if (idsp_cfg_index < 0 || idsp_cfg_index >= hdr_s->smart3a_num) {
                printf("Wrong idspcfg section via policy 0x%x, idspcfg index=%d, should in [0, %u)\n", policy, idsp_cfg_index, hdr_s->smart3a_num);
                break;
            }

            addr = (uint32_t)hdr_s + hdr_s->smart3a[idsp_cfg_index].offset;
            size = hdr_s->smart3a_size;

            if (size <= IDSPCFG_BINARY_HEAD_SIZE || 0 == addr) {
                printf("Invalid idspcfg section %d, addr=%u, size=%u!\n", idsp_cfg_index, addr, size);
                break;
            }

            if (verbose) {
                gettimeofday(&time3, NULL);
            }

            //step 3: dump 3a date to mem
            if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
                printf("open /dev/iav fail\n");
                break;
            }

            bin_buffer = (uint8_t*)malloc(MAX_DUMP_BUFFER_SIZE);
            if (!bin_buffer) {
                printf("Can not malloc idspcfg buffer\n");
                break;
            }
            memset(bin_buffer, 0, MAX_DUMP_BUFFER_SIZE);

            memset(&resource, 0, sizeof(resource));
            resource.encode_mode = DSP_CURRENT_MODE;
            if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
                perror("IAV_IOC_GET_SYSTEM_RESOURCE\n");
                break;
            }
            if (resource.encode_mode == DSP_ADVANCED_ISO_MODE) {
                // query buffer size
                struct iav_srcbuf_format  srcBufFormat;
                srcBufFormat.buf_id = 0;
                if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &srcBufFormat) < 0) {
                    perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT\n");
                }

                if (srcBufFormat.size.width == 1280 && srcBufFormat.size.height == 720) {
                    rval =adc_util_read_idspcfg_file(IDSP_LISO_DUMP_DIR, bin_buffer, idsp_cfg_index, VIDEO_MODE_H264_720P);
                } else if (srcBufFormat.size.width == 1920 && srcBufFormat.size.height == 1080) {
                    rval =adc_util_read_idspcfg_file(IDSP_LISO_DUMP_DIR, bin_buffer, idsp_cfg_index, VIDEO_MODE_H264_1080P);
                } else {
                    printf("Unknown resolution, should be 720p or 1080p\n");
                }

                if (rval < 0) {
                    printf("adc_util_read_idspcfg_file  failed\n");
                }
            } else {
                idsp_bin_size = idspcfg_dump_bin(fd_iav, bin_buffer);
                if (size != idsp_bin_size) {
                    printf("idspcfg dump bin size[%u] invalid, should be %u as saved in adcfw_header\n", idsp_bin_size, size);
                    break;
                }

#if 0
            FILE* f_idsp_cfg = NULL;
            f_idsp_cfg = fopen("/tmp/sdcard/idsp_cfg.bin", "wb");
            if (f_idsp_cfg == NULL) {
                printf("Open /tmp/sdcard/idsp_cfg.bin failed\n");
                rval = -EINVAL;
                break;
            }
            rval = fwrite(bin_buffer, 1, idsp_bin_size, f_idsp_cfg);
            fclose(f_idsp_cfg);
#endif

                //step 4: save 3a data to selected section of adc image
                memcpy((void *)addr, (void *)bin_buffer, size);//TODO:  how to check success

            }
	     if (verbose) {
                gettimeofday(&time4, NULL);
            }

            //step 5: dump and save shutter and gain params to selected section of adc image
            read_awb_ae_config(&(hdr_s->smart3a[idsp_cfg_index]), PRELOAD_AWB_FILE, PRELOAD_AE_FILE);

            vsrc_info.vsrc_id = 0;
            if (ioctl(fd_iav, IAV_IOC_VIN_GET_DEVINFO, &vsrc_info) < 0) {
                perror("IAV_IOC_VIN_GET_DEVINFO");
                break;
            }
            if (vsrc_info.sensor_id == SENSOR_IMX322) {
                value = get_reg_data(0x0202, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para0 = value;
                }

                value = get_reg_data(0x0203, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para1 = value;
                }
                value = get_reg_data(0x301E, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para2 = value;
                }
                printf("Read sensor IMX322 register: shutter0202=0x%04x, shutter0203=0x%04x, gain301E=0x%04x\n",
                     hdr_s->smart3a[idsp_cfg_index].para0,
                     hdr_s->smart3a[idsp_cfg_index].para1,
                     hdr_s->smart3a[idsp_cfg_index].para2);
            } else if (vsrc_info.sensor_id == SENSOR_OV4689) {
                value = get_reg_data(0x3500, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para0 = value;
                }

                value = get_reg_data(0x3501, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para1 = value;
                }
                value = get_reg_data(0x3502, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para2 = value;
                }
                value = get_reg_data(0x3508, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para3 = value;
                }
                value = get_reg_data(0x3509, fd_iav);
                if (value >= 0) {
                    hdr_s->smart3a[idsp_cfg_index].para4 = value;
                }
                printf("Read sensor OV4689 register: shutter3500=0x%04x, shutter3501=0x%04x, shutter3502=0x%04x," \
                    "  gain3508=0x%04x, gain3509=0x%04x\n",
                    hdr_s->smart3a[idsp_cfg_index].para0,
                    hdr_s->smart3a[idsp_cfg_index].para1,
                    hdr_s->smart3a[idsp_cfg_index].para2,
                    hdr_s->smart3a[idsp_cfg_index].para3,
                    hdr_s->smart3a[idsp_cfg_index].para4);
            } else {
                printf("This sensor type %d is not supported yet.\n", vsrc_info.sensor_id);
                break;
            }

            printf("Read AAA param: r_gain=%d, b_gain=%d, dgain=%d, shutter=%d, agc=%d\n",
                 hdr_s->smart3a[idsp_cfg_index].r_gain,
                 hdr_s->smart3a[idsp_cfg_index].b_gain,
                 hdr_s->smart3a[idsp_cfg_index].d_gain,
                 hdr_s->smart3a[idsp_cfg_index].shutter,
                 hdr_s->smart3a[idsp_cfg_index].agc);

            if (verbose) {
                gettimeofday(&time5, NULL);
            }
        }

        //step 6: save the whole updated adc image to file
        f_updated_adc = fopen(UPDATED_ADC_IMAGE_PATH, "wb");
        if (f_updated_adc == NULL) {
            printf("Open /tmp/updated_adc.bin failed\n");
            rval = -EINVAL;
            break;
        }

        rval = fwrite(p_adc_aligned_mem_s, 1, adc_img_aligned_len_s, f_updated_adc);
        if (rval != adc_img_aligned_len_s) {
            printf("/tmp/updated_adc.bin size [%d] is wrong, should be %u\n", rval, adc_img_aligned_len_s);
            break;
        }

        fclose(f_updated_adc);
        f_updated_adc = NULL;

        if (verbose) {
            gettimeofday(&time6, NULL);
        }

        //step 7: update adc partition with adc image file
        rval = update_adc_partition(verbose);
        if (rval != 0) {
            printf("update_adc_partition fail\n");
            break;
        }

        if (verbose) {
            gettimeofday(&time7, NULL);
            time2_US = ((signed long long)(time2.tv_sec) * 1000000 + (signed long long)(time2.tv_usec));
            time3_US = ((signed long long)(time3.tv_sec) * 1000000 + (signed long long)(time3.tv_usec));
            time4_US = ((signed long long)(time4.tv_sec) * 1000000 + (signed long long)(time4.tv_usec));
            time5_US = ((signed long long)(time5.tv_sec) * 1000000 + (signed long long)(time5.tv_usec));
            time6_US = ((signed long long)(time6.tv_sec) * 1000000 + (signed long long)(time6.tv_usec));
            time7_US = ((signed long long)(time7.tv_sec) * 1000000 + (signed long long)(time7.tv_usec));
            printf("[TIME]: <Find selected section of ADC image> cost %lld US!\n", time3_US-time2_US);
            printf("[TIME]: <Dump 3A date to mem> cost %lld US!\n", time4_US-time3_US);
            printf("[TIME]: <Dump&save shutter and gain params to selected section of ADC image> cost %lld US!\n", time5_US-time4_US);
            printf("[TIME]: <Save the updated ADC image to file> cost %lld US!\n", time6_US-time5_US);
            printf("[TIME]: <Update ADC partition from file> cost %lld US!\n", time7_US-time6_US);
        }
        rval = 0;
    } while (0);

    if (bin_buffer != NULL) {
        free(bin_buffer);
        bin_buffer = NULL;
    }

    if (f_updated_adc) {
        fclose(f_updated_adc);
        f_updated_adc = NULL;
    }

    if (fd_iav > 0) {
        close(fd_iav);
        fd_iav = -1;
    }
    return rval;
}
