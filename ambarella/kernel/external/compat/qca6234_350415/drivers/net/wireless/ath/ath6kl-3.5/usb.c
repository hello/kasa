/*
 * Copyright (c) 2007-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/reboot.h>
#include <linux/module.h>
#include <linux/usb.h>

#include "debug.h"
#include "core.h"
#include "cfg80211.h"

#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
#include <asm/unaligned.h>
#endif

/* constants */
#define TX_URB_COUNT            10
#define TX_URB_COUNT_LARGE        40
#define RX_URB_COUNT            32

#define ATH6KL_USB_RX_BUFFER_SIZE  2048
#define ATH6KL_USB_RX_BUNDLE_BUFFER_SIZE  16896
#define ATH6KL_USB_TX_BUNDLE_BUFFER_SIZE  16384
#define WORKER_LOCK_BIT	0

#define ATH6KL_MAX_AMSDU_SIZE  7935

/* tx/rx pipes for usb */
enum ATH6KL_USB_PIPE_ID {
	ATH6KL_USB_PIPE_TX_CTRL = 0,
	ATH6KL_USB_PIPE_TX_DATA_LP,
	ATH6KL_USB_PIPE_TX_DATA_MP,
	ATH6KL_USB_PIPE_TX_DATA_HP,
	ATH6KL_USB_PIPE_TX_DATA_VHP,
	ATH6KL_USB_PIPE_RX_CTRL,
	ATH6KL_USB_PIPE_RX_DATA,
	ATH6KL_USB_PIPE_RX_DATA2,
	ATH6KL_USB_PIPE_RX_INT,
	ATH6KL_USB_PIPE_MAX
};

#define ATH6KL_USB_PIPE_INVALID ATH6KL_USB_PIPE_MAX

struct ath6kl_usb_pipe_stat {
	u32 num_rx_comp;
	u32 num_tx_comp;
	u32 num_io_comp;

	u32 num_max_tx;
	u32 num_max_rx;

	u32 num_tx_resche;
	u32 num_rx_resche;

	u32 num_tx_sync;
	u32 num_tx;
	u32 num_tx_err;
	u32 num_tx_err_others;

	u32 num_tx_comp_err;
	u32 num_rx_comp_err;

	u32 num_rx_bundle_comp;
	u32 num_tx_bundle_comp;

	u32 num_tx_multi;
	u32 num_tx_multi_err;
	u32 num_tx_multi_err_others;

	u32 num_rx_bundle_comp_err;
	u32 num_tx_bundle_comp_err;
};

struct ath6kl_usb_pipe {
	struct list_head urb_list_head;
	struct usb_anchor urb_submitted;
	u32 urb_alloc;
	u32 urb_cnt;
	u32 urb_cnt_thresh;
	unsigned int usb_pipe_handle;
	u32 flags;
	u8 ep_address;
	u8 logical_pipe_num;
	struct ath6kl_usb *ar_usb;
	u16 max_packet_size;
	struct work_struct io_complete_work;
	struct sk_buff_head io_comp_queue;
	struct usb_endpoint_descriptor *ep_desc;
	struct ath6kl_usb_pipe_stat usb_pipe_stat;
	unsigned long worker_lock;
};

#define ATH6KL_USB_PIPE_FLAG_TX    (1 << 0)
#define ATH6KL_USB_PIPE_FLAG_RX    (1 << 1)

/* usb device object */
struct ath6kl_usb {
	spinlock_t cs_lock;
	spinlock_t tx_lock;
	spinlock_t rx_lock;
	struct ath6kl_hif_pipe_callbacks htc_callbacks;
	struct usb_device *udev;
	struct usb_interface *interface;
	struct ath6kl_usb_pipe pipes[ATH6KL_USB_PIPE_MAX];
	u8 *diag_cmd_buffer;
	u8 *diag_resp_buffer;
	struct ath6kl *ar;
	struct notifier_block reboot_notifier;  /* default mode before reboot */
	u32 max_sche_tx;
	u32 max_sche_rx;
	u32 rxq_threshold;
};

/* usb urb object */
struct ath6kl_urb_context {
	struct list_head link;
	struct ath6kl_usb_pipe *pipe;
	struct sk_buff *buf;
	struct ath6kl *ar;
	struct sk_buff_head comp_queue;
};

/* USB endpoint definitions */
#define ATH6KL_USB_EP_ADDR_APP_CTRL_IN          0x81
#define ATH6KL_USB_EP_ADDR_APP_DATA_IN          0x82
#define ATH6KL_USB_EP_ADDR_APP_DATA2_IN         0x83
#define ATH6KL_USB_EP_ADDR_APP_INT_IN           0x84

#define ATH6KL_USB_EP_ADDR_APP_CTRL_OUT         0x01
#define ATH6KL_USB_EP_ADDR_APP_DATA_LP_OUT      0x02
#define ATH6KL_USB_EP_ADDR_APP_DATA_MP_OUT      0x03
#define ATH6KL_USB_EP_ADDR_APP_DATA_HP_OUT      0x04
#define ATH6KL_USB_EP_ADDR_APP_DATA_VHP_OUT     0x05

/* diagnostic command defnitions */
#define ATH6KL_USB_CONTROL_REQ_SEND_BMI_CMD        1
#define ATH6KL_USB_CONTROL_REQ_RECV_BMI_RESP       2
#define ATH6KL_USB_CONTROL_REQ_DIAG_CMD            3
#define ATH6KL_USB_CONTROL_REQ_DIAG_RESP           4

#define ATH6KL_USB_CTRL_DIAG_CC_READ               0
#define ATH6KL_USB_CTRL_DIAG_CC_WRITE              1
#define ATH6KL_USB_CTRL_DIAG_CC_WARM_RESET         2

#define HIF_USB_RX_QUEUE_THRESHOLD              64

/* Enable it by default */
#define HIF_USB_MAX_SCHE_PKT				(64)

struct ath6kl_usb_ctrl_diag_cmd_write {
	__le32 cmd;
	__le32 address;
	__le32 value;
	__le32 _pad[1];
} __packed;

struct ath6kl_usb_ctrl_diag_cmd_read {
	__le32 cmd;
	__le32 address;
} __packed;

struct ath6kl_usb_ctrl_diag_resp_read {
	__le32 value;
} __packed;

#define USB_CTRL_MAX_DIAG_CMD_SIZE	\
	(sizeof(struct ath6kl_usb_ctrl_diag_cmd_write))
#define USB_CTRL_MAX_DIAG_RESP_SIZE	\
	(sizeof(struct ath6kl_usb_ctrl_diag_resp_read))

#ifdef ATH6KL_BUS_VOTE
u8 ath6kl_platform_has_vreg;
struct semaphore usb_probe_sem;

#define USB_PROBE_WAIT_TIMEOUT           2000
#endif

#ifdef ATH6KL_HSIC_RECOVER
struct work_struct recover_war_work;
#endif

#ifdef ATHTST_SUPPORT
struct hif_product_info_t {
	uint16_t	idVendor;
	uint16_t	idProduct;
	uint8_t	product[64];
	uint8_t	manufacturer[64];
	uint8_t	serial[64];
};
#endif
/* function declarations */
#ifdef CONFIG_PM

static int ath6kl_usb_pm_suspend(struct usb_interface *interface,
			      pm_message_t message);
static int ath6kl_usb_pm_resume(struct usb_interface *interface);
static int ath6kl_usb_pm_reset_resume(struct usb_interface *intf);

#else

#define ath6kl_usb_pm_suspend NULL
#define ath6kl_usb_pm_resume NULL
#define ath6kl_usb_pm_reset_resume NULL

#endif

#ifdef CONFIG_ANDROID
/* variables for unload driver module */
#define ATH6KL_USB_UNLOAD_TIMEOUT		(2*HZ)
enum ath6kl_usb_drv_unload_state {
	ATH6KL_USB_UNLOAD_STATE_NULL = 0,
	ATH6KL_USB_UNLOAD_STATE_DRV_DEREG,
	ATH6KL_USB_UNLOAD_STATE_TARGET_RESET,
	ATH6KL_USB_UNLOAD_STATE_DEV_DISCONNECTED,
};

static int ath6kl_usb_unload_dev_num = -1;
static wait_queue_head_t ath6kl_usb_unload_event_wq;
static atomic_t ath6kl_usb_unload_state;
#endif

static void ath6kl_usb_recv_complete(struct urb *urb);
static void ath6kl_usb_recv_bundle_complete(struct urb *urb);

#ifdef USB_AUTO_SUSPEND
static void usb_auto_pm_turnoff(struct ath6kl *ar);
#endif

#define ATH6KL_USB_IS_BULK_EP(attr) (((attr) & 3) == 0x02)
#define ATH6KL_USB_IS_INT_EP(attr)  (((attr) & 3) == 0x03)
#define ATH6KL_USB_IS_ISOC_EP(attr)  (((attr) & 3) == 0x01)
#define ATH6KL_USB_IS_DIR_IN(addr)  ((addr) & 0x80)

/* pipe/urb operations */
static struct ath6kl_urb_context *ath6kl_usb_alloc_urb_from_pipe(
						struct ath6kl_usb_pipe *pipe)
{
	struct ath6kl_urb_context *urb_context = NULL;
	unsigned long flags;

	spin_lock_irqsave(&pipe->ar_usb->cs_lock, flags);
	if (!list_empty(&pipe->urb_list_head)) {
		urb_context =
		    list_first_entry(&pipe->urb_list_head,
				     struct ath6kl_urb_context, link);
		list_del(&urb_context->link);
		pipe->urb_cnt--;
	}
	spin_unlock_irqrestore(&pipe->ar_usb->cs_lock, flags);

	return urb_context;
}

static void ath6kl_usb_free_urb_to_pipe(struct ath6kl_usb_pipe *pipe,
				     struct ath6kl_urb_context *urb_context)
{
	unsigned long flags;

	spin_lock_irqsave(&pipe->ar_usb->cs_lock, flags);
	pipe->urb_cnt++;

	list_add(&urb_context->link, &pipe->urb_list_head);
	spin_unlock_irqrestore(&pipe->ar_usb->cs_lock, flags);
}

static void ath6kl_usb_cleanup_recv_urb(struct ath6kl_urb_context *urb_context)
{
	if (urb_context->buf != NULL) {
		dev_kfree_skb(urb_context->buf);
		urb_context->buf = NULL;
	}

	ath6kl_usb_free_urb_to_pipe(urb_context->pipe, urb_context);
}

static inline struct ath6kl_usb *ath6kl_usb_priv(struct ath6kl *ar)
{
	return ar->hif_priv;
}

/* pipe resource allocation/cleanup */
static int ath6kl_usb_alloc_pipe_resources(struct ath6kl_usb_pipe *pipe,
					   int urb_cnt)
{
	int status = 0;
	int i;
	struct ath6kl_urb_context *urb_context;

	INIT_LIST_HEAD(&pipe->urb_list_head);
	init_usb_anchor(&pipe->urb_submitted);

	for (i = 0; i < urb_cnt; i++) {
		urb_context = (struct ath6kl_urb_context *)
		    kzalloc(sizeof(struct ath6kl_urb_context), GFP_KERNEL);
		if (urb_context == NULL) {
			status = -ENOMEM;
			goto fail_alloc_pipe_resources;
		}

		memset(urb_context, 0, sizeof(struct ath6kl_urb_context));
		urb_context->pipe = pipe;

		/*
		 * we are only allocate the urb contexts here, the actual URB
		 * is allocated from the kernel as needed to do a transaction
		 */
		pipe->urb_alloc++;

		if (htc_bundle_send) {
			/* In tx bundle mode, only pre-allocate bundle buffers
			* for data pipes
			*/
		    if (pipe->logical_pipe_num >= ATH6KL_USB_PIPE_TX_DATA_LP &&
			pipe->logical_pipe_num <= ATH6KL_USB_PIPE_TX_DATA_VHP) {
			urb_context->buf =
				dev_alloc_skb(ATH6KL_USB_TX_BUNDLE_BUFFER_SIZE);
			if (NULL == urb_context->buf)
				ath6kl_dbg(ATH6KL_DBG_USB,
				"athusb: alloc send bundle buffer %d-byte "
				"failed\n",
				ATH6KL_USB_TX_BUNDLE_BUFFER_SIZE);
		    }
		    skb_queue_head_init(&urb_context->comp_queue);
		}

		ath6kl_usb_free_urb_to_pipe(pipe, urb_context);
	}
	ath6kl_dbg(ATH6KL_DBG_USB,
		   "ath6kl usb: alloc resources lpipe:%d"
		   "hpipe:0x%X urbs:%d\n",
		   pipe->logical_pipe_num, pipe->usb_pipe_handle,
		   pipe->urb_alloc);

fail_alloc_pipe_resources:
	return status;
}

