
/*
 * Net_srv.c
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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <assert.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#define NO_ARG		0
#define HAS_ARG		1

#define SRVPORT 6666 			/*	server listen port 		*/
#define SRVPORT_2 6667 			/*	server listen port 2		*/

#define MAXCONN 10 				/* 	max connected number	*/

#define MAXDATASIZE 1452		/* 	TCP one  package size		*/
#define SENDBUFFER	1024		/*	default buffer size is 1KB	*/

static char file_recv[64]= "/tmp/sda1/data.recv";
static char file_send[64]= "/tmp/sda1";

static int fd_recv;				/*	write file fd	*/
static int fd_send[10];				/*	read file fd	*/

static char *buf_recv;
static char *buf_send;

static unsigned int sendMB = 1;		/* send <x> MB	*/
static unsigned int recvMB = 15;		/* receive <x> MB	*/

static unsigned int window = 1024 * 1024 * 1;		/*	1MB 	*/

static struct sockaddr_in addr_srv;		/* local host address	*/
static struct sockaddr_in addr_clnt;		/* remote host address	*/

//static struct sockaddr_in addr_srv_2;		/* local host address	*/
//static struct sockaddr_in addr_clnt_2;		/* remote host address	*/

static int fd_socket;
static int fd_socket_clnt;

//static int fd_socket_2;
//static int fd_socket_clnt_2;

static int tid_recv;			/*tid of receive date and write file*/
//static int tid_send;		/*tid of read file and send data	*/

static socklen_t len_send;
static socklen_t len_recv;

static unsigned int recvNum =0;

//static socklen_t len_send_2;
//static socklen_t len_recv_2;

static struct option long_options[] =
{
		{"Help", 					NO_ARG, 	NULL, 	'h'},
		{"The file to read",			HAS_ARG, 	NULL, 	'i'},
		{"The file to write",		HAS_ARG, 	NULL, 	'o'},
		{"Send window times", 		HAS_ARG, 	NULL, 	's'},
		{"Receive window times", 	HAS_ARG, 	NULL, 	'r'},
		{"Window buffer size",		HAS_ARG, 	NULL, 	'w'},

		{0, 0, 0, 0},
};

struct hint_s
{
	const char *arg;
	const char *str;
};

static const char *short_options = "hi:o:s:r:w:";

static const struct hint_s hint[] = {
		{"","\t\t\tPrint command parm"},
		{"","\t\tRead file & send file"},
		{"","\t\tReceive date & write file"},
		{"","\t\tSend file size = <times> x <window> MB"},
		{"","\t\tRecv file size = <times> x <window> MB"},
		{"","\t\tUnits: MB"},
};

