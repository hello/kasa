/*
 * iav_drv.c
 *
 * History:
 *	2012/10/10 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] modified file
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
#include <plat/event.h>
#include <plat/ambcache.h>
#include <mach/hardware.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <iav_devnum.h>
#include <vout_api.h>
#include <dsp_api.h>
#include <vin_api.h>
#include <dsplog_api.h>
#include "iav.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"
#include "iav_dec_api.h"

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "amba_imgproc.h"
#endif

struct amba_iav_vout_info G_voutinfo[] = {{0, 0,}, {0, 0,}};
static struct iav_imgproc_info G_img_info;

static int iav_state_proc_show(struct seq_file *m, void *v)
{
	struct ambarella_iav *iav;
	struct iav_state_info info;
	int chip;

	iav = (struct ambarella_iav *)m->private;

	iav->dsp->get_chip_id(iav->dsp, NULL, &chip);

	// TODO
	info.dsp_op_mode = 0;
	info.dsp_encode_state = 0;
	info.dsp_encode_mode = 0;
	info.dsp_decode_state = 0;
	info.decode_state = 0;
	info.encode_timecode = 0;
	info.encode_pts = 0;

	switch (chip) {
	case AMBA_CHIP_ID_S2L_22M:
	case AMBA_CHIP_ID_S2L_33M:
	case AMBA_CHIP_ID_S2L_55M:
	case AMBA_CHIP_ID_S2L_99M:
	case AMBA_CHIP_ID_S2L_TEST:
	case AMBA_CHIP_ID_S2L_33MEX:
		seq_printf(m, "AMBARELLA_CHIP=S2LM\n");
		break;
	case AMBA_CHIP_ID_S2L_63:
	case AMBA_CHIP_ID_S2L_66:
	case AMBA_CHIP_ID_S2L_88:
	case AMBA_CHIP_ID_S2L_99:
	case AMBA_CHIP_ID_S2L_22:
	case AMBA_CHIP_ID_S2L_33EX:
		seq_printf(m, "AMBARELLA_CHIP=S2L\n");
		break;
	default:
		seq_printf(m, "AMBARELLA_CHIP=UNKNOWN\n");
		break;
	}
	seq_printf(m, "dsp_op_mode: %d\n", info.dsp_op_mode);
	seq_printf(m, "dsp_encode_state: %d\n", info.dsp_encode_state);
	seq_printf(m, "dsp_encode_mode: %d\n", info.dsp_encode_mode);
	seq_printf(m, "dsp_decode_state: %d\n", info.dsp_decode_state);
	seq_printf(m, "decode_state: %d\n", info.decode_state);
	seq_printf(m, "encode timecode: %d\n", info.encode_timecode);
	seq_printf(m, "encode pts: %d\n", info.encode_pts);

	return 0;
}

static int iav_state_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, iav_state_proc_show, PDE_DATA(inode));
}

static const struct file_operations iav_state_proc_fops = {
	.open = iav_state_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
};

static int iav_vin_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long args)
{
	int rval = -EINVAL;

	/* FIXME: should call vin[1]->ioctl if using second sensor */
	if (!iav->vinc[0] || !iav->vinc[0]->ioctl) {
		iav_error("No vin ioctl\n");
	} else {
		rval = iav->vinc[0]->ioctl(iav->vinc[0], cmd, args);
	}

	return rval;
}

