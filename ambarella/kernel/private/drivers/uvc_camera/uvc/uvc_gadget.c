
/*
 * uvc_gadget.c
 *
 * Copyright (c) 2015 Ambarella Co., Ltd.
 * 		Jorney Tu <qtu@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
static void
uvc_fill_streaming_control(struct uvc_streaming_control *ctrl)// 1, 0
{

	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = 0 + 1;
	ctrl->bFrameIndex = 0 + 1;
	ctrl->dwFrameInterval = 666666;
	ctrl->dwMaxVideoFrameSize = 64 * 1024;
	ctrl->dwMaxPayloadTransferSize = 512;	/* TODO this should be filled by the driver. */
	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;
}
static void
uvc_events_process_control( uint8_t req, uint8_t cs,
		struct usb_request *resp)
{
	char *buf = (char *)resp->buf;
#if 0
	printk("control request (req %02x cs %02x)\n", req, cs);
#endif
	switch (req) {
	case UVC_SET_CUR:
		buf[0] = 0x00;
		resp->length = 1;
		break;

	case UVC_GET_CUR:
		buf[0] = 0x00;
		buf[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		buf[0] = 0x00;
		buf[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_RES:
		buf[0] = 0x00;
		buf[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_LEN:
		buf[0] = 0x00;
		buf[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_INFO:
		buf[0] = 0x03;
		resp->length = 1;
		break;
	}
}

static void
uvc_events_process_streaming( uint8_t req, uint8_t cs,
			     struct usb_request *resp)
{
	struct uvc_streaming_control *ctrl;
	char *buf = (char *)resp->buf;

#if 0
	printk("streaming request (req %02x cs %02x)\n", req, cs);
#endif

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
		return;

	ctrl = (struct uvc_streaming_control *)resp->buf;
	resp->length = sizeof *ctrl;

	switch (req) {
	case UVC_SET_CUR:
		resp->length = 34;
		break;

	case UVC_GET_CUR:

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		uvc_fill_streaming_control(ctrl);
		break;

	case UVC_GET_RES:
		memset(ctrl, 0, sizeof *ctrl);
		break;

	case UVC_GET_LEN:
		buf[0] = 0x00;
		buf[1] = 0x22;
		resp->length = 2;
		break;

	case UVC_GET_INFO:
		buf[0] = 0x03;
		resp->length = 1;
		break;
	}
}


static void
uvc_events_process_class(const struct usb_ctrlrequest *ctrl,
			 struct usb_request *resp)
{

	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		uvc_events_process_control(ctrl->bRequest, ctrl->wValue >> 8, resp);
		break;

#if AUDIO_OPEN
	case 3:
#else
	case UVC_INTF_STREAMING:
#endif
		uvc_events_process_streaming(ctrl->bRequest, ctrl->wValue >> 8, resp);
		break;

	default:
		break;
	}
}
static void
uvc_events_process_setup(const struct usb_ctrlrequest *ctrl, struct usb_request *req)
{

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		break;

	case USB_TYPE_CLASS:
		uvc_events_process_class(ctrl, req);
		break;

	default:
		break;
	}
}
