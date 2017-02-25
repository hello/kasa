/*
 * am_pid_lock.cpp
 *
 * History:
 *    2013/07/15 - [Louis Sun] Create
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
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include "am_base_include.h"
#include "am_pid_lock.h"
#include "am_log.h"

AMPIDLock::AMPIDLock() :
    m_own_lock(false)
{
#define STR_BUFFER_SIZE 128
#define FILE_NAME_SIZE 128
  int fd = -1;
  char sztmp[FILE_NAME_SIZE] = { 0 };
  char buffer[STR_BUFFER_SIZE] = { 0 };
  char *line_end = NULL;
  char *name = NULL;
  do {
    sprintf(sztmp, "/proc/%d/status", (int) syscall(SYS_gettid)); //get process name from proc filesys
    //get first line from /proc/%d/status 2nd word
    fd = open(sztmp, O_RDONLY);
    if (fd < 0) {
      PERROR("AMPIDLock: Error to open current process's pid status to get name: ");
      break;
    }
    if (read(fd, buffer, STR_BUFFER_SIZE - 1) < 0) {
      PERROR("AMPIDLock: Error reading proc staus file  :");
      break;
    }
    //find the first line end, replace it with '\0';
    line_end = strchr(buffer, '\n');
    *line_end = '\0';
    //get buffer's 2nd word, find space
    name = strrchr(buffer, '\t');
    if (!name) {
      PRINTF("AMPIDLock: Error: illegal proc status file name, string is %s \n",
             buffer);
      break;
    } else {
      name ++;
      //PRINTF("AMPIDLock: Info: The process name is %s\n", name);
      snprintf(m_pid_file, PID_LOCK_FILENAME_SIZE - 1, "/tmp/%s.pid", name);
      //PRINTF("AMPIDLock: Info: PIDFILE to lock is %s \n", m_pid_file);
    }
  } while (0);
}

int AMPIDLock::try_lock(void)
{
  int fd = -1;
  int ret = 0;
#define PIDLOCK_FILE_SIZE_MAX 256
  char buffer[PIDLOCK_FILE_SIZE_MAX] = { 0 };
  char tmpstr[PIDLOCK_FILE_SIZE_MAX] = { 0 };
  int count;
  int pid;
  struct stat stat_buf;

  do {
    fd = open(m_pid_file, O_RDWR | O_CREAT | O_EXCL, 0644); //try to create exclusive pid lock first
    if (fd < 0) {
      if (errno == EEXIST) {
        //the pid file exists, then open it.
        fd = open(m_pid_file, O_RDWR);
        if (fd < 0) {
          PERROR("AMPIDLock: Error to open existing file"); //should not happen usually
          ret = -1;
          break;
        }
        //open existing pid file OK, then check whether the pid value still exist and matches the name
        //if not, then delete the pid file, and continue create
        //if Yes, then report failure.
        count = read(fd, buffer, PIDLOCK_FILE_SIZE_MAX - 1);
        if (count <= 0) {
          PERROR("AMPIDLock: ERROR to read pid file :");
          ret = -2;
          break;
        }
        pid = atoi(buffer);
        //search pid in /proc
        sprintf(tmpstr, "/proc/%d/stat", pid);
        if (stat(tmpstr, &stat_buf) < 0) { //not found, dead pid file, remove it.
          // PRINTF("AMPIDLock: Info: pid file %s record is obsolete, re-create it \n", m_pid_file);
          close(fd); //close the opened pid lock file
          if (unlink(m_pid_file) < 0) { //remove it
            ERROR("AMPIDLock: Error to unlink pid file %s\n", m_pid_file);
            ret = -7;
            break;
          }

          fd = open(m_pid_file, O_RDWR | O_CREAT | O_EXCL, 0644); //create the pid file again
          if (fd < 0) {
            PERROR("AMPIDLock: unlink the existing pid lock but unable to create new");
            ret = -3;
          } else {
            ret = 0; //create ok
          }
          break;
        }

        //then must be valid process holding the pid lock
        PRINTF("AMPIDLock: pid %d is taking lock file %s, cannot run duplicate instance\n",
               pid,
               m_pid_file);
        ret = -4;
        break;
      } else {
        PERROR("AMPIDLock: Error to create pid file "); //should not happen usually
        ret = -5;
        break;
      }
    } else {
      //create OK. then fill pid
      ret = 0;
      break;
    }
  } while (0);

  //if OK, create current pid file
  if (ret == 0) {
    //write current pid into the pid lock file
    pid = (int) syscall(SYS_gettid);
    sprintf(buffer, "%d", pid);
    if (write(fd, buffer, strlen(buffer)) < 0) {
      PERROR("AMPIDLock: Error to write pid lock file\n");
      ret = -6;
    } else {
      //when pid file created, and write current pid OK, then own lock
      m_own_lock = true;
      DEBUG("AMPIDLock:  pid lock file %s created OK\n", m_pid_file);
    }

  }

  if (fd >= 0)
    close(fd);

  return ret;
}

AMPIDLock::~AMPIDLock(void)
{
  //auto delete the pid file if own the lock
  if (m_own_lock) {
    if (unlink(m_pid_file) < 0) {
      ERROR("AMPIDLock: Error, own pid lock %s, but unable to delete when releasing lock\n",
            m_pid_file);
    } else {
      DEBUG("AMPIDLock:  pid lock file %s removed\n", m_pid_file);
    }
  }
}
