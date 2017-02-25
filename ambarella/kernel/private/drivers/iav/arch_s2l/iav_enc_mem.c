/*
 * iav_enc_mem.c
 *
 * History:
 *	2015/03/13 - [Zhaoyang Chen] created file
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

#include <config.h>
#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/mman.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <plat/ambcache.h>
#include <mach/hardware.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"


void handle_mem_msg(struct ambarella_iav *iav, DSP_MSG *msg)
{
	struct iav_sub_mem_block *block = NULL;
	frame_buf_info_msg_t *fbp_msg = (frame_buf_info_msg_t *)msg;

	spin_lock(&iav->iav_lock);
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_RAW)) {
		if ((!fbp_msg->raw_start_daddr) ||
			(fbp_msg->raw_size == 0)) {
			iav_error("DSP failed to report raw addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW];
			block->phys = fbp_msg->raw_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->raw_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->raw_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_RAW);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_MAIN_YUV)) {
		if ((!fbp_msg->main_cap_start_daddr) ||
			(fbp_msg->main_cap_size == 0)) {
			iav_error("DSP failed to report main yuv addr, addr=0x%x, size=%d!\n",
				fbp_msg->main_cap_start_daddr, fbp_msg->main_cap_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_MAIN_YUV];
			block->phys = fbp_msg->main_cap_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->main_cap_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->main_cap_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_MAIN_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_MAIN_ME1)) {
		if ((!fbp_msg->main_cap_me1_start_daddr) ||
			(fbp_msg->main_cap_me1_size == 0)) {
			iav_error("DSP failed to report main me1 addr, addr=0x%x, size=%d!\n",
				fbp_msg->main_cap_me1_start_daddr, fbp_msg->main_cap_me1_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_MAIN_ME1];
			block->phys = fbp_msg->main_cap_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->main_cap_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->main_cap_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_MAIN_ME1);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_A_YUV)) {
		if ((!fbp_msg->prev_A_start_daddr) ||
			(fbp_msg->prev_A_size == 0)) {
			iav_error("DSP failed to report preview A yuv addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_A_start_daddr, fbp_msg->prev_A_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_A_YUV];
			block->phys = fbp_msg->prev_A_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_A_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_A_size + block->offset);
			iav->dsp_partition_mapped |=  (1 << IAV_DSP_SUB_BUF_PREV_A_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_A_ME1)) {
		if ((!fbp_msg->prev_A_me1_start_daddr) ||
			(fbp_msg->prev_A_me1_size == 0)) {
			iav_error("DSP failed to report prev A me1 addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_A_me1_start_daddr, fbp_msg->prev_A_me1_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_A_ME1];
			block->phys = fbp_msg->prev_A_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_A_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_A_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_PREV_A_ME1);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_B_YUV)) {
		if ((!fbp_msg->prev_B_start_daddr) ||
			(fbp_msg->prev_B_size == 0)) {
			iav_error("DSP failed to report preview B yuv addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_B_start_daddr, fbp_msg->prev_B_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_B_YUV];
			block->phys = fbp_msg->prev_B_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_B_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_B_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_PREV_B_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_B_ME1)) {
		if ((!fbp_msg->prev_B_me1_start_daddr) ||
			(fbp_msg->prev_B_me1_size == 0)) {
			iav_error("DSP failed to report preview B me1 addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_B_me1_start_daddr, fbp_msg->prev_B_me1_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_B_ME1];
			block->phys = fbp_msg->prev_B_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_B_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_B_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_PREV_B_ME1);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_C_YUV)) {
		if ((!fbp_msg->prev_C_start_daddr) ||
			(fbp_msg->prev_C_size == 0)) {
			iav_error("DSP failed to report preview C yuv addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_C_start_daddr, fbp_msg->prev_C_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_C_YUV];
			block->phys = fbp_msg->prev_C_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_C_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_C_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_PREV_C_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_PREV_C_ME1)) {
		if ((!fbp_msg->prev_C_me1_start_daddr) ||
			(fbp_msg->prev_C_me1_size == 0)) {
			iav_error("DSP failed to report preview C me1 addr, addr=0x%x, size=%d!\n",
				fbp_msg->prev_C_me1_start_daddr, fbp_msg->prev_C_me1_size);
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_PREV_C_ME1];
			block->phys = fbp_msg->prev_C_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->prev_C_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->prev_C_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_PREV_C_ME1);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_POST_MAIN_YUV)) {
		if ((!fbp_msg->warped_main_cap_start_daddr) ||
			(fbp_msg->warped_main_cap_size == 0)) {
			iav_error("DSP failed to report warpped main yuv addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_POST_MAIN_YUV];
			block->phys = fbp_msg->warped_main_cap_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->warped_main_cap_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->warped_main_cap_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_POST_MAIN_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_POST_MAIN_ME1)) {
		if ((!fbp_msg->warped_main_cap_me1_start_daddr) ||
			(fbp_msg->warped_main_cap_me1_size == 0)) {
			iav_error("DSP failed to report warpped main me1 addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_POST_MAIN_ME1];
			block->phys = fbp_msg->warped_main_cap_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->warped_main_cap_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->warped_main_cap_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_POST_MAIN_ME1);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_INT_MAIN_YUV)) {
		if ((!fbp_msg->warped_int_cap_start_daddr) ||
			(fbp_msg->warped_int_main_cap_size == 0)) {
			iav_error("DSP failed to report intermediate main yuv addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_INT_MAIN_YUV];
			block->phys = fbp_msg->warped_int_cap_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->warped_int_cap_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->warped_int_main_cap_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_INT_MAIN_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_EFM_YUV)) {
		if ((!fbp_msg->efm_start_daddr) ||
			(fbp_msg->efm_size == 0)) {
			iav_error("DSP failed to report efm yuv addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_EFM_YUV];
			block->phys = fbp_msg->efm_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->efm_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->efm_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_EFM_YUV);
		}
	}
	if (iav->dsp_partition_to_map & (1 << IAV_DSP_SUB_BUF_EFM_ME1)) {
		if ((!fbp_msg->efm_me1_start_daddr) ||
			(fbp_msg->efm_me1_size == 0)) {
			iav_error("DSP failed to report efm me1 addr!\n");
		} else {
			block = &iav->mmap_dsp[IAV_DSP_SUB_BUF_EFM_YUV];
			block->phys = fbp_msg->efm_me1_start_daddr & PAGE_MASK;
			block->offset = fbp_msg->efm_me1_start_daddr - block->phys;
			block->size = PAGE_ALIGN(fbp_msg->efm_me1_size + block->offset);
			iav->dsp_partition_mapped |= (1 << IAV_DSP_SUB_BUF_EFM_ME1);
		}
	}
	if (iav->dsp_partition_mapped != iav->dsp_partition_to_map) {
		iav_error("Failed to map specified dsp partitions, mapped:0x%x, map:0x%x!\n",
			iav->dsp_partition_mapped, iav->dsp_partition_to_map);
	}
	spin_unlock(&iav->iav_lock);

	wake_up_interruptible(&iav->dsp_map_wq);
}


int iav_map_dsp_partitions(struct ambarella_iav *iav)
{
	struct vin_video_format *format = NULL;
	void __iomem *base = NULL;
	u32 map_raw = 0;
	int rval = 0;

	if (likely(!iav->dsp_partition_to_map)) {
		iav->mmap[IAV_BUFFER_DSP].phys = iav->dsp->buffer_start;
		// exclude ucode size
		iav->mmap[IAV_BUFFER_DSP].size = iav->dsp->buffer_size;
		base = ioremap_nocache(iav->dsp->buffer_start, iav->dsp->buffer_size);
		if (!base) {
			iav_error("Failed to call ioremap() for IAV.\n");
			return -ENOMEM;
		}
		iav->mmap_dsp_base = (u32)base;
		iav->mmap[IAV_BUFFER_DSP].virt = iav->mmap_dsp_base;
	} else {
		// enable raw for 3A histogram
		format = iav->vinc[0]->dev_active->cur_format;
		if ((format->dual_gain_mode == 0) &&
			(format->hdr_mode == AMBA_VIDEO_INT_HDR_MODE)) {
			spin_lock_irq(&iav->iav_lock);
			iav->dsp_partition_to_map |= (1 << IAV_DSP_SUB_BUF_RAW);
			spin_unlock_irq(&iav->iav_lock);
			map_raw = 1;
			iav_debug("Enable raw mapping for AAA Histogram.\n");
		}
		cmd_get_frm_buf_pool_info(iav, NULL, iav->dsp_partition_to_map);
		rval = wait_event_interruptible_timeout(iav->dsp_map_wq,
			(iav->dsp_partition_to_map == iav->dsp_partition_mapped),
			TWO_JIFFIES);
		if (rval < 0) {
			iav_error("Failed to map all the dsp partitions!\n");
		} else if (map_raw) {
			base = ioremap_nocache(iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW].phys,
				iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW].size);
			if (!base) {
				iav_error("Failed to call ioremap() for IAV.\n");
				return -ENOMEM;
			}
			iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW].virt = (u32)base;
		}
	}

	return rval;
}

int iav_unmap_dsp_partitions(struct ambarella_iav *iav)
{
	void __iomem *base = NULL;
	int i;

	if (!iav->dsp_partition_mapped) {
		if (iav->mmap_dsp_base) {
			base = (void __iomem *)iav->mmap_dsp_base;
			iounmap(base);
			iav->mmap[IAV_BUFFER_DSP].phys = 0;
			iav->mmap[IAV_BUFFER_DSP].virt = 0;
			iav->mmap[IAV_BUFFER_DSP].size = 0;
			iav->mmap_dsp_base = 0;
		}
	} else {
		for (i = 0; i < IAV_DSP_SUB_BUF_NUM; ++i) {
			if (iav->dsp_partition_mapped & (1 << i)) {
				iounmap((void __iomem *)iav->mmap_dsp[i].virt);
				iav->mmap_dsp[i].phys = 0;
				iav->mmap_dsp[i].virt = 0;
				iav->mmap_dsp[i].size = 0;
				iav->dsp_partition_mapped &= ~(1 << i);
			}
		}
	}

	return 0;
}

static void iav_vm_open(struct vm_area_struct *area)
{
	struct ambarella_iav *iav = area->vm_private_data;
	iav->bsb_mmap_count++;
}

static void iav_vm_close(struct vm_area_struct *area)
{
	struct ambarella_iav *iav = area->vm_private_data;
	iav->bsb_mmap_count--;
}

static const struct vm_operations_struct iav_vm_ops = {
	.open = iav_vm_open,
	.close = iav_vm_close,
};

int iav_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ambarella_iav *iav = filp->private_data;
	u32 mem_addr, mem_size;
	unsigned long pfn, vma_size;
	int rval = 0, i, j = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	mem_addr = vma->vm_pgoff << PAGE_SHIFT;
	for (i = IAV_BUFFER_FIRST; i < IAV_BUFFER_LAST; ++i) {
		if ((mem_addr == iav->mmap[i].phys) && (iav->mmap[i].size > 0)) {
			mem_size = iav->mmap[i].size;

			/* Fixme: this is used to map one partition of PM to USER space,
			 * not all partitions
			 */
			if (mem_addr == iav->mmap[IAV_BUFFER_PM_BPC].phys ||
				mem_addr == (iav->mmap[IAV_BUFFER_BPC].phys + PAGE_SIZE)) {
				mem_size = ((get_pm_unit(iav) == IAV_PM_UNIT_PIXEL)
					? PM_BPC_PARTITION_SIZE : PM_MCTF_TOTAL_SIZE);
			}

			/* Fixme: this is used to map one partition of WARP to USER space,
			 * not all partitions
			 */
			if (mem_addr == iav->mmap[IAV_BUFFER_WARP].phys) {
				mem_size = WARP_VECT_PART_SIZE;
			}
			/* Fixme: this is used to map one partition of QP matrix to USER space
			 * not all partitions
			 */
			if (mem_addr == iav->mmap[IAV_BUFFER_QPMATRIX].phys) {
				mem_size = QP_MATRIX_SIZE;
			}
			break;
		}
	}
	if (i == IAV_BUFFER_LAST) {
		for (j = IAV_DSP_SUB_BUF_FIRST; j < IAV_DSP_SUB_BUF_LAST; ++j) {
			if ((mem_addr == iav->mmap_dsp[j].phys) && (iav->mmap_dsp[j].size > 0)) {
				mem_size = iav->mmap_dsp[j].size;
				break;
			}
		}
	}
	if (i == IAV_BUFFER_LAST && j == IAV_DSP_SUB_BUF_LAST) {
		iav_error("Incorrect memory address [0x%x] for MMAP.\n", mem_addr);
		return -EFAULT;
	}

	vma_size = vma->vm_end - vma->vm_start;
	if (vma_size > mem_size * 2)
		return -EINVAL;

	vma->vm_ops = &iav_vm_ops;
	vma->vm_private_data = iav;

	pfn = mem_addr >> PAGE_SHIFT;
	rval = remap_pfn_range(vma, vma->vm_start,
			pfn, mem_size, vma->vm_page_prot);
	if (rval) {
		iav_error("mmap failed with error %d.\n", rval);
		return rval;
	}

	/* it's allowed to ring-mmap to make it easier for user space access */
	if (vma_size > mem_size) {
		rval = remap_pfn_range(vma, vma->vm_start + mem_size,
				pfn, vma_size - mem_size, vma->vm_page_prot);
		if (rval) {
			iav_error("mmap failed with error %d.\n", rval);
			return rval;
		}
	}

	iav_vm_open(vma);

	return rval;
}


