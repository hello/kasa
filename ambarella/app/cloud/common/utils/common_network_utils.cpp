/*******************************************************************************
 * common_network_utils.cpp
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_network_utils.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

void gfNetCloseSocket(TSocketHandler socket)
{
#ifdef BUILD_OS_WINDOWS
    closesocket(socket);
#else
    close(socket);
#endif
}

static int connect_nonblock(TSocketHandler sockfd, struct sockaddr *saptr, socklen_t salen, int nsec)
{
#ifdef BUILD_OS_WINDOWS
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long *)&ul);
    if (ret == SOCKET_ERROR) {
        return (-1);
    }

    ret = connect(sockfd, saptr, salen);

    struct timeval timeout ;
    fd_set r;

    FD_ZERO(&r);
    FD_SET(sockfd, &r);
    timeout.tv_sec = nsec;
    timeout.tv_usec = 0;
    ret = select(0, 0, &r, 0, &timeout);
    if (ret <= 0) {
        closesocket(sockfd);
        return (-5);
    }

    ul = 0;
    ret = ioctlsocket(sockfd, FIONBIO, (unsigned long *)&ul);
    if (ret == SOCKET_ERROR) {
        closesocket(sockfd);
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
    if ((n = connect(sockfd, saptr, salen)) < 0)
        if (errno != EINPROGRESS)
        { return (-1); }

    /* Do whatever we want while the connect is taking place. */
    if (n == 0)
    { goto done; } /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ((n = select(sockfd + 1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) {
        gfNetCloseSocket(sockfd); /* timeout */
        errno = ETIMEDOUT;
        return (-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            return (-1); /* Solaris pending error */
        }
    } else {
        //err_quit("select error: sockfd not set");
        return -1;
    }

done:
    fcntl(sockfd, F_SETFL, flags); /* restore file status flags */

    if (error) {
        errno = error;
        return (-1);
    }
#endif

    return 0;
}

TSocketHandler gfNet_ConnectTo(const TChar *host_addr, TU16 port, TInt type, TInt protocol)
{
    TSocketHandler fd = -1;
    TInt ret = 0;
    //struct sockaddr_in localhost_addr;
    struct sockaddr_in dest_addr;

    //only implement TCP
    DASSERT(host_addr);
    DASSERT(SOCK_STREAM == type);
    DASSERT(IPPROTO_TCP == protocol);
    type = SOCK_STREAM;
    protocol = IPPROTO_TCP;

    //create socket
    fd = socket(AF_INET, type, protocol);
    if (DUnlikely(!DIsSocketHandlerValid(fd))) {
        perror("socket()");
        LOG_ERROR("create socket, (%d) fail, type %d, protocol %d.\n", fd, type, protocol);
        return (-1);
    }

    //connect to dest addr
    memset(&dest_addr, 0x0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host_addr);
    //bzero(&(dest_addr.sin_zero), 8);
    memset(&(dest_addr.sin_zero), 0x0, 8);

    ret = connect_nonblock(fd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in), 5);
    if (ret < 0) {
        LOG_ERROR("connect() fail, ret %d.\n", ret);
        gfNetCloseSocket(fd);
        return (-1);
    }

    return fd;
}

TInt gfNet_Send(TSocketHandler fd, TU8 *data, TInt len, TUint flag)
{
#ifdef BUILD_OS_WINDOWS
    TInt sent = 0, remain = len;
    do {
        sent = send(fd, (const char *)data, remain, flag);
        if (DUnlikely(sent < 0)) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) { continue; }
            if (err == WSAEWOULDBLOCK) {
                Sleep(20);
                continue;
            }
            LOG_ERROR("gfNet_Send failed, errno %d\n", err);
            return err;
        } else if (!sent) {
            LOG_NOTICE("peer close\n");
            return 0;
        }
        remain -= sent;
        data += sent;
        //LOG_NOTICE("sent %d\n", sent);
    } while (remain > 0);
    return len;
#else
    int bytes = 0;
    while (bytes < len) {
        int ret = send(fd, data + bytes, len - bytes, flag);
        if (DUnlikely(ret < 0)) {
            int err = errno;
            if (err == EINTR) { continue; }
            if (err == EAGAIN || err == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
            LOG_ERROR("gfNet_Send failed, errno %d\n", err);
            return 0;
        }
        bytes += ret;
    }
    //LOG_NOTICE("!!!gfNet_Send %d\n", len);
    return len;
#endif
}

TInt gfNet_Send_timeout(TSocketHandler fd, TU8 *data, TInt len, TUint flag, TInt max_count)
{
#ifdef BUILD_OS_WINDOWS
    int bytes = 0;
    TInt count = 0;
    while (bytes < len) {
        int ret = send(fd, (const char *)data + bytes, len - bytes, flag);
        if (DUnlikely(ret < 0)) {
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                if (DUnlikely(count >= max_count)) {
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
    TInt current_count = 0;
    //LOG_NOTICE("!!!gfNet_Send_timeout %d\n", len);
    while (bytes < len) {
        int ret = send(fd, data + bytes, len - bytes, flag);
        if (ret < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                if (DUnlikely(current_count >= max_count)) {
                    LOG_ERROR("timeout current_count %d, max_count %d\n", current_count, max_count);
                    return ret;
                    //break;
                }
                current_count ++;
                //usleep(1000);
                continue;
            }
            LOG_ERROR("gfNet_Send failed, errno %d\n", err);
            return 0;
        }
        bytes += ret;
    }
    return len;
#endif
}

TInt gfNet_Recv(TSocketHandler fd, TU8 *data, TInt len, TUint flag)
{
#ifdef BUILD_OS_WINDOWS
    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        TInt received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char *)data, remain, flag);
            if (DUnlikely(received < 0)) {
                int err = WSAGetLastError();
                if (err == WSAEINTR) { continue; }
                if (err == WSAEWOULDBLOCK) {
                    Sleep(20);
                    continue;
                }
                LOG_ERROR("gfNet_Recv, recv errno %d.\n", err);
                return received;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
            //LOG_NOTICE("received %d\n", received);
        } while (remain > 0);
        return len;
    }

    TInt ret = recv(fd, (char *)data, len, flag);
    //gfWriteLogStringInterger("!!!!! read %d\n", ret);
    return ret;