static void ath6kl_usb_free_pipe_resources(struct ath6kl_usb_pipe *pipe)
{
	struct ath6kl_urb_context *urb_context;
	struct sk_buff *buf = NULL;

	if (pipe->ar_usb == NULL) {
		/* nothing allocated for this pipe */
		return;
	}

	ath6kl_dbg(ATH6KL_DBG_USB,
		   "ath6kl usb: free resources lpipe:%d"
		   "hpipe:0x%X urbs:%d avail:%d\n",
		   pipe->logical_pipe_num, pipe->usb_pipe_handle,
		   pipe->urb_alloc, pipe->urb_cnt);

	if (pipe->urb_alloc != pipe->urb_cnt) {
		ath6kl_dbg(ATH6KL_DBG_USB,
			   "ath6kl usb: urb leak! lpipe:%d"
			   "hpipe:0x%X urbs:%d avail:%d\n",
			   pipe->logical_pipe_num, pipe->usb_pipe_handle,
			   pipe->urb_alloc, pipe->urb_cnt);
	}

	while (true) {
		urb_context = ath6kl_usb_alloc_urb_from_pipe(pipe);
		if (urb_context == NULL)
			break;

		if (htc_bundle_send) {
			while ((buf = skb_dequeue(&urb_context->comp_queue))
				!= NULL)
				dev_kfree_skb(buf);
			if (pipe->logical_pipe_num >=
				ATH6KL_USB_PIPE_TX_DATA_LP &&
			    pipe->logical_pipe_num <=
				ATH6KL_USB_PIPE_TX_DATA_VHP)
				if (urb_context->buf != NULL) {
					dev_kfree_skb(urb_context->buf);
					urb_context->buf = NULL;
				}
		}
		if (htc_bundle_recv)
			if (pipe->logical_pipe_num == ATH6KL_USB_PIPE_RX_DATA)
				if (urb_context->buf != NULL) {
					dev_kfree_skb(urb_context->buf);
					urb_context->buf = NULL;
				}

		kfree(urb_context);
	}

}

static void ath6kl_usb_cleanup_pipe_resources(struct ath6kl_usb *device)
{
	int i;

	for (i = 0; i < ATH6KL_USB_PIPE_MAX; i++)
		ath6kl_usb_free_pipe_resources(&device->pipes[i]);

}

static u8 ath6kl_usb_get_logical_pipe_num(struct ath6kl_usb *device,
				       u8 ep_address, int *urb_count)
{
	u8 pipe_num = ATH6KL_USB_PIPE_INVALID;

	switch (ep_address) {
	case ATH6KL_USB_EP_ADDR_APP_CTRL_IN:
		pipe_num = ATH6KL_USB_PIPE_RX_CTRL;
		*urb_count = RX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA_IN:
		pipe_num = ATH6KL_USB_PIPE_RX_DATA;
		*urb_count = RX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_INT_IN:
		pipe_num = ATH6KL_USB_PIPE_RX_INT;
		*urb_count = RX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA2_IN:
		pipe_num = ATH6KL_USB_PIPE_RX_DATA2;
		*urb_count = RX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_CTRL_OUT:
		pipe_num = ATH6KL_USB_PIPE_TX_CTRL;
		*urb_count = TX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA_LP_OUT:
		pipe_num = ATH6KL_USB_PIPE_TX_DATA_LP;
		*urb_count = TX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA_MP_OUT:
		pipe_num = ATH6KL_USB_PIPE_TX_DATA_MP;
		*urb_count = TX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA_HP_OUT:
		pipe_num = ATH6KL_USB_PIPE_TX_DATA_HP;
		*urb_count = TX_URB_COUNT;
		break;
	case ATH6KL_USB_EP_ADDR_APP_DATA_VHP_OUT:
		pipe_num = ATH6KL_USB_PIPE_TX_DATA_VHP;
		*urb_count = TX_URB_COUNT;
		break;
	default:
		/* note: there may be endpoints not currently used */
		break;
	}

	return pipe_num;
}

static int ath6kl_usb_setup_pipe_resources(struct ath6kl_usb *device)
{
	struct usb_interface *interface = device->interface;
	struct usb_host_interface *iface_desc = interface->cur_altsetting;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int urbcount;
	int status = 0;
	struct ath6kl_usb_pipe *pipe;
	u8 pipe_num;
	ath6kl_dbg(ATH6KL_DBG_USB, "setting up USB Pipes using interface\n");
	/* walk decriptors and setup pipes */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (ATH6KL_USB_IS_BULK_EP(endpoint->bmAttributes)) {
			ath6kl_dbg(ATH6KL_DBG_USB,
				   "%s Bulk Ep:0x%2.2X maxpktsz:%d\n",
				   ATH6KL_USB_IS_DIR_IN
				   (endpoint->bEndpointAddress) ?
				   "RX" : "TX", endpoint->bEndpointAddress,
				   le16_to_cpu(endpoint->wMaxPacketSize));
		} else if (ATH6KL_USB_IS_INT_EP(endpoint->bmAttributes)) {
			ath6kl_dbg(ATH6KL_DBG_USB,
				   "%s Int Ep:0x%2.2X maxpktsz:%d interval:%d\n",
				   ATH6KL_USB_IS_DIR_IN
				   (endpoint->bEndpointAddress) ?
				   "RX" : "TX", endpoint->bEndpointAddress,
				   le16_to_cpu(endpoint->wMaxPacketSize),
				   endpoint->bInterval);
		} else if (ATH6KL_USB_IS_ISOC_EP(endpoint->bmAttributes)) {
			/* TODO for ISO */
			ath6kl_dbg(ATH6KL_DBG_USB,
				   "%s ISOC Ep:0x%2.2X maxpktsz:%d interval:%d\n",
				   ATH6KL_USB_IS_DIR_IN
				   (endpoint->bEndpointAddress) ?
				   "RX" : "TX", endpoint->bEndpointAddress,
				   le16_to_cpu(endpoint->wMaxPacketSize),
				   endpoint->bInterval);
		}
		urbcount = 0;

		pipe_num =
		    ath6kl_usb_get_logical_pipe_num(device,
						 endpoint->bEndpointAddress,
						 &urbcount);
		if (pipe_num == ATH6KL_USB_PIPE_INVALID)
			continue;

		pipe = &device->pipes[pipe_num];
		if (pipe->ar_usb != NULL) {
			/* hmmm..pipe was already setup */
			continue;
		}

		pipe->ar_usb = device;
		pipe->logical_pipe_num = pipe_num;
		pipe->ep_address = endpoint->bEndpointAddress;
		pipe->max_packet_size = le16_to_cpu(endpoint->wMaxPacketSize);

		if (ATH6KL_USB_IS_BULK_EP(endpoint->bmAttributes)) {
			if (ATH6KL_USB_IS_DIR_IN(pipe->ep_address)) {
				pipe->usb_pipe_handle =
				    usb_rcvbulkpipe(device->udev,
						    pipe->ep_address);
			} else {
				pipe->usb_pipe_handle =
				    usb_sndbulkpipe(device->udev,
						    pipe->ep_address);
			}
		} else if (ATH6KL_USB_IS_INT_EP(endpoint->bmAttributes)) {
			if (ATH6KL_USB_IS_DIR_IN(pipe->ep_address)) {
				pipe->usb_pipe_handle =
				    usb_rcvintpipe(device->udev,
						   pipe->ep_address);
			} else {
				pipe->usb_pipe_handle =
				    usb_sndintpipe(device->udev,
						   pipe->ep_address);
			}
		} else if (ATH6KL_USB_IS_ISOC_EP(endpoint->bmAttributes)) {
			/* TODO for ISO */
			if (ATH6KL_USB_IS_DIR_IN(pipe->ep_address)) {
				pipe->usb_pipe_handle =
				    usb_rcvisocpipe(device->udev,
						    pipe->ep_address);
			} else {
				pipe->usb_pipe_handle =
				    usb_sndisocpipe(device->udev,
						    pipe->ep_address);
			}
		}
		pipe->ep_desc = endpoint;

		if (!ATH6KL_USB_IS_DIR_IN(pipe->ep_address))
			pipe->flags |= ATH6KL_USB_PIPE_FLAG_TX;
		else
			pipe->flags |= ATH6KL_USB_PIPE_FLAG_RX;

		status = ath6kl_usb_alloc_pipe_resources(pipe, urbcount);
		if (status != 0)
			break;

	}

	return status;
}

/* pipe operations */
static void ath6kl_usb_post_recv_transfers(struct ath6kl_usb_pipe *recv_pipe,
					int buffer_length)
{
	struct ath6kl_urb_context *urb_context;
	u8 *data;
	u32 len;
	struct urb *urb;
	int usb_status;

	while (1) {

		urb_context = ath6kl_usb_alloc_urb_from_pipe(recv_pipe);
		if (urb_context == NULL)
			break;

		urb_context->buf = dev_alloc_skb(buffer_length);
		if (urb_context->buf == NULL)
			goto err_cleanup_urb;

		data = urb_context->buf->data;
		len = urb_context->buf->len;

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (urb == NULL)
			goto err_cleanup_urb;

		usb_fill_bulk_urb(urb,
				  recv_pipe->ar_usb->udev,
				  recv_pipe->usb_pipe_handle,
				  data,
				  buffer_length,
				  ath6kl_usb_recv_complete, urb_context);

		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "ath6kl usb: bulk recv submit:%d, 0x%X"
			   "(ep:0x%2.2X), %d bytes buf:0x%p\n",
			   recv_pipe->logical_pipe_num,
			   recv_pipe->usb_pipe_handle, recv_pipe->ep_address,
			   buffer_length, urb_context->buf);

		usb_anchor_urb(urb, &recv_pipe->urb_submitted);
		usb_status = usb_submit_urb(urb, GFP_ATOMIC);

		if (usb_status) {
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb : usb bulk recv failed %d\n",
				   usb_status);
			usb_unanchor_urb(urb);
			usb_free_urb(urb);
			goto err_cleanup_urb;
		}
		usb_free_urb(urb);
	}
	return;

err_cleanup_urb:
	ath6kl_usb_cleanup_recv_urb(urb_context);
	return;
}

static void hif_usb_post_recv_bundle_transfers(
				struct ath6kl_usb_pipe *recv_pipe,
				int buffer_length)
{
	struct ath6kl_urb_context *urb_context;
	u8 *data;
	u32 len;
	struct urb *urb;
	int usb_status;

	while (1) {
		urb_context = ath6kl_usb_alloc_urb_from_pipe(recv_pipe);
		if (urb_context == NULL)
			break;
		if (buffer_length) {
			urb_context->buf = dev_alloc_skb(buffer_length);
			if (urb_context->buf == NULL) {
				ath6kl_usb_cleanup_recv_urb(urb_context);
				break;
			}
		}

		data = urb_context->buf->data;
		len = urb_context->buf->len;

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (urb == NULL) {
			ath6kl_usb_cleanup_recv_urb(urb_context);
			break;
		}

		usb_fill_bulk_urb(urb,
				recv_pipe->ar_usb->udev,
				recv_pipe->usb_pipe_handle,
				data,
				ATH6KL_USB_RX_BUNDLE_BUFFER_SIZE,
				ath6kl_usb_recv_bundle_complete, urb_context);

		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "ath6kl usb: bulk recv submit:%d, 0x%X"
			   "(ep:0x%2.2X), %d bytes buf:0x%p\n",
			   recv_pipe->logical_pipe_num,
			   recv_pipe->usb_pipe_handle, recv_pipe->ep_address,
			   buffer_length, urb_context->buf);

		usb_anchor_urb(urb, &recv_pipe->urb_submitted);
		usb_status = usb_submit_urb(urb, GFP_ATOMIC);

		if (usb_status) {
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb : usb bulk recv failed %d\n",
				   usb_status);
			usb_unanchor_urb(urb);
			usb_free_urb(urb);
			ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
						    urb_context);
			break;
		}
		usb_free_urb(urb);
	}
	return;
}


