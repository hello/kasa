/**
 * bld/header.S
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
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


#include <amboot.h>
#include <flash/flash.h>
#include <fio/partition.h>

.text

.globl begin_bst_image
.globl end_bst_image
.globl begin_bld_image
.globl end_bld_image
.globl begin_ptb_image
.globl end_ptb_image
.globl begin_splash_image
.globl end_splash_image
.globl begin_pba_image
.globl end_pba_image
.globl begin_kernel_image
.globl end_kernel_image
.globl begin_secondary_image
.globl end_secondary_image
.globl begin_backup_image
.globl end_backup_image
.globl begin_ramdisk_image
.globl end_ramdisk_image
.globl begin_romfs_image
.globl end_romfs_image
.globl begin_dsp_image
.globl end_dsp_image
.globl begin_lnx_image
.globl end_lnx_image
.globl begin_swp_image
.globl end_swp_image
.globl begin_add_image
.globl end_add_image
.globl begin_adc_image
.globl end_adc_image

#if defined(__LINK_BST__)
begin_bst_image:	.word	__BEGIN_AMBOOT_BST_IMAGE__
#else
begin_bst_image:	.word	0x0
#endif

#if defined(__LINK_BLD__)
begin_bld_image:	.word	__BEGIN_AMBOOT_BLD_IMAGE__
#else
begin_bld_image:	.word	0x0
#endif

begin_ptb_image:	.word	0x0

#if defined(__LINK_SPL__)
begin_splash_image:	.word	__BEGIN_SPL_IMAGE__
#else
begin_splash_image:	.word	0x0
#endif

#if defined(__LINK_PBA__)
begin_pba_image:	.word	__BEGIN_PBA_IMAGE__
#else
begin_pba_image:	.word	0x0
#endif

#if defined(__LINK_KERNEL__)
begin_kernel_image:	.word	__BEGIN_KERNEL_IMAGE__
#else
begin_kernel_image:	.word	0x0
#endif

#if defined(__LINK_SECONDARY__)
begin_secondary_image:	.word	__BEGIN_SECONDARY_IMAGE__
#else
begin_secondary_image:	.word	0x0
#endif

#if defined(__LINK_BACKUP__)
begin_backup_image:	.word	__BEGIN_BACKUP_IMAGE__
#else
begin_backup_image:	.word	0x0
#endif

#if defined(__LINK_RAMDISK__)
begin_ramdisk_image:	.word	__BEGIN_RAMDISK_IMAGE__
#else
begin_ramdisk_image:	.word	0x0
#endif

#if defined(__LINK_ROMFS__)
begin_romfs_image:	.word	__BEGIN_ROMFS_IMAGE__
#else
begin_romfs_image:	.word	0x0
#endif

#if defined(__LINK_DSP__)
begin_dsp_image:	.word	__BEGIN_DSP_IMAGE__
#else
begin_dsp_image:	.word	0x0
#endif

#if defined(__LINK_LNX__)
begin_lnx_image:	.word	__BEGIN_LNX_IMAGE__
#else
begin_lnx_image:	.word	0x0
#endif

#if defined(__LINK_SWP__)
begin_swp_image:	.word	__BEGIN_SWP_IMAGE__
#else
begin_swp_image:	.word	0x0
#endif

#if defined(__LINK_ADD__)
begin_add_image:	.word	__BEGIN_ADD_IMAGE__
#else
begin_add_image:	.word	0x0
#endif

#if defined(__LINK_ADC__)
begin_adc_image:	.word	__BEGIN_ADC_IMAGE__
#else
begin_adc_image:	.word	0x0
#endif

/* Reserved space for begin_image */
.rept (PART_MAX_WITH_RSV - HAS_IMG_PARTS)
.word	0x0
.endr

#if defined(__LINK_BST__)
end_bst_image:		.word	__END_AMBOOT_BST_IMAGE__
#else
end_bst_image:		.word	0x0
#endif

#if defined(__LINK_BLD__)
end_bld_image:		.word	__END_AMBOOT_BLD_IMAGE__
#else
end_bld_image:		.word	0x0
#endif

end_ptb_image:		.word	0x0

#if defined(__LINK_SPL__)
end_splash_image:	.word	__END_SPL_IMAGE__
#else
end_splash_image:	.word	0x0
#endif

#if defined(__LINK_PBA__)
end_pba_image:		.word	__END_PBA_IMAGE__
#else
end_pba_image:		.word	0x0
#endif

#if defined(__LINK_KERNEL__)
end_kernel_image:	.word	__END_KERNEL_IMAGE__
#else
end_kernel_image:	.word	0x0
#endif

#if defined(__LINK_SECONDARY__)
end_secondary_image:	.word	__END_SECONDARY_IMAGE__
#else
end_secondary_image:	.word	0x0
#endif

#if defined(__LINK_BACKUP__)
end_backup_image:	.word	__END_BACKUP_IMAGE__
#else
end_backup_image:	.word	0x0
#endif

