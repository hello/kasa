/*/*
* Copyright (c) 2010-2011 Atheros Communications Inc.
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

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <time.h>
#include "artserialagent.h"
#include "libtcmd.h"

#include <termios.h>

static int sid, cid, aid;
static char ar6kifname[32];

unsigned int readLength = 0, readLength_back = 0;
unsigned char  line[LINE_ARRAY_SIZE], line_back[LINE_ARRAY_SIZE];
int cmdId,cmdLen;

//#define DEBUG_PRINT       0

#if 1
#define  ART_PORT_COM
#undef   ART_PORT_SOCKET
#else
#undef   ART_PORT_COM
#define  ART_PORT_SOCKET
#endif
unsigned char calcCrc(unsigned char *buf, unsigned int length)
{
	unsigned char crc = 0;
	unsigned short i;

	/* calculate check sum */
	for (i = 0; i < length; i++) {
		crc ^= buf[i];
	}

	return crc;
}

void callback_rx(void *buf, int len)
{
    readLength = len;
    memcpy(line, buf, len);
}

void disable_tty(void)
{
	system("cp /etc/inittab_no /etc/inittab");
	system("init q");
	/*copy back in case of premature exit*/
	system("cp /etc/inittab_yes /etc/inittab");
}

void enable_tty(void)
{
	system("cp /etc/inittab_yes /etc/inittab");
	system("init q");
	system("init s");
}


static int initInterface(char *ifname)
{
    int err = 0;
    err = tcmd_tx_init(ifname, callback_rx);
    return err;
}

static int wmiSend(unsigned char *cmdBuf, unsigned int len, unsigned int totalLen, unsigned char version, bool resp)
{
    TC_CMDS tCmd;
    int err=0;

    memset(&tCmd,0,sizeof(tCmd));

    tCmd.hdr.testCmdId = TC_CMDS_ID;
    tCmd.hdr.u.parm.length = totalLen;
    tCmd.hdr.u.parm.version = version;
    tCmd.hdr.u.parm.bufLen = len;   // less than 256
    memcpy((void*)tCmd.buf, (void*)cmdBuf, len);

    if ((err = tcmd_tx((char*)&tCmd, sizeof(tCmd), resp))) {
        fprintf(stderr, "tcmd_tx had error: %s!\n", strerror(err));
        return 0;
    }

    return 1;
}

static void cleanup(int sig)
{
    if (cid>=0) {
        close(cid);
    }
    if (sid>=0) {
        close(sid);
    }
}
#if ART_PORT_SOCKET

int sock_init(int port)
{
    int                sockid;
    struct sockaddr_in myaddr;
    socklen_t          sinsize;
    int                i, res;

    /* Create socket */
    sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockid == -1) {
        perror(__FUNCTION__);
        printf("Create socket to PC failed\n");
        return -1;
    }

    i = 1;
    res = setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i));
    if (res == -1) {
        close(sockid);
        return -1;
    }

    i = 1;
    res = setsockopt(sockid, IPPROTO_TCP, TCP_NODELAY, (char *)&i, sizeof(i));
    if (res == -1) {
        close(sockid);
        return -1;
    }

    myaddr.sin_family      = AF_INET;
    myaddr.sin_port        = htons(port);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    memset(&(myaddr.sin_zero), 0, 8);

    res = bind(sockid, (struct sockaddr *)&myaddr, sizeof(struct sockaddr));

    if (res != 0) {
        perror(__FUNCTION__);
        printf("Bind failed\n");
		close(sockid);
        return -1;
    }

    if (listen(sockid, 4) == -1) {
        perror(__FUNCTION__);
        printf("Listen failed\n");
		close(sockid);
        return -1;
    }

    printf("Waiting for client to connect...\n");

    sinsize = sizeof(struct sockaddr_in);
    if ((cid = accept(sockid, (struct sockaddr *)&myaddr, &sinsize)) == -1) {
        printf("Accept failed\n");
		close(sockid);
        return -1;
    }

    i = 1;
    res = setsockopt(cid, IPPROTO_TCP, TCP_NODELAY, (char *)&i, sizeof(i));
    if (res == -1) {
        printf("cannot set NOdelay for cid\n");
        close(sockid);
        return -1;
    }
    printf("Client connected!\n");

    return sockid;
}

