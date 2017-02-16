
/*
 * Net_clnt.c
 *
 * History:
 *	2011/9/14 - [Tao Wu] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <memory.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <assert.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#define NO_ARG		0
#define HAS_ARG		1

#define SRVPORT 6666 			/* server listen port 	*/
#define SRVPORT_2 6667 			/* server listen port 	*/

#define MAXDATASIZE 1452		/* TCP one  package size*/

static char host_name[32]="127.0.0.1";
static char host_name_2[32]="127.0.0.1";

static int fd_socket_clnt;
static int fd_socket_clnt_2;

static char *buf_send;
static char *buf_recv;

static unsigned int sendMB = 15;		/* Units: MB	*/
static unsigned int recvMB = 1;

static unsigned int window = 1024 * 1024 * 1;		/*	1MB */

static struct sockaddr_in addr_srv;
static struct sockaddr_in addr_srv_2;
static struct hostent *host;
static struct hostent *host_2;

static int tid_recv;			/*tid of receive date	*/

static struct option long_options[] =
{
		{"Help", 					NO_ARG, 	NULL, 	'h'},
		{"Connect to <host>",		HAS_ARG, 	NULL, 	'c'},
		{"Send window times", 		HAS_ARG, 	NULL, 	's'},
		{"Recv window times", 		HAS_ARG, 	NULL, 	'r'},
		{"Window buffer size",		HAS_ARG, 	NULL, 	'w'},

		{0, 0, 0, 0},
};

struct hint_s
{
	const char *arg;
	const char *str;
};

static const char *short_options = "hc:s:r:w:";

static const struct hint_s hint[] = {
		{"","\t\t\tPrint command parm"},
		{"","\t\tRun in client mode "},
		{"","\t\tSend buffer size = <times> x <windows>MB"},
		{"","\t\tRecv buffer size = <times> x <windows>MB"},
		{"","\t\tUnits: MB"},
};

void usage(void)
{
	int i;
	printf("\nUsage: net_clnt -c [HOST] -s [TIMES] -r [TIMES] -w [WINDOW]\n");

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
}

