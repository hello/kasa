/*******************************************************************************
 * am_fd.cpp
 *
 * History:
 *   2015-1-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