int sock_recv(int sockid, unsigned char *buf, int buflen)
{
    int recvbytes;
    recvbytes = recv(sockid, buf, buflen, 0);
    if (recvbytes == 0) {
        printf("Connection close!? zero bytes received\n");
        return -1;
    } else if (recvbytes > 0) {
        return recvbytes;
    }
    return -1;
}

int sock_send(int sockid, unsigned char *buf, int bytes)
{
    int cnt;
    unsigned char* bufpos = buf;
    while (bytes) {
        cnt = write(sockid, bufpos, bytes);

        if (!cnt) {
            break;
        }
        if (cnt == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }

        bytes -= cnt;
        bufpos += cnt;
    }
    return (bufpos - buf);
}
#endif /* ART_PORT_SOCKET */

#ifdef ART_PORT_COM

static void art_print_buf(unsigned char *buf, unsigned int len)
{
#if DEBUG_PRINT
    unsigned int  i;

    printf("Buffer len %d, buffer content: \n", len);
    for (i = 0; i < len; i++)
    {
        printf("%x ", buf[i]);
        if ((i % 15) == 15)
            printf("\n");
    }
    printf("\n");
#else
    return;
#endif
}


int com_init(char *comname)
{
#if 0
    int            fd;
    struct termios options;

    /* open the port */
    fd = open("/dev/ttyUSB0", O_RDWR);
    fcntl(fd, F_SETFL, 0);

    /* get the current options */
    tcgetattr(fd, &options);

    /* set raw input, 1 second timeout */
    options.c_cflag     |= (CLOCAL | CREAD);
    options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag     &= ~OPOST;
    options.c_cc[VMIN]  = 10;
    options.c_cc[VTIME] = 10;

    /* set the options */
    tcsetattr(fd, TCSANOW, &options);
return fd;

#else
	struct termios termOptions;
	int         ttyFid;


	// Open the tty:
	//ttyFid = open( comname, O_RDWR | O_NDELAY);
	ttyFid = open( comname, O_RDWR | O_NDELAY|O_NDELAY); //HENRY
	if (ttyFid == -1)
	{
		printf( "Error unable to open port: %s\n", comname );
		return -1;
	}
	/*SET BLOCK READ--HENRY*/
	//fcntl(ttyFid, F_SETFL, 0);

	// Get the current options:
	tcgetattr( ttyFid, &termOptions );


        termOptions.c_iflag = 0;
	//termOptions.c_iflag |= (IXOFF | IXON | IXANY); //ENABLE SOFTWARE FLOW CONTROL--HENRY
        termOptions.c_oflag=0;
        termOptions.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
        termOptions.c_lflag=0;

	// Set the input/output speed
	cfsetispeed( &termOptions, B115200 );
	cfsetospeed( &termOptions, B115200 );
	//cfsetispeed(&termOptions, B57600);
	//cfsetospeed(&termOptions, B57600);

	// Now set the term options (set immediately)
	tcsetattr( ttyFid, TCSANOW, &termOptions );

	// All done
	printf( "Done, port speed set on: %s\n", comname );

	return ttyFid;
#endif
}

int com_wait_for_recv(int comfd)
{
    fd_set           rfds;
    struct timeval   tv;
    int              ret;
    int              tsec = 5;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(comfd, &rfds);
	//FD_SET(0, &rfds);
        tv.tv_sec = tsec;
        tv.tv_usec = 0;

        ret = select(comfd + 1, &rfds, NULL, NULL, &tv);
        if (ret == -1)
            perror(__FUNCTION__);
        else if (ret) {

		if (FD_ISSET(comfd, &rfds))
			return 0;
		else
			return -1;
	//    if (FD_ISSET(0, &rfds))
	//    	return 1;
	}
    }
}


int com_send(int comfd, unsigned char *buf, int buflen)
{


   // printf("send %d\n",buflen);
//	art_print_buf(buf, buflen);
    if (buflen > PLATFORM_UART_BUF_SIZE+3) {
	      printf("send too many ACK back to host%d\n",buflen);
	      return -1;
    }
	write(comfd, buf, buflen);

    return 0;
}


