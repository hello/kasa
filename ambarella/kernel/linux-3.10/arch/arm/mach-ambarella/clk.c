/*
 * arch/arm/mach-ambarella/clk.c
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <plat/clk.h>

/* ==========================================================================*/
static LIST_HEAD(ambarella_all_clocks);
DEFINE_SPINLOCK(ambarella_clock_lock);

/* ==========================================================================*/
static unsigned int ambarella_clk_ref_freq = REF_CLK_FREQ;

u32 ambarella_rct_find_pll_table_index(unsigned long rate, u32 pre_scaler,
	const struct pll_table *table, u32 table_size)
{
	u64 dividend;
	u64 divider;
	u32 start;
	u32 middle;
	u32 end;
	u32 index_limit;
	u64 diff = 0;
	u64 diff_low = 0xFFFFFFFFFFFFFFFF;
	u64 diff_high = 0xFFFFFFFFFFFFFFFF;

	pr_debug("pre_scaler = [0x%08X]\n", pre_scaler);

	dividend = rate;
	dividend *= pre_scaler;
	dividend *= (1000 * 1000 * 1000);
	divider = ambarella_clk_ref_freq / (1000 * 1000);
	AMBCLK_DO_DIV(dividend, divider);

	index_limit = (table_size - 1);
	start = 0;
	end = index_limit;
	middle = table_size / 2;
	while (table[middle].multiplier != dividend) {
		if (table[middle].multiplier < dividend) {
			start = middle;
		} else {
			end = middle;
		}
		middle = (start + end) / 2;
		if (middle == start || middle == end) {
			break;
		}
	}
	if ((middle > 0) && ((middle + 1) <= index_limit)) {
		if (table[middle - 1].multiplier < dividend) {
			diff_low = dividend - table[middle - 1].multiplier;
		} else {
			diff_low = table[middle - 1].multiplier - dividend;
		}
		if (table[middle].multiplier < dividend) {
			diff = dividend - table[middle].multiplier;
		} else {
			diff = table[middle].multiplier - dividend;
		}
		if (table[middle + 1].multiplier < dividend) {
			diff_high = dividend - table[middle + 1].multiplier;
		} else {
			diff_high = table[middle + 1].multiplier - dividend;
		}
		pr_debug("multiplier[%u] = [%llu]\n", (middle - 1),
			table[middle - 1].multiplier);
		pr_debug("multiplier[%u] = [%llu]\n", (middle),
			table[middle].multiplier);
		pr_debug("multiplier[%u] = [%llu]\n", (middle + 1),
			table[middle + 1].multiplier);
	} else if ((middle == 0) && ((middle + 1) <= index_limit)) {
		if (table[middle].multiplier < dividend) {
			diff = dividend - table[middle].multiplier;
		} else {
			diff = table[middle].multiplier - dividend;
		}
		if (table[middle + 1].multiplier < dividend) {
			diff_high = dividend - table[middle + 1].multiplier;
		} else {
			diff_high = table[middle + 1].multiplier - dividend;
		}
		pr_debug("multiplier[%u] = [%llu]\n", (middle),
			table[middle].multiplier);
		pr_debug("multiplier[%u] = [%llu]\n", (middle + 1),
			table[middle + 1].multiplier);
	} else if ((middle > 0) && ((middle + 1) > index_limit)) {
		if (table[middle - 1].multiplier < dividend) {
			diff_low = dividend - table[middle - 1].multiplier;
		} else {
			diff_low = table[middle - 1].multiplier - dividend;
		}
		if (table[middle].multiplier < dividend) {
			diff = dividend - table[middle].multiplier;
		} else {
			diff = table[middle].multiplier - dividend;
		}
		pr_debug("multiplier[%u] = [%llu]\n", (middle - 1),
			table[middle - 1].multiplier);
		pr_debug("multiplier[%u] = [%llu]\n", (middle),
			table[middle].multiplier);
	}
	pr_debug("diff_low = [%llu]\n", diff_low);
	pr_debug("diff = [%llu]\n", diff);
	pr_debug("diff_high = [%llu]\n", diff_high);
	if (diff_low < diff) {
		if (middle) {
			middle--;
		}
	}
	if (diff_high < diff) {
		middle++;
		if (middle > index_limit) {
			middle = index_limit;
		}
	}
	pr_debug("middle = [%u]\n", middle);

	return middle;
}
EXPORT_SYMBOL(ambarella_rct_find_pll_table_index);

