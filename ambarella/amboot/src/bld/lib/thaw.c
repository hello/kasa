/**
 * lib/thaw.c
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
#include <ambhw/nand.h>
#include <fio/ftl_const.h>
#include <vsprintf.h>
#include <ambhw/vic.h>
#include <ambhw/cache.h>

#define __NEW_UTS_LEN   	64
#define PAGE_SIZE       	0x1000
#define CPU_RESUME_MAIC     0x0badbeef
#define SWP_REVERVE_PAGE    1
#define THAW_MTD_NAME       "swp"

static int swp_start_blk = 0;
static int swp_total_blk = 0;
static int hibernate_div = 0;

struct new_utsname {
	char sysname[__NEW_UTS_LEN + 1];
	char nodename[__NEW_UTS_LEN + 1];
	char release[__NEW_UTS_LEN + 1];
	char version[__NEW_UTS_LEN + 1];
	char machine[__NEW_UTS_LEN + 1];
	char domainname[__NEW_UTS_LEN + 1];
};


struct swsusp_info {
	struct new_utsname	uts;
	u32			version_code;
	unsigned long		num_physpages;
	int			cpus;
	unsigned long		image_pages;
	unsigned long		pages;
	unsigned long		size;
	unsigned long		magic;
	unsigned long		addr;
} __attribute__((aligned(PAGE_SIZE)));

int block_is_bad(unsigned int page)
{
	unsigned int block = swp_start_blk + page / flnand.pages_per_block;

	while(nand_is_bad_block(block)){
		printf("page %d or block %d is a bad block\n", page, block);
		block++;
		page += flnand.pages_per_block;
	}

	return page;
}

int swp_mtd_read(unsigned int page, unsigned char *buf)
{
	int ofs, block, ret;

	block = page / flnand.pages_per_block;
	ofs   = page % flnand.pages_per_block;

	ret = nand_read_pages(swp_start_blk + block, ofs, hibernate_div, buf, NULL, 0);
	if(ret < 0){
		printf("Thaw: nand read page failed\n");
		return -1;
	}
	return 0;
}

int thaw_probe_mtd(void)
{
	int i;
	int ret = -1;
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if(strcmp(get_part_str(i), THAW_MTD_NAME))
			continue;

		swp_start_blk = flnand.sblk[i];
		swp_total_blk = flnand.nblk[i];

		ret = 0;
		break;
	}

	hibernate_div  = PAGE_SIZE / flnand.main_size ;

	if(PAGE_SIZE % flnand.main_size != 0
			|| hibernate_div <= 0){
		printf("Thaw: nand page size ...\n");
		ret = -1;
	}
	return ret;
}

static int thaw_load_header(struct swsusp_info *info, int flag)
{
	int page;
	unsigned char *buffer = NULL;

	buffer = malloc(PAGE_SIZE);
	if(buffer == NULL){
		printf("Thaw: page malloc failed !!\n");
		return -1;
	}

	page = SWP_REVERVE_PAGE * hibernate_div;

	if(page != block_is_bad(page))
		return -1;

	if(swp_mtd_read(page, buffer))
		return -1;

	memcpy(info, buffer, PAGE_SIZE);

	if(info->magic != CPU_RESUME_MAIC)
		return -1;

	if(flag){
		printf("sysname : %s\n", info->uts.sysname);
		printf("release : %s\n", info->uts.release);
		printf("version : %s\n", info->uts.version);
		printf("machine : %s\n", info->uts.machine);
		printf("image_pages : 0x%x\n", info->image_pages);
		printf("nr_meta_pages : %d\n", info->pages - info->image_pages - 1);
		printf("info_magic : 0x%x\n", info->magic);
		printf("jump addr : 0x%x\n", info->addr);
	}

	free(buffer);

	return 0;
}

static int _load_image(unsigned int addr, int nr_meta_pages)
{
	static int image_page_offset = 0;

	if(image_page_offset == 0)
		image_page_offset = hibernate_div * (nr_meta_pages + 1 + 1);

	image_page_offset = block_is_bad(image_page_offset);

	if(swp_mtd_read(image_page_offset, (unsigned char *)addr))
		return -1;

	image_page_offset += hibernate_div;

	return 0;
}

static void jump_to_resume(struct swsusp_info *info)
{
	unsigned int addr = info->addr;
	void (*jump)(void) = (void *)addr;
	void *ptr;

	if(info->magic !=  CPU_RESUME_MAIC){
		printf("Hibernate: Boot Magic error ...\n");
		return ;
	}


	printf("Start to run... %x\n", addr);
	_clean_flush_all_cache();
	_disable_icache();
	_disable_dcache();
	disable_interrupts();

	ptr = &&_return_;
	*(volatile u32 *)(DRAM_START_ADDR + 0x000ffffc) = (u32) ptr;

	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");

	jump ();


_return_:
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");

}
static int thaw_load_image(struct swsusp_info *info)
{
	int i, j;
	int nr_meta_pages;
	int page_start;
	unsigned char *buffer = NULL;
	unsigned int *addr = NULL;

	buffer = malloc(PAGE_SIZE);
	if(buffer == NULL){
		printf("Thaw: page malloc failed !!\n");
		return -1;
	}

	addr = (unsigned int *)buffer;

	nr_meta_pages = info->pages - info->image_pages - 1;

	/* swp partition:
	 *
	 * Kernel_Page_0 (0 - 1) is empty;
	 * Kernel_Page_1 (2 - 3) for header
	 * Kernel_Page_2 (4 - 5) for meta
	 * Kernel_Page_X ( ... ) for Kernel image
	   ...
	   */

	page_start = 2 * hibernate_div;

	for(i = 0; i < nr_meta_pages; i++){
		if(swp_mtd_read(page_start, buffer)){
			free(buffer);
			return -1;
		}

		for(j = 0; j < PAGE_SIZE / sizeof(int); j++){
			if(*(addr + j) == (~0UL))
				break;
			_load_image(*(addr + j), nr_meta_pages);
		}
		page_start += hibernate_div;
	}
	free(buffer);
	jump_to_resume(info);

	return 0;
}
int thaw_hibernation(void)
{
	struct swsusp_info info;

	if(thaw_probe_mtd())
		return 0;

	if(thaw_load_header(&info, 0))
		return 0;

	thaw_load_image(&info);
	return 0;

}
int thaw_info(void)
{
	struct swsusp_info info;

	if(thaw_probe_mtd())
		return 0;

	if(thaw_load_header(&info, 1))
		return 0;

	return 0;
}

