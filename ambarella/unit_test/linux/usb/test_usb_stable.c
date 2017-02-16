#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/time.h>
#include <time.h>
#include <assert.h>

enum {
	MSG_TEST = 1,
	MSG_STATUS,
};

/* Note: keep header the same */
struct usb_msg {
	int type;
	int param1;
	int param2;
	int len;
	char data[0];
};

static int direction_flag = 2;
static int fixed_size_flag = -1;  /* -1 means random size */

#define USB_MSG_HEAD_SIZE	(sizeof(struct usb_msg))
#define MAX_TRANSFER_SIZE	(512 * 1024)

static void init_msg(struct usb_msg *msg, int len)
{
	int size = USB_MSG_HEAD_SIZE + len;

	memset(msg, 0, size);
	msg->type = MSG_TEST;
	msg->len = len;
	msg->param1 = 2; /* Meaningless, just for debug  */
	msg->param2 = 3;
}

static void compose_msg_randon(struct usb_msg *msg)
{
	int num, len;

	num = rand() % 4;

	switch (num) {
	case 0:
		len = 0;
		break;
	case 1:
		len = 1024 + 323;
		break;
	case 2:
		len = 64 * 1024 + 27;
		break;
	case 3:
		len = rand() % (400 * 1024);
		break;
	default:
		break;
	}

	if (fixed_size_flag != -1)
		len = fixed_size_flag % (400 * 1024);

	init_msg(msg, len);
}

static int usb_read(int fd, char *buf, int len)
{
	int ret;
	char *tmp_buf;

	tmp_buf = buf;

	while (len > 0) {
		ret = read(fd, tmp_buf, len);
		if (ret < 0) {
			printf("%s() failed: %s\n", __func__, strerror(errno));
			return -1;
		}

		tmp_buf += ret;
		len -= ret;
	}

	return 0;
}

static int usb_write(int fd, char *buf, int len)
{
	int ret;
	char *tmp_buf;

	tmp_buf = buf;

	while (len > 0) {
		ret = write(fd, tmp_buf, len);
		if (ret < 0) {
			printf("%s() failed: %s\n", __func__, strerror(errno));
			return -1;
		}

		tmp_buf += ret;
		len -= ret;
	}

	return 0;
}

int main()
{
	int fd_usb, ret;
	struct usb_msg *msg;

	fd_usb = open("/dev/amb_gadget", O_RDWR);
	if (fd_usb < 0) {
		printf("open /dev/amb_gadget failed, errno(%d)\n", errno);
		return -1;
	}

	/* Initial message head */
	msg = malloc(MAX_TRANSFER_SIZE);
	assert(msg);

	ret = usb_read(fd_usb, (char *)msg, USB_MSG_HEAD_SIZE);
	if (ret < 0) {
		printf("faild to receive command\n");
		return -1;
	}

	direction_flag = msg->param1;
	fixed_size_flag = msg->param2;
	if (direction_flag > 2) {
		printf("invalid command from host\n");
		return -1;
	}

	printf("direction_flag = %d, fixed_size_flag = %d\n",
		direction_flag, fixed_size_flag);

//	srand(time(0));

	/* Start to transfer data */
	while(1) {
		if (direction_flag == 2 || direction_flag == 0) {
			/* Initial message head */
			compose_msg_randon(msg);

			printf("sending msg (len:%d)\n", msg->len);

			/* Send message head */
			ret = usb_write(fd_usb, (char *)msg, USB_MSG_HEAD_SIZE);
			assert(!ret);

			/* Send message body */
			ret = usb_write(fd_usb, msg->data, msg->len);
			assert(!ret);
		}

		if (direction_flag == 2 || direction_flag == 1) {
			memset(msg, 0, USB_MSG_HEAD_SIZE);
			/* Receive message head */
			ret = usb_read(fd_usb, (char *)msg, USB_MSG_HEAD_SIZE);
			assert(!ret);

			printf("reading msg (len:%d)\n", msg->len);

			/* Receive message body */
			ret = usb_read(fd_usb, msg->data, msg->len);
			assert(!ret);
		}
	}

	free(msg);
	close(fd_usb);
	return 0;
}