#else
    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        TInt received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            //LOG_NOTICE("before recv, remain %d\n", remain);
            received = recv(fd, (char *)data, remain, flag);
            //LOG_NOTICE("after recv, received %d\n", received);
            if (DUnlikely(received < 0)) {
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

    //LOG_NOTICE("recv without flag, len %d\n", len);
    //return recv(fd, (void*)data, len, flag);
    while (1) {
        int bytesRead = recv(fd, (void *)data, len, flag);
        if (bytesRead < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                //usleep(1000);
                continue;
            }
            LOG_ERROR("gfNet_Recv, recv errno %d.\n", err);
        } else if (bytesRead == 0) {
            LOG_ERROR("gfNet_Recv, recv 0, peer closed\n");
        }
        return bytesRead;
    }
#endif
}

TInt gfNet_Recv_timeout(TSocketHandler fd, TU8 *data, TInt len, TUint flag, TInt max_count)
{
#ifdef BUILD_OS_WINDOWS
    TInt count = 0;

    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        TInt received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            received = recv(fd, (char *)data, remain, flag);
            if (DUnlikely(received < 0)) {
                int err = WSAGetLastError();
                if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                    if (DUnlikely(count >= max_count)) {
                        if (remain < len) {
                            //gfWriteLogStringInterger("!!!!! read4: %d\n", (len - remain));
                            return (len - remain);
                        }
                        //gfWriteLogStringInterger("!!!!! read3: %d\n", received);
                        return received;
                    }
                    count ++;
                    continue;
                }
                LOG_ERROR("gfNet_Recv, recv errno %d.\n", err);
                return received;
            } else if (!received) {
                LOG_NOTICE("peer close\n");
                return 0;
            }
            remain -= received;
            data += received;
        } while (remain > 0);
        //gfWriteLogStringInterger("!!!!! read1: %d\n", len);
        return len;
    }

    //LOG_NOTICE("recv without flag, len %d\n", len);
    //return recv(fd, (void*)data, len, flag);
    while (1) {
        TInt bytesRead = recv(fd, (char *)data, len, flag);
        if (DUnlikely(bytesRead < 0)) {
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                if (DUnlikely(count >= max_count)) {
                    return bytesRead;
                }
                count ++;
                continue;
            }
            LOG_ERROR("gfNet_Recv, recv errno %d.\n", err);
            return bytesRead;
        } else if (!bytesRead) {
            LOG_NOTICE("peer close\n");
            return 0;
        }
        //gfWriteLogStringInterger("!!!!! read2: %d\n", bytesRead);
        return bytesRead;
    }
#else
    TInt count = 0;

    if (flag & DNETUTILS_RECEIVE_FLAG_READ_ALL) {
        TInt received = 0, remain = len;
        flag &= ~DNETUTILS_RECEIVE_FLAG_READ_ALL;
        do {
            //LOG_NOTICE("before recv, remain %d\n", remain);
            received = recv(fd, (char *)data, remain, flag);
            //LOG_NOTICE("after recv, received %d\n", received);
            if (DUnlikely(received < 0)) {
                int err = errno;
                if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                    if (DUnlikely(count >= max_count)) {
                        if (remain < len) {
                            return (len - remain);
                        }
                        return received;
                    }
                    //usleep(1000);
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

    //LOG_NOTICE("recv without flag, len %d\n", len);
    //return recv(fd, (void*)data, len, flag);
    while (1) {
        int bytesRead = recv(fd, (void *)data, len, flag);
        if (bytesRead < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                if (DUnlikely(count >= max_count)) {
                    return bytesRead;
                }
                count ++;
                //usleep(1000);
                continue;
            }
            LOG_ERROR("gfNet_Recv, recv errno %d.\n", err);
        } else if (bytesRead == 0) {
            LOG_ERROR("gfNet_Recv, recv 0, peer closed\n");
        }
        return bytesRead;
    }
#endif
}

TInt gfNet_SendTo(TSocketHandler fd, TU8 *data, TInt len, TUint flag, const void *to, TSocketSize tolen)
{
    while (1) {
        if (sendto(fd, (const char *)data, len, flag, (const struct sockaddr *)to, (socklen_t)tolen) < 0) {
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
            LOG_ERROR("sendto error, error = %d\n", err);
            return -1;
        }
        return len;
    }
}

TInt gfNet_RecvFrom(TSocketHandler fd, TU8 *data, TInt len, TUint flag, void *from, TSocketSize *fromlen)
{
    TInt recv_len = 0;
    while (1) {
        recv_len = recvfrom(fd, (char *)data, len, flag, (struct sockaddr *)from, (socklen_t *)fromlen);
        if (recv_len < 0) {
#ifdef BUILD_OS_WINDOWS
            int err = WSAGetLastError();
            if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
                continue;
            }
#else
            TInt err = errno;
            if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
                continue;
            }
#endif
            return -1;
        }
        return recv_len;
    }
}

void gfSocketSetTimeout(TSocketHandler fd, TInt seconds, TInt u_secodns)
{
#ifdef BUILD_OS_WINDOWS
    TInt timeout = seconds * 1000 + (u_secodns / 1000);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval timeout = {seconds, u_secodns};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
#endif
}

EECode gfSocketSetSendBufferSize(TSocketHandler fd, TInt size)
{
    TInt param_size = size;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&param_size, sizeof(param_size)) < 0) {
        LOG_FATAL("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
        return EECode_Error;
    }
    return EECode_OK;
}

EECode gfSocketSetRecvBufferSize(TSocketHandler fd, TInt size)
{
    TInt param_size = size;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&param_size, sizeof(param_size)) < 0) {
        LOG_FATAL("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
        return EECode_Error;
    }
    return EECode_OK;
}