#ifdef BUILD_AMBARELLA_IMGPROC_DRV
static int iav_img_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long args)
{
	int rval = 0, enc_mode;
	struct iav_rect * vin;
	struct iav_system_config *config;
	struct iav_imgproc_info *info;

	info = &G_img_info;

	get_vin_win(iav, &vin, 1);

	mutex_lock(&iav->iav_mutex);
	enc_mode = iav->encode_mode;
	config = &iav->system_config[enc_mode];
	info->iav_state = iav->state;
	info->vin_num = 1;
	info->hdr_expo_num = config->expo_num;
	info->cap_width = vin->width;
	info->cap_height = vin->height;
	info->hdr_mode = get_hdr_type(iav);
	info->img_size = iav->mmap[IAV_BUFFER_IMG].size;
	info->img_virt = iav->mmap[IAV_BUFFER_IMG].virt;
	info->img_phys = iav->mmap[IAV_BUFFER_IMG].phys;
	info->img_config_offset = IAV_DRAM_IMG_OFFET;
	if (!iav->dsp_partition_mapped) {
		info->dsp_virt = iav->mmap[IAV_BUFFER_DSP].virt;
		info->dsp_phys = iav->mmap[IAV_BUFFER_DSP].phys;
	} else {
		info->dsp_virt = iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW].virt;
		info->dsp_phys = iav->mmap_dsp[IAV_DSP_SUB_BUF_RAW].phys;
	}
	info->iav_mutex = &iav->iav_mutex;

	rval = amba_imgproc_cmd(info, cmd, args);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}
#endif

