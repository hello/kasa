/*
 * arch/arm/plat-ambarella/misc/service.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
#include <plat/iav_helper.h>
#include <mach/init.h>



/*===========================================================================*/

static LIST_HEAD(ambarella_svc_list);

int ambarella_register_service(struct ambarella_service *amb_svc)
{
	struct ambarella_service *svc;

	if (!amb_svc || !amb_svc->func)
		return -EINVAL;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == amb_svc->service) {
			pr_err("%s: service (%d) is already existed\n",
					__func__, amb_svc->service);
			return -EEXIST;
		}
	}

	list_add_tail(&amb_svc->node, &ambarella_svc_list);

	return 0;
}
EXPORT_SYMBOL(ambarella_register_service);

int ambarella_unregister_service(struct ambarella_service *amb_svc)
{
	struct ambarella_service *svc;

	if (!amb_svc)
		return -EINVAL;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == amb_svc->service) {
			list_del(&svc->node);
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_unregister_service);

int ambarella_request_service(int service, void *arg, void *result)
{
	struct ambarella_service *svc;
	int found = 0;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == service) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		pr_err("%s: no such service (%d)\n", __func__, service);
		return -ENODEV;
	}

	return svc->func(arg, result);
}
EXPORT_SYMBOL(ambarella_request_service);

/*===========================================================================*/

#define CACHE_LINE_SIZE		32
#define CACHE_LINE_MASK		~(CACHE_LINE_SIZE - 1)

void ambcache_clean_all(void *addr, unsigned int size)
{
	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	//u32					addr_tmp;
        u32 set, way;
        u32 reg_data;
        u32 sscidr = 0;
        u32 value_set = 0,value_way = 0;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;

#ifdef CONFIG_OUTER_CACHE
	pstart = ambarella_virt_to_phys(vstart);
#endif

        /*format of SSCIDR*/
        /* |31 |30  |29|28 |27-13    |12-3 |2-0    |*/
        /* |WT|WB|RA|WA|NumSets|Ways|Linsize|*/
        asm volatile("mrc p15, 1, %0, c0, c0, 0" : "=r" (sscidr));
        value_set = ((sscidr & 0x0FFFE000)>>13);
        value_way =((sscidr & 0x00001FF8)>>3);
        //printk("\033[31m[%s][%d]  cache SSCIDR (0x%x), Set(%d),Way(%d)\033[0m\n",__FILE__,__LINE__,sscidr,value_set,value_way);

        /* Cache level 1 */
        for (way = 0; way <= value_way; way++) {
                /* Way, bits[31:30], the number of the way to operate on. */
                for (set = 0; set <=value_set; set++) {
                        /* Set, bits[13:6], the number of the set to operate on. */
                        /* clean cache lines in all ways */
                        reg_data = (way << 30) | (set << 5);
                        __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 2"
                        :
                        : "r" (reg_data)
                        : );
                }
        }
        dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_clean_range(pstart, (pstart + size));
#endif
}

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
EXPORT_SYMBOL(ambcache_clean_all);

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


