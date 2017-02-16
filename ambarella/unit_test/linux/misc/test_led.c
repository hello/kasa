/*
 * test_led.c
 * Max7219 8-digit led display drivers test.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <linux/input.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <sched.h>
#include <getopt.h>
#endif

typedef unsigned char  	    u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short 	    u16;/**< UNSIGNED 16-bit data type */
int		spi_fd;
u16		cmd;

/* Check xxxx.dts file in board/xxxx/bsp for the device node */
#define MAX7219_SPI_DEV_NODE		"/dev/spidev0.2"
#define MAX7219_WRITE_REGISTER(addr, val)	\
	do {								\
		cmd = ((addr) << 8) | (val);	\
		write(spi_fd, &cmd, 2);		\
	} while (0)

#define DIGITAL_SEGMENT	(8)

struct timespec starttime, curtime;
static int shut_down_flag =0 ;
static int test_flag =0 ;
static int count_flag =0 ;
static int button_key = -1 ;
static int count = 999 ;
static int Number_flag =0 ;
static int Number = 100 ;

static int Rate = 1000*1000 ;
static int Sleep = 1000 ;

static int Skip_num = -1 ;

static const char *short_options = "stc:b:N:r:m:k:";

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"shut_down", NO_ARG, 0, 's'},
	{"test", NO_ARG, 0, 't'},
	{"count", HAS_ARG, 0, 'c'},
	{"btnkey", HAS_ARG, 0, 'b'},
	{"num", HAS_ARG, 0, 'N'},
	{"rate", HAS_ARG, 0, 'r'},
	{"sleep", HAS_ARG, 0, 'm'},
	{"skip", HAS_ARG, 0, 'k'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t Shut down LED."},
	{"", "\t Show All LED."},
	{"", "\t Start LED count."},
	{"", "\t Wait Button Key to start LED count."},
	{"", "\t Show Number on LED."},
	{"", "\t Change LED speed rate."},
	{"", "\t Sleep us to change LED count."},
	{"", "\t Skip broken LED."}
};

