/**
 * am_network_al.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
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
 */

#ifndef __AM_NETWORK_AL_H__
#define __AM_NETWORK_AL_H__

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