static void ath6kl_usb_flush_all(struct ath6kl_usb *device)
{
	int i;
	struct ath6kl_usb_pipe *pipe;

	for (i = 0; i < ATH6KL_USB_PIPE_MAX; i++) {
		/* flush only USB scheduled work, instead of flushing all */
		if (device->pipes[i].ar_usb) {
			if (&device->pipes[i].urb_submitted)
				usb_kill_anchored_urbs(
					&device->pipes[i].urb_submitted);
			pipe = &device->pipes[i].ar_usb->pipes[i];
			if (pipe)
				flush_work(&pipe->io_complete_work);
		}
	}

	/* flushing any pending I/O may schedule work
	 * this call will block until all scheduled work runs to completion */
	/* flush_scheduled_work(); */
}

static void ath6kl_usb_start_recv_pipes(struct ath6kl_usb *device)
{
	/*
	 * note: control pipe is no longer used
	 * device->pipes[ATH6KL_USB_PIPE_RX_CTRL].urb_cnt_thresh =
	 *      device->pipes[ATH6KL_USB_PIPE_RX_CTRL].urb_alloc/2;
	 * ath6kl_usb_post_recv_transfers(&device->
	 *		pipes[ATH6KL_USB_PIPE_RX_CTRL],
	 *		ATH6KL_USB_RX_BUFFER_SIZE);
	 */

	device->pipes[ATH6KL_USB_PIPE_RX_DATA].urb_cnt_thresh =
	    device->pipes[ATH6KL_USB_PIPE_RX_DATA].urb_alloc / 2;

	if (!htc_bundle_recv)
		ath6kl_usb_post_recv_transfers(
					&device->pipes[ATH6KL_USB_PIPE_RX_DATA],
					ATH6KL_USB_RX_BUFFER_SIZE);
	else
		hif_usb_post_recv_bundle_transfers(
					&device->pipes[ATH6KL_USB_PIPE_RX_DATA],
					ATH6KL_USB_RX_BUNDLE_BUFFER_SIZE);
	/*
	* Disable rxdata2 directly, it will be enabled
	* if FW enable rxdata2
	*/
	if (0) {
		device->pipes[ATH6KL_USB_PIPE_RX_DATA2].urb_cnt_thresh =
			device->pipes[ATH6KL_USB_PIPE_RX_DATA2].urb_alloc / 2;
		ath6kl_usb_post_recv_transfers(
				&device->pipes[ATH6KL_USB_PIPE_RX_DATA2],
				ATH6KL_USB_RX_BUFFER_SIZE);
	}
}

/* hif usb rx/tx completion functions */
static void ath6kl_usb_recv_complete(struct urb *urb)
{
	struct ath6kl_urb_context *urb_context =
	    (struct ath6kl_urb_context *)urb->context;
	int status = 0;
	struct sk_buff *buf = NULL;
	struct ath6kl_usb_pipe *pipe = urb_context->pipe;
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	struct ath6kl *ar = pipe->ar_usb->ar;
#endif

	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
		   "%s: recv pipe: %d, stat:%d, len:%d urb:0x%p\n", __func__,
		   pipe->logical_pipe_num, urb->status, urb->actual_length,
		   urb);

	if (urb->status != 0) {
		pipe_st->num_rx_comp_err++;
		status = -EIO;
		switch (urb->status) {
		case -EOVERFLOW:
			urb->actual_length = ATH6KL_USB_RX_BUFFER_SIZE;
			status = 0;
			break;
		case -ECONNRESET:
		case -ENOENT:
		case -ESHUTDOWN:
			/*
			 * no need to spew these errors when device
			 * removed or urb killed due to driver shutdown
			 */
			status = -ECANCELED;
			break;
		default:
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "%s recv pipe: %d (ep:0x%2.2X), failed:%d\n",
				   __func__, pipe->logical_pipe_num,
				   pipe->ep_address, urb->status);
			break;
		}
		goto cleanup_recv_urb;
	}
	if (urb->actual_length == 0)
		goto cleanup_recv_urb;

	buf = urb_context->buf;
	/* we are going to pass it up */
	urb_context->buf = NULL;
	skb_put(buf, urb->actual_length);
	/* note: queue implements a lock */
	skb_queue_tail(&pipe->io_comp_queue, buf);
	pipe_st->num_rx_comp++;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	queue_work(ar->ath6kl_wq, &pipe->io_complete_work);
#else
	schedule_work(&pipe->io_complete_work);
#endif

cleanup_recv_urb:
	ath6kl_usb_cleanup_recv_urb(urb_context);

	if (status == 0 || urb->status == -EPROTO) {
		if (pipe->urb_cnt >= pipe->urb_cnt_thresh &&
				skb_queue_len(&pipe->io_comp_queue) <
				pipe->ar_usb->rxq_threshold) {
			/* our free urbs are piling up, post more transfers */
			ath6kl_usb_post_recv_transfers(pipe,
						    ATH6KL_USB_RX_BUFFER_SIZE);
		}
	}
	return;
}

static void ath6kl_usb_recv_bundle_complete(struct urb *urb)
{
	struct ath6kl_urb_context *urb_context =
		(struct ath6kl_urb_context *)urb->context;
	int status = 0;
	struct sk_buff *buf = NULL;
	struct ath6kl_usb_pipe *pipe = urb_context->pipe;
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
	u8 *netdata, *netdata_new;
	u32 netlen, netlen_new;
	struct htc_frame_hdr *htc_hdr;
	u16 payload_len;
	struct sk_buff *new_skb = NULL;
	struct ath6kl *ar = pipe->ar_usb->ar;

	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
		"%s: recv pipe: %d, stat:%d, len:%d urb:0x%p\n", __func__,
		pipe->logical_pipe_num, urb->status, urb->actual_length,
		urb);

	do {

		if (urb->status != 0) {
			pipe_st->num_rx_bundle_comp_err++;
			status = -EIO;
			switch (urb->status) {
			case -EOVERFLOW:
				urb->actual_length =
					ATH6KL_USB_RX_BUNDLE_BUFFER_SIZE;
				status = 0;
				break;
			case -ECONNRESET:
			case -ENOENT:
			case -ESHUTDOWN:
				/*
				* no need to spew these errors when device
				* removed or urb killed due to driver shutdown
				*/
				status = -ECANCELED;
				break;
			default:
				ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "%s recv pipe: %d (ep:0x%2.2X), failed:%d\n",
				   __func__, pipe->logical_pipe_num,
				   pipe->ep_address, urb->status);
			}
			break;
		}
		if (urb->actual_length == 0)
			break;
		buf = urb_context->buf;

		netdata = buf->data;
		netlen = buf->len;
		netlen = urb->actual_length;

		do {
			u8 extra_pad = 0;
			u16 act_frame_len = 0;
			u16 frame_len;

			/* Hack into HTC header for bundle processing */
			htc_hdr = (struct htc_frame_hdr *)netdata;
			if (htc_hdr->eid >= ENDPOINT_MAX) {
				ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				  "athusb: Rx: invalid eid=%d\n", htc_hdr->eid);
				break;
			}

			payload_len =
			 le16_to_cpu(get_unaligned((u16 *)&htc_hdr->payld_len));

			if (ar->hw.flags & ATH6KL_HW_TGT_ALIGN_PADDING) {
				act_frame_len = (HTC_HDR_LENGTH + payload_len);

				if (htc_hdr->eid == 0 || htc_hdr->eid == 1)
					/* assumption: target won't pad on
					* HTC endpoint 0 & 1.
					*/
					extra_pad = 0;
				else
					extra_pad =
					 get_unaligned((u8 *)&htc_hdr->ctrl[1]);
			}

			if (payload_len > ATH6KL_MAX_AMSDU_SIZE) {
				ath6kl_dbg(ATH6KL_DBG_USB_BULK,
					"athusb: payload_len too long %u\n",
					payload_len);
				break;
			}

			if (ar->hw.flags & ATH6KL_HW_TGT_ALIGN_PADDING)
				frame_len = (act_frame_len + extra_pad);
			else
				frame_len = (HTC_HDR_LENGTH + payload_len);

			if (netlen >= frame_len) {
				/* allocate a new skb and copy */
				if (ar->hw.flags &
					ATH6KL_HW_TGT_ALIGN_PADDING) {
					new_skb = dev_alloc_skb(act_frame_len);
					if (new_skb == NULL) {
						ath6kl_dbg(ATH6KL_DBG_USB_BULK,
						 "athusb: allocate skb (len=%u) "
						 "failed\n", act_frame_len);
						break;
					}

					netdata_new = new_skb->data;
					netlen_new = new_skb->len;
					memcpy(netdata_new,
						netdata, act_frame_len);
					skb_put(new_skb, act_frame_len);
				} else {
					new_skb = dev_alloc_skb(frame_len);
					if (new_skb == NULL) {
						ath6kl_dbg(ATH6KL_DBG_USB_BULK,
						 "athusb: allocate skb (len=%u) "
						 "failed\n", frame_len);
						break;
					}

					netdata_new = new_skb->data;
					netlen_new = new_skb->len;
					memcpy(netdata_new, netdata, frame_len);
					skb_put(new_skb, frame_len);
				}

				skb_queue_tail(&pipe->io_comp_queue, new_skb);
				new_skb = NULL;

				netdata += frame_len;
				netlen -= frame_len;
			} else {
				ath6kl_dbg(ATH6KL_DBG_USB_BULK,
					"athusb: subframe length %d not fitted "
					"into bundle packet length %d\n",
					netlen, frame_len);
				break;
			}
		} while (netlen);

		pipe_st->num_rx_bundle_comp++;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
		queue_work(ar->ath6kl_wq, &pipe->io_complete_work);
#else
		schedule_work(&pipe->io_complete_work);
#endif

	} while (0);

	if (urb_context->buf == NULL)
		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "athusb: buffer in urb_context is NULL\n");

	ath6kl_usb_free_urb_to_pipe(urb_context->pipe, urb_context);

	if (status == 0 || urb->status == -EPROTO) {
		if (pipe->urb_cnt >= pipe->urb_cnt_thresh)
			/* our free urbs are piling up, post more transfers */
			hif_usb_post_recv_bundle_transfers(pipe,
			0 /* pass zero for not allocating urb-buffer again */);
	}
	return;
}


static void ath6kl_usb_usb_transmit_complete(struct urb *urb)
{
	struct ath6kl_urb_context *urb_context =
	    (struct ath6kl_urb_context *)urb->context;
	struct sk_buff *buf;
	struct ath6kl_usb_pipe *pipe = urb_context->pipe;
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	struct ath6kl *ar = pipe->ar_usb->ar;
#endif

	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			"%s: pipe: %d, stat:%d, len:%d\n",
			__func__, pipe->logical_pipe_num, urb->status,
			urb->actual_length);

	if (urb->status != 0) {
		pipe_st->num_tx_comp_err++;
		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			"%s:  pipe: %d, failed:%d\n",
			__func__, pipe->logical_pipe_num, urb->status);
	}

	buf = urb_context->buf;
	urb_context->buf = NULL;
	ath6kl_usb_free_urb_to_pipe(urb_context->pipe, urb_context);

	/* note: queue implements a lock */
	skb_queue_tail(&pipe->io_comp_queue, buf);
	pipe_st->num_tx_comp++;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	queue_work(ar->ath6kl_wq, &pipe->io_complete_work);
#else
	schedule_work(&pipe->io_complete_work);
#endif
}