/* ==========================================================================*/
unsigned long ambarella_rct_clk_get_rate(struct clk *c)
{
	u32 pre_scaler, post_scaler, intp, sdiv, sout;
	u64 dividend, divider, frac;
	union ctrl_reg_u ctrl_reg;
	union frac_reg_u frac_reg;

	BUG_ON(c->ctrl_reg == -1 || c->frac_reg == -1);

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	if ((ctrl_reg.s.power_down == 1) || (ctrl_reg.s.halt_vco == 1)) {
		c->rate = 0;
		return c->rate;
	}

	frac_reg.w = amba_rct_readl(c->frac_reg);

	if (c->pres_reg != -1) {
		pre_scaler = amba_rct_readl(c->pres_reg);
		if (c->extra_scaler == 1) {
			pre_scaler >>= 4;
			pre_scaler++;
		}
	} else {
		pre_scaler = 1;
	}

	if (c->post_reg != -1) {
		post_scaler = amba_rct_readl(c->post_reg);
		if (c->extra_scaler == 1) {
			post_scaler >>= 4;
			post_scaler++;
		}
	} else {
		post_scaler = 1;
	}

	if (ctrl_reg.s.bypass || ctrl_reg.s.force_bypass) {
		c->rate = ambarella_clk_ref_freq / pre_scaler / post_scaler;
		return c->rate;
	}

	intp = ctrl_reg.s.intp;
	sdiv = ctrl_reg.s.sdiv;
	sout = ctrl_reg.s.sout;

	dividend = (u64)ambarella_clk_ref_freq;
	dividend *= (u64)(intp + 1);
	dividend *= (u64)(sdiv + 1);
	if (ctrl_reg.s.frac_mode) {
		if (frac_reg.s.nega) {
			/* Negative */
			frac = (0x80000000 - frac_reg.s.frac);
			frac = (ambarella_clk_ref_freq * frac * (sdiv + 1));
			frac >>= 32;
			dividend = dividend - frac;
		} else {
			/* Positive */
			frac = frac_reg.s.frac;
			frac = (ambarella_clk_ref_freq * frac * (sdiv + 1));
			frac >>= 32;
			dividend = dividend + frac;
		}
	}

	divider = (pre_scaler * (sout + 1) * post_scaler);
	if (c->divider)
		divider *= c->divider;

	if (divider == 0) {
		c->rate = 0;
		return c->rate;
	}

	AMBCLK_DO_DIV(dividend, divider);
	c->rate = dividend;

	return c->rate;
}
EXPORT_SYMBOL(ambarella_rct_clk_get_rate);