EECode gfSocketSetNonblocking(TSocketHandler sock)
{
#ifdef BUILD_OS_WINDOWS
    unsigned long ul = 0;
    int ret = ioctlsocket(sock, FIONBIO, (unsigned long *)&ul);
    if (ret == SOCKET_ERROR) {
        closesocket(sock);
        return EECode_Error;
    }
#else
    TInt flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        return EECode_NoPermission;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != 0) {
        return EECode_Error;
    }
#endif
    return EECode_OK;
}

EECode gfSocketSetNoDelay(TSocketHandler fd)
{
    TInt tmp = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&tmp, sizeof(tmp)) < 0) {
        return EECode_Error;
    }
    return EECode_OK;
}

EECode gfSocketSetLinger(TSocketHandler fd, TInt onoff, TInt linger, TInt delay)
{
    struct linger linger_c;
    linger_c.l_onoff = onoff;
    linger_c.l_linger = linger;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (const TChar *)&linger_c, sizeof(linger_c)) < 0) {
        return EECode_Error;
    }
    TInt param_delay = delay;
    if (setsockopt(fd, IPPROTO_TCP, 0x4002, (const TChar *)&param_delay, sizeof(param_delay)) < 0) {
        return EECode_Error;
    }

    return EECode_OK;
}

TSocketHandler gfNet_SetupStreamSocket(TU32 localAddr,  TSocketPort localPort, TU8 makeNonBlocking)
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
                   (const char *)&reuseFlag, sizeof reuseFlag) < 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) error\n");
        gfNetCloseSocket(newSocket);
        return -1;
    }

    if (bind(newSocket, (struct sockaddr *)&servaddr, sizeof servaddr) != 0) {
        LOG_ERROR("bind() error (port number: %d)\n", localPort);
        gfNetCloseSocket(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        EECode err = gfSocketSetNonblocking(newSocket);
        if (EECode_OK != err) {
            return -1;
        }
    }

#if 0
    TInt requestedSize = 50 * 1024;
    if (setsockopt(newSocket, SOL_SOCKET, SO_SNDBUF,
                   &requestedSize, sizeof requestedSize) != 0) {
        LOG_ERROR("failed to set send buffer size\n");
        gfNetCloseSocket(newSocket);
        return -1;
    }
#endif

    if (listen(newSocket, 20) < 0) {
        LOG_ERROR("listen() failed\n");
        return -1;
    }

    return newSocket;
}

TSocketHandler gfNet_SetupDatagramSocket(TU32 localAddr,  TSocketPort localPort, TU8 makeNonBlocking, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
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
                   (const char *)&reuseFlag, sizeof reuseFlag) < 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) error\n");
        gfNetCloseSocket(newSocket);
        return -1;
    }

    if (request_receive_buffer_size) {
        int  tmp = request_receive_buffer_size;
        //setsockopt(newSocket, SOL_SOCKET, SO_RCVBUFFORCE, &tmp, sizeof(tmp));

        socklen_t optlen = sizeof(tmp);
        getsockopt(newSocket, SOL_SOCKET, SO_RCVBUF, (char *)&tmp, &optlen);
        //LOG_NOTICE("set SO_RCVBUF = %d, request_receive_buffer_size %d\n", tmp, request_receive_buffer_size);
    }

    if (bind(newSocket, (struct sockaddr *)&servaddr, sizeof servaddr) != 0) {
        LOG_ERROR("bind() error (port number: %d)\n", localPort);
        gfNetCloseSocket(newSocket);
        return -1;
    }

    if (makeNonBlocking) {
        EECode err = gfSocketSetNonblocking(newSocket);
        if (EECode_OK != err) {
            return -1;
        }
    }

    return newSocket;
}

TInt gfNetDirect_Recv(TSocketHandler fd, TU8 *data, TInt len, EECode &err_code)
{
    TInt received = recv(fd, (char *)data, len, 0);
    if (DUnlikely(0 > received)) {
#ifdef BUILD_OS_WINDOWS
        int err = WSAGetLastError();
        if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
            err_code = EECode_Busy;
            return 0;
        }
        if ((err == WSAETIMEDOUT)) {
            err_code = EECode_Busy;
            return 0;
        }
#else
        int err = errno;
        if (DLikely(err == EINTR || err == EAGAIN || err == EWOULDBLOCK)) {
            err_code = EECode_Busy;
            return 0;
        }
#endif
        LOG_ERROR("err %d\n", err);
        err_code = EECode_NetSocketRecv_Error;
        return received;
    } else if (DUnlikely(!received)) {
        err_code = EECode_Closed;
        return 0;
    }

    return received;
}

TSocketHandler gfNet_Accept(TSocketHandler fd, void *client_addr, TSocketSize *clilen, EECode &err)
{
    TSocketHandler accept_fd = accept(fd, (struct sockaddr *)client_addr, (socklen_t *)clilen);
    if (accept_fd < 0) {
#ifdef BUILD_OS_WINDOWS
        int err = WSAGetLastError();
        if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
            err = EECode_TryAgain;
            return accept_fd;
        }
        LOG_ERROR("accept fail, errno %d.\n", err);
#else
        TInt last_err = errno;
        if (last_err == EINTR || last_err == EAGAIN || last_err == EWOULDBLOCK) {
            LOG_WARN("non-blocking call?(err == %d)\n", last_err);
            err = EECode_TryAgain;
            return accept_fd;
        }
        perror("accept");
        LOG_ERROR("accept fail, errno %d.\n", last_err);
#endif
        err = EECode_Error;
        return accept_fd;
    }

    err = EECode_OK;
    return accept_fd;
}

EECode gfSocketSetDeferAccept(TSocketHandler fd, TInt param)
{
#ifndef BUILD_OS_WINDOWS
    TInt val = param;
    setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));
#endif
    return EECode_OK;
}

EECode gfSocketSetQuickACK(TSocketHandler fd, TInt val)
{
#ifndef BUILD_OS_WINDOWS
    TInt param = val;
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (void *)&param, sizeof(param));
#endif
    return EECode_OK;
}

EECode gfInitPlatformNetwork()
{
#ifdef BUILD_OS_WINDOWS
    WSADATA Ws;
    if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
        LOG_ERROR("[error]: WSAStartup fail\n");
        return EECode_OSError;
    }