static void ath6kl_usb_io_comp_work(struct work_struct *work)
{
	struct ath6kl_usb_pipe *pipe =
	    container_of(work, struct ath6kl_usb_pipe, io_complete_work);
	struct sk_buff *buf;
	struct ath6kl_usb *device;
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
	u32 tx = 0, rx = 0;
#if defined(CE_OLD_KERNEL_SUPPORT_2_6_23) || defined(USB_AUTO_SUSPEND)
	struct ath6kl *ar = pipe->ar_usb->ar;
#endif

	pipe_st->num_io_comp++;
	device = pipe->ar_usb;

	if (test_and_set_bit(WORKER_LOCK_BIT, &pipe->worker_lock))
		return;

	while ((buf = skb_dequeue(&pipe->io_comp_queue))) {

#ifdef USB_AUTO_SUSPEND
		/* to check if need to postpone auto pm timeout */
		if (pipe->flags & ATH6KL_USB_PIPE_FLAG_RX) {
			if (ar->htc_ops->skip_usb_mark_busy != NULL) {
				if (ar->htc_ops->skip_usb_mark_busy(ar, buf))
					goto skip_mark_busy;
			}
		}
		usb_mark_last_busy(device->udev);

skip_mark_busy:
#endif
		if (pipe->flags & ATH6KL_USB_PIPE_FLAG_TX) {
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb xmit callback buf:0x%p\n", buf);
			device->htc_callbacks.
				tx_completion(device->ar->htc_target, buf);
#ifdef USB_AUTO_SUSPEND
			spin_lock_bh(&ar->usb_pm_lock);
			ath6kl_auto_pm_wakeup_resume(ar);
			spin_unlock_bh(&ar->usb_pm_lock);
#endif
			if (tx++ > device->max_sche_tx) {
				clear_bit(WORKER_LOCK_BIT, &pipe->worker_lock);
				pipe_st->num_tx_resche++;
				goto reschedule;
			}
		} else {
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb recv callback buf:0x%p\n", buf);

			if (!device->htc_callbacks.rx_completion) {
				dev_kfree_skb(buf);
				continue;
			}

			device->htc_callbacks.
				rx_completion(device->ar->htc_target, buf,
					      pipe->logical_pipe_num);

			if (rx++ > device->max_sche_rx) {
				clear_bit(WORKER_LOCK_BIT, &pipe->worker_lock);
				pipe_st->num_rx_resche++;
				goto reschedule;
			}
		}
	}

	if (pipe->flags & ATH6KL_USB_PIPE_FLAG_RX &&
		pipe->urb_cnt >= pipe->urb_cnt_thresh) {
		/* our free urbs are piling up, post more transfers */
		ath6kl_usb_post_recv_transfers(pipe, ATH6KL_USB_RX_BUFFER_SIZE);
	}

	clear_bit(WORKER_LOCK_BIT, &pipe->worker_lock);

	if (tx > pipe_st->num_max_tx)
		pipe_st->num_max_tx = tx;

	if (rx > pipe_st->num_max_rx)
		pipe_st->num_max_rx = rx;

	return;

reschedule:
	/* Re-schedule it to avoid one direction to starve another direction. */
	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb re-schedule work.\n");
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	queue_work(ar->ath6kl_wq, &pipe->io_complete_work);
#else
	schedule_work(&pipe->io_complete_work);
#endif

	return;
}

#define ATH6KL_USB_MAX_DIAG_CMD (sizeof(struct ath6kl_usb_ctrl_diag_cmd_write))
#define ATH6KL_USB_MAX_DIAG_RESP (sizeof(struct ath6kl_usb_ctrl_diag_resp_read))

static void ath6kl_usb_destroy(struct ath6kl_usb *ar_usb)
{
	unregister_reboot_notifier(&ar_usb->reboot_notifier);
#ifndef CE_OLD_KERNEL_SUPPORT_2_6_23
	ath6kl_usb_flush_all(ar_usb);
#endif
	ath6kl_usb_cleanup_pipe_resources(ar_usb);

	usb_set_intfdata(ar_usb->interface, NULL);

	kfree(ar_usb->diag_cmd_buffer);
	kfree(ar_usb->diag_resp_buffer);

	kfree(ar_usb);
}

static int ath6kl_usb_reboot(struct notifier_block *nb, unsigned long val,
			       void *v)
{
	struct ath6kl_usb *ar_usb;
	struct ath6kl *ar ;

	ar_usb = container_of(nb, struct ath6kl_usb, reboot_notifier);
	if (ar_usb == NULL)
		return NOTIFY_DONE;

	ar = (struct ath6kl *) ar_usb->ar;
#ifdef CE_SUPPORT
	if ((ar != NULL) && (ar->state == ATH6KL_STATE_ON))
#else
	if (ar != NULL)
#endif
		if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode) == 0)
			ath6kl_reset_device(ar, ar->target_type, true, true);


	return NOTIFY_DONE;
}

static struct ath6kl_usb *ath6kl_usb_create(struct usb_interface *interface)
{
	struct ath6kl_usb *ar_usb = NULL;
	struct usb_device *dev = interface_to_usbdev(interface);
	struct ath6kl_usb_pipe *pipe;
	int status = 0;
	int i;

	ar_usb = kzalloc(sizeof(struct ath6kl_usb), GFP_KERNEL);
	if (ar_usb == NULL)
		goto fail_ath6kl_usb_create;

	memset(ar_usb, 0, sizeof(struct ath6kl_usb));
	usb_set_intfdata(interface, ar_usb);
	spin_lock_init(&(ar_usb->cs_lock));
	spin_lock_init(&(ar_usb->rx_lock));
	spin_lock_init(&(ar_usb->tx_lock));
	ar_usb->udev = dev;
	ar_usb->interface = interface;

#ifdef CONFIG_ANDROID
	ath6kl_usb_unload_dev_num = dev->devnum;
#endif

	for (i = 0; i < ATH6KL_USB_PIPE_MAX; i++) {
		pipe = &ar_usb->pipes[i];
		INIT_WORK(&pipe->io_complete_work,
			  ath6kl_usb_io_comp_work);
		skb_queue_head_init(&pipe->io_comp_queue);
		pipe->worker_lock = 0;
	}

	ar_usb->diag_cmd_buffer = kzalloc(ATH6KL_USB_MAX_DIAG_CMD, GFP_KERNEL);
	if (ar_usb->diag_cmd_buffer == NULL) {
		status = -ENOMEM;
		goto fail_ath6kl_usb_create;
	}

	ar_usb->diag_resp_buffer = kzalloc(ATH6KL_USB_MAX_DIAG_RESP,
					   GFP_KERNEL);
	if (ar_usb->diag_resp_buffer == NULL) {
		status = -ENOMEM;
		goto fail_ath6kl_usb_create;
	}

	ar_usb->max_sche_tx =
	ar_usb->max_sche_rx = HIF_USB_MAX_SCHE_PKT;
	ar_usb->rxq_threshold = HIF_USB_RX_QUEUE_THRESHOLD;

	status = ath6kl_usb_setup_pipe_resources(ar_usb);

	ar_usb->reboot_notifier.notifier_call = ath6kl_usb_reboot;
	register_reboot_notifier(&ar_usb->reboot_notifier);

fail_ath6kl_usb_create:
	if (status != 0) {
		ath6kl_usb_destroy(ar_usb);
		ar_usb = NULL;
	}
	return ar_usb;
}

static void ath6kl_usb_device_detached(struct usb_interface *interface)
{
	struct ath6kl_usb *ar_usb;
#ifdef USB_AUTO_SUSPEND
	struct usb_pm_skb_queue_t *entry, *p_usb_pm_skb_queue;
	struct ath6kl *ar;
#endif

	ar_usb = usb_get_intfdata(interface);
	if (ar_usb == NULL)
		return;

	ath6kl_stop_txrx(ar_usb->ar);

	/* Delay to wait for the target to reboot */
#ifdef CONFIG_ANDROID
	if (atomic_read(&ath6kl_usb_unload_state) ==
	    ATH6KL_USB_UNLOAD_STATE_DRV_DEREG)
		atomic_set(&ath6kl_usb_unload_state,
			   ATH6KL_USB_UNLOAD_STATE_TARGET_RESET);
#else
	mdelay(20);
#endif

#ifdef USB_AUTO_SUSPEND
	/*
	 * when packets are in pm_skb_queue,
	 * and usb remove before resume happens,
	 * we need to clean pm_skb_queue to avoid memory leak.
	 */

	ar = ar_usb->ar;
	p_usb_pm_skb_queue =  &ar->usb_pm_skb_queue;

	while (get_queue_depth(&(p_usb_pm_skb_queue->list)) > 0) {
		ath6kl_dbg(ATH6KL_DBG_USB, "%s  resume_wk qeue %d\n", __func__,
		get_queue_depth(&(p_usb_pm_skb_queue->list)));
		entry = list_first_entry(&p_usb_pm_skb_queue->list,
					struct usb_pm_skb_queue_t, list);
		list_del(&entry->list);
		kfree(entry);
	}
#ifdef ATH6KL_BUS_VOTE
	if (machine_is_apq8064_dma() || machine_is_apq8064_bueller() ||
		ath6kl_platform_has_vreg == 0)
#endif
		usb_auto_pm_turnoff(ar);
#endif

#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	ath6kl_usb_flush_all(ar_usb);
#endif
	ath6kl_core_cleanup(ar_usb->ar);
	ath6kl_usb_destroy(ar_usb);
}

/* exported hif usb APIs for htc pipe */
static void hif_start(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	int i;

	ath6kl_usb_start_recv_pipes(device);

	/* set the TX resource avail threshold for each TX pipe */
	for (i = ATH6KL_USB_PIPE_TX_CTRL;
	     i <= ATH6KL_USB_PIPE_TX_DATA_VHP; i++) {
		device->pipes[i].urb_cnt_thresh =
		    device->pipes[i].urb_alloc / 2;
	}
}

static void ath6kl_usb_transmit_bundle_complete(struct urb *urb)
{
	struct ath6kl_urb_context *urb_context =
		(struct ath6kl_urb_context *)urb->context;
	struct ath6kl_usb_pipe *pipe = urb_context->pipe;
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
	struct sk_buff *tmp_buf;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	struct ath6kl *ar = pipe->ar_usb->ar;
#endif

	ath6kl_dbg(ATH6KL_DBG_USB_BULK, "+%s: pipe: %d, stat:%d, len:%d " "\n",
		__func__, pipe->logical_pipe_num, urb->status,
		urb->actual_length);

	if (urb->status != 0) {
		pipe_st->num_tx_bundle_comp_err++;
		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "%s:  pipe: %d, failed:%d\n",
			   __func__, pipe->logical_pipe_num, urb->status);
	}

	ath6kl_usb_free_urb_to_pipe(urb_context->pipe, urb_context);

	while ((tmp_buf = skb_dequeue(&urb_context->comp_queue)))
		skb_queue_tail(&pipe->io_comp_queue, tmp_buf);
	pipe_st->num_tx_bundle_comp++;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	queue_work(ar->ath6kl_wq, &pipe->io_complete_work);
#else
	schedule_work(&pipe->io_complete_work);
#endif
}

static int ath6kl_usb_send_bundle(struct ath6kl *ar, u8 pid,
	struct sk_buff **msg_bundle, int num_msgs)
{
	int status = 0;
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	struct ath6kl_usb_pipe *pipe = &device->pipes[pid];
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
	struct ath6kl_urb_context *urb_context;
	struct urb *urb;
	struct sk_buff *stream_buf = NULL, *buf = NULL;
	int usb_status;
	int i;

