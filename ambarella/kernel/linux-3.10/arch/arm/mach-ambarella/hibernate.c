
/*
 * Hibernation support specific for ARM
 *
 * Derived from work on ARM hibernation support by:
 *
 * Ubuntu project, hibernation support for mach-dove
 * Copyright (C) 2010 Nokia Corporation (Hiroshi Doyu)
 * Copyright (C) 2010 Texas Instruments, Inc. (Teerth Reddy et al.)
 *  https://lkml.org/lkml/2010/6/18/4
 *  https://lists.linux-foundation.org/pipermail/linux-pm/2010-June/027422.html
 *  https://patchwork.kernel.org/patch/96442/
 *
 * Copyright (C) 2006 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * Copyright (C) 2015 Ambarella, Shanghai(Jorney Tu)
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/mm.h>
#include <linux/suspend.h>
#include <asm/system_misc.h>
#include <asm/idmap.h>
#include <asm/suspend.h>
#include <asm/memory.h>
#include <asm/sections.h>
#include <linux/mtd/mtd.h>

#define HIBERNATE_MTD_NAME  "swp"

static int mtd_page_offset = 0;

extern const void __nosave_begin, __nosave_end;

int pfn_is_nosave(unsigned long pfn)
{
	return 0;
}

void notrace save_processor_state(void)
{
	WARN_ON(num_online_cpus() != 1);
	local_fiq_disable();
}

void notrace restore_processor_state(void)
{
	local_fiq_enable();
}

/*
 * Snapshot kernel memory and reset the system.
 *
 * swsusp_save() is executed in the suspend finisher so that the CPU
 * context pointer and memory are part of the saved image, which is
 * required by the resume kernel image to restart execution from
 * swsusp_arch_suspend().
 *
 * soft_restart is not technically needed, but is used to get success
 * returned from cpu_suspend.
 *
 * When soft reboot completes, the hibernation snapshot is written out.
 */
static int notrace arch_save_image(unsigned long unused)
{
	int ret;

	ret = swsusp_save();
	if (ret == 0)
		soft_restart(virt_to_phys(cpu_resume));
	return ret;
}

/*
 * Save the current CPU state before suspend / poweroff.
 */
int notrace swsusp_arch_suspend(void)
{
	return cpu_suspend(0, arch_save_image);
}

/*
 * Restore page contents for physical pages that were in use during loading
 * hibernation image.  Switch to idmap_pgd so the physical page tables
 * are overwritten with the same contents.
 */
static void notrace arch_restore_image(void *unused)
{
	struct pbe *pbe;

	cpu_switch_mm(idmap_pgd, &init_mm);
	for (pbe = restore_pblist; pbe; pbe = pbe->next)
		copy_page(pbe->orig_address, pbe->address);

	soft_restart(virt_to_phys(cpu_resume));
}

static u64 resume_stack[PAGE_SIZE / 2 / sizeof(u64)] __nosavedata;

/*
 * Resume from the hibernation image.
 * Due to the kernel heap / data restore, stack contents change underneath
 * and that would make function calls impossible; switch to a temporary
 * stack within the nosave region to avoid that problem.
 */
int swsusp_arch_resume(void)
{
	extern void call_with_stack(void (*fn)(void *), void *arg, void *sp);
	call_with_stack(arch_restore_image, 0,
		resume_stack + ARRAY_SIZE(resume_stack));
	return 0;
}

struct mtd_info *mtd_probe_dev(void)
{
	struct mtd_info *info = NULL;
	info = get_mtd_device_nm(HIBERNATE_MTD_NAME);

	if(IS_ERR(info)){
		printk("SWP: mtd dev no found!\n");
		return NULL;
	}else{

		/* Makesure the swp partition has 64M at least */
		if(info->size < 0x4000000)
			return NULL;

		printk("MTD name: %s\n", 		info->name);
		printk("MTD size: 0x%llx\n", 	info->size);
		printk("MTD blocksize: 0x%x\n", info->erasesize);
		printk("MTD pagesize: 0x%x\n", 	info->writesize);
	}
	return info;
}


int hibernate_mtd_check(struct mtd_info *mtd, int ofs)
{

	int loff = ofs;
	int block = 0;

	while(mtd_block_isbad(mtd, loff) > 0){

		if(loff > mtd->size){
			printk("SWP: overflow mtd device ...\n");
			loff = 0;
			break;
		}

		printk("SWP: offset %d is a bad block\n" ,loff);

		block = loff / mtd->erasesize;
		loff = (block + 1) * mtd->erasesize;
	}
	return loff / PAGE_SIZE;
}


int hibernate_write_page(struct mtd_info *mtd, void *buf)
{

	int ret, retlen;
	int offset = 0;

	/* Default: The 1st 4k(one PAGE_SIZE) is empty in "swp" mtd partition */
	mtd_page_offset++;

#if 1 /* bad block checking is needed ? */
	offset = hibernate_mtd_check(mtd, mtd_page_offset * PAGE_SIZE);
#else
	offset = mtd_page_offset;
#endif

	if(offset == 0)
		return -EINVAL;

	ret = mtd_write(mtd, PAGE_SIZE * offset, PAGE_SIZE, &retlen, buf);
	if(ret < 0){
		printk("SWP: MTD write failed!\n");
		return -EFAULT;
	}

	mtd_page_offset = offset;
	return 0;
}

int hibernate_save_image(struct mtd_info *mtd, struct snapshot_handle *snapshot)
{

	int ret;
	int nr_pages = 0;

	while (1) {
		ret = snapshot_read_next(snapshot);
		if (ret <= 0)
			break;

		ret = hibernate_write_page(mtd, data_of(*snapshot));
		if (ret)
			break;

		nr_pages++;
	}

	if(!ret)
		printk("SWP: %d pages have been saved\n",  nr_pages);

	if(!nr_pages)
		ret = -EINVAL;

	return ret;
}

int hibernate_mtd_write(struct mtd_info *mtd)
{

	int error = 0;
	struct swsusp_info *header;
	struct snapshot_handle snapshot;

	memset(&snapshot, 0, sizeof(struct snapshot_handle));

	if(nr_meta_pages <= 0)
		return -EFAULT;

	error = snapshot_read_next(&snapshot);
	if (error < PAGE_SIZE) {
		if (error >= 0)
			error = -EFAULT;
		goto out_finish;
	}
	header = (struct swsusp_info *)data_of(snapshot);
	error = hibernate_write_page(mtd, header);

	if (!error) {
		error = hibernate_save_image(mtd, &snapshot);
	}

out_finish:
	return error;

}

int swsusp_write_mtd(int flags)
{

	struct mtd_info *info = NULL;

	mtd_page_offset = 0;

	info = mtd_probe_dev();
	if(!info)
		return -EFAULT;

	return hibernate_mtd_write(info);
}

