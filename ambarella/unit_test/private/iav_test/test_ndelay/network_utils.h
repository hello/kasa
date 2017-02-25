/*
 * network_utils.h
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 *
 */

#ifndef __NETWORK_UTILS_H__
#define __NETWORK_UTILS_H__

#define DNETUTILS_RECEIVE_FLAG_READ_ALL 0x08000000

#ifdef BUILD_OS_WINDOWS
#define snprintf _snprintf
#include<winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef SOCKET TSocketHandler;
#define DInvalidSocketHandler (INVALID_SOCKET)
#define DIsSocketHandlerValid(x) (INVALID_SOCKET != x)
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
typedef int TSocketHandler;
#define DInvalidSocketHandler (-1)
#define DIsSocketHandlerValid(x) (0 <= x)
#endif

typedef int TSocketSize;
typedef unsigned short TSocketPort;

void net_close_socket(TSocketHandler socket);
TSocketHandler net_connect_to(const char *host_addr, unsigned short port, int type, int protocol);

int net_send(TSocketHandler fd, unsigned char* data, int len, unsigned int flag);
int net_send_timeout(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, int max_count);
int net_recv(TSocketHandler fd, unsigned char* data, int len, unsigned int flag);
int net_recv_timeout(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, int max_count);
int net_send_to(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, const void* to, TSocketSize tolen);
int net_recv_from(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, void* from, TSocketSize* fromlen);

void socket_set_timeout(TSocketHandler fd, int seconds, int u_secodns);
int socket_set_send_buffer_size(TSocketHandler fd, int size);
int socket_set_recv_buffer_size(TSocketHandler fd, int size);
int socket_set_nonblocking(TSocketHandler sock);
int socket_set_no_delay(TSocketHandler fd);
int socket_set_linger(TSocketHandler fd, int onoff, int linger, int delay);
int socket_set_defer_accept(TSocketHandler fd, int param);
int socket_set_quickACK(TSocketHandler fd, int val);

TSocketHandler net_setup_stream_socket(unsigned int localAddr,  TSocketPort localPort, unsigned char makeNonBlocking);
TSocketHandler net_setup_datagram_socket(unsigned int localAddr,  TSocketPort localPort, unsigned char makeNonBlocking, unsigned int request_receive_buffer_size, unsigned int request_send_buffer_size);
TSocketHandler net_accept(TSocketHandler fd, void* client_addr, TSocketSize* clilen, int* ret_err);

int network_init();
void network_deinit();

int pipe_write(TSocketHandler fd, const char byte);
int pipe_read(TSocketHandler fd, char* byte);
int pipe_create(TSocketHandler fd[2]);
void pipe_close(TSocketHandler fd);

#endif

