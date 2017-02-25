/*
 * uvc_stream.c
 *
 * Copyright (c) 2015 Ambarella Co., Ltd.
 * 		Jorney Tu <qtu@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include "u_uvc.h"
#include "uvc.h"
#include "uvc_configfs.h"
#include "uvc_stream.h"


#define  VIDEO_DEV	"/dev/iav"

int video_open_iav(struct uvc_device *uvc)
{
#if VIDEO_DEBUG
	uvc->video_filp = filp_open(VIDEO_DEBUG_FILE, O_RDWR, 0);
#else
	uvc->video_filp = filp_open(VIDEO_DEV, O_RDWR, 0);
#endif
	if(IS_ERR(uvc->video_filp)){
		printk("%s unable open video device\n", __FUNCTION__);
		return PTR_ERR(uvc->video_filp);
	}
	return 0;
}
int video_kthread_get_frame(void *arg)
{
	struct uvc_device *uvc = (struct uvc_device*)arg;
	struct iav_framedesc frame_desc;
	int ret = 0;
	int len, extra_len;

	unsigned long iav_buf_start = 0;
	unsigned long iav_buf_end = 0;

	unsigned long iav_virt = 0;
	unsigned int iav_size = 0;

	int unused ;

	iav_fetch_bsbinfo(&iav_virt, &iav_size);

	iav_buf_start = iav_virt;
	iav_buf_end = iav_virt + iav_size;

	while(1){

		unused = (uvc->current_used == 0) ? 1 : 0;

#if VIDEO_DEBUG
		{
			int size;
			char *buf;
			mm_segment_t fs;
			loff_t pos;

			size = vfs_llseek(file, 0 ,SEEK_END);

			if(uvc->current_used  == -1){
				uvc->current_used = 0;
				buf = uvc->frame[0].buf;
				uvc->frame[0].total_size = size;
				uvc->frame[0].bused_size = 0;
				uvc->frame[0].nodata = 0;
			}else{
				buf = uvc->frame[unused].buf;
				uvc->frame[unused].total_size = size;
				uvc->frame[unused].bused_size = 0;
				uvc->frame[unused].nodata = 0;
			}

			pos = 0;
			fs  = get_fs();
			set_fs(KERNEL_DS);

			ret = vfs_read(file, buf, size, &pos);

			set_fs(fs);
		}
#else
		memset(&frame_desc, 0, sizeof(frame_desc));
		frame_desc.id = -1;

		ret = iav_fetch_framedesc(&frame_desc);
		if(ret < 0){
			printk("%s get frame descriptor failed\n", __FUNCTION__);
			return 0;
		}

		if (frame_desc.size > FRAME_SIZE)
			BUG();

		if (uvc->current_used == -1){

			uvc->current_used = 0;

			if (frame_desc.data_addr_offset + frame_desc.size <= iav_buf_end){
#if UVC_FLICKER_FIXED
				uvc->frame[0].buf = (void*)frame_desc.data_addr_offset;
#else
				memcpy(uvc->frame[0].buf, (void *)frame_desc.data_addr_offset, frame_desc.size );
#endif
				uvc->frame[0].total_size = frame_desc.size;
				uvc->frame[0].bused_size = 0;
				uvc->frame[0].nodata = 0;
			}else{
				len = iav_buf_end - frame_desc.data_addr_offset;
#if UVC_FLICKER_FIXED
				uvc->frame[0].buf = (void*)frame_desc.data_addr_offset;
				uvc->frame[0].total_size = len;
				uvc->frame[0].bused_size = 0;
				uvc->frame[0].nodata = 0;
#else
				memcpy(uvc->frame[0].buf, (void *)frame_desc.data_addr_offset, len);

				extra_len = frame_desc.size - len;
				memcpy(uvc->frame[0].buf + len , (void *)iav_buf_start, extra_len);

				uvc->frame[0].total_size = frame_desc.size;
				uvc->frame[0].bused_size = 0;
				uvc->frame[0].nodata = 0;
#endif
			}
			continue;
		}

		if (frame_desc.data_addr_offset + frame_desc.size <= iav_buf_end){
#if UVC_FLICKER_FIXED
			uvc->frame[unused].buf = (void*)frame_desc.data_addr_offset;
#else
			memcpy(uvc->frame[unused].buf, (void *)frame_desc.data_addr_offset, frame_desc.size );
#endif
			uvc->frame[unused].total_size = frame_desc.size;
			uvc->frame[unused].bused_size = 0;
			uvc->frame[unused].nodata = 0;
		}else{
			len = iav_buf_end - frame_desc.data_addr_offset;
#if UVC_FLICKER_FIXED
			extra_len = 0;
			uvc->frame[unused].buf = (void*)frame_desc.data_addr_offset;
			uvc->frame[unused].total_size = len;    /* FIXME lost same frame data, but it's OK ?*/
			uvc->frame[unused].bused_size = 0;
			uvc->frame[unused].nodata = 0;
#else
			memcpy(uvc->frame[unused].buf, (void *)frame_desc.data_addr_offset, len);

			extra_len = frame_desc.size - len;
			memcpy(uvc->frame[unused].buf + len , (void *)iav_buf_start, len);

			uvc->frame[unused].total_size = frame_desc.size;
			uvc->frame[unused].bused_size = 0;
			uvc->frame[unused].nodata = 0;
#endif
		}

#endif

		//msleep(1);
	}

	return 0;

}
int video_close_iav(struct uvc_device *uvc)
{
	if(uvc->video_filp)
		filp_close(uvc->video_filp, NULL);

	return 0;
}
