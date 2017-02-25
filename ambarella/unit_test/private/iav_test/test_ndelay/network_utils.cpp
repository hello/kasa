/*
 * network_utils.cpp
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simple_log.h"
#include "network_utils.h"

void net_close_socket(TSocketHandler socket)
{
#ifdef BUILD_OS_WINDOWS
    closesocket(socket);
#else
    close(socket);
#endif
}

static int __connect_nonblock(TSocketHandler sockfd, struct sockaddr*saptr, socklen_t salen, int nsec)
{
#ifdef BUILD_OS_WINDOWS
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);
    if (ret == SOCKET_ERROR) {
        LOG_ERROR("ioctlsocket error\n");
        return (-1);
    }

    ret = connect(sockfd, saptr, salen);

    struct timeval timeout ;
    fd_set r;

    FD_ZERO(&r);
    FD_SET(sockfd, &r);
    timeout.tv_sec = nsec;
    timeout.tv_usec =0;
    ret = select(0, 0, &r, 0, &timeout);
    if (ret <= 0) {
        LOG_ERROR("select error\n");
        closesocket(sockfd);
        return (-5);
    }

    ul = 0;
    ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);
    if (ret == SOCKET_ERROR) {
        LOG_ERROR("ioctlsocket error\n");
        closesocket (sockfd);
        return (-6);
    }
#else
    int flags, n, error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ((n = connect(sockfd, saptr, salen)) < 0) {
        if (errno != EINPROGRESS) {
            LOG_ERROR("connect error\n");
            return (-1);
        }
    }

    if (n == 0) {
        LOG_NOTICE("connect return ok\n");
        goto done;
    }

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ((n = select(sockfd + 1, &rset, &wset, NULL,nsec ? &tval : NULL)) == 0) {
        LOG_ERROR("select error\n");
        net_close_socket(sockfd);
        errno = ETIMEDOUT;
        return (-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            LOG_ERROR("getsockopt error\n");
            return (-1); /* Solaris pending error */
        }
    } else {
        LOG_ERROR("not set flag\n");
        return -1;
    }

done:
    fcntl(sockfd, F_SETFL, flags); /* restore file status flags */

    if (error) {
        errno = error;
        perror("socket");
        LOG_ERROR("have error %d\n", error);
        return (-1);
    }
#endif

    return 0;
}

TSocketHandler net_connect_to(const char *host_addr, unsigned short port, int type, int protocol)
{
    TSocketHandler fd = -1;
    int ret = 0;
    struct sockaddr_in dest_addr;

    DASSERT(host_addr);
    DASSERT(SOCK_STREAM == type);
    DASSERT(IPPROTO_TCP == protocol);
    type = SOCK_STREAM;
    protocol = IPPROTO_TCP;

    fd = socket(AF_INET, type, protocol);
    if (!DIsSocketHandlerValid(fd)) {
        perror("socket()");
        LOG_ERROR("create socket, (%d) fail, type %d, protocol %d.\n", fd, type, protocol);
        return (-1);
    }

    memset(&dest_addr, 0x0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host_addr);
    memset(&(dest_addr.sin_zero), 0x0, 8);

    ret = __connect_nonblock(fd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in), 5);
    if (ret < 0) {
        LOG_ERROR("connect(%s: %d) fail, ret %d.\n", host_addr, port, ret);
        net_close_socket(fd);
        return (-1);
   }

    return fd;
}

int net_send(TSocketHandler fd, unsigned char* data, int len, unsigned int flag)
{
#ifdef BUILD_OS_WINDOWS
    int sent = 0, remain = len;
    do {
        sent = send(fd, (const char*)data, remain, flag);
        if (sent < 0) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) continue;
            if (err == WSAEWOULDBLOCK) {
                Sleep(20);
                continue;
            }
            LOG_ERROR("net_send failed, errno %d\n",err);
            return err;
        } else if (!sent) {
            LOG_NOTICE("peer close\n");
            return 0;
        }
        remain -= sent;
        data += sent;
    } while (remain > 0);
    return len;
#else
    int bytes = 0;
    while (bytes < len) {
        int ret = send(fd, data + bytes, len - bytes, flag);
        if (ret < 0) {
            int err = errno;
            if(err == EINTR) continue;
            if(err == EAGAIN || err == EWOULDBLOCK){
                usleep(1000);
                continue;
            }
            perror("send");
            LOG_ERROR("net_send failed, errno %d\n",err);
            return 0;
        }
        bytes += ret;
    }
    return len;
#endif
}