int com_recv_ack(int comfd)
{
	int nbytes;
	unsigned char buf;

	if (com_wait_for_recv(comfd) !=  0) return -1;
	nbytes = read(comfd, &buf, 1);
	if (nbytes == 1 && buf == 0xbb) return 0;
	else
	{
		tcflush(comfd, 2);
		return -1;
	}
}

int com_recv(int comfd, unsigned char *buf,  unsigned char *type)
{
    int  len, totallen = 0, nbytes ;
    unsigned char tempBuf[PLATFORM_UART_BUF_SIZE+3], *bufptr, *ptr_t;
    unsigned char crc, crc_cal;

	bufptr = buf;
	ptr_t = tempBuf;
	nbytes = 0;
	while (1) {
		if (com_wait_for_recv(comfd) !=  0) return -1;
		nbytes += read(comfd, ptr_t, PLATFORM_UART_BUF_SIZE+3);
		if (nbytes > 2) break;
		else
			ptr_t = tempBuf + nbytes;
	}

	crc =  tempBuf[0];
	*type = tempBuf[1];
	len =  tempBuf[2];

	nbytes -= 3;
	memcpy(bufptr, tempBuf+3, nbytes);
	bufptr += nbytes;
	totallen += nbytes;

	while (totallen < len) {

		if (com_wait_for_recv(comfd) !=  0) return -1;
		nbytes = read(comfd, tempBuf, PLATFORM_UART_BUF_SIZE+3);

		if (nbytes > 0){
			memcpy(bufptr, tempBuf, nbytes);
			bufptr += nbytes;
			totallen += nbytes;
		}
	}
	#ifdef DEBUG_PRINT
	printf("Received:\n");
	art_print_buf(buf, len);
	#endif
	crc_cal = calcCrc(buf, len);
	if (crc != crc_cal)
	{
		printf("ERRO! CRC error %x (cal_crc:%x), len =%d\n",crc, crc_cal, len);
		art_print_buf(buf, len);
		return -1;
	}

    return len;
}
/*failed, return -1, or return received all payload length*/
int com_all_recv(int comfd, unsigned char *buf)
{
	int ret, len = 0;
	unsigned char  *ptr, tempBuf[PLATFORM_UART_BUF_SIZE+3], ack, type;
	int tries = 20;
	ptr =  buf;

	while (1)
	{
		ret = com_recv(comfd, tempBuf, &type);
		if (ret < 0) {
			tries--;
			if (tries == 0) {
				len = -1;
				break;
			} else {
				ack = 0xaa;
				tcflush(comfd, 2);
				com_send(comfd, (unsigned char  *)&ack, 1);
				continue;
			}
		}else {
			tries = 20;
			memcpy(ptr, tempBuf, ret);
			len += ret;
			ptr += ret;
			ack = 0xbb;
			com_send(comfd, &ack, 1);

			if (type == 0xdd) break;

			if (type == 0xcc) continue;
			else {
				printf("unknow type %x\n", type);
				len = -1;
				break;
			}
		}
	}

	return len;
}

/*try 20 times if failed, all successful return buflen, ow. return -1*/
int com_all_send(int comfd, unsigned char *buf, int buflen)
{
	int  loops, rest, tries;
	unsigned char  *ptr, tempBuf[PLATFORM_UART_BUF_SIZE+3],  type;

	loops  = buflen/PLATFORM_UART_BUF_SIZE;
	rest = buflen%PLATFORM_UART_BUF_SIZE;
	ptr = buf;
	type = 0xdd;

	while (loops > 0)
	{
		tries = 20;
		loops --;
		if (loops==0 && rest ==0) type = 0xdd;
		else type = 0xcc;
		tempBuf[0] = calcCrc(ptr, PLATFORM_UART_BUF_SIZE);
		tempBuf[1] = type;
		tempBuf[2] = PLATFORM_UART_BUF_SIZE;
		memcpy(tempBuf+3, ptr, PLATFORM_UART_BUF_SIZE);
		//art_print_buf(tempBuf, PLATFORM_UART_BUF_SIZE+3);
		while (tries-- >0)
		{
#ifdef DEBUG_PRINT
			printf("SEND1 %d:\n", tries);
			art_print_buf(tempBuf, PLATFORM_UART_BUF_SIZE+3);
#endif
			com_send(comfd, tempBuf, PLATFORM_UART_BUF_SIZE+3);
			if (com_recv_ack(comfd) ==0) break;
			else continue;
		}
		if (tries ==0) return -1;
		ptr += PLATFORM_UART_BUF_SIZE;

	}
	if (rest >0)
	{
		tempBuf[0] = calcCrc(ptr, rest);
		tempBuf[1] = 0xdd;
		tempBuf[2] = rest;
		memcpy(tempBuf+3, ptr, rest);
		tries = 20;

		while ( tries-- >0)
		{
#ifdef DEBUG_PRINT
			printf("SEND1:%d\n",tries);
			art_print_buf(tempBuf, rest+3);
#endif
			com_send(comfd, tempBuf, rest+3);
			if (com_recv_ack(comfd) ==0) break;
			else continue;
		}
		if (tries ==0) return -1;
	}

	return buflen;
}

