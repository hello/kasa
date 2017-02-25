
#define __USE_BSD		/* Using BSD IP header 	*/
#include <netinet/ip.h>	/* Internet Protocol 		*/
#define __FAVOR_BSD	/* Using BSD TCP header	*/
#include <netinet/tcp.h>	/* Transmission Control Protocol	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include  <time.h>

typedef struct _iphdr
{
    unsigned char h_verlen;
    unsigned char tos;
    unsigned short total_len;
    unsigned short ident;
    unsigned short frag_and_flags;
    unsigned char ttl;
    unsigned char proto;
    unsigned short checksum;
    unsigned int sourceIP;
    unsigned int destIP;
}IP_HEADER;

typedef struct _tcphdr
{
    unsigned short th_sport;
    unsigned short th_dport;
    unsigned int th_seq;
    unsigned int th_ack;
    unsigned char th_lenres;
    unsigned char th_flag;
    unsigned short th_win;
    unsigned short th_sum;
    unsigned short th_urp;
}TCP_HEADER;

//#define CONFIG_CURL 1

#if CONFIG_CURL
#include <curl/curl.h>
#endif

#define APP_VERSION	"1.0.1"

#define DEFAULT_HOST	"127.0.0.1"
#define DEFAULT_MSG	"*"
#define DEFAULT_REPLY	"Reply Done"
#define DEFAULT_QUIT	"q"

#define STR_MAC		"MAC"
#define STR_STATUS	"STATUS"

#define POST_PATH 	"?object=device&action=update&backdoor=1"

#define SERVER_IP 			"10.0.0.4"
#define SERVER_PORT 		(80)
#define DEVICE_IP			"10.0.0.100"
#define DEVICE_PORT			(2048)
#define DEVICE_MAC			"00:00:00:00:00:00"

#define SUSPEND_INPUT		"suspend"
#define SUSPEND_CMD		"echo mem > /sys/power/state"

#define RESPONSE_FILE 		"response.xml"

#define DEFAULT_PORT		(7877)
#define DATA_MAXSIZE		(32)
#define PAYLOAD_MAXSIZE 	(256)

#define MAXCONN			(10)

#define Conn(x, y) x##y
#define ToString(x) #x

#ifndef DIVIDING_LINE
#define DIVIDING_LINE() do {	\
	printf("-------------------------------------\n"); \
} while(0)
#endif

#ifndef LOGT
#define LOGT(str, arg...) do {	\
	time_t timep;		\
	time (&timep);	\
	printf(str" [ %s", ##arg, ctime(&timep)); \
} while(0)
#endif

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

typedef struct timer_info_s{
	int timeout_seconds;
	int timer_id_valid;
	timer_t timer_id;
	volatile int disconnect;
} timer_info_t;

typedef struct net_socket_s
{
	int is_close;
	int is_server;
	int is_tcp;
	int is_tcpka;
	int is_reply;
	int is_auto_reply;
	int is_update;
	int is_resume;
	int port;
	int fd_net;
	int fd_raw;
	int fd_clnt;

	timer_info_t client_timer;

	char srv_ip[DATA_MAXSIZE];
	char client_ip[DATA_MAXSIZE];

	char send_msg[PAYLOAD_MAXSIZE];
	char recv_msg[PAYLOAD_MAXSIZE];
} net_socket_t;

typedef struct thread_data_s
{
	int fd;
	struct sockaddr_in addr;
}thread_data_t;

typedef struct auto_reply_s
{
	unsigned int interval_sencods;
	int wakeup_times;
	int unexpt_times;
	int close_times;
}auto_reply_t;

typedef struct device_s
{
	int port;
	int status;

	char ip_addr[DATA_MAXSIZE];
	char mac_addr[DATA_MAXSIZE];
	char uid[DATA_MAXSIZE];
} device_t;

#if CONFIG_CURL
typedef struct {
	CURL 		*p_curl;
} Context;
#endif

static net_socket_t net_socket;
static auto_reply_t auto_reply;
static auto_reply_t auto_curr;
static thread_data_t timer_data;

static int is_suspend = 0;
static int running = 1;

#if CONFIG_CURL
static Context ctx;
#endif

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "tusc:kp:m:ra:qed";

