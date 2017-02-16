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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <stddef.h>
#include <Tlhelp32.h>
#include <io.h>

#include "athtypes_win.h"
#include "athdefs.h"
#include "art.h"

#include "export.h"

//--------------------------------------------------------------------------------
#define PLATFORM_UART_BUF_SIZE 128
#define READ_BUF_SIZE PLATFORM_UART_BUF_SIZE
#define WRITE_BUF_SIZE PLATFORM_UART_BUF_SIZE

#define IN_QUEUE	0
#define OUT_QUEUE	1

#define DEF_UART_PORT       "UART_PORT"
#define DEF_UART_SETTING    "UART_SETTING"

#define DEF_UART_PORT_DEFAULT      "COM1"
#define DEF_UART_SETTING_DEFAULT   "115200,n,8,1"

//---------------------------------------------------------------------------------

static unsigned int reduceARTPacketSize = 1;
char strUartPort[LINE_ARRAY_SIZE];
char strUartSetting[LINE_ARRAY_SIZE];

//---------------------------------------------------------------------------------
/*
[0] = 0xaa:Read, 0xbb:Write, 0xcc:Open
[1] = len
[2+] = data
*/
unsigned char line[LINE_ARRAY_SIZE];
unsigned char lineLarge[LINE_ARRAY_SIZE];
unsigned char ack[1];

typedef struct _ART_READ {
unsigned char cmd;
unsigned int len;
} ART_READ;

HANDLE hSerial;
DCB dcbSerialParams = {0};
COMMTIMEOUTS timeouts={0};
#define IN_QUEUE	0
#define OUT_QUEUE	1

A_UINT32 sendAck() {
    DWORD dwWrite;
	int counter = 0;

#ifdef DEBUG_PRINT
	printf("sendAck\n");
#endif

     if (!WriteFile((HANDLE) hSerial, ack, 1, &dwWrite, NULL)) {
	  	printf("write error!!\n");
       	 exit(-1);
      	}
	return dwWrite;

}

A_UINT32 os_com_write(A_INT32 len) {

    DWORD dwWrite;

#ifdef DEBUG_PRINT
	int counter = 0;
	printf("os_com_write len %d\n",len);
	for ( counter= 0; counter < len; counter++)
	{
		printf("%x ",line[counter]);
	}
		printf("\n");
#endif

     if (!WriteFile((HANDLE) hSerial, line, len, &dwWrite, NULL)) {
	  	printf("write error!!\n");
       	 exit(-1);
      	}
	return dwWrite;

}


static void ART_SocketSend(int length)
{

	int sendbytes =0;
	if ((sendbytes = os_com_write(length)) < 0) {
		printf("cannot send data\n");
		exit(1);
	}

}

A_BOOL UART_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID)
{
    *nTargetID = USE_ART_AGENT;
    return TRUE;	
}
void os_com_close() { 

    // reset error byte

    if (!EscapeCommFunction((HANDLE) hSerial, CLRDTR)) {

    }
    if (!PurgeComm((HANDLE) hSerial, PURGE_RXABORT | PURGE_RXCLEAR |
                       PURGE_TXABORT | PURGE_TXCLEAR)) {
    }

    // device handle
    CloseHandle((HANDLE) hSerial);
	
}

DWORD getBytesBuffered(HANDLE handle, DWORD queueType) {

    COMSTAT comStat;
	DWORD dwErrors;

	if (!ClearCommError(handle, &dwErrors, &comStat)) {
		printf("ERROR:: ClearCommError :%x\n", GetLastError());
		return 0;
	}

	switch(queueType) {
	  case IN_QUEUE:
			return comStat.cbInQue;
			break;
	  case OUT_QUEUE:
			return comStat.cbOutQue;
			break;
	}
    return 0;
}