#endif /* ART_PORT_COM*/

static void print_help(char *pname)
{

#ifdef ART_PORT_SOCKET
    printf("An agent program to connect ART host and AR6K device, must be\n");
    printf("started after AR6K device driver is installed.\n\n");
    printf("Usage: %s ifname fragsize\n\n", pname);
    printf("  ifname      AR6K interface name\n");
    printf("  fragsize    Fragment size, must be multiple of 4\n\n");
    printf("Example:\n");
    printf("%s eth1 80\n\n", pname);
#endif
/*
#ifdef 	ART_PORT_COM
	printf("An agent that connects ART host and AR6K device. Must be started\n");
    printf("after AR6K device driver is successfully installed.\n\n");
    printf("Usage: %s ifname comx fragsize\n\n", pname);
    printf("  ifname    AR6K interface name\n");
    printf("  comx      COM port name\n");
    printf("  fragsize  Fragment size, must be multiple of 4\n\n");
    printf("Example:\n");
    printf("  %s eth0 /dev/ttyHSUSB0 80\n\n", pname);
#endif
 */

#ifdef 	ART_PORT_COM
	printf("An agent that connects ART host and AR6K device. Must be started\n");
    printf("after AR6K device driver is successfully installed.\n\n");
    printf("Usage: %s comx fragsize\n\n", pname);
 //   printf("  ifname    AR6K interface name\n");
    printf("  comx      COM port name\n");
    printf("  fragsize  Fragment size, must be multiple of 4\n\n");
    printf("Example:\n");
    printf("  %s /dev/ttyHSUSB0 80\n\n", pname);
#endif
}