/* option function	*/
int init_opt(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	float fvalue = 1;

	opterr = 0;

	while ( (ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1){
		switch(ch){
			case 'c':
				strcpy(host_name, optarg);
				strcpy(host_name_2, optarg);
				break;
			case 's':
				sendMB = atol(optarg);
				break;
			case 'r':
				recvMB = atol(optarg);
				break;
			case 'w':
				window = atol(optarg) * 1024 * 1024;
				break;
			case 'h':
				usage();
				exit(0);
				break;
			default:
				printf("unknown option found %c or option requires an argument \n", ch);
				return -1;
				break;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------
 * Attempts to reads n bytes from a socket.
 * Returns number actually read, or -1 on error.
 * If number read < inLen then we reached EOF.
 *
 * from Stevens, 1998, section 3.9
 * ------------------------------------------------------------------- */

ssize_t readn( int inSock, void *outBuf, size_t inLen ) {
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

ssize_t writen( int inSock, const void *inBuf, size_t inLen ) {
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

/* show message */
void show_mesg(void)
{
	printf("TCP window size [%u]MB\n", window/(1024 *1024) );
	printf("Send file size [%u]MB\n", sendMB);
	printf("Recv file size [%u]MB\n", recvMB);
}

/*	timer excutive function	*/
void data_send(int signo)
{
	int i, nsize;
	//printf("Send start>>>>>>>>>>>>>>>>\n");
	if (signo == SIGALRM ){
		if (sendMB > 0){
			for ( i = 0 ; i < sendMB; i ++){
				if (( nsize = writen(fd_socket_clnt, buf_send, window)) != window){
					printf("Send ERROR. Actually send %d/%u\n", nsize, window);
					exit(1);
				}else{
					//printf("Send [%d]window OK.\n", i);
				}
			}
			printf("Send OK.\n");
		}
	}
	//printf("exit send.\n");
}

/*  open timer for frequency send file	*/
int init_timer(int sec, int usec)
{
	struct itimerval value, ovalue;

	signal(SIGALRM, data_send);
	value.it_value.tv_sec = sec;
	value.it_value.tv_usec = usec;
	value.it_interval = value.it_value;
	setitimer(ITIMER_REAL, &value, &ovalue);

	printf("[%d]sec, [%d]usec.\n", sec, usec);
	return 0;
}

/*	receive from client and */
void *receive_data(void *arg)
{
	int i, nsize;

	printf("Enter receive loop...\n");
	while(1){
		if (recvMB > 0){
			for (i = 0 ; i < recvMB; i++){
				if ( (nsize = readn(fd_socket_clnt_2, buf_recv, window)) == window){
					//printf("Receive [%d]window OK.\n", i);
				}
				else if( nsize > 0 )
					printf("Receive ERROR. Actullay receive %d/%u\n",nsize, window);
				else {
					printf("Server Aborted. Client Exit.\n");
					exit(1);
				}
			}
			printf("Receive OK.\n");
		}
	}
}

/* init socket and create pthread for loop receive data from server	*/
int init_socket_clnt(void)
{
	if((host=gethostbyname(host_name)) == NULL){
		perror("gethostbyname");
		exit(1);
	}
	if((fd_socket_clnt=socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	addr_srv.sin_family=AF_INET;
	addr_srv.sin_port=htons(SRVPORT);
	addr_srv.sin_addr=*((struct in_addr *)host->h_addr);
	bzero(&(addr_srv.sin_zero),8);
	if(connect(fd_socket_clnt, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr)) == -1){
		perror("connect");
		exit(1);
	}
	if (init_timer(1, 0) != 0){
		printf("init timer erro\n");
		return -1;
	}

	printf("Connect to [%s]. Init socket[%d] succeed.\n", host_name, fd_socket_clnt);
	return 0;
}

/* init socket and create pthread for loop receive data from server	*/
int init_socket_clnt_2(void)
{
	if((host_2=gethostbyname(host_name_2)) == NULL){
		perror("gethostbyname");
		exit(1);
	}
	if((fd_socket_clnt_2=socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	addr_srv_2.sin_family=AF_INET;
	addr_srv_2.sin_port=htons(SRVPORT_2);
	addr_srv_2.sin_addr=*((struct in_addr *)host_2->h_addr);
	bzero(&(addr_srv_2.sin_zero),8);
	if(connect(fd_socket_clnt_2, (struct sockaddr *)&addr_srv_2, sizeof(struct sockaddr)) == -1){
		perror("connect");
		exit(1);
	}
	if(pthread_create((pthread_t *)&tid_recv, NULL, receive_data, NULL) != 0){
		perror("pthread_create");
		exit(1);
	}

	printf("Connect to [%s]. Init socket_2[%d] succeed.\n", host_name, fd_socket_clnt_2);
	return 0;
}
/*	stop terminal function	*/
static void sigstop()
{
	close(fd_socket_clnt);
	close(fd_socket_clnt_2);
	free(buf_recv);
	free(buf_send);

	printf("Good Game!\n");
	exit(0);
}

int main(int argc, char **argv)
{
	/* register signal handler for Ctrl+C,	Ctrl+'\'  ,  and "kill" sys cmd */
	signal(SIGINT,sigstop);
	signal(SIGQUIT,sigstop);
	signal(SIGTERM,sigstop);

	if (init_opt(argc, argv) != 0){
		printf("Init option error\n");
		return -1;
	}

	buf_send = (char *)malloc(window);
	buf_recv = (char *)malloc(window);

	show_mesg();

	if (init_socket_clnt() != 0){
		printf("init client socket erro\n");
		return -1;
	}
/*
	if (init_socket_clnt_2() != 0){
		printf("init client socket_2 erro\n");
		return -1;
	}
*/


	while(1)
		pause();

	return 0;
}