int ambarella_rct_clk_set_rate(struct clk *c, unsigned long rate)
{
	u32 pre_scaler, post_scaler, middle;
	u32 intp, sdiv, sout, post, ctrl2, ctrl3;
	u64 dividend, divider, diff;
	union ctrl_reg_u ctrl_reg;
	union frac_reg_u frac_reg;

	if (!rate)
		return -1;

	BUG_ON(c->ctrl_reg == -1 || c->ctrl2_reg == -1 || c->ctrl3_reg == -1);
	BUG_ON(c->post_reg != -1 && !c->max_divider);
	BUG_ON(!c->table || c->table_size == 0);
#if 0
	if (c->divider)
		rate *= c->divider;
#endif
	if (c->pres_reg != -1) {
		if (c->pres_val) {
			pre_scaler = c->pres_val;
			if (c->extra_scaler == 1)
				amba_rct_writel_en(c->pres_reg, (pre_scaler - 1) << 4);
			else
				amba_rct_writel(c->pres_reg, pre_scaler);
		} else {
			pre_scaler = amba_rct_readl(c->pres_reg);
			if (c->extra_scaler == 1) {
				pre_scaler >>= 4;
				pre_scaler++;
			}
		}
	} else {
		pre_scaler = 1;
	}

	middle = ambarella_rct_find_pll_table_index(rate,
			pre_scaler, c->table, c->table_size);
	intp = c->table[middle].intp;
	sdiv = c->table[middle].sdiv;
	sout = c->table[middle].sout;
	post = c->post_val ? c->post_val : c->table[middle].post;

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.intp = intp;
	ctrl_reg.s.sdiv = sdiv;
	ctrl_reg.s.sout = sout;
	ctrl_reg.s.bypass = 0;
	ctrl_reg.s.frac_mode = 0;
	ctrl_reg.s.force_reset = 0;
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.tristate = 0;
	ctrl_reg.s.force_lock = 1;
	ctrl_reg.s.force_bypass = 0;
	ctrl_reg.s.write_enable = 0;
	amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);

	if (c->post_reg != -1) {
		post_scaler = min(post, c->max_divider);
		if (c->extra_scaler == 1)
			amba_rct_writel_en(c->post_reg, (post_scaler - 1) << 4);
		else
			amba_rct_writel(c->post_reg, post_scaler);
	}

	if (c->frac_mode) {
		c->rate = ambarella_rct_clk_get_rate(c);
		if (c->rate < rate)
			diff = rate - c->rate;
		else
			diff = c->rate - rate;

		dividend = diff * pre_scaler * (sout + 1) * post;
		if (c->divider)
			dividend *= c->divider;
		dividend = dividend << 32;
		divider = (u64)ambarella_clk_ref_freq * (sdiv + 1);
		AMBCLK_DO_DIV_ROUND(dividend, divider);
		if (c->rate <= rate) {
			frac_reg.s.nega	= 0;
			frac_reg.s.frac	= dividend;
		} else {
			frac_reg.s.nega	= 1;
			frac_reg.s.frac	= 0x80000000 - dividend;
		}
		amba_rct_writel(c->frac_reg, frac_reg.w);

		ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
		if (diff)
			ctrl_reg.s.frac_mode = 1;
		else
			ctrl_reg.s.frac_mode = 0;

		ctrl_reg.s.force_lock = 1;
		ctrl_reg.s.write_enable = 1;
		amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

		ctrl_reg.s.write_enable	= 0;
		amba_rct_writel(c->ctrl_reg, ctrl_reg.w);
	}

	ctrl2 = c->ctrl2_val ? c->ctrl2_val : 0x3f770000;
	ctrl3 = c->ctrl3_val ? c->ctrl3_val : ctrl_reg.s.frac_mode ? 0x00069300 : 0x00068300;
	amba_rct_writel(c->ctrl2_reg, ctrl2);
	amba_rct_writel(c->ctrl3_reg, ctrl3);

	c->rate = ambarella_rct_clk_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_set_rate);

int ambarella_rct_clk_enable(struct clk *c)
{
	union ctrl_reg_u ctrl_reg;

	if (c->ctrl_reg == -1) {
		return -1;
	}

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.write_enable = 1;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	ctrl_reg.s.write_enable	= 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	c->rate = ambarella_rct_clk_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_enable);

int ambarella_rct_clk_disable(struct clk *c)
{
	union ctrl_reg_u ctrl_reg;

	if (c->ctrl_reg == -1) {
		return -1;
	}

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.power_down = 1;
	ctrl_reg.s.halt_vco = 1;
	ctrl_reg.s.write_enable = 1;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	ctrl_reg.s.write_enable	= 0;
	amba_rct_writel(c->ctrl_reg, ctrl_reg.w);

	c->rate = ambarella_rct_clk_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_clk_disable);


