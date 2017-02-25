/*******************************************************************************
 * am_pid_lock.h
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
 *
 ******************************************************************************/

#ifndef __AM_PID_LOCK_H__
#define __AM_PID_LOCK_H__

/*! @file am_pid_lock.h
 *  @brief This file defines AMPIDLock class.
 */

/*!
 * @addtogroup HelperClass
 * @{
 */

#define PID_LOCK_FILENAME_SIZE  256

/*! @class AMPIDLock
 *  @brief This class is designed to prevent two instances of process
 *         running the same program.
 *
 *  A process name "xyz" will have a pid lock file under /tmp/xyz.pid,
 *  the content is pid of the process.
 */

class AMPIDLock
{
  public:
    /*!
     * Constructor
     */
    AMPIDLock(void);

    /*!
     * Destructor
     *
     * auto delete the pid file if owned by current process
     */
    virtual ~AMPIDLock(void);

    /*! Try to lock the process
     * @return 0: means lock OK.
     *         negative: means lock failed, and cannot continue run this process
     *         (usually should exit).
     */
    int try_lock(void);

  private:
    char m_pid_file[PID_LOCK_FILENAME_SIZE];
    bool m_own_lock;
};

/*!
 * @}
 */

#endif // __AM_PID_LOCK_H__