	pipe_st->num_tx_multi++;
	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			"+%s pipe : %d\n",
			__func__, pid);

	do {
		u8 *stream_netdata, *netdata, *stream_netdata_start;
		u32 stream_netlen, netlen;

		urb_context = ath6kl_usb_alloc_urb_from_pipe(pipe);

		if (urb_context == NULL) {
			pipe_st->num_tx_multi_err_others++;
			/*
			* TODO: it is possible to run out of urbs if
			* 2 endpoints map to the same pipe ID
			*/
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				"%s pipe:%d no urbs left. URB Cnt : %d\n",
				__func__, pid, pipe->urb_cnt);
			status = -ENOMEM;
			break;
		}

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (urb == NULL) {
			pipe_st->num_tx_multi_err_others++;
			status = -ENOMEM;
			ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
						    urb_context);
			break;
		}

		stream_buf = urb_context->buf;

		stream_netdata = stream_buf->data;
		stream_netlen = stream_buf->len;

		stream_netlen = 0;
		stream_netdata_start = stream_netdata;

		for (i = 0; i < num_msgs; i++) {
			buf = msg_bundle[i];
			netdata = buf->data;
			netlen = buf->len;

			memcpy(stream_netdata, netdata, netlen);

			/* add additional dummy padding */
			stream_netdata += 1664; /* target credit size */
			stream_netlen += 1664;

			/* note: queue implements a lock */
			skb_queue_tail(&urb_context->comp_queue, buf);
		}

		usb_fill_bulk_urb(urb,
				device->udev,
				pipe->usb_pipe_handle,
				stream_netdata_start,
				stream_netlen,
				ath6kl_usb_transmit_bundle_complete,
				urb_context);

		if ((stream_netlen % pipe->max_packet_size) == 0)
			/* hit a max packet boundary on this pipe */
			urb->transfer_flags |= URB_ZERO_PACKET;

		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			"athusb bulk send submit:%d, "
			"0x%X (ep:0x%2.2X), %d bytes\n",
			pipe->logical_pipe_num, pipe->usb_pipe_handle,
			pipe->ep_address, stream_netlen);

		usb_anchor_urb(urb, &pipe->urb_submitted);

		spin_lock_bh(&ar->state_lock);
		if ((ar->state == ATH6KL_STATE_DEEPSLEEP) ||
		    (ar->state == ATH6KL_STATE_WOW))
			usb_status = -EINVAL;
		else
			usb_status = usb_submit_urb(urb, GFP_ATOMIC);
		spin_unlock_bh(&ar->state_lock);

		if (usb_status) {
			pipe_st->num_tx_multi_err++;
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				"ath6kl usb : usb bulk transmit failed %d "
				"\n", usb_status);
			usb_unanchor_urb(urb);
			usb_free_urb(urb);
			ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
						    urb_context);
			status = -EINVAL;
			break;
		}
		usb_free_urb(urb);
	} while (0);

	return status;
}

#ifdef USB_AUTO_SUSPEND
int usb_debugfs_get_pm_usage_cnt(struct ath6kl *ar)
{
	struct ath6kl_usb *device = (struct ath6kl_usb *)ar->hif_priv;
	struct usb_interface *interface = device->interface;
	return atomic_read(&interface->pm_usage_cnt);
}

void usb_auto_pm_disable(struct ath6kl *ar)
{
	struct ath6kl_usb *device = (struct ath6kl_usb *)ar->hif_priv;
	struct usb_interface *interface = device->interface;
	usb_autopm_get_interface_async(interface);
	ar->auto_pm_cnt++;
	ath6kl_dbg(ATH6KL_DBG_USB, "autopm +1 refcnt=%d my=%d\n",
		usb_debugfs_get_pm_usage_cnt(ar), ar->auto_pm_cnt);
	/*usb_debugfs_get_pm_usage_cnt(ar);*/

}


void usb_auto_pm_enable(struct ath6kl *ar)
{
	struct ath6kl_usb *device = (struct ath6kl_usb *)ar->hif_priv;
	struct usb_interface *interface = device->interface;
	usb_autopm_put_interface_async(interface);
	ar->auto_pm_cnt--;
	ath6kl_dbg(ATH6KL_DBG_USB, "autopm -1 refcnt=%d my=%d\n",
		usb_debugfs_get_pm_usage_cnt(ar), ar->auto_pm_cnt);
	/*usb_debugfs_get_pm_usage_cnt(ar);*/
}

void usb_auto_pm_turnoff(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	if (!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_USB_AUTO_PM))
		usb_disable_autosuspend(device->udev);
}

void usb_auto_pm_turnon(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	if (!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_USB_AUTO_PM))
		usb_enable_autosuspend(device->udev);
}


void ath6kl_auto_pm_wakeup_resume(struct ath6kl *wk)
{
	struct sk_buff *buf;
	int status = 0;
	struct ath6kl_usb *device;
	struct ath6kl_usb_pipe *pipe;
	struct ath6kl_usb_pipe_stat *pipe_st;
	struct ath6kl_urb_context *urb_context;
	u8 *data;
	u32 len;
	struct urb *urb;
	int usb_status;
	struct usb_pm_skb_queue_t *entry, *p_usb_pm_skb_queue;
	struct ath6kl *pm_ar;
	int pm_PipeID;

	pm_ar = wk;
	p_usb_pm_skb_queue =  &pm_ar->usb_pm_skb_queue;


	ath6kl_dbg(ATH6KL_DBG_USB, "%s  resume_wk qeue %d  empty=%d\n",
		__func__,
		get_queue_depth(&(p_usb_pm_skb_queue->list)),
		list_empty(&p_usb_pm_skb_queue->list));

	while (get_queue_depth(&(p_usb_pm_skb_queue->list)) > 0) {
		ath6kl_dbg(ATH6KL_DBG_USB, "%s  resume_wk qeue %d\n", __func__,
		get_queue_depth(&(p_usb_pm_skb_queue->list)));
		entry = list_first_entry(&p_usb_pm_skb_queue->list,
					struct usb_pm_skb_queue_t, list);

		pm_PipeID = entry->pipeID;
		pm_ar = entry->ar;
		buf = entry->skb;

		device = ath6kl_usb_priv(pm_ar);
		pipe = &device->pipes[pm_PipeID];
		pipe_st = &pipe->usb_pipe_stat;

		usb_mark_last_busy(device->udev);

		urb_context = ath6kl_usb_alloc_urb_from_pipe(pipe);

		if (urb_context == NULL) {
			pipe_st->num_tx_err_others++;
			/*
			 * TODO: it is possible to run out of urbs if
			 * 2 endpoints map to the same pipe ID
			 */
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "%s pipe:%d no urbs left. URB Cnt : %d\n",
				   __func__, pm_PipeID, pipe->urb_cnt);
			status = -ENOMEM;
			goto fail_hif_send_usb;
		}
		urb_context->buf = buf;

		data = buf->data;
		len = buf->len;
		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (urb == NULL) {
			pipe_st->num_tx_err_others++;
			status = -ENOMEM;
			ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
				urb_context);
			goto fail_hif_send_usb;
		}

		usb_fill_bulk_urb(urb,
				  device->udev,
				  pipe->usb_pipe_handle,
				  data,
				  len,
				  ath6kl_usb_usb_transmit_complete,
				  urb_context);

		if ((len % pipe->max_packet_size) == 0) {
			/* hit a max packet boundary on this pipe */
			urb->transfer_flags |= URB_ZERO_PACKET;
		}

		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "athusb bulk send submit:%d, 0x%X (ep:0x%2.2X), %d bytes\n",
			   pipe->logical_pipe_num, pipe->usb_pipe_handle,
			   pipe->ep_address, len);

		usb_anchor_urb(urb, &pipe->urb_submitted);
		list_del(&entry->list);

		/* spin_lock_bh(&pm_ar->state_lock); */
		usb_status = usb_submit_urb(urb, GFP_ATOMIC);
		/* spin_unlock_bh(&pm_ar->state_lock); */

		if (usb_status) {
			pipe_st->num_tx_err++;
			ath6kl_dbg(ATH6KL_DBG_USB_BULK,
				   "ath6kl usb : usb bulk transmit failed %d\n",
				   usb_status);
			usb_unanchor_urb(urb);
			ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
						 urb_context);
			status = -EINVAL;
		}
		usb_free_urb(urb);
		pipe_st->num_tx++;

		kfree(entry);
	}	/* end of while (Dequeu ...) */

fail_hif_send_usb:
	ath6kl_dbg(ATH6KL_DBG_USB, "wakeup_resume done\n");
}



#endif /* USB_AUTO_SUSPEND */

static int ath6kl_usb_send(struct ath6kl *ar, u8 PipeID,
	struct sk_buff *hdr_buf, struct sk_buff *buf)
{
	int status = 0;
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	struct ath6kl_usb_pipe *pipe = &device->pipes[PipeID];
	struct ath6kl_usb_pipe_stat *pipe_st = &pipe->usb_pipe_stat;
	struct ath6kl_urb_context *urb_context;
	u8 *data;
	u32 len;
	struct urb *urb;
	int usb_status;
#ifdef USB_AUTO_SUSPEND
	struct usb_pm_skb_queue_t *p_pmskb;
	int qlen, usb_pm_increament;
	struct usb_pm_skb_queue_t *p_usb_pm_skb_queue =  &ar->usb_pm_skb_queue;
#endif
#ifdef USB_AUTO_SUSPEND

#elif defined(CONFIG_ANDROID)
	struct usb_interface *interface = device->interface;
#endif

	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			"+%s pipe : %d, buf:0x%p, ar state:%d\n",
			__func__, PipeID, buf, ar->state);

#ifdef USB_AUTO_SUSPEND
	if (ar->state == ATH6KL_STATE_PRE_SUSPEND_DEEPSLEEP) {
		ath6kl_dbg(ATH6KL_DBG_USB, "%s: deep sleep state=%d\n",
		__func__, ar->state);
		status = -ETXTBSY;
		pipe_st->num_tx++;
		return status;
	}

	usb_pm_increament = 0;
	if (ar->state != ATH6KL_STATE_PRE_SUSPEND) {
		usb_pm_increament++;
		usb_auto_pm_disable(ar);
	}

	spin_lock_bh(&ar->usb_pm_lock);
	if (!list_empty(&p_usb_pm_skb_queue->list) ||
		(ar->state == ATH6KL_STATE_WOW) ||
		(ar->state == ATH6KL_STATE_DEEPSLEEP)) {

		ath6kl_dbg(ATH6KL_DBG_USB, "usb_send sleep Q  %d  queue len =%d\n",
			ar->state,
			get_queue_depth(&(p_usb_pm_skb_queue->list)));
		p_pmskb = kmalloc(sizeof(struct usb_pm_skb_queue_t),
			GFP_ATOMIC);
		if (p_pmskb == NULL)
			ath6kl_dbg(ATH6KL_DBG_USB, "p_pmskb = kmalloc fila\n");

		p_pmskb->pipeID = PipeID;
		p_pmskb->ar = ar;
		p_pmskb->skb = buf;

		list_add_tail(&(p_pmskb->list), &(p_usb_pm_skb_queue->list));
		qlen = get_queue_depth(&(p_usb_pm_skb_queue->list));
		ath6kl_dbg(ATH6KL_DBG_USB, "qlen = %d\n", qlen);

		/*
		msleep_interruptible(3000);
		ath6kl_auto_pm_wakeup_resume(&auto_pm_wakeup_resume_wk);
		*/
		spin_unlock_bh(&ar->usb_pm_lock);

		if (usb_pm_increament != 0)
			usb_auto_pm_enable(ar);
		return 0;
	}

	spin_unlock_bh(&ar->usb_pm_lock);
	ath6kl_dbg(ATH6KL_DBG_USB, "usb_send 2\n");

#elif defined(CONFIG_ANDROID)
	if (PipeID != ATH6KL_USB_PIPE_TX_CTRL)
		usb_autopm_get_interface_async(interface);
#endif
	urb_context = ath6kl_usb_alloc_urb_from_pipe(pipe);

	if (urb_context == NULL) {
		pipe_st->num_tx_err_others++;
		/*
		 * TODO: it is possible to run out of urbs if
		 * 2 endpoints map to the same pipe ID
		 */
		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "%s pipe:%d no urbs left. URB Cnt : %d\n",
			   __func__, PipeID, pipe->urb_cnt);
		status = -ENOMEM;
		goto fail_hif_send;
	}
	urb_context->buf = buf;

	data = buf->data;
	len = buf->len;
	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (urb == NULL) {
		pipe_st->num_tx_err_others++;
		status = -ENOMEM;
		ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
			urb_context);
		goto fail_hif_send;
	}

	usb_fill_bulk_urb(urb,
			  device->udev,
			  pipe->usb_pipe_handle,
			  data,
			  len,
			  ath6kl_usb_usb_transmit_complete, urb_context);

	if ((len % pipe->max_packet_size) == 0) {
		/* hit a max packet boundary on this pipe */
		urb->transfer_flags |= URB_ZERO_PACKET;
	}

	ath6kl_dbg(ATH6KL_DBG_USB_BULK,
		   "athusb bulk send submit:%d, 0x%X (ep:0x%2.2X), %d bytes\n",
		   pipe->logical_pipe_num, pipe->usb_pipe_handle,
		   pipe->ep_address, len);

	usb_anchor_urb(urb, &pipe->urb_submitted);

	spin_lock_bh(&ar->state_lock);
