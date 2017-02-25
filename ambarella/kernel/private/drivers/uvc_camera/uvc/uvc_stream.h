#ifndef __UVC_STREAM__
#define __UVC_STREAM__
#include <iav_ioctl.h>


extern int iav_fetch_bsbinfo(unsigned long *iav_bsb_virt, u32 *iav_bsb_size);

extern int iav_fetch_framedesc(struct iav_framedesc *);

int video_open_iav(struct uvc_device *uvc);
int video_kthread_get_frame(void *arg);
int video_close_iav(struct uvc_device *uvc);

#endif
