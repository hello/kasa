/**
 * am_network_al.cpp
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_playback_new_if.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_network_al.h"

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
#ifndef BUILD_OS_WINDOWS
    setsockopt(newSocket, SOL_SOCKET, SO_RCVBUFFORCE, &tmp, sizeof(tmp));
#endif
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
    err_code = EECode_Error;
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

