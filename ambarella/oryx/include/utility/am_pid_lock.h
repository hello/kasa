/*******************************************************************************
 * am_pid_lock.h
 *
 * History:
 *    2013/07/15 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