#if defined(__LINK_RAMDISK__)
end_ramdisk_image:	.word	__END_RAMDISK_IMAGE__
#else
end_ramdisk_image:	.word	0x0
#endif

#if defined(__LINK_ROMFS__)
end_romfs_image:	.word	__END_ROMFS_IMAGE__
#else
end_romfs_image:	.word	0x0
#endif

#if defined(__LINK_DSP__)
end_dsp_image:		.word	__END_DSP_IMAGE__
#else
end_dsp_image:		.word	0x0
#endif

#if defined(__LINK_LNX__)
end_lnx_image:		.word	__END_LNX_IMAGE__
#else
end_lnx_image:		.word	0x0
#endif

#if defined(__LINK_SWP__)
end_swp_image:		.word	__END_SWP_IMAGE__
#else
end_swp_image:		.word	0x0
#endif

#if defined(__LINK_ADD__)
end_add_image:		.word	__END_ADD_IMAGE__
#else
end_add_image:		.word	0x0
#endif

#if defined(__LINK_ADC__)
end_adc_image:		.word	__END_ADC_IMAGE__
#else
end_adc_image:		.word	0x0
#endif

/* Reserved space for end_image */
.rept (PART_MAX_WITH_RSV - HAS_IMG_PARTS)
.word 0x0
.endr

/*
 * The 'extra' struct
 */

magic:		.word		0x33219fbd

#if defined(AMBOOT_BST_FIXED_SIZE)
size_bst:	.word		AMBOOT_BST_FIXED_SIZE
#elif defined(AMBOOT_BST_SIZE)
size_bst:	.word		AMBOOT_BST_SIZE
#else
size_bst:	.word		0x0
#endif
#if defined(AMBOOT_BLD_SIZE)
size_bld:	.word		AMBOOT_BLD_SIZE
#else
size_bld:	.word		0x0
#endif
#if defined(AMBOOT_PTB_SIZE)
size_ptb:	.word		AMBOOT_PTB_SIZE
#else
size_ptb:	.word		0x0
#endif
#if defined(AMBOOT_SPL_SIZE)
size_splash:	.word		AMBOOT_SPL_SIZE
#else
size_splash:	.word		0x0
#endif
#if defined(AMBOOT_PBA_SIZE)
size_pba:	.word		AMBOOT_PBA_SIZE
#else
size_pba:	.word		0x0
#endif
#if defined(AMBOOT_PRI_SIZE)
size_pri:	.word		AMBOOT_PRI_SIZE
#else
size_pri:	.word		0x0
#endif
#if defined(AMBOOT_SEC_SIZE)
size_sec:	.word		AMBOOT_SEC_SIZE
#else
size_sec:	.word		0x0
#endif
#if defined(AMBOOT_BAK_SIZE)
size_bak:	.word		AMBOOT_BAK_SIZE
#else
size_bak:	.word		0x0
#endif
#if defined(AMBOOT_RMD_SIZE)
size_rmd:	.word		AMBOOT_RMD_SIZE
#else
size_rmd:	.word		0x0
#endif
#if defined(AMBOOT_ROM_SIZE)
size_rom:	.word		AMBOOT_ROM_SIZE
#else
size_rom:	.word		0x0
#endif
#if defined(AMBOOT_DSP_SIZE)
size_dsp:	.word		AMBOOT_DSP_SIZE
#else
size_dsp:	.word		0x0
#endif
#if defined(AMBOOT_LNX_SIZE)
size_lnx:	.word		AMBOOT_LNX_SIZE
#else
size_lnx:	.word		0x0
#endif
#if defined(AMBOOT_SWP_SIZE)
size_swp:	.word		AMBOOT_SWP_SIZE
#else
size_swp:	.word		0x0
#endif
#if defined(AMBOOT_ADD_SIZE)
size_add:	.word		AMBOOT_ADD_SIZE
#else
size_add:	.word		0x0
#endif
#if defined(AMBOOT_ADC_SIZE)
size_adc:	.word		AMBOOT_ADC_SIZE
#else
size_adc:	.word		0x0
#endif
#if defined(MP_RAW_SIZE)
size_raw:	.word		MP_RAW_SIZE
#else
size_raw:	.word		0x0
#endif
#if defined(MP_STG2_SIZE)
size_stg2:	.word		MP_STG2_SIZE
#else
size_stg2:	.word		0x0
#endif
#if defined(MP_STG_SIZE)
size_stg:	.word		MP_STG_SIZE
#else
size_stg:	.word		0x0
#endif
#if defined(MP_PRF_SIZE)
size_prf:	.word		MP_PRF_SIZE
#else
size_prf:	.word		0x0
#endif
#if defined(MP_CAL_SIZE)
size_cal:	.word		MP_CAL_SIZE
#else
size_cal:	.word		0x0
#endif

/* Reserved space for partition size */
.rept (PART_MAX_WITH_RSV - PART_MAX)
.word	0x0
.endr

/*
 * mp_info
 */