static struct option long_options[] = {
	{"server",	NO_ARG, 0, 's'},
	{"client",		HAS_ARG, 0, 'c'},
	{"tcp", 		NO_ARG, 0, 't'},
	{"udp",		NO_ARG, 0,'u'},
	{"keepalive", 		NO_ARG, 0, 'k'},
	{"port",		HAS_ARG, 0, 'p'},
	{"msg",		HAS_ARG, 0, 'm'},
	{"reply",		NO_ARG, 0, 'r'},
	{"auto",		HAS_ARG, 0, 'a'},
	{"resume",	NO_ARG, 0, 'q'},
	{"close",	NO_ARG, 0, 'e'},
	{"update",	NO_ARG, 0, 'd'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\trun in server mode"},
	{"host", "run in client mode, connecting to [host]"},
	{"",	"\tuse TCP"},
	{"",	"\tuse UDP"},
	{"", "\trun tcp keepalive, open raw socket. Use Root right"},
	{"port", "server port to listen on/connect to, default is [7877]"},
	{"",	"\tclient send messages to server, default is [" DEFAULT_MSG "]\n"\
		"\t\t\t type ["DEFAULT_QUIT"] to quit process\n"\
		"\t\t\t type ["SUSPEND_INPUT"] to suspend board\n"\
		"\t\tserver used to suspend client"},
	{"",	"\treply message"},
	{"",	"\t server: auto reply client in timer <timer sencods>/<wake times>/<close times>/<unexpected times>\n"
		"\t\t\t client: auto connect to server when dissconnected."},
	{"",	"\treset IP/TCP header after resume, seq/ack "},
	{"",	"\tclose fd after exit, it is disable by default"},
	{"",	"\tupdate database via curl"},
};

static void usage(void)
{
	int i;
	char *itself = "net_socket";
	printf("Usage:\nThis program is used for send/receive net socket\n");
	printf("net_socket version : %s, compile time : %s .\n", APP_VERSION, __TIME__);
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

	printf("[TCP-KeepAlive]\tServer:\t# %s -s -k -p 7877 -r -e\n", itself);
	printf("\tClient:\t# %s -k -c 127.0.0.1 -p 7877 -m Hi -r -e\n\n", itself);

	printf("[TCP]\tServer:\t# %s -s -p 7877 -r -e\n", itself);
	printf("\tClient:\t# %s -c 127.0.0.1 -p 7877 -m Hi -r -e [-q]\n\n", itself);

	printf("Autoreply Server:# %s -s -p 7877 -a 18/10/5/1 -e\n", itself);
	printf("Autoconn Client:# %s -c 127.0.0.1 -p 7877 -m Hi -r -a 1 -e \n\n", itself);

	printf("[UDP]\tServer:\t# %s -s -u -p 7877 -r -e\n", itself);
	printf("\tClient:\t# %s -c 127.0.0.1 -u -p 7877 -m Hi -r\n\n", itself);

	printf("Autoreply Server:# %s -s -p 7877 -a 18/10/1/0 -e\n", itself);
	printf("Autoconn Client:# %s -c 127.0.0.1 -p 7877 -m Hi -r -a 1 -e \n\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;
	char auto_reply_str[256];
	char *cut_str = NULL;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 't':
			net_socket.is_tcp = 1;
			break;
		case 'k':
			net_socket.is_tcpka = 1;
			break;
		case 'u':
			net_socket.is_tcp = 0;
			break;
		case 's':
			net_socket.is_server = 1;
			break;
		case 'c':
			net_socket.is_server = 0;
			strncpy(net_socket.srv_ip, optarg, sizeof(net_socket.srv_ip));
			break;
		case 'p':
			value = atoi(optarg);
			if ((value < 0) || (value > 65535)) {
				printf("Please set port in %d ~ %d.\n",
					0, 65535);
				return -1;
			}
			net_socket.port = value;
			break;
		case 'm':
			strncpy(net_socket.send_msg, optarg, sizeof(net_socket.send_msg));
			break;
		case 'r':
			net_socket.is_reply = 1;
			break;
		case 'a':
			strncpy(auto_reply_str, optarg, sizeof(auto_reply_str));
			if (strlen(auto_reply_str) >= 7) {
				cut_str = strtok(auto_reply_str, "/");
				auto_reply.interval_sencods = atoi(cut_str);
				cut_str = strtok(NULL, "/");
				auto_reply.wakeup_times = atoi(cut_str);
				cut_str = strtok(NULL, "/");
				auto_reply.unexpt_times= atoi(cut_str);
				cut_str = strtok(NULL, "/");
				auto_reply.close_times = atoi(cut_str);
				printf("Auto Reply Param: interval[%u], wakeup[%u], close[%u], unexpected[%u]\n",
					auto_reply.interval_sencods, auto_reply.wakeup_times,
					auto_reply.unexpt_times, auto_reply.close_times);
			}
			net_socket.is_auto_reply = 1;
			break;
		case 'q':
			net_socket.is_resume = 1;
			break;
		case 'e':
			net_socket.is_close = 1;
			break;
		case 'd':
			net_socket.is_update = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static void close_fd(int *fd)
{
	if (net_socket.is_close && (*fd >= 0)) {
		close(*fd);
		printf("Close fd [%d].\n", *fd);
		*fd = -1;
	}
}

#if CONFIG_CURL
/* Start of curl update database */

static int ctx_init(Context *p_ctx)
{
	p_ctx->p_curl = curl_easy_init();
	return ;
}

static void ctx_deinit(Context *p_ctx)
{
	curl_easy_cleanup(p_ctx->p_curl);
}

// for receive response
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int len = size * nmemb;
	int written = len;
	FILE *fp = NULL;
	if (access((char*) stream, 0) == -1) {
		fp = fopen((char*) stream, "wb");
	} else {
		fp = fopen((char*) stream, "ab");
	}
	if (fp) {
		fwrite(ptr, size, nmemb, fp);
	}
	// printf("%s\n",ptr);
	fclose(fp);
	return written;
}

static void device_udpate(Context *p_ctx, const char *server_ip, int server_port,
	const char *mac, const char *ip, int port, int status)
{
	char buf[512];
	char *p_str = buf;

	int len = sprintf(p_str, "http://%s:%d%s", server_ip, server_port, POST_PATH);
	p_str = p_str + len;

	if (mac) {
		len = sprintf(p_str, "&mac=%s", mac);
		p_str += len;
	}

	if (ip) {
		len = sprintf(p_str, "&ip=%s", ip);
		p_str += len;
	}

	if (port > 0) {
		len = sprintf(p_str, "&port=%d", port);
		p_str += len;
	}

	if (status >= 0) {
		len = sprintf(p_str, "&status=%d", status);
		p_str += len;
	}

	curl_easy_setopt(p_ctx->p_curl, CURLOPT_URL, buf);
	curl_easy_setopt(p_ctx->p_curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(p_ctx->p_curl, CURLOPT_WRITEDATA, RESPONSE_FILE);

	CURLcode res = curl_easy_perform(p_ctx->p_curl);
	if (res == CURLE_OK) {
		printf("Curl OK: %s \n", buf);
	} else {
		printf("Curl ERROR: %s\n", buf);
	}
}
/* END of curl update database */
#endif

static int parse_msg_device(const char *msg, device_t *dev)
{
	#define MAC_LEN 17

	int ret = 0;
	char *p_str = NULL;
	char data[8];

	p_str = strstr(msg, STR_MAC);
	if (p_str) {
		strncpy(dev->mac_addr, p_str + strlen(STR_MAC), MAC_LEN);
	} else {
		ret = -1;
	}

	p_str = strstr(msg, STR_STATUS);
	if (p_str) {
		memset(data, 0, sizeof(data));
		strncpy(data, p_str + strlen(STR_STATUS), sizeof(data)-1);
		dev->status = atoi(data);
	} else {
		ret = -1;
	}

	return ret;
}

/* -------------------------------------------------------------------
 * Attempts to reads n bytes from a socket.
 * Returns number actually read, or -1 on error.
 * If number read < inLen then we reached EOF.
 *
 * from Stevens, 1998, section 3.9
 * ------------------------------------------------------------------- */

static ssize_t readn( int inSock, void *outBuf, size_t inLen )
{
    size_t  nleft;
    ssize_t nread;
    char *ptr;

    assert( inSock >= 0 );
    assert( outBuf != NULL );
    assert( inLen > 0 );

    ptr   = (char*) outBuf;
    nleft = inLen;

    while ( nleft > 0 ) {
        nread = read( inSock, ptr, nleft );
        if ( nread < 0 ) {
            if ( errno == EINTR )
                nread = 0;  /* interupted, call read again */
            else
                return -1;  /* error */
        } else if ( nread == 0 )
            break;        /* EOF */

        nleft -= nread;
        ptr   += nread;
    }

    return(inLen - nleft);
} /* end readn */

/* -------------------------------------------------------------------
 * Attempts to write  n bytes to a socket.
 * returns number actually written, or -1 on error.
 * number written is always inLen if there is not an error.
 *
 * from Stevens, 1998, section 3.9
 * ------------------------------------------------------------------- */

static ssize_t writen( int inSock, const void *inBuf, size_t inLen )
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    assert( inSock >= 0 );
    assert( inBuf != NULL );
    assert( inLen > 0 );

    ptr   = (char*) inBuf;
    nleft = inLen;

    while ( nleft > 0 ) {
        nwritten = write( inSock, ptr, nleft );
        if ( nwritten <= 0 ) {
            if ( errno == EINTR )
                nwritten = 0; /* interupted, call write again */
            else
                return -1;    /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }

    return inLen;
} /* end writen */

static void auto_reply_action(int signo)
{
	int ret = -1;
	int len = -1;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	static unsigned int auto_susp_cnt = 0;

	char msg[PAYLOAD_MAXSIZE];

	if (signo != SIGALRM) {
		return ;
	}
	if (!running) {
		return;
	}
	if (timer_data.fd < 0) {
		printf("Auto reply socket fd [%d] is invalid\n", timer_data.fd);
		return ;
	}

	DIVIDING_LINE();
	memset(msg, 0, sizeof(msg));
	if (is_suspend) {
		strncpy(msg, net_socket.send_msg, sizeof(msg));
		printf("Auto Action (suspend) count [%u]\n", auto_susp_cnt++);
	} else {
		printf("Auto Action : wakeup [%u], unexpected [%u], close [%u]\n",
			auto_curr.wakeup_times, auto_curr.unexpt_times, auto_curr.close_times);
		if (auto_curr.wakeup_times > 0) {
			strncpy(msg, "wake", sizeof(msg));
			printf("Auto Action (wake) count [%u]\n", auto_curr.wakeup_times--);
		} else if (auto_curr.unexpt_times > 0) {
			strncpy(msg, "unexpt", sizeof(msg));
			printf("Auto Action (unexpt) count [%u]\n", auto_curr.unexpt_times--);
		} else if (auto_curr.close_times > 0) {
			close_fd(&timer_data.fd);
			printf("Auto Action (close) count [%u]\n", auto_curr.close_times--);
			return;
		}
		if ((auto_curr.wakeup_times == 0) &&
			(auto_curr.unexpt_times == 0) &&
			(auto_curr.close_times == 0)) {
			auto_curr = auto_reply;
			printf("Auto Action Reload Timers\n");
		}
	}
	is_suspend = ~is_suspend;
	len = strlen(msg);
	if (len < 1) {
		return;
	}
	if (net_socket.is_tcp) {
		ret = writen(timer_data.fd, msg, strlen(msg));
	} else {
		ret = sendto(timer_data.fd, msg, strlen(msg),
			0, (struct sockaddr *)&(timer_data.addr), sock_len);
	}
	if (ret == strlen(msg)) {
		printf("Auto Send [%s] Length [%d] OK\n", msg, ret);
	} else {
		perror("Auto Send");
	}
}

static int set_timer(int sec, int usec)
{
	int ret = -1;
	struct itimerval value, ovalue;

	is_suspend = 0;
	auto_curr = auto_reply;
	signal(SIGALRM, auto_reply_action);
	value.it_value.tv_sec = sec;
	value.it_value.tv_usec = usec;
	value.it_interval = value.it_value;
	ret = setitimer(ITIMER_REAL, &value, &ovalue);
	if (ret < 0) {
		perror("setitimer");
	} else {
		printf("Set Timer: [%d] sec, [%d] usec done.\n", sec, usec);
	}
	return ret;
}

/* Created by Server */
static void read_console_reply_send(const void *args)
{
	int ret = -1;
	int len = -1;
	char msg[PAYLOAD_MAXSIZE];
	socklen_t sock_len = sizeof(struct sockaddr_in);
	thread_data_t *pData = NULL;

	printf("Create Reply Thread\n");
	pData = (thread_data_t *)args;
	if (pData->fd < 0) {
		printf("Reply send fd [%d] is invalid\n", pData->fd);
		return ;
	}
	memset(msg, 0, sizeof(msg));
	while (running && (fgets(msg, sizeof(msg), stdin) != NULL)) {
		len = strlen(msg);
		if (len < 2) {
			printf("Continue\n");
			continue;
		}
		msg[len-1] = '\0';

		if (strncmp(msg, DEFAULT_QUIT, sizeof(msg)) == 0) {
			printf("Quit\n");
			running = 0;
			break;
		} else if (strncmp(msg, SUSPEND_INPUT, sizeof(msg)) == 0) {
			printf("Go to suspend mode\n");
			printf("...\n");
			system(SUSPEND_CMD);
			//break;
		}

		if (net_socket.is_tcp) {
			ret = writen(pData->fd, msg, strlen(msg));
		} else {
			ret = sendto(pData->fd, msg, strlen(msg), 0,
				(struct sockaddr *)&(pData->addr), sock_len);
		}

		if (ret == strlen(msg)) {
			printf("Reply Send [%s] Length [%d] OK\n", msg, ret);
		} else {
			perror("Reply Send");
		}
		memset(msg, 0, sizeof(msg));
	}
	printf("Exit Reply Thread\n");
}

/* Created by Client */
static void read_socket( const void *args )
{
	int ret = -1;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	thread_data_t *pData = (thread_data_t *)args;
	char msg[PAYLOAD_MAXSIZE];

	printf("Create Read Thread\n");
	while (running) {
		memset(msg, 0, sizeof(msg));
		if (net_socket.is_tcp) {
			ret = read(pData->fd, msg, sizeof(msg));
		} else {
			ret = recvfrom(pData->fd, msg, sizeof(msg), 0,
				(struct sockaddr *)&(pData->addr), &sock_len);
		}
		if (ret == 0) {
			printf("==== disconnect ====\n");
			break;
		} else if (ret < 0) {
			perror("recv");
			break;
		} else{
			printf("Receive Server MSG [%lu]: %s.\n", strlen(msg), msg);
		}
	}
	printf("Exit Read Thread\n");
}

static int setsockopt_seq(int fd)
{
	int ret = -1;
	char msg[DATA_MAXSIZE];
	int reset_domain = 0;
	int seq_num = 0;

	printf("Please insert seq_num >\n");
	if (fgets(msg, sizeof(msg), stdin) != NULL) {
		reset_domain = 1;
		seq_num = atoi(msg);

		printf("Replace domain [%d], seq_num [%d]\n", reset_domain, seq_num);
		ret = setsockopt(fd, IPPROTO_TCP, TCP_REPAIR, &reset_domain, sizeof(reset_domain));
		if (ret) {
			perror("setsockopt TCP_REPAIR ");
			return ret;
		}
		reset_domain = 1;
		ret = setsockopt(fd, IPPROTO_TCP, TCP_REPAIR_QUEUE, &reset_domain, sizeof(reset_domain));
		if (ret) {
			perror("setsockopt TCP_REPAIR_QUEUE ");
			return ret;
		}
		ret = setsockopt(fd, IPPROTO_TCP, TCP_QUEUE_SEQ, &seq_num, sizeof(seq_num));
		if (ret) {
			perror("setsockopt TCP_QUEUE_SEQ");
			return ret;
		}
		printf("Set seq[%d] Done\n", seq_num);
	}
	return ret;
}

static int net_socket_server_tcpka(void)
{
	int fd_raw = -1;
	int fd_srv = -1;
	int fd_client = -1;
	int max_fd = -1;
	int new_fd = -1;
	int yes = 1;
	int ret = -1;
	ssize_t recv_len = 0;
	pthread_t tid = 0;

	fd_set fdset;
	char buf[PAYLOAD_MAXSIZE];
	socklen_t sock_len = -1;
	struct timeval tv;
	struct sockaddr_in addr_srv;
	struct sockaddr_in addr_client;
	thread_data_t thread_data;
	//timer_info_t timer_tcpka;
	device_t device;

	signal(SIGPIPE, SIG_IGN);
	memset(&addr_srv, 0, sizeof(addr_srv));
	memset(&addr_client, 0, sizeof(addr_client));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	sock_len = sizeof(addr_srv);

	/* capture ip datagram without ethernet header */
	if ((fd_raw = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP))) < 0){
		perror("raw socket");
		goto err_exit;
	}
	if ((fd_srv = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		goto err_exit;
	}
	if (setsockopt(fd_srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		perror("setsockopt");
		goto err_exit;
	}
	if (bind(fd_srv, (struct sockaddr *)&addr_srv, sizeof(addr_srv)) < 0) {
		perror("bind");
		goto err_exit;
	}
	if (listen(fd_srv, MAXCONN) < 0) {
		perror("listen");
		goto err_exit;
	}
	net_socket.fd_net = fd_srv;
	net_socket.fd_raw = fd_raw;

	printf("TCP-KeepAlive: Raw [%d], Socket [%d], Port [%d], Bind OK, Accept ...\n",
		fd_raw, fd_srv, net_socket.port);
	DIVIDING_LINE();

	while (running) {
		FD_ZERO(&fdset);
		FD_SET(fd_raw, &fdset);
		FD_SET(fd_srv, &fdset);
		max_fd = (fd_raw > fd_srv) ? fd_raw : fd_srv;

		/* Add Client fd */
		if (fd_client > 0) {
			max_fd = (max_fd > fd_client) ? max_fd: fd_client;
			FD_SET(fd_client, &fdset);
		}

		tv.tv_sec = 30;
		tv.tv_usec = 0;
		ret = select(max_fd + 1, &fdset, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			running = 0;
			break;
		} else if (ret == 0) {
			printf("Timeout\n");
			continue;
		}

		if (FD_ISSET(fd_srv, &fdset)) {
			new_fd = accept(fd_srv, (struct sockaddr *)&addr_client, &sock_len);
			if (new_fd == 0) {
				printf("accept zero\n");
			} else if (ret < 0) {
				perror("accept");
				//running = 0;
				break;
			} else {
				if (fd_client != new_fd) {
					if (net_socket.is_reply && (tid > 0) && (fd_client > 0)) {
						printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
						pthread_cancel(tid);
						tid = 0;
						printf("Accpet new client [%d], close old client [%d]\n", new_fd, fd_client);
						close_fd(&fd_client);
					}
				}
				fd_client = new_fd;
				net_socket.fd_clnt = fd_client;

				strncpy(device.mac_addr, DEVICE_MAC, sizeof(device.mac_addr));
				strncpy(device.ip_addr, inet_ntoa(addr_client.sin_addr), sizeof(device.ip_addr));
				device.port = ntohs(addr_client.sin_port);

				LOGT("Receive Client[%d], Src[%s # %s:%d], Status:%d, MSG[%d].",
					fd_client, device.mac_addr, device.ip_addr, device.port, device.status, ret);

				/* Reply send */
				if (net_socket.is_reply) {
					thread_data.fd = fd_client;
					ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
						(void *)&thread_data);
					if (ret < 0) {
						perror("pthread_create");
					}
				}

				/* Auto Reply send */
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
					timer_data.fd = fd_client;
					ret = set_timer(auto_reply.interval_sencods, 0);
					if (ret != 0) {
						printf("Set timer error\n");
					}
				}

				#if 0
				/* Create timer */
				memset(&timer_tcpka, 0, sizeof(timer_tcpka));
				timer_tcpka.timer_id_valid = 0;
				timer_tcpka.timeout_seconds = 15;
				timer_tcpka.disconnect = 1;
				if (create_posix_timer(&timer_tcpka) < 0) {
					printf("Failed to create timer for this client, close it, TODO\n");
					memset(&timer_tcpka, 0, sizeof(timer_tcpka));
				}
				#endif

			}
		}

		if (FD_ISSET(fd_client, &fdset)) {
			memset(buf, 0, sizeof(buf));
			recv_len = recv(fd_client, buf, sizeof(buf), 0);
			if (recv_len <= 0) {
				if (recv_len < 0) {
					perror("client recv");
				}
				/* Destory dead Client send data thread*/
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
					set_timer(0, 0);
				}
				printf("=== Client [%d] recv zero, disconnect ===\n", fd_client);
				//stop_posix_timer(&timer_tcpka);
				close_fd(&fd_client);
			} else {
				//start_posix_timer(&timer_tcpka);
				printf("Receive Client MSG[%d]: %s.\n", (int)recv_len, buf);
			}
		}

		if (FD_ISSET(fd_raw, &fdset)) {
			recv_len = recv(fd_raw, buf, sizeof(buf), 0);
			if (recv_len == 0) {
				printf("raw recv zero\n");
				break;
			} else if (recv_len < 0) {
				perror("raw recv");
				//running = 0;
				break;
			} else {
				if (fd_client <= 0) {
					continue;
				}
				IP_HEADER *ip = ( IP_HEADER *)(buf);
				size_t iplen =	(ip->h_verlen&0x0f)*4;
				if (ip->proto == IPPROTO_TCP) {
					TCP_HEADER *tcp = (TCP_HEADER *)(buf +iplen);
					size_t tcplen = ((tcp->th_lenres >> 4)& 0x0f) * 4;
					if ( (recv_len <= iplen + tcplen + 1) && ( (tcp->th_flag & 0x3F) == 0x10/*ACK*/)
						&& ( addr_client.sin_addr.s_addr == ip->sourceIP)
						&& ( addr_client.sin_port == tcp->th_sport)) {
						//start_posix_timer(&timer_tcpka);
						LOGT("Raw client [%d] KeepAlive received", fd_client);
					} else {
						//printf("FIN/RST\n");
					}
				}
			}
		}
	}

err_exit:
	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}
	close_fd(&fd_client);
	close_fd(&fd_srv);
	close_fd(&fd_raw);
	//stop_posix_timer(&timer_tcpka);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static int net_socket_server_tcp(void)
{
	int ret = -1;
	int len = 0;
	int fd_srv = -1;
	int fd_client = -1;
	pthread_t tid = 0;
	socklen_t sock_len = 0;
	thread_data_t thread_data;
	struct sockaddr_in addr_srv, addr_client;
	char msg[PAYLOAD_MAXSIZE];
	device_t device;

	if ((fd_srv = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	net_socket.fd_net = fd_srv;
	memset(&addr_srv, 0, sizeof(addr_srv));
	memset(&addr_client, 0, sizeof(addr_client));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	sock_len = sizeof(addr_srv);

	if (bind(fd_srv, (struct sockaddr *)&addr_srv, sock_len) < 0) {
		perror("bind");
		close_fd(&fd_srv);
		return -1;
	}
	if (listen(fd_srv, MAXCONN) < 0 ) {
		perror("listen");
		close_fd(&fd_srv);
		return -1;
	}
	printf("TCP: Socket [%d], Port [%d], Bind OK. Accept ...\n", fd_srv, net_socket.port);
	DIVIDING_LINE();

	do {
		if ((fd_client = accept(fd_srv, (struct sockaddr *)&addr_client, &sock_len)) < 0) {
			perror("accept");
			break;
		}
		net_socket.fd_clnt = fd_client;

		/* Reply send */
		if (net_socket.is_reply) {
			thread_data.fd = fd_client;
			ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
				(void *)&thread_data);
			if (ret < 0) {
				perror("pthread_create");
			}
		}

		/* Auto Reply send */
		if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
			timer_data.fd = fd_client;
			ret = set_timer(auto_reply.interval_sencods, 0);
			if (ret != 0) {
				printf("Set timer error\n");
			}
		}

		while (running && fd_client > 0) {
			/* Receive */
			memset(msg, 0, sizeof(msg));
			ret = read(fd_client, msg, sizeof(msg));
			if (ret <= 0) {
				if (ret < 0) {
					perror("read");
				}
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
					set_timer(0, 0);
				}
				close_fd(&fd_client);
				printf("==== Client disconnect. Accept ... ====\n");
				break;
			} else {
				strncpy(device.mac_addr, DEVICE_MAC, sizeof(device.mac_addr));
				strncpy(device.ip_addr, inet_ntoa(addr_client.sin_addr), sizeof(device.ip_addr));
				device.port = ntohs(addr_client.sin_port);

				LOGT("Receive Client[%d], Src[%s # %s:%d], Status:%d, MSG[%d]: %s.",
					fd_client, device.mac_addr, device.ip_addr, device.port, device.status,
					ret, msg);

				#if CONFIG_CURL
				/* Update database */
				if (net_socket.is_update) {
					parse_msg_device(msg, &device);
					device_udpate(&ctx, SERVER_IP, SERVER_PORT, device.mac_addr,
						device.ip_addr, device.port, device.status);
				}
				#endif
			}
		}
	}while (running);

	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}
	close_fd(&fd_client);
	close_fd(&fd_srv);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static int net_socket_server_udp(void)
{
	int fd_srv = -1;
	int ret = -1;
	int reply_update = 0;
	int auto_reply_update = 0;
	pthread_t tid = 0;
	socklen_t sock_len = -1;
	thread_data_t thread_data;
	char msg[PAYLOAD_MAXSIZE];
	struct sockaddr_in addr_srv, addr_client, addr_client_old;
	device_t device;

	memset(&addr_srv, 0, sizeof(addr_srv));
	memset(&addr_client, 0, sizeof(addr_client));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	sock_len = sizeof(addr_srv);

	do {
		if ((fd_srv = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd_net = fd_srv;
		if (bind(fd_srv, (struct sockaddr *)&addr_srv, sock_len) < 0 ) {
			perror("bind");
			close_fd(&fd_srv);
			break;
		}
		printf("UDP: Socket [%d], Port [%d], Bind OK, Recvfrom ...\n", fd_srv,net_socket.port);

		while (running && fd_srv > 0) {
			/* Receive */
			memset(msg, 0, sizeof(msg));
			ret = recvfrom(fd_srv, msg, sizeof(msg), 0,
				(struct sockaddr *)&addr_client, &sock_len);

			if (ntohs(addr_client.sin_port) != ntohs(addr_client_old.sin_port)) {
				reply_update = 1;
				auto_reply_update = 1;
				addr_client_old = addr_client;
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
					set_timer(0, 0);
				}
				DIVIDING_LINE();
				printf("New Device\n");
			}

			if (ret <= 0) {
				if (ret < 0) {
					perror("recvfrom");
				}
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0) {
					set_timer(0, 0);
				}
				close_fd(&fd_srv);
				printf("==== Client disconnect, Recvfrom ... ====\n");
				break;
			} else {
				strncpy(device.mac_addr, DEVICE_MAC, sizeof(device.mac_addr));
				strncpy(device.ip_addr, inet_ntoa(addr_client.sin_addr), sizeof(device.ip_addr));
				device.port = ntohs(addr_client.sin_port);
				parse_msg_device(msg, &device);

				LOGT("Receive Src[%s # %s:%d], Status:%d, MSG[%d]: %s.",
					device.mac_addr, device.ip_addr, device.port, device.status,
					ret, msg);

				#if CONFIG_CURL
				/* Update database */
				if (net_socket.is_update) {
					device_udpate(&ctx, SERVER_IP, SERVER_PORT, device.mac_addr,
						device.ip_addr, device.port, device.status);
				}
				#endif

				/* Reply send */
				if (net_socket.is_reply && reply_update) {
					thread_data.fd = fd_srv;
					thread_data.addr = addr_client;
					ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
						(void *)&thread_data);
					if (ret < 0) {
						perror("pthread_create");
					}
					reply_update = 0;
				}

				/* Auto reply send */
				if (net_socket.is_auto_reply && auto_reply.interval_sencods > 0 &&
					auto_reply_update) {
					timer_data.fd = fd_srv;
					timer_data.addr = addr_client;
					ret = set_timer(auto_reply.interval_sencods, 0);
					if (ret != 0) {
						printf("Set timer error\n");
					}
					auto_reply_update = 0;
				}
			}
		}
	}while (running);

	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}

	close_fd(&fd_srv);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static int net_socket_client_tcpka(void)
{
	int ret = -1;
	int len = 0;
	int fd_client = -1;
	pthread_t tid = 0;
	thread_data_t thread_data;
	struct sockaddr_in addr_srv;
	char msg[PAYLOAD_MAXSIZE];

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);

	do {
		if ((fd_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd_clnt = fd_client;
		if (inet_pton(AF_INET, net_socket.srv_ip, &addr_srv.sin_addr) < 0 ) {
			perror("inet_pton");
			break;
		}

		/* TCP KeepAlive */
		int keepalive = 1;
		int keepidle = 15;
		int keepinterval = 5;
		int keepcount = 3;
		setsockopt(fd_client, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
		setsockopt(fd_client, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle));
		setsockopt(fd_client, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
		setsockopt(fd_client, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));

		if (connect(fd_client, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr)) < 0) {
			perror("connect");
			break;
		}
		printf("TCP-KeepAlive: Socket [%d], Port [%d], Connect OK\n", fd_client, net_socket.port);

		/* Send once */
		ret = writen(fd_client, net_socket.send_msg, strlen(net_socket.send_msg));
		if (ret == strlen(net_socket.send_msg)) {
			printf("Send [%s] Length [%d] OK\n", net_socket.send_msg, ret);
		} else {
			perror("Send Failed");
		}

		/* Create receive thread */
		if (net_socket.is_reply) {
			thread_data.fd = fd_client;
			ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
				(void *)&thread_data);
			if (ret < 0) {
				perror("pthread_create");
			}
		}

		while (running && fd_client > 0) {
			/* Receive */
			memset(msg, 0, sizeof(msg));
			ret = read(fd_client, msg, sizeof(msg));
			if (ret <= 0) {
				if (ret < 0) {
					perror("read");
				}
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				close_fd(&fd_client);
				printf("==== Client disconnect. %s ====\n",
					net_socket.is_auto_reply ? "Connect ..." : "Exit.");
				break;
			} else {
				printf("Receive Server MSG[%lu]: %s.\n", strlen(msg), msg);
			}
		}
	}while(running && net_socket.is_auto_reply);

	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}

	close_fd(&fd_client);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static int net_socket_client_tcp(void)
{
	int ret = -1;
	int len = 0;
	int fd_client = -1;
	pthread_t tid = 0;
	thread_data_t thread_data;
	struct sockaddr_in addr_srv;
	char msg[PAYLOAD_MAXSIZE];

	memset(&addr_srv,0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);

	do {
		if ((fd_client = socket(AF_INET, SOCK_STREAM,
			(net_socket.is_resume ) ? IPPROTO_TCP : 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd_clnt = fd_client;
		if (inet_pton(AF_INET, net_socket.srv_ip, &addr_srv.sin_addr) < 0 ) {
			perror("inet_pton");
			break;
		}
		if (connect(fd_client, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr)) < 0) {
			perror("connect");
			break;
		}
		printf("TCP: Socket [%d], Port [%d], Connect OK\n", fd_client, net_socket.port);

		/* Send once */
		ret = writen(fd_client, net_socket.send_msg, strlen(net_socket.send_msg));
		if (ret == strlen(net_socket.send_msg)) {
			printf("Send [%s] Length [%d] OK\n", net_socket.send_msg, ret);
		} else {
			perror("Send Failed");
		}

		/* Replace seq, ack after resume */
		if (net_socket.is_resume) {
			setsockopt_seq(fd_client);
		}

		/* Create reply thread */
		if (net_socket.is_reply) {
			thread_data.fd = fd_client;
			ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
				(void *)&thread_data);
			if (ret < 0) {
				perror("pthread_create");
			}
		}

		while (running && fd_client > 0) {
			/* Receive */
			memset(msg, 0, sizeof(msg));
			ret = read(fd_client, msg, sizeof(msg));
			if (ret <= 0) {
				if (ret < 0) {
					perror("read");
				}
				if (net_socket.is_reply && (tid > 0)) {
					printf("Cancel pthread [%lu]\n", (unsigned long int)tid);
					pthread_cancel(tid);
					tid = 0;
				}
				close_fd(&fd_client);
				printf("==== Client disconnect. %s ====\n",
					net_socket.is_auto_reply ? "Connect ..." : "Exit.");
				break;
			} else {
				printf("Receive Server MSG[%lu]: %s.\n", strlen(msg), msg);
			}
		}
	}while (running && net_socket.is_auto_reply);

	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}

	close_fd(&fd_client);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static int net_socket_client_udp(void)
{
	int ret = -1;
	int len = -1;
	int fd_client = -1;
	pthread_t tid = 0;
	socklen_t sock_len = -1;
	thread_data_t thread_data;
	struct sockaddr_in addr_srv;
	char msg[PAYLOAD_MAXSIZE];

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(net_socket.port);
	sock_len = sizeof(addr_srv);

	do {
		if ((fd_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket");
			break;
		}
		net_socket.fd_clnt = fd_client;
		if (inet_pton(AF_INET, net_socket.srv_ip, &addr_srv.sin_addr) < 0 ) {
			perror("inet_pton");
			break;
		}
		printf("UDP: Socket [%d], Port [%d] OK\n", fd_client, net_socket.port);

		/* Send once */
		ret = sendto(fd_client, net_socket.send_msg, strlen(net_socket.send_msg), 0,
				(struct sockaddr *)&(addr_srv), sock_len);
		if (ret == strlen(net_socket.send_msg)) {
			printf("Send [%s] Length [%d] OK\n", net_socket.send_msg, ret);
		} else {
			perror("Send Failed");
		}

		/* Create Receive thread */
		if (net_socket.is_reply) {
			thread_data.fd = fd_client;
			thread_data.addr = addr_srv;
			ret = pthread_create(&tid, NULL, (void *)read_console_reply_send,
				(void *)&thread_data);
			if (ret < 0) {
				perror("pthread_create");
			}
		}

		while (running && fd_client > 0) {
			/* Read */
			memset(msg, 0 , sizeof(msg));
			ret = recvfrom(fd_client, msg, sizeof(msg), 0,
				(struct sockaddr *)&(addr_srv), &sock_len);
			if (ret <= 0) {
				if (ret < 0) {
					perror("recvfrom");
				}
				close_fd(&fd_client);
				printf("==== Client disconnect. %s ====\n",
					net_socket.is_auto_reply ? "Connect ..." : "Exit.");
			} else {
				printf("Receive Server MSG[%lu]: %s.\n", strlen(msg), msg);
			}
		}
	} while (running && net_socket.is_auto_reply);

	if (net_socket.is_reply && (tid > 0)) {
		pthread_cancel(tid);
		pthread_join(tid, NULL);
		tid = 0;
	}

	close_fd(&fd_client);
	printf("%s exit\n", __FUNCTION__);
	return ret;
}

static void default_param(void)
{
	/* Init net_socket */
	net_socket.is_close = 0;
	net_socket.is_server = 1;
	net_socket.is_tcp = 1;
	net_socket.is_tcpka = 0;
	net_socket.is_reply = 0;
	net_socket.is_auto_reply = 0;
	net_socket.is_resume = 0;
	net_socket.is_update = 0;
	net_socket.fd_net = -1;
	net_socket.fd_raw = -1;
	net_socket.port = DEFAULT_PORT;

	timer_data.fd = -1;
	auto_reply.interval_sencods = 0;
	auto_reply.wakeup_times = 0;
	auto_reply.unexpt_times = 0;
	auto_reply.close_times = 0;

	memset(net_socket.recv_msg, 0, sizeof(net_socket.recv_msg));
	memset(net_socket.send_msg, 0, sizeof(net_socket.send_msg));
	memset(net_socket.srv_ip, 0, sizeof(net_socket.srv_ip));
	memset(net_socket.client_ip, 0, sizeof(net_socket.client_ip));

	strncpy(net_socket.srv_ip, DEFAULT_HOST, sizeof(net_socket.srv_ip));
	strncpy(net_socket.send_msg, DEFAULT_MSG, sizeof(net_socket.send_msg));

#if CONFIG_CURL
	/* Init curl */
	ctx.p_curl = NULL;
#endif
}

static void sigstop()
{
	running = 0;
	close_fd(&net_socket.fd_clnt);
	close_fd(&net_socket.fd_net);
	close_fd(&net_socket.fd_raw);

#if CONFIG_CURL
	if (ctx.p_curl != NULL) {
		ctx_deinit(&ctx);
	}
#endif
	printf("GG!\n");
}

int main(int argc, char ** argv)
{
	int rval = 0;

	/* register signal handler for Ctrl+C,	Ctrl+'\'  ,  and "kill" sys cmd */
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}
	default_param();
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

#if CONFIG_CURL
	if (net_socket.is_server && net_socket.is_update) {
		rval = ctx_init(&ctx);
		if (rval < 0) {
			printf("curl init failed\n");
			return -1;
		}
	}
#endif

	if (net_socket.is_server) {
		if (net_socket.is_tcp) {
			if (net_socket.is_tcpka) {
				rval = net_socket_server_tcpka();
			} else {
				rval = net_socket_server_tcp();
			}
		} else {
			rval = net_socket_server_udp();
		}
	} else {
		if (net_socket.is_tcp) {
			if (net_socket.is_tcpka) {
				rval = net_socket_client_tcpka();
			} else {
				rval = net_socket_client_tcp();
			}
		} else {
			rval = net_socket_client_udp();
		}
	}

#if CONFIG_CURL
	if (ctx.p_curl != NULL) {
		ctx_deinit(&ctx);
	}
#endif

	if (rval < 0) {
		printf("Failed to create net socket\n");
	}
	printf("Exit Process\n");

	return rval;
}