static long iav_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ambarella_iav *iav = filp->private_data;
	int rval = 0;

	switch (_IOC_TYPE(cmd)) {
	case IAVENCIOC_MAGIC:
		rval = iav_encode_ioctl(iav, cmd, arg);
		break;
	case IAVDECIOC_MAGIC:
		rval = iav_decode_ioctl(iav, cmd, arg);
		break;
	case VINIOC_MAGIC:
		rval = iav_vin_ioctl(iav, cmd, arg);
		break;
	case VOUTIOC_MAGIC:
		rval = iav_vout_ioctl(iav, cmd, arg);
		break;
#ifdef BUILD_AMBARELLA_IMGPROC_DRV
	case IAVIOC_IMAGE_MAGIC:
		rval = iav_img_ioctl(iav, cmd, arg);
		break;
#endif
	default:
		iav_error("Invalid IOCTL [0x%x].\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

static int iav_open(struct inode *inode, struct file *filp)
{
	struct ambarella_iav *iav;

	iav = container_of(inode->i_cdev, struct ambarella_iav, iav_cdev);
	filp->private_data = iav;

	return 0;
}

static int iav_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations iav_fops = {
	.owner = THIS_MODULE,
	.open = iav_open,
	.release = iav_release,
	.mmap = iav_mmap,
	.unlocked_ioctl = iav_ioctl,
};

static int iav_create_dev(struct ambarella_iav *iav)
{
	dev_t dev_id;
	int rval;

	dev_id = MKDEV(IAV_DEV_MAJOR, IAV_DEV_MINOR);
	rval = register_chrdev_region(dev_id, 1, "iav");
	if (rval) {
		iav_error("failed to get dev region for iav\n");
		return rval;
	}

	cdev_init(&iav->iav_cdev, &iav_fops);
	iav->iav_cdev.owner = THIS_MODULE;

	rval = cdev_add(&iav->iav_cdev, dev_id, 1);
	if (rval) {
		iav_error("cdev_add failed for iav: %d\n", rval);
		unregister_chrdev_region(iav->iav_cdev.dev, 1);
		return rval;
	}

	return 0;
}

static void iav_destroy_dev(struct ambarella_iav *iav)
{
	cdev_del(&iav->iav_cdev);
	unregister_chrdev_region(iav->iav_cdev.dev, 1);
}

/* store dsp status in amboot, restore it after enter Linux IAV */
static int iav_drv_probe_status(struct ambarella_iav *iav)
{
	u32 iav_state = 0;
	u32 dsp_enc_state = 0;

	if (DSP_FASTDATA_SIZE > 0) {
		iav_state = *((u32 *)(iav->mmap[IAV_BUFFER_FB_DATA].virt));
	} else {
		iav_state = IAV_STATE_INIT;
	}

	switch (iav_state) {
	case IAV_STATE_IDLE:
	case IAV_STATE_PREVIEW:
	case IAV_STATE_ENCODING:
		dsp_enc_state = DSP_ENCODE_MODE;
		break;
	case IAV_STATE_INIT:
	default:
		iav_state = IAV_STATE_INIT;
		dsp_enc_state = DSP_UNKNOWN_MODE;
		break;
	}
	iav->state = iav_state;
	iav->probe_state = iav_state;
	iav->dsp_enc_state = dsp_enc_state;

	iav_debug("IAV probe status [%u], dsp enc mode [%u].\n", iav_state, dsp_enc_state);

	return 0;
}

/* store vin video format in amboot, restore it after enter Linux IAV */
static int iav_drv_probe_vin_video_format(struct ambarella_iav *iav)
{
	iav->vin_probe_format = (struct vin_video_format *)
		(iav->mmap[IAV_BUFFER_FB_DATA].virt + DSP_STATUS_STORE_SIZE);

	if ((iav->vin_probe_format->video_mode != AMBA_VIDEO_MODE_INVALID) &&
		(iav->vin_probe_format->hdr_mode != AMBA_VIDEO_HDR_MODE_INVALID)) {
		iav->vin_enabled = 1;
	}

	return 0;
}

/* store vin dsp config in amboot, restore it after enter Linux IAV */
static int iav_drv_probe_vin_dsp_config (struct ambarella_iav *iav)
{
	memcpy(iav->vinc[0]->dsp_config,
		(struct vin_dsp_config *)(iav->mmap[IAV_BUFFER_FB_DATA].virt +
		DSP_STATUS_STORE_SIZE + DSP_VIN_VIDEO_FORMAT_STORE_SIZE),
		sizeof(struct vin_dsp_config));

	return 0;
}

static int iav_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;

	struct ambarella_iav *iav;
	struct proc_dir_entry *iav_state_proc = NULL;

	iav = kzalloc(sizeof(struct ambarella_iav), GFP_KERNEL);
	if (!iav) {
		iav_error("No memory!\n");
		return -ENOMEM;
	}

	iav->of_node = of_find_node_by_path("/iav");
	iav->dsp = ambarella_request_dsp();

	rval = iav_create_mmap_table(iav);
	if (rval < 0)
		goto iav_init_err;

	rval = iav_drv_probe_status(iav);
	if (rval < 0)
		goto iav_init_err;

	iav->cmd_read_delay = MIN_CRD_IN_CLK;
	iav->pvoutinfo[0] = &G_voutinfo[0];
	iav->pvoutinfo[1] = &G_voutinfo[1];
	iav->mixer_a_enable = 0;
	iav->mixer_b_enable = 1;
	iav->osd_from_mixer_a = 1;
	iav->osd_from_mixer_b = 0;
	iav->vin_overflow_protection = 0;
	iav->err_vsync_handling = 0;
	iav->err_vsync_again = 0;
	iav->err_vsync_lost = 0;
	memset(&iav->dsplog_setup, 0, sizeof(struct iav_dsplog_setup));
	mutex_init(&iav->iav_mutex);
	mutex_init(&iav->enc_mutex);
	spin_lock_init(&iav->iav_lock);

	iav_init_source_buffer(iav);

	iav_init_bufcap(iav);

	iav_init_streams(iav);

	rval = iav_encode_init(iav);
	if (rval < 0)
		goto iav_init_err;

	iav_init_warp(iav);

	iav_init_pm(iav);

	iav_init_debug(iav);

	init_netlink(iav);

	rval = iav_create_dev(iav);
	if (rval < 0)
		goto iav_init_err;

	ambpriv_set_drvdata(ambdev, iav);

	iav_state_proc = proc_create_data("iav", S_IRUGO,
		get_ambarella_proc_dir(), &iav_state_proc_fops, iav);
	if (iav_state_proc == NULL) {
		rval = -ENOMEM;
		iav_error("failed to create proc file (iav_state)!\n");
		goto iav_init_err;
	}

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	amb_set_dram_arbiter_priority(HAL_BASE_VP,
		AMB_DRAM_ARBITER_DSP_VERY_HIGH_PRIORITY);
#endif

	if (iav->probe_state != IAV_STATE_INIT) {
		/* Must be called before iav_vin_init() for it will read vin_enabled */
		rval = iav_drv_probe_vin_video_format(iav);
		if (rval < 0)
			goto iav_init_err;
	}

	rval = iav_vin_init(iav);
	if (rval < 0)
		goto iav_init_err;

	if (iav->probe_state != IAV_STATE_INIT) {
		rval = iav_drv_probe_vin_dsp_config(iav);
		if (rval < 0)
			goto iav_init_err;

		rval = iav_restore_dsp_cmd(iav);
		if (rval < 0)
			goto iav_init_err;

		if (iav->probe_state == IAV_STATE_ENCODING) {
			iav_sync_bsh_queue(iav);
		}
	}

	rval = iav_init_isr(iav);
	if (rval < 0)
		goto iav_init_err;

	return 0;

iav_init_err:
	/* FIXME: todo */
	iav_error("Failed to probe 'IAV' module.\n");

	if (iav->vinc[0] || iav->vinc[1]) {
		iav_vin_exit(iav);
	}
	if (iav_state_proc) {
		remove_proc_entry("iav", get_ambarella_proc_dir());
	}
	if (iav->iav_cdev.owner) {
		iav_destroy_dev(iav);
	}
	if (iav->dsp_enc_data_virt) {
		iav_encode_exit(iav);
	}
	if (iav->mmap_base) {
		iounmap((void __iomem *)iav->mmap_base);
	}
	if (iav->mmap_dsp_base) {
		iounmap((void __iomem *)iav->mmap_dsp_base);
	}
	if (iav) {
		kfree(iav);
	}
	return rval;
}

static int iav_drv_remove(struct ambpriv_device *ambdev)
{
	struct ambarella_iav *iav = ambpriv_get_drvdata(ambdev);

	/* FIXME: todo */
	iav_vin_exit(iav);
	remove_proc_entry("iav", get_ambarella_proc_dir());
	iav_destroy_dev(iav);
	iav_encode_exit(iav);

	iav_unmap_dsp_partitions(iav);
	if (iav->mmap_dsp_base) {
		iounmap((void __iomem *)iav->mmap_dsp_base);
	}
	kfree(iav);

	return 0;
}


struct ambpriv_device *iav_device;


#ifdef CONFIG_PM
static int iav_suspend(struct device *dev)
{
	struct ambarella_iav *iav = NULL;
	struct vin_device *vdev = NULL;

	iav = ambpriv_get_drvdata(iav_device);
	vdev = iav->vinc[0]->dev_active;

	if (iav->state == IAV_STATE_PREVIEW || iav->state == IAV_STATE_ENCODING) {
		// Imgproc suspend
		amba_imgproc_suspend(iav->encode_mode);
	}

	if (iav->dsp->suspend) {
		// DSP suspend
		iav->dsp->suspend(iav->dsp);
	}

	// DSP log suspends after DSP suspends
	amba_dsplog_suspend();

	if (vdev) {
		// VIN suspend
		vin_pm_suspend(vdev);
	}
	if (iav->state == IAV_STATE_PREVIEW || iav->state == IAV_STATE_ENCODING) {
		// Vout Suspend
		amba_vout_pm(AMBA_EVENT_PRE_PM);
	}

	hwtimer_suspend();

	return 0;
}

static int iav_resume(struct device *dev)
{
	struct ambarella_iav *iav = NULL;
	struct vin_device *vdev = NULL;
	u32 stream_map = 0;
	int i = 0;
	int max_enc_stream_num = 0;

	if (iav_device == NULL) {
		iav_error("iav_device should not be NULL!\n");
		return -1;
	}

	iav = ambpriv_get_drvdata(iav_device);
	vdev = iav->vinc[0]->dev_active;

	hwtimer_resume();
	if (iav->state == IAV_STATE_PREVIEW || iav->state == IAV_STATE_ENCODING) {
		// Vout resume
		amba_vout_pm(AMBA_EVENT_POST_PM);
	}
	if (vdev) {
		// VIN resume
		vin_pm_resume(vdev);
	}

	// DSP log resumes before DSP resumes
	amba_dsplog_resume();
	// set DSP debug level last sent before suspending
	if (iav->dsplog_setup.args[1]) {
		iav_set_dsplog_debug_level(iav, &iav->dsplog_setup);
	}

	if (iav->dsp->resume) {
		// DSP resume
		iav->dsp->resume(iav->dsp);
	}

	// IAV resume
	iav->resume_flag = 1;
	iav->fast_resume = 1;
	if (iav->dsp_enc_state == DSP_ENCODE_MODE) {
		switch (iav->state) {
		case IAV_STATE_IDLE:
			iav->state = IAV_STATE_INIT;
			iav->dsp_enc_state = DSP_UNKNOWN_MODE;
			iav_boot_dsp_action(iav);
			break;
		case IAV_STATE_PREVIEW:
			iav->state = IAV_STATE_INIT;
			iav->dsp_enc_state = DSP_UNKNOWN_MODE;
			iav_boot_dsp_action(iav);
			iav_enable_preview(iav);
			iav->state = IAV_STATE_PREVIEW;
			amba_imgproc_resume(iav->encode_mode, iav->fast_resume);
			if (iav->pm.enable) {
				iav_pm_resume(iav, 1);
			}
			// wait one vsync to let DSP process 3A cmds last sent
			wait_vcap_count(iav, 1);
			break;
		case IAV_STATE_ENCODING:
			iav->state = IAV_STATE_INIT;
			iav->dsp_enc_state = DSP_UNKNOWN_MODE;
			stream_map = iav_get_stream_map(iav);
			iav_clear_stream_state(iav);
			iav_boot_dsp_action(iav);
			iav_enable_preview(iav);
			iav->state = IAV_STATE_PREVIEW;
			amba_imgproc_resume(iav->encode_mode, iav->fast_resume);
			iav_ioc_start_encode(iav, (void __user *)stream_map);
			if (iav->pm.enable) {
				iav_pm_resume(iav, 0);
			}
			max_enc_stream_num = iav->system_config[iav->encode_mode].max_stream_num;
			for (i = 0; i < max_enc_stream_num; i++) {
				if (iav->stream[i].osd.enable) {
					iav_overlay_resume(iav, &iav->stream[i]);
				}
			}
			// wait one vsync to let DSP process 3A cmds last sent
			wait_vcap_count(iav, 1);
			break;
		default:
			break;
		}
	}
	iav->resume_flag = 0;
	iav->fast_resume = 0;

	return 0;
}
static SIMPLE_DEV_PM_OPS(iav_pm_ops, iav_suspend, iav_resume);
#endif

static struct ambpriv_driver iav_driver = {
	.probe = iav_drv_probe,
	.remove = iav_drv_remove,
	.driver = {
		.name = "iav_s2l",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &iav_pm_ops,
#endif
	}
};

static int __init iav_init(void)
{
	int rval = 0;

	iav_device = ambpriv_create_bundle(&iav_driver, NULL, -1, NULL, -1);
	if (IS_ERR(iav_device))
		rval = PTR_ERR(iav_device);

	return rval;
}

static void __exit iav_exit(void)
{
	ambpriv_device_unregister(iav_device);
	ambpriv_driver_unregister(&iav_driver);
}

module_init(iav_init);
module_exit(iav_exit);


MODULE_AUTHOR("Cao Rongrong<rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella iav driver");
MODULE_LICENSE("Proprietary");