#ifndef USB_AUTO_SUSPEND /* QQQQ: double check here	*/
	if ((ar->state == ATH6KL_STATE_DEEPSLEEP) ||
	    (ar->state == ATH6KL_STATE_WOW))
		usb_status = -EINVAL;
	else
#endif
		usb_status = usb_submit_urb(urb, GFP_ATOMIC);
	spin_unlock_bh(&ar->state_lock);

	if (usb_status) {
		pipe_st->num_tx_err++;
		ath6kl_dbg(ATH6KL_DBG_USB_BULK,
			   "ath6kl usb : usb bulk transmit failed %d\n",
			   usb_status);
		usb_unanchor_urb(urb);
		ath6kl_usb_free_urb_to_pipe(urb_context->pipe,
					 urb_context);
		status = -EINVAL;
	}
	usb_free_urb(urb);
	pipe_st->num_tx++;

fail_hif_send:

#ifdef USB_AUTO_SUSPEND
	if (usb_pm_increament != 0)
		usb_auto_pm_enable(ar);

#elif defined(CONFIG_ANDROID)
	if (PipeID != ATH6KL_USB_PIPE_TX_CTRL)
		usb_autopm_put_interface_async(interface);
#endif
	return status;
}

static void hif_stop(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	ath6kl_usb_flush_all(device);
}

static void ath6kl_usb_get_default_pipe(struct ath6kl *ar,
	u8 *ul_pipe, u8 *dl_pipe)
{
	*ul_pipe = ATH6KL_USB_PIPE_TX_CTRL;
	*dl_pipe = ATH6KL_USB_PIPE_RX_CTRL;
}

static int ath6kl_usb_map_service_pipe(struct ath6kl *ar, u16 svcId, u8 *ULPipe,
			 u8 *DLPipe)
{
	int status = 0;

	switch (svcId) {
	case HTC_CTRL_RSVD_SVC:
	case WMI_CONTROL_SVC:
		*ULPipe = ATH6KL_USB_PIPE_TX_CTRL;
		*DLPipe = ATH6KL_USB_PIPE_RX_DATA;
		break;
	case WMI_DATA_BE_SVC:
	case WMI_DATA_BK_SVC:
		*ULPipe = ATH6KL_USB_PIPE_TX_DATA_LP;
		*DLPipe = ATH6KL_USB_PIPE_RX_DATA;
		break;
	case WMI_DATA_VI_SVC:
		*ULPipe = ATH6KL_USB_PIPE_TX_DATA_LP;
		*DLPipe = ATH6KL_USB_PIPE_RX_DATA;
		break;
	case WMI_DATA_VO_SVC:
		*ULPipe = ATH6KL_USB_PIPE_TX_DATA_LP;
		*DLPipe = ATH6KL_USB_PIPE_RX_DATA;
		break;
	default:
		status = -EPERM;
		break;
	}

	return status;
}

static void ath6kl_usb_register_callback(struct ath6kl *ar,
		void *unused,
		struct ath6kl_hif_pipe_callbacks *callbacks)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	memcpy(&device->htc_callbacks, callbacks,
	       sizeof(struct ath6kl_hif_pipe_callbacks));
}

static u16 ath6kl_usb_get_free_queue_number(struct ath6kl *ar, u8 PipeID)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	return device->pipes[PipeID].urb_cnt;
}

static u16 ath6kl_usb_get_max_queue_number(struct ath6kl *ar, u8 PipeID)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	return device->pipes[PipeID].urb_alloc;
}

static void hif_detach_htc(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	ath6kl_usb_flush_all(device);

	memset(&device->htc_callbacks, 0,
		sizeof(struct ath6kl_hif_pipe_callbacks));
}

static int ath6kl_usb_submit_ctrl_out(struct ath6kl_usb *ar_usb,
				   u8 req, u16 value, u16 index, void *data,
				   u32 size)
{
	u8 *buf = NULL;
	int ret;

	if (size > 0) {
		buf = kmalloc(size, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;

		memcpy(buf, data, size);
	}

	/* note: if successful returns number of bytes transfered */
	ret = usb_control_msg(ar_usb->udev,
			      usb_sndctrlpipe(ar_usb->udev, 0),
			      req,
			      USB_DIR_OUT | USB_TYPE_VENDOR |
			      USB_RECIP_DEVICE, value, index, buf,
			      size, 1000);

	if (ret < 0) {
		ath6kl_dbg(ATH6KL_DBG_USB, "%s failed,result = %d\n",
			   __func__, ret);
	}

	kfree(buf);

	return 0;
}

static int ath6kl_usb_submit_ctrl_in(struct ath6kl_usb *ar_usb,
				  u8 req, u16 value, u16 index, void *data,
				  u32 size)
{
	u8 *buf = NULL;
	int ret;

	if (size > 0) {
		buf = kmalloc(size, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;
	}

	/* note: if successful returns number of bytes transfered */
	ret = usb_control_msg(ar_usb->udev,
				 usb_rcvctrlpipe(ar_usb->udev, 0),
				 req,
				 USB_DIR_IN | USB_TYPE_VENDOR |
				 USB_RECIP_DEVICE, value, index, buf,
				 size, 2 * HZ);

	if (ret < 0) {
		ath6kl_dbg(ATH6KL_DBG_USB, "%s failed,result = %d\n",
			   __func__, ret);
	}

	memcpy((u8 *) data, buf, size);

	kfree(buf);

	return 0;
}

static int ath6kl_usb_ctrl_msg_exchange(struct ath6kl_usb *ar_usb,
				     u8 req_val, u8 *req_buf, u32 req_len,
				     u8 resp_val, u8 *resp_buf, u32 *resp_len)
{
	int ret;

	/* send command */
	ret = ath6kl_usb_submit_ctrl_out(ar_usb, req_val, 0, 0,
					 req_buf, req_len);

	if (ret != 0)
		return ret;

	if (resp_buf == NULL) {
		/* no expected response */
		return ret;
	}

	/* get response */
	ret = ath6kl_usb_submit_ctrl_in(ar_usb, resp_val, 0, 0,
					resp_buf, *resp_len);

	return ret;
}

static int ath6kl_usb_diag_read32(struct ath6kl *ar, u32 address, u32 *data)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	struct ath6kl_usb_ctrl_diag_resp_read *resp;
	struct ath6kl_usb_ctrl_diag_cmd_read *cmd;
	u32 resp_len;
	int ret;

	cmd = (struct ath6kl_usb_ctrl_diag_cmd_read *) ar_usb->diag_cmd_buffer;

	memset(cmd, 0, sizeof(*cmd));
	cmd->cmd = ATH6KL_USB_CTRL_DIAG_CC_READ;
	cmd->address = cpu_to_le32(address);
	resp_len = sizeof(*resp);

	ret = ath6kl_usb_ctrl_msg_exchange(ar_usb,
				ATH6KL_USB_CONTROL_REQ_DIAG_CMD,
				(u8 *) cmd,
				sizeof(struct ath6kl_usb_ctrl_diag_cmd_write),
				ATH6KL_USB_CONTROL_REQ_DIAG_RESP,
				ar_usb->diag_resp_buffer, &resp_len);

	if (ret)
		return ret;

	resp = (struct ath6kl_usb_ctrl_diag_resp_read *)
		ar_usb->diag_resp_buffer;

	*data = le32_to_cpu(resp->value);

	return ret;
}

static int ath6kl_usb_diag_write32(struct ath6kl *ar, u32 address, __le32 data)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	struct ath6kl_usb_ctrl_diag_cmd_write *cmd;

	cmd = (struct ath6kl_usb_ctrl_diag_cmd_write *) ar_usb->diag_cmd_buffer;

	memset(cmd, 0, sizeof(struct ath6kl_usb_ctrl_diag_cmd_write));
	cmd->cmd = cpu_to_le32(ATH6KL_USB_CTRL_DIAG_CC_WRITE);
	cmd->address = cpu_to_le32(address);
	cmd->value = data;

	return ath6kl_usb_ctrl_msg_exchange(ar_usb,
					    ATH6KL_USB_CONTROL_REQ_DIAG_CMD,
					    (u8 *) cmd,
					    sizeof(*cmd),
					    0, NULL, NULL);

}

static int ath6kl_usb_bmi_read(struct ath6kl *ar, u8 *buf, u32 len)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	int ret;

	/* get response */
	ret = ath6kl_usb_submit_ctrl_in(ar_usb,
					ATH6KL_USB_CONTROL_REQ_RECV_BMI_RESP,
					0, 0, buf, len);
	if (ret != 0) {
		ath6kl_err("Unable to read the bmi data from the device: %d\n",
			   ret);
		return ret;
	}

	return 0;
}

static int ath6kl_usb_bmi_write(struct ath6kl *ar, u8 *buf, u32 len)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	int ret;

	/* send command */
	ret = ath6kl_usb_submit_ctrl_out(ar_usb,
					 ATH6KL_USB_CONTROL_REQ_SEND_BMI_CMD,
					 0, 0, buf, len);
	if (ret != 0) {
		ath6kl_err("unable to send the bmi data to the device: %d\n",
			   ret);
		return ret;
	}

	return 0;
}

static int ath6kl_usb_power_on(struct ath6kl *ar)
{
	if (test_bit(USB_REMOTE_WKUP, &ar->flag)) {
		struct ath6kl_usb *ar_usb = (struct ath6kl_usb *)ar->hif_priv;
		usb_reset_device(ar_usb->udev);
	}

	hif_start(ar);
	return 0;
}

static int ath6kl_usb_power_off(struct ath6kl *ar)
{
	hif_detach_htc(ar);
	return 0;
}

static void ath6kl_usb_stop(struct ath6kl *ar)
{
	hif_stop(ar);
}

static int ath6kl_usb_pipe_stat(struct ath6kl *ar, u8 *buf, int buf_len)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	struct ath6kl_usb_pipe_stat *pipe_st;
	int i, len = 0;

	if ((!device) || (!buf))
		return 0;

	for (i = 0; i < ATH6KL_USB_PIPE_MAX; i++) {
		if ((i == ATH6KL_USB_PIPE_RX_INT) ||
		    (i == ATH6KL_USB_PIPE_RX_DATA2))
			continue;

		pipe_st = &device->pipes[i].usb_pipe_stat;

		len += snprintf(buf + len, buf_len - len, "\nPIPE-%d\n", i);
		len += snprintf(buf + len, buf_len - len,
				" num_rx_comp        : %d\n",
				pipe_st->num_rx_comp);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_comp        : %d\n",
				pipe_st->num_tx_comp);
		len += snprintf(buf + len, buf_len - len,
				" num_io_comp        : %d\n",
				pipe_st->num_io_comp);
		len += snprintf(buf + len, buf_len - len,
				" num_max_tx         : %d\n",
				pipe_st->num_max_tx);
		len += snprintf(buf + len, buf_len - len,
				" num_max_rx         : %d\n",
				pipe_st->num_max_rx);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_resche      : %d\n",
				pipe_st->num_tx_resche);
		len += snprintf(buf + len, buf_len - len,
				" num_rx_resche      : %d\n",
				pipe_st->num_rx_resche);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_sync        : %d\n",
				pipe_st->num_tx_sync);
		len += snprintf(buf + len, buf_len - len,
				" num_tx             : %d\n",
				pipe_st->num_tx);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_err         : %d\n",
				pipe_st->num_tx_err);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_err_others  : %d\n",
				pipe_st->num_tx_err_others);
		len += snprintf(buf + len, buf_len - len,
				" num_tx_comp_err    : %d\n",
				pipe_st->num_tx_comp_err);
		len += snprintf(buf + len, buf_len - len,
				" num_rx_comp_err    : %d\n",
				pipe_st->num_rx_comp_err);

		/* Bundle mode */
		if (htc_bundle_recv || htc_bundle_send) {
			len += snprintf(buf + len, buf_len - len,
					" num_rx_bundle_comp      : %d\n",
					pipe_st->num_rx_bundle_comp);
			len += snprintf(buf + len, buf_len - len,
					" num_tx_bundle_comp      : %d\n",
					pipe_st->num_tx_bundle_comp);
			len += snprintf(buf + len, buf_len - len,
					" num_tx_multi            : %d\n",
					pipe_st->num_tx_multi);
			len += snprintf(buf + len, buf_len - len,
					" num_tx_multi_err        : %d\n",
					pipe_st->num_tx_multi_err);
			len += snprintf(buf + len, buf_len - len,
					" num_tx_multi_err_others : %d\n",
					pipe_st->num_tx_multi_err_others);
			len += snprintf(buf + len, buf_len - len,
					" num_rx_bundle_comp_err  : %d\n",
					pipe_st->num_rx_bundle_comp_err);
			len += snprintf(buf + len, buf_len - len,
					" num_tx_bundle_comp_err  : %d\n",
					pipe_st->num_tx_bundle_comp_err);
		}
	}

	return len;
}