int iav_create_mmap_table(struct ambarella_iav *iav)
{
	u32 bsb_start = 0;
	u32 bsb_offset = 0;
	u32 total_map_size = 0;
	void __iomem *base = NULL;

	/* sanity check for BSB and IAV reserved memory */
	if (DSP_BSB_SIZE + DSP_IAVRSVD_SIZE < IAV_DRAM_MAX) {
		iav_warn("====== Pre-allocated BSB + IAV reserved memory [%d KB] is "
			"smaller than the required [%d KB].\n",
			(DSP_BSB_SIZE + DSP_IAVRSVD_SIZE) >> 10, IAV_DRAM_MAX >> 10);
	}

	iav->mmap[IAV_BUFFER_DSP].phys = iav->dsp->buffer_start;
	// exclude ucode size
	iav->mmap[IAV_BUFFER_DSP].size = iav->dsp->buffer_size;

	base = ioremap_nocache(iav->dsp->buffer_start, iav->dsp->buffer_size);
	if (!base) {
		iav_error("Failed to call ioremap() for IAV.\n");
		return -ENOMEM;
	}
	iav->mmap_dsp_base = (u32)base;
	iav->mmap[IAV_BUFFER_DSP].virt = iav->mmap_dsp_base;

	bsb_start = iav->dsp->bsb_start;
	total_map_size = DSP_BSB_SIZE + DSP_IAVRSVD_SIZE +
		IAV_DRAM_FB_DATA + IAV_DRAM_FB_AUDIO;
	base = ioremap_nocache(bsb_start, total_map_size);
	if (!base) {
		iav_error("Failed to call ioremap() for IAV.\n");
		return -ENOMEM;
	}
	iav->mmap_base = (u32)base;

	iav->mmap[IAV_BUFFER_BSB].phys = IAV_DRAM_BSB ? bsb_start : 0;
	iav->mmap[IAV_BUFFER_BSB].virt = IAV_DRAM_BSB ? (u32)base : 0;
	iav->mmap[IAV_BUFFER_BSB].size = IAV_DRAM_BSB;
	iav->bsb_free_bytes = iav->mmap[IAV_BUFFER_BSB].size;
	bsb_offset += iav->mmap[IAV_BUFFER_BSB].size;

	iav->mmap[IAV_BUFFER_USR].phys = IAV_DRAM_USR ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_USR].virt = IAV_DRAM_USR ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_USR].size = IAV_DRAM_USR;
	bsb_offset += iav->mmap[IAV_BUFFER_USR].size;

	iav->mmap[IAV_BUFFER_MV].phys = IAV_DRAM_MV ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_MV].virt = IAV_DRAM_MV ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_MV].size = IAV_DRAM_MV;
	bsb_offset += iav->mmap[IAV_BUFFER_MV].size;

	iav->mmap[IAV_BUFFER_OVERLAY].phys = IAV_DRAM_OVERLAY ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_OVERLAY].virt = IAV_DRAM_OVERLAY ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_OVERLAY].size = IAV_DRAM_OVERLAY;
	bsb_offset += iav->mmap[IAV_BUFFER_OVERLAY].size;

	iav->mmap[IAV_BUFFER_QPMATRIX].phys = IAV_DRAM_QPM ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_QPMATRIX].virt = IAV_DRAM_QPM ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_QPMATRIX].size = IAV_DRAM_QPM;
	bsb_offset += iav->mmap[IAV_BUFFER_QPMATRIX].size;

	iav->mmap[IAV_BUFFER_WARP].phys = IAV_DRAM_WARP ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_WARP].virt = IAV_DRAM_WARP ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_WARP].size = IAV_DRAM_WARP;
	bsb_offset += iav->mmap[IAV_BUFFER_WARP].size;

	iav->mmap[IAV_BUFFER_QUANT].phys = IAV_DRAM_QUANT ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_QUANT].virt = IAV_DRAM_QUANT ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_QUANT].size = IAV_DRAM_QUANT;
	bsb_offset += iav->mmap[IAV_BUFFER_QUANT].size;

	iav->mmap[IAV_BUFFER_IMG].phys = IAV_DRAM_IMG ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_IMG].virt = IAV_DRAM_IMG ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_IMG].size = IAV_DRAM_IMG;
	bsb_offset += iav->mmap[IAV_BUFFER_IMG].size;

	/* BPC and MCTF share the same buffer */
	iav->mmap[IAV_BUFFER_PM_BPC].phys = IAV_DRAM_PM ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_PM_BPC].virt = IAV_DRAM_PM ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_PM_BPC].size = IAV_DRAM_PM;
	iav->mmap[IAV_BUFFER_PM_MCTF].phys = IAV_DRAM_PM ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_PM_MCTF].virt = IAV_DRAM_PM ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_PM_MCTF].size = IAV_DRAM_PM;
	bsb_offset += IAV_DRAM_PM;

	iav->mmap[IAV_BUFFER_BPC].phys = IAV_DRAM_BPC ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_BPC].virt = IAV_DRAM_BPC ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_BPC].size = IAV_DRAM_BPC;
	bsb_offset += iav->mmap[IAV_BUFFER_BPC].size;

	iav->mmap[IAV_BUFFER_CMD_SYNC].phys = IAV_DRAM_CMD_SYNC ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_CMD_SYNC].virt = IAV_DRAM_CMD_SYNC ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_CMD_SYNC].size = IAV_DRAM_CMD_SYNC;
	bsb_offset += iav->mmap[IAV_BUFFER_CMD_SYNC].size;

	iav->mmap[IAV_BUFFER_VCA].phys = IAV_DRAM_VCA ? (bsb_start + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_VCA].virt = IAV_DRAM_VCA ? (u32)(base + bsb_offset) : 0;
	iav->mmap[IAV_BUFFER_VCA].size = IAV_DRAM_VCA;
	bsb_offset += iav->mmap[IAV_BUFFER_VCA].size;

	BUG_ON(bsb_offset > IAV_DRAM_MAX);

	iav->mmap[IAV_BUFFER_FB_DATA].phys =
		IAV_DRAM_FB_DATA ? (bsb_start + DSP_BSB_SIZE + DSP_IAVRSVD_SIZE) : 0;
	iav->mmap[IAV_BUFFER_FB_DATA].virt =
		IAV_DRAM_FB_DATA ? (u32)(base + DSP_BSB_SIZE + DSP_IAVRSVD_SIZE) : 0;
	iav->mmap[IAV_BUFFER_FB_DATA].size = IAV_DRAM_FB_DATA;

	iav->mmap[IAV_BUFFER_FB_AUDIO].phys =
		IAV_DRAM_FB_AUDIO ? (iav->mmap[IAV_BUFFER_FB_DATA].phys + DSP_FASTDATA_SIZE) : 0;
	iav->mmap[IAV_BUFFER_FB_AUDIO].virt =
		IAV_DRAM_FB_AUDIO ? (iav->mmap[IAV_BUFFER_FB_DATA].virt + DSP_FASTDATA_SIZE)  : 0;
	iav->mmap[IAV_BUFFER_FB_AUDIO].size = IAV_DRAM_FB_AUDIO;

	return 0;
}