int net_send_timeout(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, int max_count)
{
#ifdef BUILD_OS_WINDOWS
    int bytes = 0;
    int count = 0;
    while (bytes < len) {
        int ret = send(fd, (const char*)data + bytes, len - bytes, flag);
        if (ret < 0) {
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                if (count >= max_count) {
                    return ret;
                }
                count ++;
                continue;
            }
            LOG_ERROR("send, errno %d.\n", err);
            return ret;
        } else if (!ret) {
            LOG_NOTICE("peer close\n");
            return 0;
        }
        bytes += ret;
    }
    return len;
#else
    int bytes = 0;
    int current_count = 0;
    while (bytes < len) {
        int ret = send(fd, data + bytes, len - bytes, flag);
        if (ret < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                if (current_count >= max_count) {
                    LOG_ERROR("timeout current_count %d, max_count %d\n", current_count, max_count);
                    return ret;
                }
                current_count ++;
                continue;
            }
            LOG_ERROR("net_send failed, errno %d\n",err);
            return 0;
        }
        bytes += ret;
    }
    return len;
#endif
}

int net_recv(TSocketHandler fd, unsigned char* data, int len, unsigned int flag)
{
#ifdef BUILD_OS_WINDOWS
    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        int received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char*)data, remain, flag);
            if (received < 0) {
                int err = WSAGetLastError();
                if (err == WSAEINTR) continue;
                if (err == WSAEWOULDBLOCK) {
                    Sleep(20);
                    continue;
                }
                LOG_ERROR("net_recv, recv errno %d.\n", err);
                return received;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
        } while (remain > 0);
        return len;
    }

    int ret = recv(fd, (char*)data, len, flag);
    return ret;
#else
    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        int received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char*)data, remain, flag);
            if (received < 0) {
                LOG_ERROR("recv error, ret %d\n", received);
                return 0;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
        } while (remain > 0);
        return len;
    }

    while(1) {
        int bytesRead = recv(fd,(void*)data, len, flag);
        if (bytesRead < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                continue;
            }
            LOG_ERROR("net_recv, recv errno %d.\n",err);
        }else if(bytesRead == 0){
            LOG_ERROR("net_recv, recv 0, peer closed\n");
        }
        return bytesRead;
    }
#endif
}

int net_recv_timeout(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, int max_count)
{
#ifdef BUILD_OS_WINDOWS
    int count = 0;

    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        int received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char*)data, remain, flag);
            if (received < 0) {
                int err = WSAGetLastError();
                if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                    if (count >= max_count) {
                        if (remain < len) {
                            return (len - remain);
                        }
                        return received;
                    }
                    count ++;
                    continue;
                }
                LOG_ERROR("net_recv, recv errno %d.\n", err);
                return received;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
        } while (remain > 0);
        return len;
    }

    while (1) {
        int bytesRead = recv(fd,(char*)data, len, flag);
        if (bytesRead < 0) {
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                if (count >= max_count) {
                    return bytesRead;
                }
                count ++;
                continue;
            }
            LOG_ERROR("net_recv, recv errno %d.\n", err);
            return bytesRead;
        } else if (!bytesRead) {
            LOG_NOTICE("peer close\n");
            return 0;
        }
        return bytesRead;
    }
#else
    int count = 0;

    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        int received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char*)data, remain, flag);
            if (received < 0) {
                int err = errno;
                if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                    if (count >= max_count) {
                        if (remain < len) {
                            return (len - remain);
                        }
                        return received;
                    }
                    count ++;
                    continue;
                }
                LOG_ERROR("recv error, ret %d, errno=%d\n", received, err);
                return 0;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
        } while (remain > 0);
        return len;
    }

    while (1) {
        int bytesRead = recv(fd,(void*)data, len, flag);
        if (bytesRead < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                if (count >= max_count) {
                    return bytesRead;
                }
                count ++;
                continue;
            }
            LOG_ERROR("net_recv, recv errno %d.\n",err);
        } else if(bytesRead == 0) {
            LOG_ERROR("net_recv, recv 0, peer closed\n");
        }
        return bytesRead;
    }
#endif
}

int net_send_to(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, const void* to, TSocketSize tolen)
{
    while (1) {
        if (sendto(fd, (const char*)data, len, flag, (const struct sockaddr *)to, (socklen_t)tolen) < 0) {
#ifdef BUILD_OS_WINDOWS
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                continue;
            }
#else
            int err = errno;
            if(err == EINTR || err == EAGAIN || err == EWOULDBLOCK){
                continue;
            }
#endif
            LOG_ERROR("sendto error, error = %d\n", err);
            return -1;
        }
        return len;
    }
}

int net_recv_from(TSocketHandler fd, unsigned char* data, int len, unsigned int flag, void* from, TSocketSize* fromlen)
{
    int recv_len = 0;
    while (1) {
        recv_len = recvfrom(fd, (char*)data, len, flag, (struct sockaddr*)from, (socklen_t*)fromlen);
        if (recv_len < 0) {
#ifdef BUILD_OS_WINDOWS
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                continue;
            }
#else
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                continue;
            }
#endif
            return -1;
        }
        return recv_len;
    }
}