static int ath6kl_usb_set_max_sche(struct ath6kl *ar,
				   u32 max_sche_tx, u32 max_sche_rx)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	device->max_sche_tx = max_sche_tx;
	device->max_sche_rx = max_sche_rx;

	ath6kl_dbg(ATH6KL_DBG_USB, "max_sche_tx = %d, max_sche_rx = %d\n",
				device->max_sche_tx, device->max_sche_rx);

	return 0;
}

int ath6kl_usb_suspend(struct ath6kl *ar, struct cfg80211_wowlan *wow)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	struct usb_interface *interface = device->interface;
	pm_message_t message;
	int ret;

	ath6kl_dbg(ATH6KL_DBG_EXT_INFO1, "usb suspend\n");

#ifdef CONFIG_ANDROID
	if (ath6kl_android_need_wow_suspend(ar)) {
#else
	if (wow) {
#endif
		ret = ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_WOW, wow);
		if (ret)
			return ret;
	} else {
		memset(&message, 0, sizeof(message));
		ret = ath6kl_usb_pm_suspend(interface, message);
	}

	return ret;
}

int ath6kl_usb_resume(struct ath6kl *ar)
{
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);
	struct usb_interface *interface = device->interface;

	ath6kl_dbg(ATH6KL_DBG_EXT_INFO1, "usb resume: state %d\n", ar->state);

	return ath6kl_usb_pm_resume(interface);
}

static void ath6kl_usb_cleanup_scatter(struct ath6kl *ar)
{
	ath6kl_dbg(ATH6KL_DBG_USB, "Init target fail?\n");
	return;
}

static int ath6kl_usb_set_rxq_threshold(struct ath6kl *ar, u32 rxq_threshold)
{
	struct ath6kl_usb *ar_usb = ath6kl_usb_priv(ar);

	ar_usb->rxq_threshold = rxq_threshold;

	ath6kl_dbg(ATH6KL_DBG_USB,
		"rxq_threshold = %d\n", ar_usb->rxq_threshold);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void ath6kl_usb_early_suspend(struct ath6kl *ar)
{
#ifndef USB_AUTO_SUSPEND
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	if (!ath6kl_mod_debug_quirks(ar,
			ATH6KL_MODULE_DISABLE_USB_AUTO_SUSPEND)) {
		if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode)) {
			struct usb_device *udev = device->udev;
			pm_runtime_set_autosuspend_delay(&udev->dev, 2000);
		}
		usb_enable_autosuspend(device->udev);
	}
#endif
}

static void ath6kl_usb_late_resume(struct ath6kl *ar)
{
#ifndef USB_AUTO_SUSPEND
	struct ath6kl_usb *device = ath6kl_usb_priv(ar);

	if (!ath6kl_mod_debug_quirks(ar,
			ATH6KL_MODULE_DISABLE_USB_AUTO_SUSPEND))
		usb_disable_autosuspend(device->udev);
#endif /* define USB_AUTO_SUSPEND */
}

#endif

/* FIXME: revisit a proper place to issue the bus reset*/
int ath6kl_usb_reconfig(struct ath6kl *ar)
{
	int ret = 0;
	u32 data, addr;

	/* To reenumerate the usb device if host want to enable the
	 * usb remote wakeup feature,
	 * ar6004 hw1.1, hw1.2 and hw1.3 would support this,
	 * hw1.6 would enable by default
	 */
	if (!ath6kl_mod_debug_quirks(ar,
			ATH6KL_MODULE_ENABLE_USB_REMOTE_WKUP)) {
		ret = 0;
		ath6kl_dbg(ATH6KL_DBG_BOOT, "Disable USB remote wakeup.\n");
	} else {
		if (ar->version.target_ver == AR6004_HW_1_3_VERSION)
			addr = 0x409754;
		else if (ar->version.target_ver == AR6004_HW_1_2_VERSION)
			addr = 0x4087d4;
		else if (ar->version.target_ver == AR6004_HW_1_1_VERSION)
			addr = 0x408304;
		else
			addr = 0;

		if (addr) {
			ath6kl_bmi_read(ar, addr, (u8 *)&data,
					sizeof(unsigned long));
			data |= (1<<29);
			ath6kl_bmi_write(ar, addr, (u8 *)&data,
					 sizeof(unsigned long));
			ret = 1;
			ath6kl_dbg(ATH6KL_DBG_BOOT,
				   "Enable USB remote wakeup.\n");
		} else {
			ret = 0;
			ath6kl_dbg(ATH6KL_DBG_BOOT,
			  "Hw not supporting USB remote wakeup, disable it.\n");
		}
	}

	return ret;
}

int ath6kl_usb_diag_warm_reset(struct ath6kl *ar)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	struct ath6kl_usb_ctrl_diag_cmd_write *cmd;
	u32 data, address;

	address = ath6kl_get_hi_item_addr(ar, HI_ITEM(hi_reset_flag_valid));
	ath6kl_usb_diag_read32(ar, address, &data);

#ifdef ATH6KL_BUS_VOTE
	if (ath6kl_platform_has_vreg == 0)
		data = 0x12345678;
	else
		data = 0;
#else
	data |= 0x12345678;
#endif

	ath6kl_usb_diag_write32(ar, address, data);

	address = ath6kl_get_hi_item_addr(ar, HI_ITEM(hi_reset_flag));
	ath6kl_usb_diag_read32(ar, address, &data);
	data |= 0x20;
	ath6kl_usb_diag_write32(ar, address, data);

	cmd = (struct ath6kl_usb_ctrl_diag_cmd_write *)ar_usb->diag_cmd_buffer;
	memset(cmd, 0, sizeof(struct ath6kl_usb_ctrl_diag_cmd_write));
	cmd->cmd = cpu_to_le32(ATH6KL_USB_CTRL_DIAG_CC_WARM_RESET);

	return ath6kl_usb_ctrl_msg_exchange(ar_usb,
			ATH6KL_USB_CONTROL_REQ_DIAG_CMD,
			(u8 *) cmd,
			sizeof(*cmd),
			0, NULL, NULL);
}

#ifdef ATH6KL_HSIC_RECOVER
static void ath6kl_recover_war_work(struct work_struct *work)
{
	ath6kl_info("%s do HSIC rediscovery\n", __func__);
	ath6kl_hsic_rediscovery();
}

int ath6kl_hsic_sw_recover(struct ath6kl *ar)
{
	struct ath6kl_usb *ar_usb = ar->hif_priv;
	struct ath6kl_vif *vif;
	struct net_device *netdev;

	vif = ath6kl_vif_first(ar);

	usb_disable_autosuspend(ar_usb->udev);

	netdev = vif->ndev;
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
	netdev->stop(netdev);
#else
	netdev->netdev_ops->ndo_stop(netdev);
#endif

	ath6kl_info("%s schedule recover worker thread\n", __func__);
	schedule_work(&recover_war_work);
	return 0;
}
#endif

static const struct ath6kl_hif_ops ath6kl_usb_ops = {
	.diag_read32 = ath6kl_usb_diag_read32,
	.diag_write32 = ath6kl_usb_diag_write32,
	.bmi_read = ath6kl_usb_bmi_read,
	.bmi_write = ath6kl_usb_bmi_write,
	.power_on = ath6kl_usb_power_on,
	.power_off = ath6kl_usb_power_off,
	.stop = ath6kl_usb_stop,
	.get_stat = ath6kl_usb_pipe_stat,
	.pipe_register_callback = ath6kl_usb_register_callback,
	.pipe_send = ath6kl_usb_send,
	.pipe_get_default = ath6kl_usb_get_default_pipe,
	.pipe_map_service = ath6kl_usb_map_service_pipe,
	.pipe_get_free_queue_number = ath6kl_usb_get_free_queue_number,
	.pipe_send_bundle = ath6kl_usb_send_bundle,
	.pipe_get_max_queue_number = ath6kl_usb_get_max_queue_number,
	.pipe_set_max_sche = ath6kl_usb_set_max_sche,
	.suspend = ath6kl_usb_suspend,
	.resume = ath6kl_usb_resume,
	.cleanup_scatter = ath6kl_usb_cleanup_scatter,
	.diag_warm_reset = ath6kl_usb_diag_warm_reset,
#ifdef CONFIG_HAS_EARLYSUSPEND
	.early_suspend = ath6kl_usb_early_suspend,
	.late_resume = ath6kl_usb_late_resume,
#endif
	.bus_config = ath6kl_usb_reconfig,
#ifdef USB_AUTO_SUSPEND
	.auto_pm_disable = usb_auto_pm_disable,
	.auto_pm_enable = usb_auto_pm_enable,
	.auto_pm_turnon = usb_auto_pm_turnon,
	.auto_pm_turnoff = usb_auto_pm_turnoff,
	.auto_pm_get_usage_cnt = usb_debugfs_get_pm_usage_cnt,
#endif
	.pipe_set_rxq_threshold = ath6kl_usb_set_rxq_threshold,
#ifdef ATH6KL_HSIC_RECOVER
	.sw_recover = ath6kl_hsic_sw_recover,
#endif
};

#ifdef ATHTST_SUPPORT
static struct hif_product_info_t g_product_info;
void ath6kl_usb_get_usbinfo(void *product_info)
{
	memcpy(product_info, &g_product_info, sizeof(g_product_info));
	return;
}
#endif

/* ath6kl usb driver registered functions */
static int ath6kl_usb_probe(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(interface);
	struct ath6kl *ar;
	struct ath6kl_usb *ar_usb = NULL;
	int vendor_id, product_id;
	int ret = 0;

	usb_get_dev(dev);

	vendor_id = le16_to_cpu(dev->descriptor.idVendor);
	product_id = le16_to_cpu(dev->descriptor.idProduct);

	ath6kl_dbg(ATH6KL_DBG_USB | ATH6KL_DBG_EXT_INFO1,
			"usb new card added, vendor_id %04x product_id %04x\n",
			vendor_id, product_id);

#ifdef ATHTST_SUPPORT
	g_product_info.idVendor = vendor_id =
					le16_to_cpu(dev->descriptor.idVendor);
	g_product_info.idProduct = product_id =
					le16_to_cpu(dev->descriptor.idProduct);
	if (dev->product)
		memcpy(g_product_info.product, dev->product,
				sizeof(g_product_info.product));
	if (dev->manufacturer)
		memcpy(g_product_info.manufacturer, dev->manufacturer,
				sizeof(g_product_info.manufacturer));
	if (dev->serial)
		memcpy(g_product_info.serial, dev->serial,
				sizeof(g_product_info.serial));
#endif

	if (interface->cur_altsetting)
		ath6kl_dbg(ATH6KL_DBG_USB, "USB Interface %d\n",
			   interface->cur_altsetting->desc.bInterfaceNumber);


	if (dev->speed == USB_SPEED_HIGH)
		ath6kl_dbg(ATH6KL_DBG_USB, "USB 2.0 Host\n");
	else
		ath6kl_dbg(ATH6KL_DBG_USB, "USB 1.1 Host\n");

	ar_usb = ath6kl_usb_create(interface);

#ifdef ATH6KL_BUS_VOTE
#ifdef CONFIG_ANDROID
	if (ath6kl_bt_on == 1 || ath6kl_platform_has_vreg == 0)
		usb_reset_device(ar_usb->udev);
#endif
#endif

	if (ar_usb == NULL) {
		ath6kl_err("Failed to create USB interface\n");
		ret = -ENOMEM;
		goto err_usb_put;
	}

	ar = ath6kl_core_alloc(&ar_usb->udev->dev);
	if (ar == NULL) {
		ath6kl_err("Failed to alloc ath6kl core\n");
		ret = -ENOMEM;
		goto err_usb_destroy;
	}

	ar->hif_priv = ar_usb;
	ar->hif_type = ATH6KL_HIF_TYPE_USB;
	ar->hif_ops = &ath6kl_usb_ops;
	ar->mbox_info.block_size = 16;
	ar->bmi.max_data_size = 252;

	ar_usb->ar = ar;
#ifdef CONFIG_ANDROID
	if (!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_USB_AUTO_PM))
		usb_disable_autosuspend(ar_usb->udev);
