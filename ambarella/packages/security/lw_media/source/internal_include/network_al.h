/**
 * network_al.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __NETWORK_AL_H__
#define __NETWORK_AL_H__

#ifdef BUILD_OS_WINDOWS

#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#define DNETUTILS_RECEIVE_FLAG_READ_ALL 0x08000000

//platform variable
void gfNetCloseSocket(TSocketHandler socket);

//network related
extern TSocketHandler gfNet_ConnectTo(const TChar *host_addr, TSocketPort port, TInt type, TInt protocol);
extern TInt gfNet_Send(TSocketHandler fd, TU8 *data, TInt len, TUint flag);
extern TInt gfNet_Send_timeout(TSocketHandler fd, TU8 *data, TInt len, TUint flag, TInt max_count);
extern TInt gfNet_Recv(TSocketHandler fd, TU8 *data, TInt len, TUint flag);
extern TInt gfNet_Recv_timeout(TSocketHandler fd, TU8 *data, TInt len, TUint flag, TInt max_count);
extern TInt gfNet_SendTo(TSocketHandler fd, TU8 *data, TInt len, TUint flag, const void *to, TSocketSize tolen);
extern TInt gfNet_RecvFrom(TSocketHandler fd, TU8 *data, TInt len, TUint flag, void *from, TSocketSize *fromlen);

extern TInt gfNetDirect_Recv(TSocketHandler fd, TU8 *data, TInt len, EECode &err_code);

extern TSocketHandler gfNet_SetupStreamSocket(TU32 localAddr,  TSocketPort localPort, TU8 makeNonBlocking);
extern TSocketHandler gfNet_SetupDatagramSocket(TU32 localAddr,  TSocketPort localPort, TU8 makeNonBlocking, TU32 request_receive_buffer_size, TU32 request_send_buffer_size);

void gfSocketSetTimeout(TSocketHandler fd, TInt seconds, TInt u_secodns);
EECode gfSocketSetSendBufferSize(TSocketHandler fd, TInt size);
EECode gfSocketSetRecvBufferSize(TSocketHandler fd, TInt size);
EECode gfSocketSetDeferAccept(TSocketHandler fd, TInt param);
EECode gfSocketSetQuickACK(TSocketHandler fd, TInt val);
EECode gfSocketSetNonblocking(TSocketHandler sock);
EECode gfSocketSetNoDelay(TSocketHandler fd);
EECode gfSocketSetLinger(TSocketHandler fd, TInt onoff, TInt linger, TInt delay);

TSocketHandler gfNet_Accept(TSocketHandler fd, void *client_addr, TSocketSize *clilen, EECode &err);

#endif