/* ==========================================================================*/
unsigned long ambarella_rct_scaler_get_rate(struct clk *c)
{
	u32 parent_rate, divider;

	if (!c->parent || !c->parent->ops || !c->parent->ops->get_rate)
		parent_rate = ambarella_clk_ref_freq;
	else
		parent_rate = c->parent->ops->get_rate(c->parent);

	if (c->divider) {
		c->rate = parent_rate / c->divider;
		return c->rate;
	} else if (c->post_reg != -1) {
		divider = amba_rct_readl(c->post_reg);
		if (c->extra_scaler == 1) {
			divider >>= 4;
			divider++;
		}
		if (divider) {
			c->rate = parent_rate / divider;
			return c->rate;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_scaler_get_rate);

int ambarella_rct_scaler_set_rate(struct clk *c, unsigned long rate)
{
	u32 parent_rate, divider, post_scaler;

	if (!rate)
		return -1;

	BUG_ON(c->post_reg == -1 || !c->max_divider);

	if (!c->parent || !c->parent->ops || !c->parent->ops->get_rate)
		parent_rate = ambarella_clk_ref_freq;
	else
		parent_rate = c->parent->ops->get_rate(c->parent);

	if (c->divider)
		rate *= c->divider;

	divider = (parent_rate + rate - 1) / rate;
	if (!divider)
		return -1;

	post_scaler = min(divider, c->max_divider);
	if (c->extra_scaler == 1) {
		post_scaler--;
		post_scaler <<= 4;
		amba_rct_writel_en(c->post_reg, post_scaler);
	} else {
		amba_rct_writel(c->post_reg, post_scaler);
	}

	c->rate = ambarella_rct_scaler_get_rate(c);

	return 0;
}
EXPORT_SYMBOL(ambarella_rct_scaler_set_rate);


/* ==========================================================================*/
struct clk *clk_get_sys(const char *dev_id, const char *con_id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);

	spin_lock(&ambarella_clock_lock);
	list_for_each_entry(p, &ambarella_all_clocks, list) {
		if (dev_id && (strcmp(p->name, dev_id) == 0)) {
			clk = p;
			break;
		}
		if (con_id && (strcmp(p->name, con_id) == 0)) {
			clk = p;
			break;
		}
	}
	spin_unlock(&ambarella_clock_lock);

	return clk;
}
EXPORT_SYMBOL(clk_get_sys);

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p;
	struct clk *clk = ERR_PTR(-ENOENT);

	if (id == NULL) {
		return clk;
	}

	spin_lock(&ambarella_clock_lock);
	list_for_each_entry(p, &ambarella_all_clocks, list) {
		if (strcmp(p->name, id) == 0) {
			clk = p;
			break;
		}
	}
	spin_unlock(&ambarella_clock_lock);

	return clk;
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

int clk_enable(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	clk_enable(clk->parent);

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->enable) {
		(clk->ops->enable)(clk);
	}
	spin_unlock(&ambarella_clock_lock);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return;
	}

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->disable) {
		(clk->ops->disable)(clk);
	}
	spin_unlock(&ambarella_clock_lock);

	clk_disable(clk->parent);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return 0;
	}

	if (clk->ops != NULL && clk->ops->get_rate != NULL) {
		return (clk->ops->get_rate)(clk);
	}

	if (clk->parent != NULL) {
		return clk_get_rate(clk->parent);
	}

	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return rate;
	}

	if (clk->ops && clk->ops->round_rate) {
		return (clk->ops->round_rate)(clk, rate);
	}

	return rate;
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;

	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	if ((clk->ops == NULL) || (clk->ops->set_rate == NULL)) {
		return -EINVAL;
	}

	spin_lock(&ambarella_clock_lock);
	ret = (clk->ops->set_rate)(clk, rate);
	spin_unlock(&ambarella_clock_lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

struct clk *clk_get_parent(struct clk *clk)
{
	if (IS_ERR(clk) || (clk == NULL)) {
		return ERR_PTR(-EINVAL);
	}

	return clk->parent;
}
EXPORT_SYMBOL(clk_get_parent);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (IS_ERR(clk) || (clk == NULL)) {
		return -EINVAL;
	}

	spin_lock(&ambarella_clock_lock);
	if (clk->ops && clk->ops->set_parent) {
		ret = (clk->ops->set_parent)(clk, parent);
	}
	spin_unlock(&ambarella_clock_lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_parent);

int ambarella_clk_add(struct clk *clk)
{
	spin_lock(&ambarella_clock_lock);
	list_add(&clk->list, &ambarella_all_clocks);
	spin_unlock(&ambarella_clock_lock);

	return 0;
}
EXPORT_SYMBOL(ambarella_clk_add);

/* ==========================================================================*/
#if defined(CONFIG_AMBARELLA_PLL_PROC)
static int ambarella_clock_proc_show(struct seq_file *m, void *v)
{
	int retlen = 0;
	struct clk *p;

	retlen += seq_printf(m, "\nClock Information:\n");
	spin_lock(&ambarella_clock_lock);
	list_for_each_entry_reverse(p, &ambarella_all_clocks, list) {
		retlen += seq_printf(m, "\t%s:\t%lu Hz\n",
			p->name, p->ops->get_rate(p));
	}
	spin_unlock(&ambarella_clock_lock);

	return retlen;
}

static int ambarella_clock_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ambarella_clock_proc_show, PDE_DATA(inode));
}

static const struct file_operations proc_clock_fops = {
	.open = ambarella_clock_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
};
#endif

/* ==========================================================================*/
int __init ambarella_clk_init(void)
{
	int ret_val = 0;

	ambarella_clk_ref_freq = REF_CLK_FREQ;
#if defined(CONFIG_AMBARELLA_PLL_PROC)
	proc_create_data("clock", S_IRUGO, get_ambarella_proc_dir(),
		&proc_clock_fops, NULL);
#endif

	return ret_val;
}

/* ==========================================================================*/
unsigned int ambarella_clk_get_ref_freq(void)
{
	return ambarella_clk_ref_freq;
}
EXPORT_SYMBOL(ambarella_clk_get_ref_freq);