#endif

#ifdef USB_AUTO_SUSPEND
	spin_lock_init(&ar->usb_pm_lock);
	INIT_LIST_HEAD(&ar->usb_pm_skb_queue.list);
	pm_runtime_set_autosuspend_delay(&dev->dev, 2000);
	if (!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_USB_AUTO_PM) &&
		!(ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_TESTMODE_ENABLE) ||
		ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_ENABLE_EPPING))) {
			usb_enable_autosuspend(dev);
	}
	ar->auto_pm_cnt = 0;
#endif
	ath6kl_htc_pipe_attach(ar);
	ret = ath6kl_core_init(ar);
	if (ret) {
		ath6kl_err("Failed to init ath6kl core: %d\n", ret);
		goto err_core_free;
	}

	if (ar->version.target_ver == AR6004_HW_1_3_VERSION) {
		/* Reset TX URB count for Mck1.3.
		    TX_URB_COUNT less than 22 will degrade TX throughput*/

		ath6kl_usb_free_pipe_resources(
				&ar_usb->pipes[ATH6KL_USB_PIPE_TX_DATA_LP]);
		if (ath6kl_usb_alloc_pipe_resources(
				&ar_usb->pipes[ATH6KL_USB_PIPE_TX_DATA_LP],
				TX_URB_COUNT_LARGE) != 0) {
			ath6kl_usb_destroy(ar_usb);
			ar_usb = NULL;
			ret = -ENOMEM;
			goto err_usb_put;
		}
	}

#ifdef ATH6KL_HSIC_RECOVER
	/* Initialize the worker */
	INIT_WORK(&recover_war_work,
		  ath6kl_recover_war_work);
#endif

#ifdef ATH6KL_BUS_VOTE
	up(&usb_probe_sem);
#endif

	return ret;

err_core_free:
	ath6kl_core_free(ar);
err_usb_destroy:
	ath6kl_usb_destroy(ar_usb);
err_usb_put:
	usb_put_dev(dev);

#ifdef ATH6KL_BUS_VOTE
	up(&usb_probe_sem);
#endif

	return ret;
}

static void ath6kl_usb_remove(struct usb_interface *interface)
{
	ath6kl_dbg(ATH6KL_DBG_EXT_INFO1, "usb card removed\n");

	usb_put_dev(interface_to_usbdev(interface));
	ath6kl_usb_device_detached(interface);
}

#ifdef CONFIG_PM

#ifdef CONFIG_ANDROID
static int ath6kl_usb_pm_suspend(struct usb_interface *interface,
			      pm_message_t message)
{
	struct ath6kl_usb *device;
	struct ath6kl *ar;
	struct ath6kl_vif *vif;
	int ret;

	device = (struct ath6kl_usb *)usb_get_intfdata(interface);
	ar = device->ar;

	vif = ath6kl_vif_first(ar);

	if (ath6kl_android_need_wow_suspend(ar))
		ret = ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_WOW, NULL);
	else
		ret = ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_DEEPSLEEP,
						NULL);
	if (ret == 0)
		ath6kl_usb_flush_all(device);

	return ret;
}
#else
static int ath6kl_usb_pm_suspend(struct usb_interface *interface,
			      pm_message_t message)
{
	struct ath6kl_usb *device;
	struct ath6kl *ar;
#ifdef USB_AUTO_SUSPEND
	int ret;
#endif
	device = (struct ath6kl_usb *)usb_get_intfdata(interface);
	ar = device->ar;

#ifdef USB_AUTO_SUSPEND

	if (ath6kl_android_need_wow_suspend(ar))
		ret = ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_WOW, NULL);
	else
		ret = ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_DEEPSLEEP,
						NULL);


#else
	if (ar->state != ATH6KL_STATE_WOW)
		ath6kl_cfg80211_suspend(ar, ATH6KL_CFG_SUSPEND_DEEPSLEEP, NULL);

#endif
	ath6kl_usb_flush_all(device);
	return 0;
}
#endif


static int ath6kl_usb_pm_resume(struct usb_interface *interface)
{
	struct ath6kl_usb *device;
	struct ath6kl *ar;

	device = (struct ath6kl_usb *)usb_get_intfdata(interface);
	ar = device->ar;

	/* re-post urbs? */
	if (0) {
		ath6kl_usb_post_recv_transfers(
					&device->pipes[ATH6KL_USB_PIPE_RX_CTRL],
					ATH6KL_USB_RX_BUFFER_SIZE);
	}
	if (!htc_bundle_recv) {
		ath6kl_usb_post_recv_transfers(
				&device->pipes[ATH6KL_USB_PIPE_RX_DATA],
				ATH6KL_USB_RX_BUFFER_SIZE);
	} else {
		hif_usb_post_recv_bundle_transfers(
				&device->pipes[ATH6KL_USB_PIPE_RX_DATA],
				0 /* not allocating urb-buffer again */);
		if (0)/* no need for bundle mode resume */
			hif_usb_post_recv_bundle_transfers(
				&device->pipes[ATH6KL_USB_PIPE_RX_DATA2],
				0 /* not allocating urb-buffer again */);

	}

	ath6kl_cfg80211_resume(ar);

	return 0;
}

static int ath6kl_usb_pm_reset_resume(struct usb_interface *intf)
{
	struct ath6kl_usb *device;
	struct ath6kl *ar;

	device = (struct ath6kl_usb *)usb_get_intfdata(intf);
	ar = device->ar;
	/*
	instead of call remove directly, In HSIC mode,
	we call pm_resume to make usb continue to work
	*/
	if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode)) {
		ath6kl_info("ath6kl_usb_pm_reset_resume\n");
		ath6kl_usb_pm_resume(intf);
	} else {
		if (usb_get_intfdata(intf))
			ath6kl_usb_remove(intf);
	}
	return 0;
}
#endif

/* table of devices that work with this driver */
static struct usb_device_id ath6kl_usb_ids[] = {
	{USB_DEVICE(0x0cf3, 0x9375)},
	{USB_DEVICE(0x0cf3, 0x9374)},
	{USB_DEVICE(0x0cf3, 0x9372)},
	{ /* Terminating entry */ },
};

MODULE_DEVICE_TABLE(usb, ath6kl_usb_ids);

static struct usb_driver ath6kl_usb_driver = {
	.name = "ath6kl_usb",
	.probe = ath6kl_usb_probe,
#ifdef CONFIG_PM
	.suspend = ath6kl_usb_pm_suspend,
	.resume = ath6kl_usb_pm_resume,
	.reset_resume = ath6kl_usb_pm_reset_resume,
#endif
	.disconnect = ath6kl_usb_remove,
	.id_table = ath6kl_usb_ids,
	.supports_autosuspend = true,
};

#ifdef CONFIG_ANDROID
static int ath6kl_usb_dev_notify(struct notifier_block *nb,
				 unsigned long action, void *dev)
{
	struct usb_device *udev;
	int ret = NOTIFY_DONE;

	if (action != USB_DEVICE_REMOVE)
		goto done;

	udev = (struct usb_device *) dev;
	if (ath6kl_usb_unload_dev_num != udev->devnum)
		goto done;

	if (atomic_read(&ath6kl_usb_unload_state) ==
			ATH6KL_USB_UNLOAD_STATE_TARGET_RESET) {
		atomic_set(&ath6kl_usb_unload_state,
			   ATH6KL_USB_UNLOAD_STATE_DEV_DISCONNECTED);
		wake_up(&ath6kl_usb_unload_event_wq);
	}

done:
	return ret;
}

static struct notifier_block ath6kl_usb_dev_nb = {
	.notifier_call = ath6kl_usb_dev_notify,
};

static int ath6kl_usb_init(void)
{
	init_waitqueue_head(&ath6kl_usb_unload_event_wq);
	atomic_set(&ath6kl_usb_unload_state, ATH6KL_USB_UNLOAD_STATE_NULL);
	usb_register_notify(&ath6kl_usb_dev_nb);

#ifdef ATH6KL_BUS_VOTE
	sema_init(&usb_probe_sem, 1);
	down(&usb_probe_sem);
#endif

	usb_register(&ath6kl_usb_driver);

#ifdef ATH6KL_BUS_VOTE
#ifdef CONFIG_ANDROID
	if (machine_is_apq8064_dma() || machine_is_apq8064_bueller()) {
		ath6kl_platform_has_vreg = 1;
		ath6kl_hsic_bind(1);
	}
#endif

	if (ath6kl_hsic_init_msm(&ath6kl_platform_has_vreg) != 0)
		ath6kl_err("%s ath6kl_hsic_init_msm failed\n", __func__);

	if (ath6kl_platform_has_vreg) {
		/* Waiting for usb probe callback called */
		if (down_timeout(&usb_probe_sem,
			msecs_to_jiffies(USB_PROBE_WAIT_TIMEOUT)) != 0) {
			ath6kl_info("can't wait for usb probe done\n");
		}
	}
#endif

	return 0;
}

static void ath6kl_usb_exit(void)
{
	long timeleft = 0;
	atomic_set(&ath6kl_usb_unload_state, ATH6KL_USB_UNLOAD_STATE_DRV_DEREG);
	usb_deregister(&ath6kl_usb_driver);

	if (atomic_read(&ath6kl_usb_unload_state) !=
					ATH6KL_USB_UNLOAD_STATE_TARGET_RESET)
		goto finish;

	timeleft = wait_event_interruptible_timeout(ath6kl_usb_unload_event_wq,
				atomic_read(&ath6kl_usb_unload_state) ==
				ATH6KL_USB_UNLOAD_STATE_DEV_DISCONNECTED,
				ATH6KL_USB_UNLOAD_TIMEOUT);

finish:
	usb_unregister_notify(&ath6kl_usb_dev_nb);
#ifdef ATH6KL_BUS_VOTE
	ath6kl_hsic_exit_msm();
#endif

#ifdef ATH6KL_BUS_VOTE
	if (machine_is_apq8064_dma() || machine_is_apq8064_bueller())
		ath6kl_hsic_bind(0);
#endif

}
#else
static int ath6kl_usb_init(void)
{
	usb_register(&ath6kl_usb_driver);
	return 0;
}

static void ath6kl_usb_exit(void)
{
	usb_deregister(&ath6kl_usb_driver);
}
#endif

module_init(ath6kl_usb_init);
module_exit(ath6kl_usb_exit);

MODULE_AUTHOR("Atheros Communications, Inc.");
MODULE_DESCRIPTION("Driver support for Atheros AR600x USB devices");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_FIRMWARE(AR6004_HW_1_0_FW_DIR "/" AR6004_HW_1_0_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6004_HW_1_0_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_0_DEFAULT_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_1_FW_DIR "/" AR6004_HW_1_1_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6004_HW_1_1_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_1_DEFAULT_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_2_FW_DIR "/" AR6004_HW_1_2_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6004_HW_1_2_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_2_DEFAULT_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_3_FW_DIR "/" AR6004_HW_1_3_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6004_HW_1_3_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_1_3_DEFAULT_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_2_0_FW_DIR "/" AR6004_HW_2_0_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6004_HW_2_0_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6004_HW_2_0_DEFAULT_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6006_HW_1_0_FW_DIR "/" AR6006_HW_1_0_FIRMWARE_FILE);
MODULE_FIRMWARE(AR6006_HW_1_0_BOARD_DATA_FILE);
MODULE_FIRMWARE(AR6006_HW_1_0_DEFAULT_BOARD_DATA_FILE);
