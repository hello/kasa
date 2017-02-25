/*******************************************************************************
 * wowl.c
 *
 * History:
 *    2015/4/1 - [Tao Wu] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
******************************************************************************/

#define __USE_BSD		/* Using BSD IP header 	*/
#include <netinet/ip.h>	/* Internet Protocol 		*/
#define __FAVOR_BSD	/* Using BSD TCP header	*/
#include <netinet/tcp.h>	/* Transmission Control Protocol	*/

#include <pcap.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <poll.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <log_level.h>
#include <net_util.h>
#include <brcm_types.h>
#include <brcm_ioc.h>
#include <basetypes.h>

#define WOWL_VERSION		"1.0.9"

#define MAXBYTES2CAPTURE	(2048)
#define DATA_MAXSIZE 		(32)
#define PAYLOAD_MAXSIZE 	(256)
#define FILTER_MAXSIZE		(512)

#define DEFAULT_HOST		"127.0.0.1"
#define DEFAULT_PORT		(7877)
#define DEFAULT_INTERVAL	(10)
#define DEFAULT_DTIM_INTERVAL	(1000)
#define DEFAULT_MSG		"*"
#define WAKE_PAYLOAD_HEX 	"77616b65" // the ascii of "wake"

#define DEFAULT_NET_FILTER	"tcp src port 7877"

#define SUSPEND_CMD		"echo mem > /sys/power/state"

#define PROC_WIFI_STATE	"/proc/ambarella/wifi_pm_state"