A_UINT32 os_com_receive_ack() {

    A_UINT32 dwCommEvent;
    DWORD dwRead, pos = 0, numCharsToRead, numCharsInBuffer, iIndex;
	A_UINT8 *chReadBuf;
    A_UINT32 nComErr;
    A_UINT32 len = 1;

    // reset error byte
    nComErr = 0;

#ifdef DEBUG_PRINT
	printf("os_com_receive_ack len %d\n", len);
#endif

	chReadBuf=ack;

    while(pos < (A_UINT32)len) {

	  numCharsInBuffer = getBytesBuffered((HANDLE)hSerial, IN_QUEUE);

	  if (numCharsInBuffer == 0) { 

		  // Wait for the event

          if ( WaitCommEvent((HANDLE) hSerial, &dwCommEvent, NULL)) {

	         if (!(dwCommEvent&EV_RXCHAR) ) continue;
	      }
	      else continue;
	  }

       // read receive buffer
	  dwRead=0;

	  numCharsInBuffer = getBytesBuffered((HANDLE)hSerial, IN_QUEUE);
	
	  if (numCharsInBuffer == 0)  continue;

	  if (numCharsInBuffer > (A_UINT32)(len-pos))
		numCharsToRead = (len-pos);
	  else
		numCharsToRead = numCharsInBuffer;
		  

      if (ReadFile((HANDLE) hSerial, chReadBuf, numCharsToRead, &dwRead, NULL)) {
		 if (dwRead != numCharsToRead) {
			printf("WARNING:: Number of bytes in buffer=%d:requested=%d:read=%d\n", numCharsInBuffer, numCharsToRead, dwRead);
		 }
		 for(iIndex=0; iIndex<dwRead; iIndex++) {
#ifdef DEBUG_PRINT
	        printf("%x ", ack[pos]);
#endif
			pos++;
		    chReadBuf++;
		 }
      }
      else {
        printf("Read ERROR!!\n");
		exit(-1);
      }
	} 
    len=pos;

#ifdef DEBUG_PRINT
	printf("\n");
#endif

    return len;
}

static unsigned int wait_for_reply()
{
	os_com_receive_ack();
	if ((ack[0] == 0xb1) ||(ack[0] == 0xa1))
	{
#ifdef DEBUG_PRINT
		printf("got reply %d\n",ack[0]);
#endif
	}
	else
	{
		printf("ERROR corrupted ack\n");
		exit(-1);
	}
	return 0;
}

A_UINT32 os_com_read(A_INT32 len) {

    A_UINT32 dwCommEvent;
    DWORD dwRead, pos = 0, numCharsToRead, numCharsInBuffer, iIndex;
	A_UINT8 *chReadBuf;
    A_UINT32 nComErr;

    // reset error byte
    nComErr = 0;

#ifdef DEBUG_PRINT
	printf("os_com_read len %d\n", len);
#endif

	chReadBuf=line;

    while(pos < (A_UINT32)len) {

	  numCharsInBuffer = getBytesBuffered((HANDLE)hSerial, IN_QUEUE);

	  if (numCharsInBuffer == 0) { 

		  // Wait for the event
          if ( WaitCommEvent((HANDLE) hSerial, &dwCommEvent, NULL)) {

	         if (!(dwCommEvent&EV_RXCHAR) ) continue;
	      }
	      else continue;
	  }

       // read receive buffer
	  dwRead=0;

	  numCharsInBuffer = getBytesBuffered((HANDLE)hSerial, IN_QUEUE);
	
	  if (numCharsInBuffer == 0)  continue;

	  if (numCharsInBuffer > (A_UINT32)(len-pos))
		numCharsToRead = (len-pos);
	  else
		numCharsToRead = numCharsInBuffer;
		  
      if (ReadFile((HANDLE) hSerial, chReadBuf, numCharsToRead, &dwRead, NULL)) {
		 if (dwRead != numCharsToRead) {
			printf("WARNING:: Number of bytes in buffer=%d:requested=%d:read=%d\n", numCharsInBuffer, numCharsToRead, dwRead);
		 }
		 for(iIndex=0; iIndex<dwRead; iIndex++) {

#ifdef DEBUG_PRINT
	        printf("%x ", line[pos]);
#endif
			pos++;
		    chReadBuf++;
		 }
      }
      else {
        printf("Read ERROR!!\n");
		exit(-1);
      }
	} 
    len=pos;
#ifdef DEBUG_PRINT
	printf("\n");
#endif
    return len;
}