#endif

    return EECode_OK;
}

void gfDeInitPlatformNetwork()
{
#ifdef BUILD_OS_WINDOWS
    WSACleanup();
#endif
}

ITCPPulseSender *gfCreateTCPPulseSender()
{
#ifdef DCONFIG_COMPILE_OBSOLETE
    return CITCPPulseSender::Create();
#else
    LOG_FATAL("not compile module\n");
    return NULL;
#endif
}

#ifdef DCONFIG_COMPILE_OBSOLETE

//-----------------------------------------------------------------------
//
// CITCPPulseSender
//
//-----------------------------------------------------------------------
class CITCPPulseSender: public CObject, public IActiveObject, public ITCPPulseSender
{
    typedef CObject inherited;

public:
    static CITCPPulseSender *Create();
    virtual void Destroy();

protected:
    CITCPPulseSender();
    virtual ~CITCPPulseSender();
    EECode Construct();

public:
    virtual STransferDataChannel *AddClient(TInt fd, TMemSize max_send_buffer_size = (1024 * 1024), TInt framecount = 60);
    virtual void RemoveClient(STransferDataChannel *channel);

    virtual EECode SendData(STransferDataChannel *channel, TU8 *pdata, TMemSize data_size);

public:
    virtual EECode Start();
    virtual EECode Stop();

protected:
    virtual void OnRun();

private:
    void processCmd(SCMD &cmd);
    void removeClient(STransferDataChannel *channel);
    STransferDataChannel *allocDataChannel(TMemSize size, TInt framecount);
    void releaseDataChannel(STransferDataChannel *data_channel);
    void purgeChannel(STransferDataChannel *channel);
    EECode sendDataPiece(TInt fd, TU8 *p_data, TInt data_size);
    EECode tryAllocDataPiece(STransferDataChannel *pchannel, TU8 *p_data, TMemSize data_size);
    EECode sendDataChannel(STransferDataChannel *channel);
    void deleteDataChannel(STransferDataChannel *channel);
    void deleteAllDataChannel();

private:
    IMutex *mpMutex;
    CIWorkQueue *mpWorkQueue;

private:
    CIDoubleLinkedList mTransferDataChannelList;
    CIDoubleLinkedList mTransferDataFreeList;

private:
    TInt mPipeFd[2];
    TInt mMaxFd;

    fd_set mAllReadSet;
    fd_set mAllWriteSet;
    fd_set mReadSet;
    fd_set mWriteSet;

private:
    TU8 mbRun;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

    EPulseSenderState msState;

private:
    TU32 mDebugHeartBeat;
};

CITCPPulseSender *CITCPPulseSender::Create()
{
    CITCPPulseSender *result = new CITCPPulseSender();
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CITCPPulseSender::Destroy()
{
    Delete();
}

CITCPPulseSender::CITCPPulseSender()
    : inherited("CITCPPulseSender")
    , mpMutex(NULL)
    , mpWorkQueue(NULL)
    , mMaxFd(0)
    , mbRun(1)
    , msState(EPulseSenderState_idle)
{
    mPipeFd[0] = -1;
    mPipeFd[1] = -1;
}

CITCPPulseSender::~CITCPPulseSender()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpWorkQueue) {
        mpWorkQueue->Delete();
        mpWorkQueue = NULL;
    }

    deleteAllDataChannel();
}

EECode CITCPPulseSender::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_FATAL("CITCPPulseSender::Contruct: CIWorkQueue::Create fail\n");
        return EECode_NoMemory;
    }

    TInt ret = pipe(mPipeFd);
    DASSERT(0 == ret);

    LOGM_INFO("before CITCPPulseSender:: mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    EECode err = mpWorkQueue->Run();
    DASSERT_OK(err);
    LOGM_INFO("after CITCPPulseSender:: mpWorkQueue->Run().\n");

    //init
    FD_ZERO(&mAllReadSet);
    FD_ZERO(&mAllWriteSet);
    FD_ZERO(&mReadSet);
    FD_ZERO(&mWriteSet);

    FD_SET(mPipeFd[0], &mAllReadSet);
    mMaxFd = mPipeFd[0];

    return EECode_OK;
}

STransferDataChannel *CITCPPulseSender::AddClient(TInt fd, TMemSize max_send_buffer_size, TInt framecount)
{
    STransferDataChannel *channel = NULL;
    AUTO_LOCK(mpMutex);

    channel = allocDataChannel(max_send_buffer_size, framecount);

    if (DLikely(channel)) {
        channel->fd = fd;
    } else {
        LOGM_FATAL("allocDataChannel(%ld, %d) fail\n", max_send_buffer_size, framecount);
        return NULL;
    }

    channel->is_closed = 0;
    channel->has_socket_error = 0;
    mTransferDataChannelList.InsertContent(NULL, (void *) channel, 1);

    FD_SET(fd, &mAllWriteSet);
    if (fd > mMaxFd) {
        mMaxFd = fd;
    }

    TChar wake_char = 'c';
    size_t ret = 0;

    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    mpWorkQueue->PostMsg(ECMDType_AddClient);

    return channel;
}

void CITCPPulseSender::RemoveClient(STransferDataChannel *channel)
{
    DASSERT(channel);

    if (DLikely(!channel->is_closed)) {
        SCMD cmd;

        cmd.code = ECMDType_RemoveClient;
        cmd.needFreePExtra = 0;
        cmd.pExtra = (void *)channel;

        TChar wake_char = 'c';
        size_t ret = 0;

        ret = write(mPipeFd[1], &wake_char, 1);
        DASSERT(1 == ret);

        mpWorkQueue->SendCmd(cmd);
    } else {
        LOGM_ERROR("already removed!\n");
    }

}