unsigned char cipher_str1[] = {
	0x0F,0x62,0xB5,0x08,0x5B,0xAE,0x01,0x54,
	0xA7,0xFA,0x4D,0xA0,0xF3,0x46,0x99,0xEC,
	0x28,0x8F,0xF6,0x5D,0xC4,0x2B,0x92,0xF9,
	0x60,0xC7,0x2E,0x95,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char cipher_str2[] = {
	0x0F,0x62,0xB5,0x08,0x5B,0xAE,0x01,0x54,
	0xA7,0xFA,0x4D,0xA0,0xF3,0x46,0x99,0xEC,
	0x28,0x8F,0xF6,0x5D,0xC4,0x2B,0x92,0xF9,
	0x60,0xC7,0x2E,0x95,0xFF,0x00,0x00,0x00,
	0xAA,0x00,0x00,0x00,0x00,0x00,0x00,0x11
};

unsigned char cipher_str3[] = {
	0x05,0x58,0xAB,0xFE,0x51,0xA4,0xF7,0x4A,
	0x9D,0xF0,0x43,0x96,0xE9,0x3C,0x8F,0xE2,
	0x16,0x7D,0xE4,0x4B,0xB2,0x19,0x80,0xE7,
	0x4E,0xB5,0x1C,0x83,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#define NO_ARG		(0)
#define HAS_ARG		(1)

typedef enum {
	CPU_SUSPEND = 0,
	CPU_NORMAL = 1,
	CPU_UNKNOWN = 2,
} cpu_state;

typedef struct net_socket_s
{
	int fd;
	int is_session;
	int is_close;
	int is_tcp;

	int is_add_tcp_payload;
	int is_enable_pattern;
	int is_enable_filter;
	int is_enable_host_cipher;
	int is_enable_host_sleep;

	int is_enable_suspend;
	int is_poll;
	int is_add_resume;
	int is_reconn;

	int is_sys_exec;
	int suspend_pm;
	int port;
	int interval;
	int dtim_interval;

	wifi_chip_t wifi_chip_id;
	log_level_t log_level;

	char iface[IFNAMSIZ];
	char srv_ip[DATA_MAXSIZE];
	char send_msg[PAYLOAD_MAXSIZE];
	unsigned int send_msg_len;
} net_socket_t;

static net_socket_t net_socket;

static pcap_t *pG_pcap = NULL;
static pthread_t G_pcap_tid = 0;
static tcpka_conn_sess_info_t G_tcp_info_old = {0, 0, 0};
static int G_pm_pipefd[2] = {-1, -1};
static int G_running = 1;

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "i:c:p:tusm:d:k:bnrqP:yloeagxw:v:h";
static struct option long_options[] = {
	{"interface",	HAS_ARG, 0, 'i'},
	{"host",		HAS_ARG, 0, 'c'},
	{"port",		HAS_ARG, 0, 'p'},
	{"udp",			NO_ARG,  0, 'u'},
	{"tcp",			NO_ARG,  0, 't'},
	{"session",		NO_ARG,  0, 's'},
	{"kalive-tvl",	HAS_ARG, 0, 'm'},
	{"dtim-tvl",	HAS_ARG, 0, 'd'},
	{"payload",		HAS_ARG, 0, 'k'},
	{"no-payload", 	NO_ARG,  0, 'b'},
	{"wowl-pattern",	NO_ARG,  0, 'n'},
	{"pkt-filter",	NO_ARG,  0, 'y'},
	{"host-cipher",	NO_ARG,  0, 'r'},
	{"host-sleep",	NO_ARG,  0, 'q'},
	{"PM",			HAS_ARG, 0, 'P'},
	{"suspend", 	NO_ARG,  0, 'l'},
	{"poll", 		NO_ARG,  0, 'o'},
	{"resume",		NO_ARG,  0, 'a'},
	{"reconn",		NO_ARG,  0, 'g'},
	{"close",		NO_ARG,  0, 'e'},
	{"system",		NO_ARG,  0, 'x'},
	{"wifi-chip",	HAS_ARG, 0, 'w'},
	{"loglevel",	HAS_ARG, 0, 'v'},
	{"help",		NO_ARG,  0, 'h'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "Listen on interface"},
	{"", "Host IP address"},
	{"", "Host Port, default is [7877]"},
	{"", "UDP Keep Alive and Wakeup"},
	{"", "TCP Keep Alive and Wakeup"},
	{"", "Create one TCP/UDP session"},
	{"", "Keep Alive interval time, in seconds. Disable KeepAlive when set 0"},
	{"", "DTIM interval time, in mseconds. Equal to AP's DTIM when set 0"},
	{"", "Add Payload. Default is '*' "},
	{"", "Do not add Payload in TCP KeepAlive"},
	{"", "Set wowl pattern when do TCP/UDP KeepAlive. Default is disable"},
	{"", "Set pkt filter when do UDP KeepAlive. Default is disable"},
	{"", "Add Host Cipher"},
	{"", "Add Host Sleep, enable by default (Discard All packet finally avoid WiFi memory overflow)"},
	{"", "\tSet PM value before suspend. 0: CAM, 1: PS, 2: FAST PS. Default is 1"},
	{"", "Add suspend after set TCP KeepAlive"},
	{"", "Use poll to detect Wifi driver resume state, otherwise use sleep after suspend"},
	{"", "Add TCP resume and transfer data"},
	{"", "Reconnect to server when TCP is dissconnect"},
	{"", "Close fd after exit. Default is disable"},
	{"", "Using system exec to call wl(Keep-alive, Wakeup Pattern)"},
	{"", "WiFi chip ID. 0:BCM43340; 1:BCM43438A0/BCM43438A1/BCM43455. Default is 0"},
	{"", "Set Log Level. 0:ERROR, 1:WAR, 2:INFO, 3:DEBUG, 4:VERBOSE. Default is 2"},
	{"", "Show usage"},
};

static void usage(void)
{
	u32 i = 0;
	char *itself = "wowl";

	printf("This program used to set wake up on wireless (include keepalive and wakeup pattern)\n");
	printf("\n");

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

	printf("\nExample:\n");

	printf("\n");
	printf("\t TCP Resume # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -t -s -m 10 -d 1000 -e -n -l -o -a -P 1 -q \n", itself);
	printf("(BCM43438)TCP Resume# %s -w 1 -i wlan0 -c 127.0.0.1 -p 7877 -t -s -m 10 -d 1000 -e -n -l -o -a -P 1 -q \n", itself);
	printf("\t UDP Resume # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -u -s -m 10 -d 1000 -e -n -l -o -a -P 1 -q \n", itself);

	printf("\n");
	printf("\t TCP        # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -t -s -m 10 -d 1000 -n -a -P 1 -q \n", itself);
	printf("\t UDP        # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -u -s -m 10 -d 1000 -n -a -P 1 -q \n", itself);

	printf("\n");
	printf("TCP(No Suspend)     # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -t -s -m 10 -d 1000 -e -n -a -P 1 -q \n", itself);

	printf("\n");
	printf("\t UDP Filter # %s -w 0 -i wlan0 -c 127.0.0.1 -p 7877 -u -s -m 10 -y -a -P 1 -q \n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch = 0;
	int value = 0;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'i':
			strncpy(net_socket.iface, optarg, sizeof(net_socket.iface));
			break;
		case 'c':
			strncpy(net_socket.srv_ip, optarg, sizeof(net_socket.srv_ip));
			break;
		case 'p':
			value = atoi(optarg);
			if ((value < 0) || (value > 65535)) {
				printf("Please set port in %d ~ %d.\n", 0, 65535);
				return -1;
			}
			net_socket.port = value;
			break;
		case 'm':
			value = atoi(optarg);
			if ((value < 0) || (value > 65535)) {
				printf("Please set interval in %d ~ %d.\n", 0, 65535);
				return -1;
			}
			net_socket.interval = value;
			break;
		case 'd':
			value = atoi(optarg);
			if ((value < 0) || (value > 65535)) {
				printf("Please set DTIM interval in %d ~ %d.\n", 0, 65535);
				return -1;
			}
			net_socket.dtim_interval = value;
			break;
		case 't':
			net_socket.is_tcp = 1;
			break;
		case 'u':
			net_socket.is_tcp = 0;
			break;
		case 's':
			net_socket.is_session = 1;
			break;
		case 'k':
			strncpy(net_socket.send_msg, optarg, sizeof(net_socket.send_msg));
			net_socket.send_msg_len = strlen(net_socket.send_msg);
			break;
		case 'b':
			net_socket.is_add_tcp_payload = 0;
			break;
		case 'n':
			net_socket.is_enable_pattern = 1;
			break;
		case 'y':
			net_socket.is_enable_filter= 1;
			break;
		case 'r':
			net_socket.is_enable_host_cipher = 1;
			break;
		case 'q':
			net_socket.is_enable_host_sleep = 1;
			break;
		case 'P':
			value = atoi(optarg);
			if (value != 0 && value != 1 && value != 2) {
				printf("Please set PM in 0|1|2.\n");
				return -1;
			}
			net_socket.suspend_pm = value;
			break;
		case 'l':
			net_socket.is_enable_suspend = 1;
			break;
		case 'o':
			net_socket.is_poll = 1;
			break;
		case 'a':
			net_socket.is_add_resume = 1;
			break;
		case 'g':
			net_socket.is_reconn = 1;
			break;
		case 'e':
			net_socket.is_close = 1;
			break;
		case 'x':
			net_socket.is_sys_exec = 1;
			break;
		case 'w':
			value = atoi(optarg);
			if ((value < WIFI_CHIP_FIRST) || (value > WIFI_CHIP_LAST)) {
				printf("Please chosse WiFi Chip ID in [%d, %d].\n",
					WIFI_CHIP_FIRST, WIFI_CHIP_LAST);
				return -1;
			}
			net_socket.wifi_chip_id = value;
			break;
		case 'v':
			value = atoi(optarg);
			if ((value < LOG_LEVEL_FIRST) || (value > LOG_LEVEL_LAST)) {
				printf("Please chosse Log Level in [%d, %d].\n",
					LOG_LEVEL_FIRST, LOG_LEVEL_LAST);
				return -1;
			}
			net_socket.log_level= value;
			break;
		case 'h':
			usage();
			return -1;
			break;
		default:
			printf("unknown option found: %c.\n", ch);
			return -1;
			break;
		}
	}

	return 0;
}

static void close_fd(int *fd)
{
	if (net_socket.is_close && (*fd >= 0)) {
		close(*fd);
		logi("Close fd [%d].\n", *fd);
		*fd = -1;
	}
}

static void sys_enter_suspend(void)
{
#define SLEEP_BEFORE_SUSPEND (200000)

	static u32 suspend_count = 0;
	logd("Go to suspend mode, count [%u] \n", suspend_count++);
	system("sync");
	fflush(stdin);
	fflush(stdout);
	system("sync");
	fflush(stdin);
	fflush(stdout);
	usleep(SLEEP_BEFORE_SUSPEND);
	logd("Sleep %d us done\n", SLEEP_BEFORE_SUSPEND);

	system(SUSPEND_CMD);
}

static void poll_wait_sys_state(void)
{
#define POLL_TIMEOUT (3000) // 3s

	int ret = -1;
	int state_fd = -1;
	cpu_state curr_state = CPU_UNKNOWN;
	struct pollfd fds;

	state_fd = open(PROC_WIFI_STATE, O_RDWR);
	if (state_fd < 0) {
		perror("open" PROC_WIFI_STATE );
		return;
	}
	fds.fd = state_fd;
	fds.events = POLLIN;

	do {
		ret = poll(&fds, 1, POLL_TIMEOUT);
		if(ret == 0) {
			printf("Poll time out\n");
		} else {
			ret = read(state_fd, &curr_state, sizeof(curr_state));
			if (ret < 0) {
				perror("read\n");
			} else {
				logd("Poll system current state [%d]\n", curr_state);
				break;
			}
		}
	} while (0);

	close(state_fd);
	state_fd = -1;
}

/* Do call this function before suspend in every case, whatever AP or Server APP is connected */
static void wifi_enter_sleep(void)
{
	int hostsleep = 1;

	if (net_socket.is_enable_host_sleep) {
		hostsleep = 1;
		wl_set_get_host_sleep(&hostsleep, 1);
		wl_set_get_host_sleep(&hostsleep, 0);
	}
	wl_set_dtim_interval(net_socket.dtim_interval);
	wl_set_get_pm_mode(&net_socket.suspend_pm, 1);
}

static void wifi_enter_normal(tcpka_conn_sess_info_t *p_tcp_info,
	wl_wowl_wakeind_t *p_wakeind, wl_pkt_filter_stats_t *p_filter_stats)
{
	int pm = 2;
	int hostsleep = 0;

	logd("WiFi Normal Mode Start\n");
	if (net_socket.interval > 0) {
		if (net_socket.is_tcp) {
			wl_tcpka_conn_enable(1, 0, 0, 0, 0);
			wl_tcpka_conn_sess_info(1, p_tcp_info);
			wl_tcpka_conn_del(1);
		} else {
			wl_mkeep_alive(1, 0, 0, NULL);
		}
	}
	if (net_socket.is_enable_host_sleep) {
		hostsleep = 0;
		wl_set_get_host_sleep(&hostsleep, 1);
	}
	if (net_socket.is_enable_pattern) {
		wl_wowl_wakeind(p_wakeind);
		wl_wowl_wakeind_clear();
		wl_wowl_clear();
		wl_wowl_pattern_clr();
	} else if (net_socket.is_enable_filter) {
		wl_pkt_filter_stats(200, p_filter_stats);
		wl_pkt_filter_enable(200, 0);
		wl_pkt_filter_delete(200);
	}

	pm = 2;
	wl_set_get_pm_mode(&pm, 1);
	wl_set_bcn_li_dtim(0);

	logd("WiFi Normal Mode Done\n");
}

static int fix_tcp_info(int sock_fd, tcpka_conn_sess_info_t tcp_info)
{
	int ret = -1;

	if (sock_fd < 0) {
		loge("Socket fd [%d] is invalid\n", sock_fd);
		return -1;
	}

	logd("Modify TCP info start\n");
	ret = ioctl(sock_fd, SET_TCP_FIX, &tcp_info);
	if (ret < 0) {
		perror("SET_TCP_FIX");
	} else {
		logi("Modify TCP info done\n");
	}

	return ret;
}

static void suspend_and_resume(int sockfd, int wpipefd)
{
#define SLEEP_SECONDS (20)

	int ret = 0;
	cpu_state sys_state = CPU_NORMAL;
	tcpka_conn_sess_info_t tcp_info = {0, 0, 0};
	wl_wowl_wakeind_t wakeind;
	wl_pkt_filter_stats_t filter_stats;
	static u32 resume_count = 0;

	/* Prepare suspend */
	sys_state = CPU_SUSPEND;
	ret = write(wpipefd, &sys_state, sizeof(sys_state));
	if (ret < 0) {
		perror("write pipe");
		return;
	}
	logd("Notify pipe SUSPEND mode\n");

	/* Execute suspend */
	if (net_socket.is_enable_suspend) {
		sys_enter_suspend();
	}

	/* Wait Resume back */
	if (net_socket.is_poll) {
		poll_wait_sys_state();
	} else {
		logi("Sleep %d senconds\n", SLEEP_SECONDS);
		sleep(SLEEP_SECONDS);
	}
	logd("Back from resume mode, count [%u] ...\n", resume_count++);

	wifi_enter_normal(&tcp_info, &wakeind, &filter_stats);

	if (net_socket.is_tcp) {
		logi("Resume TCP info: ipid[%u], seq[%u], ack[%u].\n",
			tcp_info.tcpka_sess_ipid, tcp_info.tcpka_sess_seq, tcp_info.tcpka_sess_ack);
		if (tcp_info.tcpka_sess_ipid > (G_tcp_info_old.tcpka_sess_ipid + 1)) {
			fix_tcp_info(sockfd, tcp_info);
		}
	}

	/* Resume done */
	sys_state = CPU_NORMAL;
	ret = write(wpipefd, &sys_state, sizeof(sys_state));
	if (ret < 0) {
		perror("write pipe");
		return;
	}
	logd("Notify pipe NORMAL mode\n");

	if ((wakeind.ucode_wakeind & WL_WOWL_TCPFIN) == WL_WOWL_TCPFIN) {
		logi("Wake on TCP FIN packet\n");
		/* TODO: Close fd if not send "Online" pkt to server after resume,
		 * beacuse server will send RST pkt when receive PSH pkt in half close state.
		 * Otherwise, please need close fd after resume when no pkt send to server. */
	}
	if (((wakeind.ucode_wakeind & WL_WOWL_DIS) == WL_WOWL_DIS) ||
		((wakeind.ucode_wakeind & WL_WOWL_BCN) == WL_WOWL_BCN) ) {
		logi("Wake on dissconnect with AP\n");
		/* TODO: Resume connect to AP */
	}
}

static void transfer_data(int rpipefd, int sock_fd, struct sockaddr_in *addr)
{
#define SELECT_TIMEOUT (30)

	int ret = 0;
	int max_fd = -1;
	fd_set fdset;
	struct timeval tv;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	cpu_state curr_state = CPU_UNKNOWN;
	char online[32] = "Online";
	char msg[PAYLOAD_MAXSIZE];
	logd("Transfer data. Pipe fd [%d], Sockt fd [%d], Addr[%p]\n", rpipefd, sock_fd, addr);

	while (G_running) {
		FD_ZERO(&fdset);
		FD_SET(rpipefd, &fdset);
		FD_SET(sock_fd, &fdset);
		max_fd = (rpipefd > sock_fd) ? rpipefd : sock_fd;
		tv.tv_sec = SELECT_TIMEOUT;
		tv.tv_usec = 0;

		ret = select(max_fd + 1, &fdset, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			break;
		} else if (ret == 0) {
			//printf("Select timeout\n");
			continue;
		}

		if (FD_ISSET(rpipefd, &fdset)) {
			ret = read(rpipefd, &curr_state, sizeof(curr_state));
			if (ret < 0) {
				perror("read pipe");
				break;
			}
			logd("Read pipe msg state [%d]\n", curr_state);
			if (curr_state == CPU_NORMAL) {
				if (addr) { /* UDP */
					ret = sendto(sock_fd, online, strlen(online), 0, (struct sockaddr *) addr, sock_len);
				} else { /* TCP */
					ret = writen(sock_fd, online, strlen(online));
				}

				if (ret == strlen(online)) {
					logd("Send [%s] Length [%d] OK\n", online, ret);
				} else {
					logd("Send [%s] Length [%d] Error: %s\n", online, ret, strerror(errno));
					break;
				}
			}
		}

		if (FD_ISSET(sock_fd, &fdset)) {
			/*
			if (curr_state != CPU_NORMAL) {
				loge("System is not in NORMAL mode");
				continue;
			}*/
			memset(msg, 0, sizeof(msg));
			if (addr) { /* UDP */
				ret = recvfrom(sock_fd, msg, sizeof(msg), 0, (struct sockaddr *) addr, &sock_len);
			} else { /* TCP */
				ret = read(sock_fd, msg, sizeof(msg));
			}

			if (ret == 0) {
				logw("==== disconnect ====\n");
				break;
			} else if (ret < 0) {
				perror("recv");
				break;
			} else {
				logd("Receive Server MSG[%d]:%s.\n", strlen(msg), msg);
				if (addr) { /* UDP */
					ret = sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *) addr, sock_len);
				} else { /* TCP */
					ret = writen(sock_fd, msg, strlen(msg));
				}
				if (ret == strlen(msg)) {
					logd("Send [%s] Length [%d] OK\n", msg, ret);
				} else {
					logd("Send [%s] Length [%d] Error: %s\n", msg, ret, strerror(errno));
					break;
				}
			}
		}
	}

	logd("%s exit\n", __FUNCTION__);
	return;
}