void usage(void)
{
	int i;
	printf("\nUsage: net_srv -o [SENDFILE] -i [RECVFILE] -s [TIMES] -r [TIMES] -w [WINDOW]\n");

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

	opterr = 0;

	while ( (ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1){
		switch(ch){
			case 'o':
				strcpy(file_send, optarg);
				break;
			case 'i':
				strcpy(file_recv, optarg);
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
 * [Modify] Tao Wu 2011/09/14
 * ------------------------------------------------------------------- */

ssize_t readn_to_file( int inSock, void *outBuf, size_t inLen , int outFile) {
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

		/* Add start */
		if ( write(outFile, ptr, nread) < 0 ){		/* recive and wirte */
			break;
		}
		/* Add end */

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
	int windMB = window/(1024 * 1024);
	printf("TCP window size [%u]MB\n",windMB );
	printf("Read & Send file size [%u]MB\n", sendMB * windMB);
	printf("Recv & Write file size [%u]MB\n", recvMB * windMB);
}

/*	create received file	*/
int create_recv_file(const char *fileName)
{
	if ((fd_recv = open(fileName, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0 ){
		perror("open recv file");
		exit(1);
	}

	printf("Create received file [%s] succeed.\n", fileName);
	return 0;
}

/*	create a <sizeMB>  size file with given file name 	*/
int create_send_file(const char *file, unsigned int sizeMB )
{
	char buf[SENDBUFFER];
	char filename[32];
	int i,j, windMB = window/(1024 * 1024);
	unsigned int size = sizeMB * 1024 ;
	if (chdir(file) < 0){
		perror("chdir");
		return -1;
	}
	for (j = 0 ; j < 10 ; j ++){			/* create 10 file for random read*/
		sprintf(filename, "%d", j);
		if ((fd_send[j] = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0){
			perror("open send file");
			exit(1);
		}
		for (i = 0; i < size; i++){
			if ((write( fd_send[j], buf, SENDBUFFER)) < 0 ){
				perror("write send file");
				exit(1);
			}
		}
		printf("Create file [%s] with [%d]MB succeed.\n", filename, sizeMB * windMB);
	}

	return 0;

}

/*	timer excutive function	*/
void read_data_send(int signo)
{
	int i,j;

	if (signo == SIGALRM){
		if( sendMB > 0){
			srand( (int)  time(0) );
			j = (int) (10.0 * rand() / (RAND_MAX + 1.0));
			printf("Read [%d]... ",j);
			for (i = 0; i < sendMB; i ++){
				if ( (read(fd_send[j], buf_send, window) ) < 0 ){
					perror("read");
					exit(1);
				}
				/*
				if ( (nsize = writen(fd_socket_clnt_2, buf_send, window) ) != window){
					printf("Send ERROR. Actually send %d/%u\n",nsize, window);
					exit(1);
				}
				*/
			}
			printf("OK. \n");
		}
	}
}

/*  open timer for frequency send file	*/
int init_timer(int sec, int usec)
{
	struct itimerval value, ovalue;

	signal(SIGALRM, read_data_send);
	value.it_value.tv_sec = sec;
	value.it_value.tv_usec = usec;
	value.it_interval = value.it_value;
	setitimer(ITIMER_REAL, &value, &ovalue);

	printf("[%d]sec, [%d]usec.\n", sec, usec);
	return 0;
}

/*
void *open_timer_send_data(void *arg)
{
	printf("Wait client_2...\n");
	if ((fd_socket_clnt_2 = accept(fd_socket_2, (struct sockaddr *)&addr_clnt_2, &len_recv_2) ) == -1 ){
		perror("accept");
		exit(1);
	}
	if (init_timer(1, 0) != 0 ){
		printf("init timer error\n");
		exit(1);
	}
	printf("Accept client_2 OK. Start send data.\n");
}
*/

/*	receive from client and */
void *receive_data_write(void *arg)
{
	int i, nsize;

	printf("Wait client...\n");
	if((fd_socket_clnt = accept(fd_socket, (struct sockaddr *)&addr_clnt, &len_recv) ) == -1 ){
		perror("accept");
		exit(1);
	}

	//printf("receive from %s\n", inet_ntoa(addr_clnt.sin_addr));

	printf("Enter receive loop...\n");

	while(1){
		if (recvMB > 0){
			for (i = 0; i < recvMB; i++){
				if ( ( nsize = readn_to_file(fd_socket_clnt, buf_recv, window, fd_recv)) == window){
					//printf("Recieve [%d]windos OK\n", i);
				}
				else if( nsize > 0)
					printf("Receive ERROR. Actually receive %d/%u\n", nsize, window);
				else{			// client aborted, restart accpet
					printf("Client aborted, waite client...\n");
					if ((fd_socket_clnt = accept(fd_socket, (struct sockaddr *)&addr_clnt, &len_recv) ) == -1 ){
						perror("accept");
						exit(1);
					}
					printf("Restart Accept OK.\n");
				}
			}
			printf("Receive [%u] OK.\n", recvNum++);

		}
	}
}

/* init socket and create pthread for loop receive data from client	*/
int init_socket_srv(void)
{
	len_send = sizeof(struct sockaddr_in);
	len_recv = sizeof(struct sockaddr);

	if ((fd_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}
	addr_srv.sin_family=AF_INET;
	addr_srv.sin_port=htons(SRVPORT);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	bzero(&(addr_srv.sin_zero),8);
	if (bind(fd_socket, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr)) == -1 ) {
		perror("bind");
		exit(1);
	}
	if (listen(fd_socket, MAXCONN) == -1) {
		perror("listen");
		exit(1);
	}
	if (pthread_create((pthread_t *)&tid_recv, NULL, receive_data_write, NULL) != 0){
		perror("pthread_create");
		exit(1);
	}

	//pthread_join (tid_recv, NULL);

	printf("Init socket[%d] succeed.\n", fd_socket);
	return 0 ;
}

/* init socket and create pthread for loop receive data from client	*/
/*
int init_socket_srv_2(void)
{
	len_send_2 = sizeof(struct sockaddr_in);
	len_recv_2 = sizeof(struct sockaddr);

	if ((fd_socket_2 = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}
	addr_srv_2.sin_family=AF_INET;
	addr_srv_2.sin_port=htons(SRVPORT_2);
	addr_srv_2.sin_addr.s_addr = INADDR_ANY;
	bzero(&(addr_srv_2.sin_zero),8);
	if (bind(fd_socket_2, (struct sockaddr *)&addr_srv_2, sizeof(struct sockaddr)) == -1 ){
		perror("bind");
		exit(1);
	}
	if (listen(fd_socket_2, MAXCONN) == -1){
		perror("listen");
		exit(1);
	}
	if (pthread_create((pthread_t *)&tid_send, NULL, open_timer_send_data, NULL) != 0){
		perror("pthread_create");
		exit(1);
	}

	printf("Init socket_2[%d] succeed.\n", fd_socket_2);
	return 0 ;
}
*/

/*	stop terminal function	*/
static void sigstop()
{
	int i;
	close(fd_socket);
	close(fd_socket_clnt);
//	close(fd_socket_2);
//	close(fd_socket_clnt_2);
	close(fd_recv);

	for (i = 0 ; i < 10 ; i++){
		close(fd_send[i]);
	}

	free(buf_recv);
	free(buf_send);

	printf("Good Game!\n");
	exit(0);
}

int loop_parm()
{
	char buffer[256];
	 if (read (STDIN_FILENO, buffer, sizeof (buffer)) < 0)
	 	return -1;
	 else if ( buffer[0] == 'q' ){
	 	printf("Quit!\n");
	 }
	 return 0;
}

int main(int argc , char **argv)
{
	/* register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd	*/
	signal(SIGINT,sigstop);
	signal(SIGQUIT,sigstop);
	signal(SIGTERM,sigstop);

	if (init_opt(argc, argv) != 0){
		printf("Init option error\n");
		return -1;
	}

	buf_send = (char *)malloc(window);
	if ( buf_send == NULL){
		printf("malloc buf_send error\n");
		return -1;
	}

	buf_recv = (char *)malloc(window);
	if ( buf_recv == NULL){
		printf("malloc buf_recv error\n");
		return -1;
	}

	show_mesg();

	if (create_send_file(file_send, sendMB) != 0 ){
		printf("Create send file error\n");
		return -1;
	}

	if (create_recv_file(file_recv) != 0 ){
		printf("Create recv file error\n");
		return -1;
	}

	if ( init_socket_srv()  != 0){
		printf("Init socket service error\n");
		return -1;
	}

	if (init_timer(1, 0) != 0 ){
		printf("init timer error\n");
		exit(1);
	}


	/**** server not need send data
	if ( init_socket_srv_2()  != 0){
		printf("Init socket_2 service error\n");
		return -1;
	}
	*/

	while ( 1 ){
		if( !(loop_parm( argc, argv) ) ){
			sigstop();
			return 0;
		}
	}

	return 0;
}

