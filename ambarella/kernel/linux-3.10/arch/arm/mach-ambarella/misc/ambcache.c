/*
 * arch/arm/plat-ambarella/misc/ambcache.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>

#include <asm/cacheflush.h>
#ifdef CONFIG_CACHE_PL310
#include <asm/hardware/cache-l2x0.h>
#endif
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <plat/ambcache.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
#define CACHE_LINE_SIZE		32
#define CACHE_LINE_MASK		~(CACHE_LINE_SIZE - 1)

/* ==========================================================================*/
#ifdef CONFIG_OUTER_CACHE
#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
static u32 cache_l2_status = 0;
#endif
#ifdef CONFIG_CACHE_PL310
static void __iomem *ambcache_l2_base = __io(AMBARELLA_VA_L2CC_BASE);
#endif
#endif

#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
static int cache_check_start = 1;
module_param(cache_check_start, int, 0644);
static int cache_check_end = 0;
module_param(cache_check_end, int, 0644);
static int cache_check_fail_halt = 0;
module_param(cache_check_fail_halt, int, 0644);
#endif

/* ==========================================================================*/
void ambcache_clean_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
	if (cache_check_start && (vstart != (u32)addr)) {
		if (cache_check_fail_halt) {
			panic("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		} else {
			pr_warn("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		}
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		if (cache_check_fail_halt) {
			panic("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		} else {
			pr_warn("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		}
	}
#endif
#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_clean_range(pstart, (pstart + size));
#endif
}
EXPORT_SYMBOL(ambcache_clean_range);

void ambcache_inv_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
	if (cache_check_start && (vstart != (u32)addr)) {
		if (cache_check_fail_halt) {
			panic("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		} else {
			pr_warn("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		}
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		if (cache_check_fail_halt) {
			panic("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		} else {
			pr_warn("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		}
	}
#endif
#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);
	outer_inv_range(pstart, (pstart + size));
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_inv_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif
}
EXPORT_SYMBOL(ambcache_inv_range);

void ambcache_flush_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
	if (cache_check_start && (vstart != (u32)addr)) {
		if (cache_check_fail_halt) {
			panic("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		} else {
			pr_warn("%s start:0x%08x vs 0x%08x\n",
				__func__, vstart, (u32)addr);
		}
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		if (cache_check_fail_halt) {
			panic("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		} else {
			pr_warn("%s end:0x%08x vs 0x%08x\n",
				__func__, vend, ((u32)addr + size));
		}
	}
#endif
#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

	outer_flush_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#else
	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c14, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif
}
EXPORT_SYMBOL(ambcache_flush_range);

void ambcache_pli_range(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
#if __LINUX_ARM_ARCH__ >= 7
		__asm__ __volatile__ (
			"pli [%0]" : : "r" (addr_tmp));
#elif __LINUX_ARM_ARCH__ >= 5
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c13, 1" : : "r" (addr_tmp));
#else
#error "PLI not supported"
#endif
	}
}
EXPORT_SYMBOL(ambcache_pli_range);

#ifdef CONFIG_OUTER_CACHE

static u32 setup_l2_ctrl(void)
{
	u32 ctrl = 0;

#ifdef CONFIG_CACHE_PL310_FULL_LINE_OF_ZERO
	ctrl |= (0x1 << L2X0_AUX_CTRL_FULL_LINE_OF_ZERO_SHIFT);
#endif
#if (CHIP_REV == A8)
	ctrl |= (0x1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT);
#elif (CHIP_REV == S2L)
	ctrl |= (0x0 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT);
#elif (CHIP_REV == S2E) || (CHIP_REV == S3)
	ctrl |= (0x1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);
	ctrl |= (0x3 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT);
	ctrl |= (0x1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT);
#else
	ctrl |= (0x1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);
	ctrl |= (0x2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);
#endif
	ctrl |= (0x1 << L2X0_AUX_CTRL_CR_POLICY_SHIFT);
#ifdef CONFIG_CACHE_PL310_EARLY_BRESP
	ctrl |= (0x1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT);
#endif

	return ctrl;
}

static void setup_l2_prefetch_ctrl(void)
{
	u32 ctrl = 0;

	ctrl = readl(ambcache_l2_base + L2X0_PREFETCH_CTRL);
	ctrl &= ~L2X0_PREFETCH_CTRL_PREFETCH_OFFSET_MASK;
	ctrl |= (CONFIG_CACHE_PL310_PREFETCH_OFFSET <<
		 L2X0_PREFETCH_CTRL_PREFETCH_OFFSET_SHIFT);
#ifdef CONFIG_CACHE_PL310_DOUBLE_LINEFILL
	ctrl |= (1 << L2X0_PREFETCH_CTRL_DOUBLE_LINEFILL_SHIFT);
#endif
	writel(ctrl, ambcache_l2_base + L2X0_PREFETCH_CTRL);
}

/*
 * The FULL LINE OF ZERO feature mandates the following programming sequence
 *
 *   1. Enable the feature in PL310 controller
 *   2. Enable PL310 controller
 *   3. Enable the feature in Cortex AUX CONTROL co-processor register
 *
 * So this function should be called only after lx20_init(), and since it's
 * a per-core setting.  This bit should be enabled on all the cores.
 */
static void setup_full_line_of_zero(void *dummy)
{
#ifdef CONFIG_CACHE_PL310_FULL_LINE_OF_ZERO
	u32 val;

	/* read aux control register */
	asm volatile( "mrc	p15, 0, %0, c1, c0, 1" : "=r" (val));

	/* set Full Line of Zero to 1 */
	val |= 0x8;

        /* write aux control register */
	asm volatile( "mcr	p15, 0, %0, c1, c0, 1" : : "r" (val));
#endif
}


/* ==========================================================================*/
static int ambcache_l2_init = 0;
void ambcache_l2_enable_raw()
{
	if (outer_is_enabled())
		return;

#ifdef CONFIG_CACHE_PL310
	if (ambcache_l2_init == 0) {
		u32 ctrl = setup_l2_ctrl();
#if (CHIP_REV == S2E) || (CHIP_REV == S3)
		writel(0x00000222, (ambcache_l2_base + L2X0_TAG_LATENCY_CTRL));
		writel(0x00000222, (ambcache_l2_base + L2X0_DATA_LATENCY_CTRL));
#endif
		setup_l2_prefetch_ctrl();
		l2x0_init(ambcache_l2_base, ctrl, L2X0_AUX_CTRL_MASK);
		on_each_cpu(setup_full_line_of_zero, (void*)0, 1);
		ambcache_l2_init = 1;
	} else
#endif
		outer_enable();
}
EXPORT_SYMBOL(ambcache_l2_enable_raw);

void ambcache_l2_disable_raw()
{
	flush_cache_all();
	outer_flush_all();
	outer_disable();
	outer_inv_all();
	flush_cache_all();
}
EXPORT_SYMBOL(ambcache_l2_disable_raw);

int ambcache_l2_enable()
{
	ambcache_l2_enable_raw();
	return outer_is_enabled() ? 0 : -1;
}
EXPORT_SYMBOL(ambcache_l2_enable);

int ambcache_l2_disable()
{
	int ret_val = 0;
#if (!defined(CONFIG_SMP) || defined(CONFIG_HOTPLUG_CPU))
	unsigned long flags;
#endif

	if (outer_is_enabled()) {
#if defined(CONFIG_SMP)
#if defined(CONFIG_HOTPLUG_CPU)
		disable_nonboot_cpus();
		local_irq_save(flags);
		ambcache_l2_disable_raw();
		local_irq_restore(flags);
		enable_nonboot_cpus();
#endif
#else
		local_irq_save(flags);
		ambcache_l2_disable_raw();
		local_irq_restore(flags);
#endif
	}
	if (outer_is_enabled())
		ret_val = -1;

	return ret_val;
}
EXPORT_SYMBOL(ambcache_l2_disable);

#if defined(CONFIG_AMBARELLA_SYS_CACHE_CALL)
/* =========================Debug Only========================================*/
int cache_l2_set_status(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_uint(val, kp);
	if (!ret) {
		if (cache_l2_status) {
			ret = ambcache_l2_enable();
		} else {
			ret = ambcache_l2_disable();
		}
	}

	return ret;
}

static int cache_l2_get_status(char *buffer, const struct kernel_param *kp)
{
	cache_l2_status = outer_is_enabled();

	return param_get_uint(buffer, kp);
}

static struct kernel_param_ops param_ops_cache_l2 = {
	.set = cache_l2_set_status,
	.get = cache_l2_get_status,
};
module_param_cb(cache_l2, &param_ops_cache_l2, &cache_l2_status, 0644);
#endif
#endif