static int socket_client_tcp(void)
{
	int fd_client = -1;
	int ret = -1;
	struct sockaddr_in addr_srv;

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);

	do {
		if ((fd_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd = fd_client;

		if (inet_pton(AF_INET, net_socket.srv_ip, &addr_srv.sin_addr) < 0 ) {
			perror("inet_pton");
			break;
		}
		if (connect(fd_client, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr)) < 0) {
			perror("connect");
			break;
		}
		logd("Socket fd [%d], Host: %s:%d, Connect OK\n",
			fd_client, net_socket.srv_ip, net_socket.port);

		/* Fixme: send once, need send twice if pcap have not capture it */
		ret = writen(fd_client, net_socket.send_msg, strlen(net_socket.send_msg));
		if (ret == strlen(net_socket.send_msg)) {
			logd("Send [%s] Length [%d] OK\n", net_socket.send_msg, ret);
		} else {
			logd("Send [%s] Length [%d] Error: %s\n", net_socket.send_msg, ret, strerror(errno));
		}

		if (net_socket.is_add_resume) {
			transfer_data(G_pm_pipefd[0], fd_client, NULL);
		}
		close_fd(&fd_client);
		net_socket.fd = -1;
		logi("TCP Socket Dissconnect\n");
	} while (G_running && net_socket.is_reconn);

	logw("%s exit\n", __FUNCTION__);
	return ret;
}