static void parseRemoteIP_ini_Uart(void)
{
	static char delimiters[]   = " \t";
	FILE *fStream;
	char lineBuf[222], *pLine;

	//set default Uart configure
	strcpy(strUartPort, DEF_UART_PORT_DEFAULT);
	strcpy(strUartSetting, DEF_UART_SETTING_DEFAULT);

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
		else if(strncmp(DEF_UART_PORT, pLine, strlen(DEF_UART_PORT)) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%s", &strUartPort)) {
				strcpy(strUartPort, DEF_UART_PORT_DEFAULT);
			}
		}
		else if(strncmp(DEF_UART_SETTING, pLine, strlen(DEF_UART_SETTING)) == 0) {
			pLine = strchr(pLine, '=');
			pLine++;
			pLine = strtok( pLine, delimiters ); //get past any white space etc
			
			if(!sscanf(pLine, "%s", &strUartSetting)) {
				strcpy(strUartSetting, DEF_UART_SETTING_DEFAULT);
			}
		}
	}
}

HANDLE UART_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName){

    DCB          dcb;
    A_UINT32 err=0;
    A_UINT32 i=0;

	printf("\n** open UART ART **\n");

	// Close the port 
    (void)os_com_close();

	// get Uart configure
	parseRemoteIP_ini_Uart();
        
    // device handle
    hSerial = CreateFile(strUartPort,                    // port name
                       GENERIC_READ | GENERIC_WRITE,     // allow r/w access
                       0,                                // always no sharing
                       0,                                // no security atributes for file
                       OPEN_EXISTING,                    // always open existing
                       //FILE_FLAG_OVERLAPPED,            // overlapped operation   
                       FILE_FLAG_NO_BUFFERING,            // non-overlapped operation   
                       0);                               // always no file template

    if ((HANDLE)hSerial == INVALID_HANDLE_VALUE) {
       printf("open %s error!!\n", strUartPort);
       return 0;
    }
	
    // port configuration
    memset (&dcb,0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);

    if (!BuildCommDCB(strUartSetting, &dcb)) {
    //if (!BuildCommDCB("115200,n,8,1", &dcb)) {
		printf("ART: BuildCommDCB Error:%s\n", strUartSetting);
       return 0;
    }
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    if (!SetCommState((HANDLE) hSerial, &dcb)) {
       return 0;
    }
    if (!SetupComm((HANDLE) hSerial, READ_BUF_SIZE, WRITE_BUF_SIZE)) {
       return 0;
    }
    if (!EscapeCommFunction((HANDLE) hSerial, SETDTR)) {
       return 0;
    }
    if (!PurgeComm((HANDLE) hSerial, PURGE_RXABORT | PURGE_RXCLEAR |
                       PURGE_TXABORT | PURGE_TXCLEAR)) {
       return 0;
    }
	
    // set mask to notify thread if a character was received
    if (!SetCommMask((HANDLE) hSerial, EV_RXCHAR|EV_BREAK|EV_RXFLAG)) {
       // error setting communications event mask
       return 0;
    }

	printf("ART: Uart Port Open Success with %s:%s\n", strUartPort, strUartSetting);

	memset(line,0,LINE_ARRAY_SIZE);
	line[0] = 0xcc;
	os_com_write(1);
	
	return ((HANDLE)1234);
}

static int DRG_Read_secondTime = 0;
static int DRG_Read_ignored = 0; //for writes, we don't really need to read the length.
static A_UINT8 currentCmdId = 0;
static A_UINT32 fakeReturnLength = sizeof(A_UINT32) + sizeof(A_UINT32);