int main (int argc, char **argv)
{
    int recvbytes=0,bytesRcvd=0, sendbytes=0;
    int chunkLen = 0;
    unsigned char *bufpos;
    int frag_size = 200;
    //int i=0;
    //int port = ART_PORT;
    bool resp = false;
    bool firstpacket = true;

    struct sigaction sa;
    int cmdIndx, statusIndx;
	static char comname[100]; //add2012

    printf("setup signal\n");
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = cleanup;

    printf("before call sigaction\n");
  //  sigaction(SIGTERM, &sa, NULL);
   // sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    cid = sid = aid = -1;
    //printf("setup ifname\n");
    memset(ar6kifname, '\0', sizeof(ar6kifname));
	memset(comname, '\0', sizeof(comname));

    if (argc == 1 ) {
        print_help(argv[0]);
        return -1;
    }

strcpy(ar6kifname, "wlan0");

// add2012
	if (argc > 1) {
        strcpy(comname, argv[1]);
    }
    if (argc > 2) {
        frag_size = atoi(argv[2]);
    }
#if 0 //add2012
    if (argc > 4) {
        port = atoi(argv[4]);
    }

    if (port == 0)
	port = ART_PORT;
    else if (port < 0 || port >65534) {
	printf("Invalid port number\n");
	goto main_exit;
    }
#endif
    //NOTE: issue with bigger size on ath6kl driver..
    if ( ((frag_size == 0) || ((frag_size % 4) != 0)) || (frag_size > 200) ) {
        printf("Invalid fragsize, should be multiple of 4 and frag size less than 200\n");
        goto main_exit;
    }

    disable_tty();

    if ( initInterface(ar6kifname) ) {
        printf("Init interface cfg80211 failed\n");
        cleanup(0);
        return -1;
    }
#if 0
    printf("open sock\n");
    sid = sock_init(port);
    if (sid < 0) {
        printf("Create socket to ART failed\n");
        cleanup(0);
        return -1;
    }

    if ((recvbytes=sock_recv(cid, line, LINE_ARRAY_SIZE)) < 0) {
        printf("Cannot nego packet size\n");
        cleanup(0);
        return -1;
    }

    printf("Get nego bytes %d\n", recvbytes);

    if (1 == (*(unsigned int *)line)) {
	reducedARTPacket = 1;
        printf("Not supporting reducedARTPacket\n");
        goto main_exit;
    }
    else {
	reducedARTPacket = 0;
    }

    sock_send(cid, &(line[0]), 1);

    printf("Ready to loop for art packet reduce %d\n", reducedARTPacket);
#endif

	cid = com_init(comname); //add2012
	tcflush(cid, 2);

    while (1) {

#if 0
		//printf("wait for tcp socket\n");
        if ((recvbytes = sock_recv(cid, line, LINE_ARRAY_SIZE)) < 0) {
            printf("Cannot recv packet size %d\n", recvbytes);
            cleanup(0);
            return -1;
        }
#endif

	recvbytes = com_all_recv(cid, line);

	if (recvbytes < 0)
	{
		printf("recvbytes:Cannot recv all packets --Quit!!!!\n");
		cleanup(0);
            	system("rmmod ath6kl_sdio.ko");
            	system("rmmod cfg80211.ko");
		enable_tty();
		exit(1);

	}

        bytesRcvd = recvbytes;
        readLength = 0;
        resp = false;

        if ( firstpacket == true )
        {
            cmdLen = *(unsigned short *)&(line[0]);
            cmdId = *(unsigned char *)&(line[2]);

            printf("->FW len %d Command %d recvbytes %d\n",cmdLen,cmdId,recvbytes);
            firstpacket = false;
        }

            //printf("Recived bytes from NART %d frag size %d\n",recvbytes,frag_size);
            bufpos = line;

            while (recvbytes) {
                if (recvbytes > frag_size) {
                    chunkLen = frag_size;
                } else {
                    chunkLen = recvbytes;
                }

                //we have to find out whether we need a resp or not for the last packet..
                recvbytes-=chunkLen;

                if ( recvbytes <=0 )
                {
                    resp = true;
                    firstpacket = true; //look for first packet again..
                }

                //printf("Chunk Len %d total size %d respNeeded %d\n",chunkLen,bytesRcvd,resp);
		//art_print_buf(bufpos, chunkLen);
                wmiSend(bufpos, chunkLen, bytesRcvd, 1, resp);

                bufpos+=chunkLen;
            }


        //line and readLength is populated in the callback
	cmdIndx = sizeof(TC_CMDS_HDR) + 4;
        statusIndx = cmdIndx + 4;
	/*
	    if ((REG_WRITE_CMD_ID != line[cmdIndx]) && (MEM_WRITE_CMD_ID != line[cmdIndx]) &&
            (M_PCI_WRITE_CMD_ID != line[cmdIndx]) && (M_PLL_PROGRAM_CMD_ID != line[cmdIndx]) &&
            (M_CREATE_DESC_CMD_ID != line[cmdIndx])) {
            printf("<- N/ART len %d Command %d status %d\n", readLength,(int)line[cmdIndx],(int)line[statusIndx]);
            //sock_send(cid, line, readLength);
	*/
	//com_send(cid, line, readLength);
	sendbytes = com_all_send(cid, line, readLength);

	if (sendbytes < 0 || cmdId == DISCONNECT_PIPE_CMD_ID)
	{
		if (cmdId == DISCONNECT_PIPE_CMD_ID)
			printf("DISCONNECT_PIPE_CMD_ID --Quit!!!!\n");
		else
			printf("Cannot send all packets --Quit!!!!\n");

		cleanup(0);
            	system("rmmod ath6kl_sdio.ko");
            	system("rmmod cfg80211.ko");
		enable_tty();
		exit(1);
	}

    }


main_exit:

    printf("Normal exit\n");
    enable_tty();
    close(cid);
    close(sid);
    close(aid);
    cleanup(0);
    return 0;
}