static int socket_client_udp(void)
{
	int fd_client = -1;
	int ret = -1;
	struct sockaddr_in addr_srv;

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);

	do {
		if ((fd_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd = fd_client;
		if (inet_pton(AF_INET, net_socket.srv_ip, &addr_srv.sin_addr) < 0 ) {
			perror("inet_pton");
			break;
		}
		logi("Socket fd [%d], Port [%d] OK\n", fd_client, net_socket.port);

		/* Send */
		ret = sendto(fd_client, net_socket.send_msg, strlen(net_socket.send_msg),
			0, (struct sockaddr *)&addr_srv, sizeof(addr_srv));
		if (ret == strlen(net_socket.send_msg)) {
			logd("Send [%s] Length [%d] OK\n", net_socket.send_msg, ret);
		} else {
			logd("Send [%s] Length [%d] Error: %s\n", net_socket.send_msg, ret, strerror(errno));
		}

		if (net_socket.is_add_resume) {
			transfer_data(G_pm_pipefd[0], fd_client, &addr_srv);
		}
		close_fd(&fd_client);
		net_socket.fd = -1;
		logi("UDP Socket Dissconnect\n");
	} while (G_running && net_socket.is_reconn);

	logw("%s exit\n", __FUNCTION__);
	return ret;
}

void process_tcp(const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	int i = 0;
	tcpka_conn_t *p_tcpka = NULL;
	char tcp_wowl_pattern[FILTER_MAXSIZE];
	char dst_mac[DATA_MAXSIZE];

	struct ip *iphdr = NULL;		/* IPv4 Header */
	struct tcphdr *tcphdr = NULL;	/* TCP Header  */
	unsigned int *tsval = NULL;		/* Time Stamp (optional) */
	unsigned int *tsvalR = NULL;	/* Time Stamp Reply (optional) */

	iphdr = (struct ip *)(packet + 14);
	tcphdr = (struct tcphdr *)(packet + 14 + 20);
	tsval = (unsigned int *)(packet + 58);
	tsvalR = tsval + 1;

	/* 1. Filter PSH flag.
	 *  2. Filter total size is (66 + payload length) byte for when send tcp data.
	 *  3. Filter payload memory.
	 */
	if (tcphdr->psh && pkthdr->len == (net_socket.send_msg_len + TCP_PL_OFFSET) &&
		(!memcmp((void *)(packet + TCP_PL_OFFSET), (void *)net_socket.send_msg,
			net_socket.send_msg_len))) {

		memset(dst_mac, 0, sizeof(dst_mac));
		sprintf(dst_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
			packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);

		logv("	FLags: PSH [%d]\n", tcphdr->psh);
		logv("	DST MAC: %s\n", dst_mac);
		logv("	DST IP: %s\n", inet_ntoa(iphdr->ip_dst));
		logv("	SRC IP: %s\n", inet_ntoa(iphdr->ip_src));
		logv("	SRC PORT: %d\n", ntohs(tcphdr->th_sport));
		logv("	DST PORT: %d\n", ntohs(tcphdr->th_dport));
		logv("	ID: %d\n", ntohs(iphdr->ip_id));
		logv("	SEQ: %u\n", ntohl(tcphdr->th_seq));
		logv("	ACK: %u\n", ntohl(tcphdr->th_ack));
		logv("	Win: %d\n", ntohs(tcphdr->th_win));
		logv("	TS val: %u\n", ntohl(*tsval));
		logv("	TS valR: %u\n", ntohl(*tsvalR));
		logv("	Data (Hex): ");
		for ( i = TCP_PL_OFFSET; i < pkthdr->len; i++ ) {
			logv("%02x ", packet[i]);
		}
		logv("\n");

		/* Fill WiFi FW to KeepAlive and Wakeup Pattern, Session ID: 1 */
		if (net_socket.interval > 0) {
			do {
				int tcpka_len = sizeof (tcpka_conn_t) + pkthdr->len - TCP_PL_OFFSET;
				if (p_tcpka == NULL) {
					p_tcpka = malloc(tcpka_len);
				}
				if (p_tcpka == NULL) {
					perror("malloc tcpka");
					break;
				} else {
					memset(p_tcpka, 0, tcpka_len);
				}
				p_tcpka->sess_id = 1;
				p_tcpka->dst_mac = (struct ether_addr)* ether_aton(dst_mac);
				memcpy(&p_tcpka->src_ip, &iphdr->ip_src.s_addr, IPV4_ADDR_LEN);
				memcpy(&p_tcpka->dst_ip, &iphdr->ip_dst.s_addr, IPV4_ADDR_LEN);
				p_tcpka->ipid = ntohs(iphdr->ip_id);
				p_tcpka->srcport = ntohs(tcphdr->th_sport);
				p_tcpka->dstport = ntohs(tcphdr->th_dport);
				p_tcpka->seq = ntohl(tcphdr->th_seq);
				p_tcpka->ack = ntohl(tcphdr->th_ack);
				p_tcpka->tcpwin = ntohs(tcphdr->th_win);
				p_tcpka->tsval = ntohl(*tsval);
				p_tcpka->tsecr = ntohl(*tsvalR);
				if (net_socket.is_add_tcp_payload) {
					p_tcpka->len = pkthdr->len - TCP_PL_OFFSET;
					p_tcpka->ka_payload_len = pkthdr->len - TCP_PL_OFFSET;
				} else {
					p_tcpka->len = 0;
					p_tcpka->ka_payload_len = 0;
				}
				memcpy ((void *)&p_tcpka->ka_payload[0], (void *)&net_socket.send_msg[0],
					net_socket.send_msg_len);

				G_tcp_info_old.tcpka_sess_ipid = p_tcpka->ipid;
				G_tcp_info_old.tcpka_sess_seq = p_tcpka->seq;
				G_tcp_info_old.tcpka_sess_ack = p_tcpka->ack;
				logi("Suspend TCP info: ipid[%u], seq[%u], ack[%u]\n",
					G_tcp_info_old.tcpka_sess_ipid,
					G_tcp_info_old.tcpka_sess_seq,
					G_tcp_info_old.tcpka_sess_ack);

				if (net_socket.is_enable_host_cipher) {
					wl_set_get_host_cipher(cipher_str1, sizeof(cipher_str1), 1);
				}
				wl_tcpka_conn_add(p_tcpka);
				wl_tcpka_conn_enable(1, 1, net_socket.interval, 1, 8);
				if (p_tcpka) {
					free(p_tcpka);
					p_tcpka = NULL;
				}
			}while (0);
		}

		if (net_socket.is_enable_pattern) {
			memset(tcp_wowl_pattern, 0, sizeof(tcp_wowl_pattern));
			snprintf(tcp_wowl_pattern, sizeof(tcp_wowl_pattern),
				"0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"\
				"00000000000000000000000000000000000000000000000000000000%s",
				packet[30], packet[31], packet[32], packet[33],
				packet[26], packet[27], packet[28], packet[29],
				packet[36], packet[37], packet[34], packet[35], WAKE_PAYLOAD_HEX);

			wl_wowl_pattern(26, "0xff0f0000000f", tcp_wowl_pattern);
			wl_wowl(0x00016);
			wl_wowl_activate(1);
		}
		wifi_enter_sleep();

		logi("Set WiFi %s %s Done\n",
			net_socket.interval ? "TCP KeepAlive" : "",
			net_socket.is_enable_pattern ? "Wowl Pattern" : "");

		if (net_socket.is_add_resume) {
			suspend_and_resume(net_socket.fd, G_pm_pipefd[1]);
		}
	}
}

void process_udp(const struct pcap_pkthdr* pkthdr, const u_char * packet)
{
	char udp_wowl_pattern[FILTER_MAXSIZE];
	char pattern_type[64]= {0};

	if (!memcmp((void *)(packet + UDP_PL_OFFSET), (void *)net_socket.send_msg,
		net_socket.send_msg_len)) {

		if (net_socket.interval > 0) {
			wl_mkeep_alive(1, net_socket.interval, pkthdr->len, packet);
		}
		memset(udp_wowl_pattern, 0, sizeof(udp_wowl_pattern));
		snprintf(udp_wowl_pattern, sizeof(udp_wowl_pattern),
			"0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			packet[30], packet[31], packet[32], packet[33],
			packet[26], packet[27], packet[28], packet[29],
			packet[36], packet[37], packet[34], packet[35]);

		if (net_socket.is_enable_pattern) {
			wl_wowl_pattern(26, "0xff0f", udp_wowl_pattern);
			wl_wowl(0x00016);
			wl_wowl_activate(1);
			snprintf(pattern_type, sizeof(pattern_type), "Wowl Pattern");
		} else if (net_socket.is_enable_filter) {
			wl_pkt_filter_add(200, 26, "0xffffffffffffffffffffffff", udp_wowl_pattern);
			wl_pkt_filter_enable(200, 1);
			snprintf(pattern_type, sizeof(pattern_type), "Pkt Filter");
		}
		wifi_enter_sleep();

		logi("Set %s %s Done\n", net_socket.interval ? "UDP KeepAlive" : "", pattern_type);

		if (net_socket.is_add_resume) {
			suspend_and_resume(net_socket.fd, G_pm_pipefd[1]);
		}
	}
}

void process_packet(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char * packet)
{
	int i = 0;
	int *counter = (int *)arg;

	logd("Packet[%d] Count: %d.\n", pkthdr->len, ++(*counter));
	logv("Packet DUMP:\n");
	for(i = 0; i < pkthdr->len; ++i) {
		logv(" %02x", packet[i]);
		if( (i + 1) % 16 == 0 ) {
			logv("\n");
		}
	}
	logv("\n\n");

	if (net_socket.is_tcp) {
		process_tcp(pkthdr, packet);
	} else {
		process_udp(pkthdr, packet);
	}
	return;
}

void start_pcap(void)
{
	int count = 0;
	if (pcap_loop(pG_pcap, -1, process_packet, (u_char *)&count) == -1) {
		loge("ERROR pcap_loop: %s\n", pcap_geterr(pG_pcap) );
	}
	pcap_close(pG_pcap);
	pG_pcap = NULL;
}

static void default_param(void)
{
	net_socket.fd = -1;
	net_socket.is_session = 0;
	net_socket.is_close = 0;
	net_socket.is_tcp = 1;
	net_socket.is_add_tcp_payload = 1;
	net_socket.is_enable_pattern = 0;
	net_socket.is_enable_filter= 0;
	net_socket.is_enable_host_cipher = 0;
	net_socket.is_enable_host_sleep = 0;
	net_socket.is_enable_suspend = 0;
	net_socket.is_poll = 0;
	net_socket.is_add_resume = 0;
	net_socket.is_reconn= 0;

	net_socket.is_sys_exec = 0;
	net_socket.suspend_pm = 1;
	net_socket.wifi_chip_id = WIFI_BCM43340;
	net_socket.log_level = LOG_INFO;
	net_socket.port = DEFAULT_PORT;
	net_socket.interval = DEFAULT_INTERVAL;
	net_socket.dtim_interval = DEFAULT_DTIM_INTERVAL;

	memset(net_socket.iface, 0, sizeof(net_socket.iface));
	memset(net_socket.srv_ip, 0, sizeof(net_socket.srv_ip));
	strncpy(net_socket.srv_ip, DEFAULT_HOST, sizeof(net_socket.srv_ip));
	strncpy(net_socket.send_msg, DEFAULT_MSG, sizeof(net_socket.send_msg));
	net_socket.send_msg_len = strlen(net_socket.send_msg);
}

static void show_wifi_chip()
{
	char wifi_chip_str[32];

	switch(net_socket.wifi_chip_id) {
	case WIFI_BCM43340:
		sprintf(wifi_chip_str, "BCM43340");
		break;
	case WIFI_BCM43438:
		sprintf(wifi_chip_str, "BCM43438/BCM43455");
		break;
	default:
		sprintf(wifi_chip_str, "Unknown");
		break;
	}
	logd("WiFi Chip ID: %s\n", wifi_chip_str);
}

static void sigstop()
{
	G_running = 0;
	close_fd(&net_socket.fd);

	if (pG_pcap) {
		pcap_close(pG_pcap);
		pG_pcap = NULL;
	}
	pthread_cancel(G_pcap_tid);
}

int main(int argc, char *argv[])
{
	int ret = -1;

	char *device = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	char sniffer_filter[FILTER_MAXSIZE];
	struct bpf_program filter;

	printf("wowl version : %s, compile time : %s .\n", WOWL_VERSION, __TIME__);

	if (argc < 2) {
		usage();
		return -1;
	}

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	default_param();
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	set_log_level(net_socket.log_level);
	show_wifi_chip();
	brcm_ioc_wowl_init(net_socket.iface, net_socket.wifi_chip_id, net_socket.is_sys_exec);

	if (pipe(G_pm_pipefd) < 0) {
		perror("pipe");
		return -1;
	}
	logd("Pipe.0[%d], Pipe.1[%d]\n", G_pm_pipefd[0], G_pm_pipefd[1]);

	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	if (net_socket.iface[0] != '\0') {
		device = net_socket.iface;
	} else {
		if ((device = pcap_lookupdev(errbuf)) == NULL) {
			loge("ERROR pcap_lookupdev: %s\n", errbuf);
			return -1;
		}
	}
	logd("Libpcap iface: %s\n", device);
	if (net_socket.is_tcp) {
		snprintf(sniffer_filter, sizeof(sniffer_filter), "tcp dst port %d", net_socket.port);
	} else {
		snprintf(sniffer_filter, sizeof(sniffer_filter), "udp dst port %d", net_socket.port);
	}
	if ((pG_pcap = pcap_open_live(device, MAXBYTES2CAPTURE, 1,
		FILTER_MAXSIZE, errbuf)) == NULL) {
		loge("ERROR pcap_live : %s\n", errbuf);
		return -1;
	}
	pcap_compile(pG_pcap, &filter, sniffer_filter, 1, 0);
	pcap_setfilter(pG_pcap, &filter);
	logi("Set Sniffer Network Filter [%s]\n", sniffer_filter);
	ret = pthread_create(&G_pcap_tid, NULL, (void *)start_pcap, NULL);
	if (ret != 0) {
		perror("Create pthread pcap");
		return -1;
	}

	if (net_socket.is_session) {
		if (net_socket.is_tcp) {
			ret = socket_client_tcp();
		} else {
			ret = socket_client_udp();
		}
	}

	/* Exit Process */
	if (G_pcap_tid) {
		pthread_join(G_pcap_tid, NULL);
		G_pcap_tid = -1;
	}
	if (pG_pcap) {
		pcap_close(pG_pcap);
		pG_pcap = NULL;
	}
	close_fd(&G_pm_pipefd[0]);
	close_fd(&G_pm_pipefd[1]);
	close_fd(&net_socket.fd);

	return ret;
}