DWORD UART_DRG_Read( HANDLE pContext,  PUCHAR buf, ULONG length,  PULONG pBytesRead)
{
	unsigned char *rx_buf = NULL;
	unsigned int counter = 0;
	unsigned int readLength = 0;
	ULONG totalBytes;
	unsigned int bufpos;
	unsigned int orgLength;
	ART_READ *artRead = NULL;

	orgLength = length;

#ifdef DEBUG_PRINT
	printf("DRG_Read+ length %d\n",length);
#endif
	if (length > PLATFORM_UART_BUF_SIZE)
	{
#ifdef DEBUG_PRINT
		printf("**** WARNING length > DRG_Read :  %d\n",length);
#endif
		bufpos = 0;
		totalBytes = length;

		memset(line,0,LINE_ARRAY_SIZE);
		memset(lineLarge,0,LINE_ARRAY_SIZE);
		artRead = (ART_READ *)line;
		artRead->cmd = 0xa1;
		artRead->len = orgLength;
		ART_SocketSend(5);
		
		while (totalBytes)
		{
			if (totalBytes > PLATFORM_UART_BUF_SIZE)
			{
				os_com_read(2);
				if ((line[0] != 0xa1) || (line[1] != PLATFORM_UART_BUF_SIZE))
				{
					printf("correupted line[0] %x\n",line[0]);
					exit(-1);
				}

				readLength = os_com_read(PLATFORM_UART_BUF_SIZE);
				if (readLength != PLATFORM_UART_BUF_SIZE)
				{
					printf("ERROR: readLength check failed\n");
					exit(-1);
				}

				memcpy(&lineLarge[bufpos],line,readLength);

				totalBytes -= readLength;
				bufpos += readLength;
				ack[0] = 0xa1;
				sendAck();
			}
			else
			{
				os_com_read(2);
				if ((line[0] != 0xaa) || (line[1] != totalBytes))
				{
					printf("correupted line[0] %x len %d\n",line[0], line[1]);
					exit(-1);
				}
				
				readLength = os_com_read(totalBytes);
				if (readLength != totalBytes)
				{
					printf("ERROR: readLength2 check failed\n");
					exit(-1);
				}
				memcpy(&lineLarge[bufpos],line,totalBytes);
				totalBytes -= readLength;
			}
		}
		memcpy(buf,lineLarge,orgLength);
#ifdef DEBUG_PRINT
		printf("DRG_Read- length length %d readLength %d\n",length, orgLength);
#endif
		return orgLength;

	}
	else
	{
		memset(line,0,LINE_ARRAY_SIZE);
		memcpy(line+2,buf,length);
		line[0] = 0xaa;
		line[1] = (unsigned char)length;
		ART_SocketSend(2);
		os_com_read(2);
		if (line[0] != 0xaa)
		{
			printf("correupted line[0]\n");
			exit(-1);
		}

#ifdef DEBUG_PRINT
		printf("line[1] %d\n",line[1]);
#endif

		readLength = os_com_read(length);

		memcpy(buf,line,length);
#ifdef DEBUG_PRINT
		printf("DRG_Read- length length %d readLength %d\n",length, readLength);
#endif
		return length;
	}
}

DWORD UART_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length )
{
#ifdef DEBUG_PRINT
	ULONG iIndex;
#endif
	ULONG totalBytes;
	PUCHAR	bufpos;  
	A_UINT16 endMarker = 0xbeef;
	bufpos = buf;

#ifdef DEBUG_PRINT
	printf("DRG_Write cmdID %d, length %d\n",(A_UINT8)(buf[2]),length);
#endif

#ifdef DEBUG_PRINT
	for(iIndex=0; iIndex<length; iIndex++)
	{
		printf("%x ", bufpos[iIndex]);
	}
		printf("\n");
#endif
						
		totalBytes = length;
		while (totalBytes)
		{
			if (totalBytes > PLATFORM_UART_BUF_SIZE)
			{
				memset(line,0,LINE_ARRAY_SIZE);
				memcpy(line+2,bufpos,PLATFORM_UART_BUF_SIZE);
				line[0] = 0xb1;
				line[1] = PLATFORM_UART_BUF_SIZE;
				ART_SocketSend(PLATFORM_UART_BUF_SIZE+2);
				totalBytes -= PLATFORM_UART_BUF_SIZE;
				bufpos += PLATFORM_UART_BUF_SIZE;
				wait_for_reply();
			}
			else
			{
				memset(line,0,LINE_ARRAY_SIZE);
				memcpy(line+2,bufpos,totalBytes);
				line[0] = 0xbb;
				line[1] = (unsigned char)totalBytes;
				ART_SocketSend(totalBytes+2);
				totalBytes = 0;
			}
		}

	return length;
}