EECode CITCPPulseSender::SendData(STransferDataChannel *channel, TU8 *pdata, TMemSize data_size)
{
    //debug assert
    DASSERT(channel);
    DASSERT(pdata);
    DASSERT(data_size);

    AUTO_LOCK(mpMutex);
    if (DUnlikely(channel->is_closed)) {
        LOGM_ERROR("should not comes here\n");
        return EECode_Error;
    } else if (DUnlikely(channel->has_socket_error)) {
        return EECode_Closed;
    }

    EECode err = tryAllocDataPiece(channel, pdata, data_size);

    if (DUnlikely(EECode_OK != err)) {
        LOG_WARN("detected slow client, close it\n");
        return EECode_Closed;
    }

    return EECode_OK;
}

EECode CITCPPulseSender::Start()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CITCPPulseSender::Start() start.\n");

    TChar wake_char = 'a';
    size_t ret = 0;

    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("CITCPPulseSender::Start() end, ret %d.\n", err);
    return err;
}

EECode CITCPPulseSender::Stop()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CITCPPulseSender::Stop() start.\n");
    TChar wake_char = 'b';
    size_t ret = 0;

    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("CITCPPulseSender::Stop() end, ret %d.\n", err);
    return err;
}

void CITCPPulseSender::OnRun()
{
    SCMD cmd;
    TInt nready = 0;
    TInt write_count = 0;
    EECode err = EECode_OK;

    CIDoubleLinkedList::SNode *pnode = NULL;
    STransferDataChannel *pchannel = NULL;

    mpWorkQueue->CmdAck(EECode_OK);

    while (mbRun) {

        LOGM_STATE("CITCPPulseSender::OnRun start switch state %d, mbRun %d.\n", msState, mbRun);

        switch (msState) {

            case EPulseSenderState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EPulseSenderState_running:
                __LOCK(mpMutex);
                mReadSet = mAllReadSet;
                mWriteSet = mAllWriteSet;
                __UNLOCK(mpMutex);

                //LOGM_INFO("[CITCPPulseSender]: before select.\n");
                nready = select(mMaxFd + 1, &mReadSet, &mWriteSet, NULL, NULL);
                if (DLikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    perror("select");
                    msState = EPulseSenderState_error;
                    break;
                } else if (nready == 0) {
                    break;
                }

                //LOGM_INFO("[CITCPPulseSender]: after select, nready %d.\n", nready);
                //process cmd
                if (DUnlikely(FD_ISSET(mPipeFd[0], &mReadSet))) {
                    //LOGM_DEBUG("[CITCPPulseSender]: from pipe fd, nready %d, mpWorkQueue->MsgQ() %d.\n", nready, mpWorkQueue->MsgQ()->GetDataCnt());
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (EPulseSenderState_running != msState) {
                        LOGM_INFO(" transit from EPulseSenderState_running to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                write_count = 0;
                __LOCK(mpMutex);
                pnode = mTransferDataChannelList.FirstNode();
                while (pnode) {
                    pchannel = (STransferDataChannel *)pnode->p_context;

                    pnode = mTransferDataChannelList.NextNode(pnode);
                    DASSERT(pchannel);

                    if (DUnlikely(NULL == pchannel)) {
                        LOGM_FATAL("Fatal error(NULL == pchannel), must not get here.\n");
                        break;
                    }

                    if (DLikely(pchannel->has_socket_error)) {
                        continue;
                    }

                    if (FD_ISSET(pchannel->fd, &mWriteSet)) {
                        nready --;

                        err = sendDataChannel(pchannel);
                        if (EECode_OK == err) {
                            write_count ++;
                        } else if (EECode_NotRunning == err) {

                        } else if (EECode_Closed == err) {
                            pchannel->has_socket_error = 1;
                            LOG_WARN("peer close %d\n", pchannel->fd);
                        } else {
                            pchannel->has_socket_error = 1;
                            LOGM_FATAL("sendDataChannel return %s, %d\n", gfGetErrorCodeString(err), err);
                        }
                    }

                    if (nready <= 0) {
                        // done
                        break;
                    }
                }
                __UNLOCK(mpMutex);

                if (DUnlikely(!write_count)) {
                    LOGM_DEBUG("sender idle loop\n");
                    usleep(20000);
                }
                break;

            case EPulseSenderState_error:
                //todo
                LOGM_ERROR("NEED implement this case.\n");
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                mbRun = 0;
                LOGM_FATAL("bad state %d.\n", msState);
                break;
        }

        mDebugHeartBeat ++;

    }

}

void CITCPPulseSender::processCmd(SCMD &cmd)
{
    //LOGM_FLOW("cmd.code %d.\n", cmd.code);
    DASSERT(mpWorkQueue);
    TChar char_buffer;

    //bind with pipe fd
    gfOSReadPipe(mPipeFd[0], char_buffer);
    //LOGM_DEBUG("cmd.code %d, TChar %c.\n", cmd.code, char_buffer);

    switch (cmd.code) {
        case ECMDType_Start:
            msState = EPulseSenderState_running;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbRun = 0;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_AddClient:
            break;

        case ECMDType_RemoveClient:
            removeClient((STransferDataChannel *)cmd.pExtra);
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    //LOGM_FLOW("****CITCPPulseSender::ProcessCmd, cmd.code %d end.\n", cmd.code);
}

void CITCPPulseSender::removeClient(STransferDataChannel *channel)
{
    //DASSERT(channel);
    if (DLikely(!channel->is_closed)) {
        channel->is_closed = 1;
        channel->has_socket_error = 0;
        purgeChannel(channel);
        releaseDataChannel(channel);
        FD_CLR(channel->fd, &mAllWriteSet);
    } else {
        LOGM_ERROR("already removed!\n");
    }
}

STransferDataChannel *CITCPPulseSender::allocDataChannel(TMemSize size, TInt framecount)
{
    CIDoubleLinkedList::SNode *node = NULL;
    STransferDataChannel *new_channel = NULL;

    node = mTransferDataFreeList.FirstNode();
    if (!node) {
        //LOGM_DEBUG("alloc new STransferDataChannel\n");
        new_channel = (STransferDataChannel *) DDBG_MALLOC(sizeof(STransferDataChannel), "C0PS");
        if (DUnlikely(!new_channel)) {
            LOGM_FATAL("alloc(%lu) fail\n", (TULong)sizeof(STransferDataChannel));
            return NULL;
        }
        memset(new_channel, 0x0, sizeof(STransferDataChannel));
        new_channel->data_queue = CIQueue::Create(mpWorkQueue->MsgQ(), this, sizeof(SDataPiece *), framecount);
        if (DUnlikely(!new_channel->data_queue)) {
            LOGM_FATAL("CIQueue::Create fail\n");
            return NULL;
        }
        new_channel->ring_pool = CRingMemPool::Create(size);
        if (DUnlikely(!new_channel->ring_pool)) {
            LOGM_FATAL("CRingMemPool::Create(%ld) fail\n", size);
            return NULL;
        }
        new_channel->data_piece_pool = CISimplePool::Create(framecount);
        if (DUnlikely(!new_channel->data_piece_pool)) {
            LOGM_FATAL("CISimplePool::Create(60) fail\n");
            return NULL;
        }

        return new_channel;
    }

    mTransferDataFreeList.RemoveContent(node->p_context);

    new_channel = (STransferDataChannel *)node->p_context;
    return new_channel;
}

void CITCPPulseSender::releaseDataChannel(STransferDataChannel *data_channel)
{
    DASSERT(data_channel);

    mTransferDataChannelList.RemoveContent((void *) data_channel);
    mTransferDataFreeList.InsertContent(NULL, (void *) data_channel, 1);
}

void CITCPPulseSender::purgeChannel(STransferDataChannel *channel)
{
    SDataPiece *piece = channel->p_cached_datapiece;

    if (piece) {
        channel->ring_pool->ReleaseMemBlock(piece->data_size, piece->p_data);
        channel->data_piece_pool->ReleaseDataPiece(piece);
    }

    while (channel->data_queue->PeekData((void *) &piece, sizeof(SDataPiece *))) {
        channel->ring_pool->ReleaseMemBlock(piece->data_size, piece->p_data);
        channel->data_piece_pool->ReleaseDataPiece(piece);
    }
}

EECode CITCPPulseSender::sendDataPiece(TInt fd, TU8 *p_data, TInt data_size)
{
    TInt ret = send(fd, p_data, data_size, 0);
    if (DUnlikely(0 > ret)) {
        TInt err = errno;
        if (DUnlikely((EINTR == err) || (EAGAIN == err) || (EWOULDBLOCK == err))) {
            return EECode_TryAgain;
        }
        LOGM_ERROR("send failed, errno %d\n", err);
        return EECode_NetSocketSend_Error;
    } else if (DUnlikely(!ret)) {
        LOGM_ERROR("send failed, peer closed\n");
        return EECode_Closed;
    }

    DASSERT(ret == data_size);
    return EECode_OK;
}

EECode CITCPPulseSender::tryAllocDataPiece(STransferDataChannel *pchannel, TU8 *p_data, TMemSize data_size)
{
    if (DUnlikely(!pchannel->data_piece_pool->GetFreeDataPieceCnt())) {
        LOGM_ERROR("channel would be blocked!\n");
        return EECode_Busy;
    }

    TU8 *ptmp = pchannel->ring_pool->RequestMemBlock((TMemSize)data_size);
    memcpy(ptmp, p_data, data_size);

    SDataPiece *piece = NULL;
    pchannel->data_piece_pool->AllocDataPiece(piece, sizeof(SDataPiece *));

    DASSERT(piece);
    piece->data_size = data_size;
    piece->p_data = ptmp;

    pchannel->data_queue->PutData((void *)&piece, sizeof(SDataPiece *));

    return EECode_OK;
}

EECode CITCPPulseSender::sendDataChannel(STransferDataChannel *channel)
{
    if (DLikely(channel)) {
        EECode ret = EECode_OK;
        SDataPiece *piece = channel->p_cached_datapiece;
        if (piece) {
            ret = sendDataPiece(channel->fd, piece->p_data, (TInt)piece->data_size);

            if (DLikely(EECode_OK == ret)) {
                channel->p_cached_datapiece = NULL;
                channel->ring_pool->ReleaseMemBlock(piece->data_size, piece->p_data);
                channel->data_piece_pool->ReleaseDataPiece(piece);
                return EECode_OK;
            } else if (EECode_TryAgain == ret) {
                return EECode_NotRunning;
            }

            return EECode_Closed;
        } else if (DLikely(channel->data_queue)) {
            SDataPiece *piece = NULL;

            if (channel->data_queue->PeekData((void *) &piece, sizeof(SDataPiece *))) {
                EECode ret = sendDataPiece(channel->fd, piece->p_data, (TInt)piece->data_size);

                if (DLikely(EECode_OK == ret)) {
                    channel->ring_pool->ReleaseMemBlock(piece->data_size, piece->p_data);
                    channel->data_piece_pool->ReleaseDataPiece(piece);
                    return EECode_OK;
                } else if (EECode_TryAgain == ret) {
                    channel->p_cached_datapiece = piece;
                    return EECode_NotRunning;
                }
                channel->p_cached_datapiece = piece;
                return EECode_Closed;
            } else {
                return EECode_NotRunning;
            }
        } else {
            LOGM_FATAL("NULL channel->data_queue\n");
            return EECode_InternalLogicalBug;
        }
    } else {
        LOGM_FATAL("NULL channel\n");
        return EECode_InternalLogicalBug;
    }

}

void CITCPPulseSender::deleteDataChannel(STransferDataChannel *channel)
{
    if (channel) {
        if (channel->data_piece_pool) {
            channel->data_piece_pool->Delete();
            channel->data_piece_pool = NULL;
        }

        if (channel->ring_pool) {
            channel->ring_pool->Delete();
            channel->ring_pool = NULL;
        }

        if (channel->data_queue) {
            channel->data_queue->Delete();
            channel->data_queue = NULL;
        }

        DDBG_FREE(channel, "C0PS");
    }
}

void CITCPPulseSender::deleteAllDataChannel()
{
    CIDoubleLinkedList::SNode *node = mTransferDataChannelList.FirstNode();
    STransferDataChannel *channel = NULL;

    while (node) {
        channel = (STransferDataChannel *)node->p_context;
        node = mTransferDataChannelList.NextNode(node);
        deleteDataChannel(channel);
    }

    node = mTransferDataFreeList.FirstNode();
    while (node) {
        channel = (STransferDataChannel *)node->p_context;
        node = mTransferDataFreeList.NextNode(node);
        deleteDataChannel(channel);
    }

}
#endif

#ifndef BUILD_OS_WINDOWS
//-----------------------------------------------------------------------
//
// CIGuardian
//
//-----------------------------------------------------------------------

class CIGuardian: public CObject, public IActiveObject, public IGuardian
{
public:
    static CIGuardian *Create();
    virtual CObject *GetObject() const;

protected:
    CIGuardian();
    EECode Construct();
    ~CIGuardian();

public:
    virtual EECode Start(TSocketPort port, TGuardianCallBack callback, void *context);
    virtual EECode Stop();

public:
    virtual void OnRun();

public:
    virtual void Destroy();

private:
    void processCmd(SCMD &cmd);

private:
    CIWorkQueue *mpWorkQueue;

private:
    TSocketHandler mListenSocket;
    TSocketHandler mPairingSocket;

    TSocketPort mPort;
    TU8 mbRun;
    TU8 mReserved0;

    TSocketHandler mMaxFd;
    TSocketHandler mPipeFd[2];

    fd_set mAllSet;
    fd_set mReadSet;

private:
    void *mpCallbackContext;
    TGuardianCallBack mfStateChangeCallback;
    EGuardianState msState;

};

CIGuardian *CIGuardian::Create()
{
    CIGuardian *thiz = new CIGuardian();
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DUnlikely(EECode_OK != err)) {
            delete thiz;
            LOG_FATAL("CIGuardian::Construct() fail, ret %d %s.\n", err, gfGetErrorCodeString(err));
            return NULL;
        }
        return thiz;
    }

    LOG_FATAL("new CIGuardian() fail.\n");
    return NULL;
}

CObject *CIGuardian::GetObject() const
{
    return (CObject *) this;
}

CIGuardian::CIGuardian()
    : CObject("CIGuardian", 0)
    , mpWorkQueue(NULL)
    , mListenSocket(-1)
    , mPairingSocket(-1)
    , mPort(0)
    , mbRun(1)
    , mMaxFd(-1)
    , mpCallbackContext(NULL)
    , mfStateChangeCallback(NULL)
    , msState(EGuardianState_idle)
{
    mPipeFd[0] = -1;
    mPipeFd[1] = -1;
}

EECode CIGuardian::Construct()
{
    TInt ret = 0;

    ret = pipe(mPipeFd);
    DASSERT(0 == ret);

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    LOGM_INFO("before CICommunicationServerPort::mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    LOGM_INFO("after CICommunicationServerPort::mpWorkQueue->Run().\n");

    return EECode_OK;
}

CIGuardian::~CIGuardian()
{
    if (DIsSocketHandlerValid(mListenSocket)) {
        gfNetCloseSocket(mListenSocket);
        mListenSocket = DInvalidSocketHandler;
    }

    if (DIsSocketHandlerValid(mPairingSocket)) {
        gfNetCloseSocket(mPairingSocket);
        mPairingSocket = DInvalidSocketHandler;
    }
}

EECode CIGuardian::Start(TSocketPort port, TGuardianCallBack callback, void *context)
{
    EECode err = EECode_OK;
    SCMD cmd;
    TChar wake_char = 'a';
    size_t ret = 0;

    mfStateChangeCallback = callback;
    mpCallbackContext = context;

    DASSERT(mpWorkQueue);

    LOGM_INFO("CIGuardian::Start() start.\n");
    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    cmd.code = ECMDType_Start;
    cmd.res32_1 = port;
    cmd.pExtra = NULL;
    cmd.needFreePExtra = 0;

    err = mpWorkQueue->SendCmd(cmd);
    LOGM_INFO("CIGuardian::Start end, ret %d.\n", err);
    return err;
}

EECode CIGuardian::Stop()
{
    EECode err = EECode_OK;
    SCMD cmd;
    TChar wake_char = 'b';
    size_t ret = 0;

    DASSERT(mpWorkQueue);

    LOGM_INFO("CIGuardian::Stop() start.\n");
    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    cmd.code = ECMDType_Stop;
    cmd.pExtra = NULL;
    cmd.needFreePExtra = 0;

    err = mpWorkQueue->SendCmd(cmd);
    LOGM_INFO("CIGuardian::Stop end, ret %d.\n", err);
    return err;
}

void CIGuardian::OnRun()
{
    SCMD cmd;
    TInt nready = 0;

    struct sockaddr_in client_addr;
    socklen_t   clilen = sizeof(struct sockaddr_in);

    FD_ZERO(&mAllSet);
    FD_ZERO(&mReadSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];
    msState = EGuardianState_idle;

    mpWorkQueue->CmdAck(EECode_OK);

    //signal(SIGPIPE,SIG_IGN);

    while (mbRun) {

        LOGM_STATE("CIGuardian::OnRun start switch state %d, mbRun %d.\n", msState, mbRun);

        switch (msState) {

            case EGuardianState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EGuardianState_wait_paring:
                mReadSet = mAllSet;

                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    msState = EGuardianState_error;
                    if (mpCallbackContext && mfStateChangeCallback) {
                        mfStateChangeCallback(mpCallbackContext, msState);
                    }
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (EGuardianState_wait_paring != msState) {
                        LOGM_INFO(" transit from EGuardianState_wait_paring to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                if (FD_ISSET(mListenSocket, &mReadSet)) {
                    nready --;

                    mPairingSocket = accept(mListenSocket, (struct sockaddr *)&client_addr, &clilen);
                    if (DUnlikely(0 > mPairingSocket)) {
                        LOGM_ERROR("accept fail, return %d, how to handle it?\n", mPairingSocket);
                        break;
                    } else {
                        LOGM_PRINTF(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), mPairingSocket);
                    }

                    FD_SET(mPipeFd[0], &mAllSet);
                    FD_SET(mListenSocket, &mAllSet);
                    if (mPipeFd[0] > mListenSocket) {
                        mMaxFd = mPipeFd[0];
                    } else {
                        mMaxFd = mListenSocket;
                    }
                    FD_SET(mPairingSocket, &mAllSet);
                    if (mPairingSocket > mMaxFd) {
                        mMaxFd = mPairingSocket;
                    }
                    msState = EGuardianState_guard;
                    if (mpCallbackContext && mfStateChangeCallback) {
                        mfStateChangeCallback(mpCallbackContext, msState);
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }
                break;

            case EGuardianState_guard:
                mReadSet = mAllSet;

                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    msState = EGuardianState_error;
                    if (mpCallbackContext && mfStateChangeCallback) {
                        mfStateChangeCallback(mpCallbackContext, msState);
                    }
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (EGuardianState_guard != msState) {
                        LOGM_INFO(" transit from EGuardianState_guard to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                if (FD_ISSET(mListenSocket, &mReadSet)) {
                    nready --;
                    LOGM_ERROR("someone else try connect to listen port(%d)?\n", mPort);
                    FD_CLR(mListenSocket, &mAllSet);
                    gfNetCloseSocket(mListenSocket);
                    mListenSocket = DInvalidSocketHandler;
                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                if (FD_ISSET(mPairingSocket, &mReadSet)) {
                    nready --;
                    LOGM_NOTICE("pairing socket has data comes\n");
                    TU8 read_data[16] = {0};
                    EECode err = EECode_OK;
                    TInt read = gfNetDirect_Recv(mPairingSocket, read_data, 8, err);
                    if (DLikely(!read)) {
                        DASSERT(EECode_Closed == err);
                        LOGM_NOTICE("peer closed\n");
                        FD_CLR(mPairingSocket, &mAllSet);
                        gfNetCloseSocket(mPairingSocket);
                        mPairingSocket = DInvalidSocketHandler;
                        if (0 > mListenSocket) {
                            mListenSocket = gfNet_SetupStreamSocket(INADDR_ANY, mPort, 0);
                            if (0 > mListenSocket) {
                                LOGM_ERROR("setup socket fail\n");
                                msState = EGuardianState_error;
                                if (mpCallbackContext && mfStateChangeCallback) {
                                    mfStateChangeCallback(mpCallbackContext, msState);
                                }
                                break;
                            }
                        }

                        msState = EGuardianState_wait_paring;
                        if (mpCallbackContext && mfStateChangeCallback) {
                            mfStateChangeCallback(mpCallbackContext, msState);
                        }
                        break;
                    } else {
                        LOGM_ERROR("why can read data here?\n");
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }
                break;

            case EGuardianState_halt:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EGuardianState_error:
                //todo
                LOGM_ERROR("NEED implement this case.\n");
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                mbRun = 0;
                LOGM_FATAL("bad state %d.\n", msState);
                break;
        }

    }

}

void CIGuardian::Destroy()
{
}

void CIGuardian::processCmd(SCMD &cmd)
{
    DASSERT(mpWorkQueue);
    TChar char_buffer;

    //bind with pipe fd
    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {
        case ECMDType_Start:
            if (DLikely(EGuardianState_idle == msState)) {
                if (DIsSocketHandlerValid(mPairingSocket)) {
                    gfNetCloseSocket(mPairingSocket);
                    mPairingSocket = DInvalidSocketHandler;
                }

                if (0 <= mListenSocket) {
                    if (cmd.res32_1 != mPort) {
                        gfNetCloseSocket(mListenSocket);
                        mPort = cmd.res32_1;
                        mListenSocket = gfNet_SetupStreamSocket(INADDR_ANY, mPort, 0);
                    }
                } else {
                    mPort = cmd.res32_1;
                    mListenSocket = gfNet_SetupStreamSocket(INADDR_ANY, mPort, 0);
                }

                if (DUnlikely(0 > mListenSocket)) {
                    LOGM_ERROR("setup listen socket fail, mListenSocket %d\n", mListenSocket);
                    msState = EGuardianState_error;
                    if (mpCallbackContext && mfStateChangeCallback) {
                        mfStateChangeCallback(mpCallbackContext, msState);
                    }
                    mpWorkQueue->CmdAck(EECode_BadParam);
                    break;
                }

                FD_SET(mPipeFd[0], &mAllSet);
                FD_SET(mListenSocket, &mAllSet);
                if (mPipeFd[0] > mListenSocket) {
                    mMaxFd = mPipeFd[0];
                } else {
                    mMaxFd = mListenSocket;
                }

                msState = EGuardianState_wait_paring;
                if (mpCallbackContext && mfStateChangeCallback) {
                    mfStateChangeCallback(mpCallbackContext, msState);
                }
                mpWorkQueue->CmdAck(EECode_OK);
            } else {
                LOGM_ERROR("not expected state %d\n", msState);
                mpWorkQueue->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_Stop:
            mbRun = 0;
            msState = EGuardianState_halt;
            if (mpCallbackContext && mfStateChangeCallback) {
                mfStateChangeCallback(mpCallbackContext, msState);
            }
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    LOG_PRINTF("****CIGuardian::ProcessCmd, cmd.code %d end.\n", cmd.code);
}

IGuardian *gfCreateGuardian(EGuardianType type)
{
    IGuardian *thiz = NULL;
    switch (type) {

        case EGuardianType_socket:
            thiz = CIGuardian::Create();
            break;

        default:
            LOG_FATAL("not support type %d\n", type);
            break;
    }

    return thiz;
}

TSocketHandler gfCreateGuardianAgent(EGuardianType type, TSocketPort port)
{
    TSocketHandler ret = 0;

    switch (type) {

        case EGuardianType_socket:
            ret = gfNet_ConnectTo("127.0.0.1", port, SOCK_STREAM, IPPROTO_TCP);
            break;

        default:
            LOG_FATAL("not support type %d\n", type);
            break;
    }

    return ret;
}
#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