void socket_set_timeout(TSocketHandler fd, int seconds, int u_secodns)
{
#ifdef BUILD_OS_WINDOWS
    int timeout = seconds * 1000 + (u_secodns / 1000);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout = {seconds, u_secodns};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
#endif
}

int socket_set_send_buffer_size(TSocketHandler fd, int size)
{
    int param_size = size;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&param_size, sizeof(param_size)) < 0) {
        LOG_FATAL("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
        return (-1);
    }
    return 0;
}

int socket_set_recv_buffer_size(TSocketHandler fd, int size)
{
    int param_size = size;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&param_size, sizeof(param_size)) < 0) {
        LOG_FATAL("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
        return (-1);
    }
    return 0;
}

int socket_set_nonblocking(TSocketHandler sock)
{
#ifdef BUILD_OS_WINDOWS
    unsigned long ul = 0;
    int ret = ioctlsocket(sock, FIONBIO, (unsigned long*)&ul);
    if (ret == SOCKET_ERROR) {
        LOG_FATAL("ioctlsocket fail\n");
        closesocket (sock);
        return (-1);
    }
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOG_FATAL("fcntl fail\n");
        return (-2);
    }
    if (fcntl(sock, F_SETFL, flags |O_NONBLOCK) != 0) {
        LOG_FATAL("fcntl(non block) fail\n");
        return (-3);
    }
#endif
    return 0;
}

int socket_set_no_delay(TSocketHandler fd)
{
    int tmp = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&tmp, sizeof(tmp)) < 0) {
        LOG_FATAL("setsockopt(no delay) fail\n");
        return (-1);
    }
    return 0;
}

int socket_set_linger(TSocketHandler fd, int onoff, int linger, int delay)
{
    struct linger linger_c;
    linger_c.l_onoff = onoff;
    linger_c.l_linger = linger;
    if (setsockopt(fd, SOL_SOCKET,SO_LINGER, (const char *)&linger_c, sizeof(linger_c)) < 0) {
        LOG_FATAL("setsockopt(linger) fail\n");
        return (-1);
    }
    int param_delay = delay;
    if (setsockopt(fd, IPPROTO_TCP, 0x4002, (const char *)&param_delay, sizeof(param_delay)) < 0) {
        LOG_FATAL("setsockopt(IPPROTO_TCP delay) fail\n");
        return (-2);
    }

    return 0;
}

int socket_set_defer_accept(TSocketHandler fd, int param)
{
#ifndef BUILD_OS_WINDOWS
    int val = param;
    setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));
#endif
    return 0;
}

int socket_set_quickACK(TSocketHandler fd, int val)
{
#ifndef BUILD_OS_WINDOWS
    int param = val;
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (void *)&param, sizeof(param));
#endif
    return 0;
}

TSocketHandler net_setup_stream_socket(unsigned int localAddr,  TSocketPort localPort, unsigned char makeNonBlocking)
{
    struct sockaddr_in  servaddr;
    TSocketHandler newSocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr =  htonl(localAddr);
    servaddr.sin_port  = htons(localPort);

    if (newSocket < 0) {
        LOG_ERROR("unable to create stream socket\n");
        return newSocket;
    }

    int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&reuseFlag, sizeof reuseFlag) < 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) error\n");
        net_close_socket(newSocket);
        return -1;
    }

    if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        LOG_ERROR("bind() error (port number: %d)\n", localPort);
        net_close_socket(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        int err = socket_set_nonblocking(newSocket);
        if (err) {
            LOG_ERROR("set nonblock fail\n");
            return -1;
        }
    }

    if (listen(newSocket, 20) < 0) {
        LOG_ERROR("listen() failed\n");
        return -1;
    }

    return newSocket;
}

TSocketHandler net_setup_datagram_socket(unsigned int localAddr,  TSocketPort localPort, unsigned char makeNonBlocking, unsigned int request_receive_buffer_size, unsigned int request_send_buffer_size)
{
    struct sockaddr_in  servaddr;

    TSocketHandler newSocket = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr    =  htonl(localAddr);
    servaddr.sin_port   =   htons(localPort);

    if (newSocket < 0) {
        LOG_ERROR("unable to create stream socket\n");
        return newSocket;
    }

    int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&reuseFlag, sizeof reuseFlag) < 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) error\n");
        net_close_socket(newSocket);
        return -1;
    }

    if (request_receive_buffer_size) {
        int  tmp = request_receive_buffer_size;

        socklen_t optlen = sizeof(tmp);
        getsockopt(newSocket, SOL_SOCKET, SO_RCVBUF, (char*)&tmp, &optlen);
    }

    if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        LOG_ERROR("bind() error (port number: %d)\n", localPort);
        net_close_socket(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        int err = socket_set_nonblocking(newSocket);
        if (err) {
            LOG_ERROR("set nonblock fail\n");
            return -1;
        }
    }

    return newSocket;
}