static void usage(void)
{
	int i;
	printf("test_led usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
	printf("E.g.: Start LED Count\n");
	printf("\t> test_led -c 88888888\n");
	printf("E.g.: Waiting Button Key [211] to Start LED Count\n");
	printf("\t> test_led -c 88888888 -b 211\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 's':
			shut_down_flag = 1;
			break;
		case 't':
			test_flag = 1;
			break;
		case 'c':
			count_flag = 1;
			count = atoi(optarg);
			break;
		case 'b':
			button_key = atoi(optarg);
			break;
		case 'N':
			Number_flag = 1;
			Number = atoi(optarg);
			break;
		case 'r':
			Rate = atoi(optarg);
			break;
		case 'm':
			Sleep = atoi(optarg);
			break;
		case 'k':
			Skip_num = atoi(optarg);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int convert_decimal(int source, unsigned char * target)
{
	int i = 0;
	do {
		target[i]=source % 10;
		source = source / 10;
		i++;
	} while (source);

	return i;
}

void display_segments(int i, unsigned char * target)
{
	int j;

	for (j = 1; (j <= i) && (j <= DIGITAL_SEGMENT); j++) {
		if((Skip_num <= j) && (Skip_num > -1) && (Skip_num < 8)) {
			MAX7219_WRITE_REGISTER(j+1, target[j-1]);
		} else {
			MAX7219_WRITE_REGISTER(j, target[j-1]);
		}
	}
}

struct keypadctrl {
	int running;
	int *pfd;
	unsigned int count;
};

static int key_cmd_process(struct input_event *cmd)
{

	if (cmd->type != EV_KEY)
		return 0;

	if (cmd->value)
		printf(" == Key [%d] is pressed ==\n", cmd->code);
	else
		printf(" == Key [%d] is release ==\n", cmd->code);

	return 0;
}

/* Used for wait button key to start count */
void waiting_button(int btn_key) {
	struct keypadctrl keypad;
	struct input_event event;
	fd_set rfds;
	int rval = -1;
	int i = 0;
	char str[64] = {0};
	static int previous_btn_key = -1;

	keypad.running = 1;
	keypad.count = 0;
	keypad.pfd = NULL;

	do {
		sprintf(str, "/dev/input/event%d", keypad.count);
		keypad.count++;
	} while(access(str, F_OK) == 0);

	keypad.count--;
	if (keypad.count == 0) {
		printf("No any event device!\n");
		goto done;
	}

	printf("Totally %d event\n", keypad.count);
	keypad.pfd = (int *)malloc(keypad.count * sizeof(int));
	if (!keypad.pfd) {
		printf("Malloc failed!\n");
		goto done;
	}

	for (i = 0; i < keypad.count; i++) {
		sprintf(str, "/dev/input/event%d", i);
		keypad.pfd[i] = open(str, O_NONBLOCK);
		if (keypad.pfd[i] < 0) {
			printf("open %s error", str);
			goto done;
		}
	}
	printf("Waiting button[%d]...\n", btn_key);

	while (keypad.running) {
		struct timeval timeout = {0, 1};
		FD_ZERO(&rfds);
		for (i = 0; i < keypad.count; i++) {
			FD_SET(keypad.pfd[i], &rfds);
		}

		rval = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout);
		if (rval < 0) {
			perror("select error!");
			if (errno == EINTR) {
			continue;
		}
		  goto done;
		} else if (rval == 0) {
		  continue;
		}

		for (i = 0; i < keypad.count; i++) {
			if (FD_ISSET(keypad.pfd[i], &rfds)) {
				rval = read(keypad.pfd[i], &event, sizeof(event));
				if (rval < 0) {
					printf("read keypad->fd[%d] error",i);
					continue;
				}

				key_cmd_process(&event);
				if ((previous_btn_key == btn_key) && (event.code == 0)) {
					keypad.running = 0;
					break;
				}
				previous_btn_key = event.code;
				}
			}
		}
done:

	keypad.running = 0;
	for (i = 0; i < keypad.count; i++) {
		close(keypad.pfd[i]);
	}

	keypad.count = 0;
	if (keypad.pfd){
		free(keypad.pfd);
		keypad.pfd = NULL;
	}
}

static void sigstop(int signum)
{
	if ((count_flag) && (starttime.tv_sec || starttime.tv_sec)) {
		printf("Stop LED Count\n");
		clock_gettime(CLOCK_REALTIME, &curtime);
		printf("Elapsed Time (ms): [%05ld]\n",
		(curtime.tv_sec - starttime.tv_sec) * 1000
			+ (curtime.tv_nsec - starttime.tv_nsec) / 1000000);
	}
	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	int		mode = 0, bits = 16;
	int		ret = 0;
	unsigned char 	decimal[DIGITAL_SEGMENT];
	int 		display_number;

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	spi_fd = open(MAX7219_SPI_DEV_NODE, O_RDWR);
	if (spi_fd < 0) {
		printf("Can't open MAX7219_SPI_DEV_NODE to write \n");
		return -1;
	}

	/* Mode */
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_WR_MODE");
		return -1;
	}

	/* bpw */
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_WR_BITS_PER_WORD");
		return -1;
	}

	/* speed */
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &Rate);
	if (ret < 0) {
		perror("SPI_IOC_WR_MAX_SPEED_HZ");
		return -1;
	}

	printf("Mode = %d\n", mode);
	printf("Bpw = %d\n", bits);
	printf("Baudrate = %d\n", Rate);

	MAX7219_WRITE_REGISTER(0xa, 0xa);
	MAX7219_WRITE_REGISTER(0x9, 0xff);
	MAX7219_WRITE_REGISTER(0xb, 0x7);

	if (shut_down_flag) {
		MAX7219_WRITE_REGISTER(0xc, 0x0);
	} else {
		MAX7219_WRITE_REGISTER(0xc, 0x1);
	}

	if (test_flag) {
		MAX7219_WRITE_REGISTER(0xf, 0x1);
		return 0;
	} else {
		MAX7219_WRITE_REGISTER(0xf, 0x0);
		int j;
		for (j = 1; j <= DIGITAL_SEGMENT; j++) {
			MAX7219_WRITE_REGISTER(j, 0x7f);
		}
	}

	if (count_flag) {
		if (button_key >= 0) {
			waiting_button(button_key);
		}
		printf("Start LED Count...\n");
		clock_gettime(CLOCK_REALTIME, &starttime);
		display_number=0;
		do {
			if ((ret = convert_decimal(display_number, decimal)) < 0)
				return -1;
			display_segments(ret, decimal);
			usleep(Sleep);
			display_number++;
		} while (display_number <= count);
	}

	if (Number_flag) {
		printf ("Number = %d \n",Number);
		if ((ret = convert_decimal(Number, decimal)) <= 0)
			return -1;
		display_segments(ret, decimal);
	}

	close(spi_fd);
	return 0;
}