#if defined(PREF_ZONE_THRESHOLD)
mp_prf_zonet:	.word		PREF_ZONE_THRESHOLD
#else
mp_prf_zonet:	.word		0x0
#endif
#if defined(CALIB_ZONE_THRESHOLD)
mp_cal_zonet:	.word		CALIB_ZONE_THRESHOLD
#else
mp_cal_zonet:	.word		0x0
#endif
#if defined(RAW_ZONE_THRESHOLD)
mp_raw_zonet:	.word		RAW_ZONE_THRESHOLD
#else
mp_raw_zonet:	.word		0x0
#endif
#if defined(STG_ZONE_THRESHOLD)
mp_stg_zonet:	.word		STG_ZONE_THRESHOLD
#else
mp_stg_zonet:	.word		0x0
#endif
#if defined(STG2_ZONE_THRESHOLD)
mp_stg2_zonet:	.word		STG2_ZONE_THRESHOLD
#else
mp_stg2_zonet:	.word		0x0
#endif

/* Reserved space for zonet */
.rept (MP_MAX_WITH_RSV - MP_MAX)
.word	0x0
.endr

#if defined(RAW_RSV_BLOCKS_PER_ZONET)
mp_raw_rb_pzt:	.word		RAW_RSV_BLOCKS_PER_ZONET
#else
mp_raw_rb_pzt:	.word		0x0
#endif
#if defined(STG2_RSV_BLOCKS_PER_ZONET)
mp_stg2_rb_pzt:	.word		STG2_RSV_BLOCKS_PER_ZONET
#else
mp_stg2_rb_pzt:	.word		0x0
#endif
#if defined(STG_RSV_BLOCKS_PER_ZONET)
mp_stg_rb_pzt:	.word		STG_RSV_BLOCKS_PER_ZONET
#else
mp_stg_rb_pzt:	.word		0x0
#endif
#if defined(PREF_RSV_BLOCKS_PER_ZONET)
mp_prf_rb_pzt:	.word		PREF_RSV_BLOCKS_PER_ZONET
#else
mp_prf_rb_pzt:	.word		0x0
#endif
#if defined(CALIB_RSV_BLOCKS_PER_ZONET)
mp_cal_rb_pzt:	.word		CALIB_RSV_BLOCKS_PER_ZONET
#else
mp_cal_rb_pzt:	.word		0x0
#endif

/* Reserved space for rb_pzt */
.rept (MP_MAX_WITH_RSV - MP_MAX)
.word	0x0
.endr

#if defined(RAW_MIN_RSV_BLOCKS_PER_ZONE)
mp_raw_mrb_pz:	.word		RAW_MIN_RSV_BLOCKS_PER_ZONE
#else
mp_raw_mrb_pz:	.word		0x0
#endif
#if defined(STG2_MIN_RSV_BLOCKS_PER_ZONE)
mp_stg2_mrb_pz:	.word		STG2_MIN_RSV_BLOCKS_PER_ZONE
#else
mp_stg2_mrb_pz:	.word		0x0
#endif
#if defined(STG_MIN_RSV_BLOCKS_PER_ZONE)
mp_stg_mrb_pz:	.word		STG_MIN_RSV_BLOCKS_PER_ZONE
#else
mp_stg_mrb_pz:	.word		0x0
#endif
#if defined(PREF_MIN_RSV_BLOCKS_PER_ZONE)
mp_prf_mrb_pz:	.word		PREF_MIN_RSV_BLOCKS_PER_ZONE
#else
mp_prf_mrb_pz:	.word		0x0
#endif
#if defined(CALIB_MIN_RSV_BLOCKS_PER_ZONE)
mp_cal_mrb_pz:	.word		CALIB_MIN_RSV_BLOCKS_PER_ZONE
#else
mp_cal_mrb_pz:	.word		0x0
#endif

/* Reserved space for mrb_pz */
.rept (MP_MAX_WITH_RSV - MP_MAX)
.word	0x0
.endr

#if defined(RAW_TRL_TABLES)
mp_raw_trlb:	.word		RAW_TRL_TABLES
#else
mp_raw_trlb:	.word		0x0
#endif
#if defined(STG2_TRL_TABLES)
mp_stg2_trlb:	.word		STG2_TRL_TABLES
#else
mp_stg2_trlb:	.word		0x0
#endif
#if defined(STG_TRL_TABLES)
mp_stg_trlb:	.word		STG_TRL_TABLES
#else
mp_stg_trlb:	.word		0x0
#endif
#if defined(PREF_TRL_TABLES)
mp_prf_trlb:	.word		PREF_TRL_TABLES
#else
mp_prf_trlb:	.word		0x0
#endif
#if defined(CALIB_TRL_TABLES)
mp_cal_trlb:	.word		CALIB_TRL_TABLES
#else
mp_cal_trlb:	.word		0x0
#endif

/* Reserved space for trlb */
.rept (MP_MAX_WITH_RSV - MP_MAX)
.word	0x0
.endr

#if defined(FW_MODEL_NAME)
model_name:	.ascii		FW_MODEL_NAME
#else
model_name:	.ascii		""
#endif