TSocketHandler net_accept(TSocketHandler fd, void* client_addr, TSocketSize* clilen, int* ret_err)
{
    TSocketHandler accept_fd = accept(fd, (struct sockaddr*)client_addr, (socklen_t*)clilen);
    if (accept_fd < 0) {
#ifdef BUILD_OS_WINDOWS
        int err = WSAGetLastError();
        if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
            *ret_err = 1;
            return accept_fd;
        }
        LOG_ERROR("accept fail, errno %d.\n", err);
#else
        int last_err = errno;
        if (last_err == EINTR || last_err == EAGAIN || last_err == EWOULDBLOCK) {
            LOG_WARN("non-blocking call?(err == %d)\n", last_err);
            *ret_err = 1;
            return accept_fd;
        }
        perror("accept");
        LOG_ERROR("accept fail, errno %d.\n", last_err);
#endif
        *ret_err = (-1);
        return accept_fd;
    }

    *ret_err = 0;
    return accept_fd;
}

int network_init()
{
#ifdef BUILD_OS_WINDOWS
    WSADATA Ws;
    if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
        LOG_ERROR("[error]: WSAStartup fail\n");
        return (-1);
    }
#endif

    return 0;
}

void network_deinit()
{
#ifdef BUILD_OS_WINDOWS
    WSACleanup();
#endif
}

#ifdef BUILD_OS_WINDOWS
static int __pipe_by_socket(TSocketHandler fildes[2])
{
    TSocketHandler tcp1, tcp2;
    sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int namelen = sizeof(name);
    tcp1 = tcp2 = -1;
    int tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp == -1){
        goto clean;
    }
    if (bind(tcp, (sockaddr*)&name, namelen) == -1){
        goto clean;
    }
    if (listen(tcp, 5) == -1){
        goto clean;
    }
    if (getsockname(tcp, (sockaddr*)&name, &namelen) == -1){
        goto clean;
    }
    tcp1 = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp1 == -1){
        goto clean;
    }
    if (-1 == connect(tcp1, (sockaddr*)&name, namelen)){
        goto clean;
    }
    tcp2 = accept(tcp, (sockaddr*)&name, &namelen);
    if (tcp2 == -1){
        goto clean;
    }
    if (closesocket(tcp) == -1){
        goto clean;
    }
    fildes[0] = tcp1;
    fildes[1] = tcp2;
    return 0;
clean:
    if (tcp != -1){
        closesocket(tcp);
    }
    if (tcp2 != -1){
        closesocket(tcp2);
    }
    if (tcp1 != -1){
        closesocket(tcp1);
    }
    return -1;
}

int pipe_write(TSocketHandler fd, const char byte)
{
    size_t ret = 0;

    ret = send(fd, &byte, 1, 0);
    if (1 == ret) {
        return 0;
    }

    LOG_ERROR("write fail, %d\n", ret);
    return (-1);
}

int pipe_read(TSocketHandler fd, char* byte)
{
    size_t ret = 0;

    ret = recv(fd, byte, 1, 0);
    if (1 == ret) {
        return 0;
    }

    LOG_ERROR("read fail, %d\n", ret);
    return (-1);
}

int pipe_create(TSocketHandler fd[2])
{
    int ret = __pipe_by_socket(fd);
    if (0 == ret) {
        return 0;
    }

    LOG_ERROR("pipe fail, %d\n", ret);
    return (-1);
}

void pipe_close(TSocketHandler fd)
{
    closesocket(fd);
}

#else

int pipe_write(TSocketHandler fd, const char byte)
{
    size_t ret = 0;
    ret = write(fd, &byte, 1);
    if (1 == ret) {
        return 0;
    }

    LOG_ERROR("write fail, %d\n", ret);
    return (-1);
}

int pipe_read(TSocketHandler fd, char* byte)
{
    size_t ret = 0;
    ret = read(fd, byte, 1);
    if (1 == ret) {
        return 0;
    }

    LOG_ERROR("read fail, %d\n", ret);
    return (-1);
}

int pipe_create(TSocketHandler fd[2])
{
    int ret = pipe(fd);
    if (0 == ret) {
        return 0;
    }

    LOG_ERROR("pipe fail, %d\n", ret);
    return (-1);
}

void pipe_close(TSocketHandler fd)
{
    close(fd);
}
#endif

