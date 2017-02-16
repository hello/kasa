/*
 *
 * Copyright (c) 2004-2009 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6003
// </summary>
//
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "athtypes_win.h"
#include "athdefs.h"
#include "art.h"
#include "export.h"

#define TCP_PROTOCOL    1
#define UDP_PROTOCOL    2

static unsigned int reduceARTPacketSize = 1;

char line[LINE_ARRAY_SIZE];
char remoteIP[16];
int targetPort = ART_PORT;
int ipProtocol = TCP_PROTOCOL;
struct sockaddr_in	serv_addr;
int    serverSocket;

static int
ART_socketConnect(char *target_hostname, int target_port_num)
{
	int    res;
	
	int    i;
	//	  int	   ffd;
	struct hostent *hostent;
	WORD   wVersionRequested;
	WSADATA wsaData;
	
	//	  ffd = fileno(stdout);
	wVersionRequested = MAKEWORD( 2, 2 );
	
	res = WSAStartup( wVersionRequested, &wsaData );
	if ( res != 0 ) {
		printf("socketConnect: Could not find windows socket library\n");
		return -1;
	}
	
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 ) {
		printf("socketConnect: Could not find windows socket library\n");
		WSACleanup( );
		return -1; 
	}
	
	printf("socket start\n");
	serverSocket = WSASocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (GROUP)NULL, 0);
	printf("socket end\n");
	if (serverSocket == INVALID_SOCKET) {
		printf("ERROR::socketConnect: socket failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}
	
	/* Allow immediate reuse of port */
	printf("setsockopt SO_REUSEADDR start\n");
	i = 1;
	res = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));
	if (res != 0) {
		printf("ERROR::socketConnect: setsockopt SO_REUSEADDR failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}	
	printf("setsockopt SO_REUSEADDR end\n");
	
	/* Set TCP Nodelay */
	printf("setsockopt TCP_NODELAY start\n");
	i = 1;
	res = setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));
	if (res != 0) {
		printf("ERROR::socketCreateAccept: setsockopt TCP_NODELAY failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}	
	printf("setsockopt TCP_NODELAY end\n");
	
	printf("gethostbyname start\n");
	printf("socket_connect: target_hostname = '%s' at port %d\n", target_hostname, target_port_num);
	hostent = gethostbyname(target_hostname);
	printf("gethostbyname end\n");
	if (!hostent) {
		printf("ERROR::socketConnect: gethostbyname failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}	
	
	//	  memcpy(ip_addr, hostent->h_addr_list[0], hostent->h_length);
	//	  *ip_addr = ntohl(*ip_addr);
	memset(&serv_addr,'\0',sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
	serv_addr.sin_port = htons((short)target_port_num);
	//printf("****sin_port :%d\n",sin.sin_port);
	//printf("***************sin_addr :%s\n",(sin.sin_addr.s_addr));
	for (i = 0; i < 120; i++) {
		//printf("***CONNECT ******\n");
		printf("connect start %d\n", i);
		res = connect(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
		printf("connect end %d\n", i);
		//printf("***CONNECT END  ******\n");
		if (res == 0) {
			break;
		}
		Sleep(1000);
	}
	if (i == 120) {
		printf("ERROR::connect failed completely\n");
		WSACleanup( );
		return -1;
	}
	
	return serverSocket;
}


static int
ART_socketConnectUDP(char *target_hostname, int target_port_num)
{
	int    res;
	//int    i;
	struct hostent *hostent;
	WORD   wVersionRequested;
	WSADATA wsaData;
	
	wVersionRequested = MAKEWORD( 2, 2 );
	
	res = WSAStartup( wVersionRequested, &wsaData );
	if ( res != 0 ) {
		printf("socketConnect: Could not find windows socket library\n");
		return -1;
	}
	
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 ) {
		printf("socketConnect: Could not find windows socket library\n");
		WSACleanup( );
		return -1; 
	}
	
	printf("socket start\n");
	//serverSocket = WSASocket(PF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, (GROUP)NULL, 0);
	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	printf("socket end\n");
	if (serverSocket == INVALID_SOCKET) {
		printf("ERROR::socketConnect: socket failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}
	
	printf("gethostbyname start\n");
	printf("socket_connect: target_hostname = '%s' at port %d\n", target_hostname, target_port_num);
	hostent = gethostbyname(target_hostname);
	printf("gethostbyname end\n");
	if (!hostent) {
		printf("ERROR::socketConnect: gethostbyname failed: %d\n", WSAGetLastError());
		WSACleanup( );
		return -1;
	}	
	
	memset(&serv_addr, 0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
	serv_addr.sin_port = htons((short)target_port_num);
	
	return serverSocket;
}


static unsigned int wait_for_reply()
{
	int recvbytes=0;
    int length = sizeof(struct sockaddr_in);

#ifdef DEBUG_PRINT
	printf("wait_for_reply\n");
#endif
    if (((ipProtocol == TCP_PROTOCOL) && ((recvbytes = recv(serverSocket, line, LINE_ARRAY_SIZE, 0)) < 0)) ||
        ((ipProtocol == UDP_PROTOCOL) && 
            ((recvbytes = recvfrom(serverSocket, line, LINE_ARRAY_SIZE, 0, (struct sockaddr *)&serv_addr, &length)) < 0)))
    {
		    printf("FAILED: IO Problem");
		    exit(1);
    }
#ifdef DEBUG_PRINT
	printf("recvbytes %d\n",recvbytes);
#endif
	return recvbytes;
}

static void ART_SocketSend(char *buf, int length)
{
	int sendbytes =0;

    if (((ipProtocol == TCP_PROTOCOL) && ((sendbytes = send(serverSocket, buf, length, 0)) < 0)) ||
        ((ipProtocol == UDP_PROTOCOL) && ((sendbytes = sendto(serverSocket, buf, length, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)))
    {
	    printf("cannot send data\n");
		closesocket(serverSocket);
		exit(1);
    }
#ifdef DEBUG_PRINT
	printf("sendbytes %d\n",sendbytes);
#endif
}

void parseRemoteIP_ini()
{
	static char delimiters[]   = " \t";
	FILE *fStream;
	char lineBuf[222], *pLine;
	
	if( (fStream = fopen("remoteIP.ini", "r")) == NULL ) {
		printf("Failed to open remoteIP.ini\n");
        return;
	}
	
	while(fgets(lineBuf, sizeof(lineBuf), fStream) != NULL) {
		pLine = lineBuf;
		while(isspace(*pLine)) pLine++;
		if(*pLine == '#' || *pLine == '\0') {
			continue;
		}
		else if(strncmp("REMOTE_IP", pLine, strlen("REMOTE_IP")) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%s", &remoteIP)) {
				printf("Unable to read the remoteIP\n");
				return;
			}
		}
		else if(strncmp("REDUCE_ART_PACKET", pLine, strlen("REDUCE_ART_PACKET")) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%i", &reduceARTPacketSize)) {
				printf("Unable to read the remoteIP\n");
				return;
			}
	    }
		else if(strncmp("TARGET_PORT", pLine, strlen("TARGET_PORT")) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%i", &targetPort)) {
				printf("Unable to read the target port\n");
				return;
			}
	    }
		else if(strncmp("IP_PROTOCOL", pLine, strlen("IP_PROTOCOL")) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%i", &ipProtocol)) {
				printf("Unable to read the IP protocol\n");
				return;
			}
	    }
	}
}

int init_all()
{
	parseRemoteIP_ini();
    if (ipProtocol == TCP_PROTOCOL)
    {
	    ART_socketConnect(remoteIP,targetPort);
    }
    else if (ipProtocol == UDP_PROTOCOL)
    {
	    ART_socketConnectUDP(remoteIP,targetPort);
    }
	return 0;
}

A_BOOL __cdecl ETH_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID)
{
    *nTargetID = USE_ART_AGENT;
    return TRUE;	
}

HANDLE __cdecl ETH_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName){

	printf("\n** open_device_ene **\n");
	
	init_all();

	printf("negotiate packet size with ART client\n");
	ART_SocketSend((char *)&reduceARTPacketSize,sizeof(reduceARTPacketSize));
	wait_for_reply();
	printf("negotate completed\n");

	return ((HANDLE)1234);
}

static int DRG_Read_secondTime = 0;
static int DRG_Read_ignored = 0; //for writes, we don't really need to read the length.
static A_UINT8 currentCmdId = 0;
static A_UINT32 fakeReturnLength = sizeof(A_UINT32) + sizeof(A_UINT32);

DWORD __cdecl ETH_DRG_Read( HANDLE pContext,  PUCHAR buf, ULONG length,  PULONG pBytesRead)
{
	unsigned char *rx_buf = NULL;
	unsigned int returnLength;
#ifdef DEBUG_PRINT
	unsigned int iIndex;
#endif


//	printf("DRG_Read length %d\n",length);
	if (reduceARTPacketSize && (length >= 1000))
	{
		printf("**** WARNING length > DRG_Write_size :  %d\n",length);
		while(1);
	}

	if (DRG_Read_secondTime)
	{
		DRG_Read_secondTime = 0;

		if (!DRG_Read_ignored)
			memcpy(buf,line,length);
		else
			memcpy(buf,&currentCmdId,1);

#ifdef DEBUG_PRINT
		printf("osSockRead %d B\n", length);
		for(iIndex=0; iIndex<length; iIndex++)
		{
			printf("%x ", buf[iIndex]);
		}
		printf("\n");
#endif

		return length;
	}
		
	returnLength = wait_for_reply();
	DRG_Read_secondTime = 1;

	if (!DRG_Read_ignored)
		memcpy(buf,&returnLength,length);
	else
		memcpy(buf, &fakeReturnLength, length);

#ifdef DEBUG_PRINT
	printf("osSockRead %d B\n", length);
	for(iIndex=0; iIndex<length; iIndex++)
	{
		printf("%x ", buf[iIndex]);
	}
	printf("\n");
#endif

	return length; //TONY: HACK first DRG_Read to piggybag first length read
}

DWORD __cdecl ETH_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length )
{
#ifdef DEBUG_PRINT
	A_INT32 iIndex;
#endif
	ULONG totalBytes;
	PUCHAR	bufpos;  
	A_UINT16 endMarker = 0xbeef;
	bufpos = buf;

#ifdef DEBUG_PRINT
	printf("DRG_Write cmdID %d, length %d\n",(A_UINT8)(buf[2]),length);
#endif

	if ((REG_WRITE_CMD_ID == (A_UINT8)(buf[2])) || (MEM_WRITE_CMD_ID == (A_UINT8)(buf[2])) ||
		(M_PCI_WRITE_CMD_ID == (A_UINT8)(buf[2])) || (M_PLL_PROGRAM_CMD_ID == (A_UINT8)(buf[2])) ||
		(M_CREATE_DESC_CMD_ID == (A_UINT8)(buf[2])))
		{
			DRG_Read_ignored = 1;
			currentCmdId = (A_UINT8)(buf[2]);
		}
		else
			DRG_Read_ignored = 0;


#ifdef DEBUG_PRINT
	for(iIndex=0; iIndex<length; iIndex++)
	{
		printf("%x ", bufpos[iIndex]);
	}
		printf("\n");
#endif
						
	if (reduceARTPacketSize)
	{
		totalBytes = length;
		while (totalBytes)
		{
			if (totalBytes > DRG_Write_size)
			{
				ART_SocketSend(bufpos, DRG_Write_size);
				totalBytes -= DRG_Write_size;
				bufpos += DRG_Write_size;
				wait_for_reply();
			}
			else
			{
				ART_SocketSend(bufpos,totalBytes);
				totalBytes = 0;
			}
		}
		if (0 == (length % DRG_Write_size))
		{
			ART_SocketSend((char *)&endMarker,sizeof(endMarker));		
		}
	}
	else
	{
	ART_SocketSend(bufpos,length);
	}

	return length;
}

