/*
 * network_utils.h
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
 *
 * Copyright (C) 2015 -2020, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

