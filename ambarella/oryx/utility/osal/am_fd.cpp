/*******************************************************************************
 * am_fd.cpp
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "am_fd.h"

bool send_fd(int fd, int sendfd)
{
  struct msghdr msg;
  struct iovec  iov[1];
  char nothing = 0;
  bool ret = false;

  union {
      struct cmsghdr cm;
      uint8_t control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);
  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int*)CMSG_DATA(cmptr)) = sendfd;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  iov[0].iov_base = &nothing;
  iov[0].iov_len = sizeof(nothing);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  ret = (sendmsg(fd, &msg, 0) > 0);
  if (AM_UNLIKELY(!ret)) {
    PERROR("sendmsg");
  }

  return ret;
}

bool recv_fd(int fd, int *recvfd)
{
  struct msghdr msg;
  struct iovec  iov[1];
  char nothing = 0;

  union {
      struct cmsghdr cm;
      uint8_t control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  *recvfd = -1;
  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  iov[0].iov_base = &nothing;
  iov[0].iov_len = sizeof(nothing);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  if (AM_LIKELY(recvmsg(fd, &msg, 0) > 0)) {
    if (AM_LIKELY(((cmptr = CMSG_FIRSTHDR(&msg)) != NULL) &&
                  (cmptr->cmsg_len == CMSG_LEN(sizeof(int))))) {
      do {
        if (AM_UNLIKELY(cmptr->cmsg_level != SOL_SOCKET)) {
          ERROR("control level != SOL_SOCKET");
          break;
        }
        if (AM_UNLIKELY(cmptr->cmsg_type != SCM_RIGHTS)) {
          ERROR("control type != SCM_RIGHTS");
          break;
        }
        *recvfd = *((int*)CMSG_DATA(cmptr));
      }while(0);
    }
  } else {
    PERROR("recvmsg");
  }

  return (*recvfd >= 0);
}
