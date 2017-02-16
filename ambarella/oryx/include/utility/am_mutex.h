/*******************************************************************************
 * am_mutex.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MUTEX_H_
#define AM_MUTEX_H_

#include <stdint.h>

/*! @file am_mutex.h
 *  @brief This file defines multiple locks and signal-event handlers.
 */
class  AMCondition;
struct AMMutexData;
struct AMSpinLockData;

/*! @addtogroup HelperClass
 *  @{
 */

/*! @class AMSpinLock
 *  @brief A lock class implemented by std::atomic_flag in C++11.
 *
 * This user space spin lock is implemented by std::atomic_flag in C++11.
 * This implementation is faster than pthread_mutex and std::mutex.
 * This lock CANNOT be used recursively.
 * @sa AMAutoSpinLock
 * @sa AMMutex
 */
class AMSpinLock
{
  public:
    /*! Constructor
     * @return a pointer to AMSpinLock object,
     *         or NULL if failed to create
     */
    static AMSpinLock* create();

    /*! Destroy the object of AMSpinLock
     */
    void destroy();

  public:
    /*! lock */
    void lock();

    /*! unlock */
    void unlock();

  private:
    AMSpinLock();
    virtual ~AMSpinLock();
    bool init();

  private:
    AMSpinLockData *m_lock;
};

/*! @class AMAutoSpinLock
 *  @brief Helper class to manage AMSpinLock.
 *
 *  This class takes a pointer of AMSpinLock as its parameter when construct,
 *  and call AMSpinLock::lock inside contructor, and call AMSpinLock::unlock
 *  inside destructor. This mechanism makes it possible to lock a snippet
 *  based on the life cycle of the AMAutoSpinLock object.
 *  @sa AMSpinLock
 */
class AMAutoSpinLock
{
  public:
    /*!
     * Constructor. Call AMSpinLock::lock.
     * @param spin_lock AMSpinLock pointer type.
     */
    AMAutoSpinLock(AMSpinLock *spin_lock) :
      m_lock(spin_lock)
    {
      m_lock->lock();
    }

    /*!
     * Destructor. Call AMSpinLock::unlock.
     */
    ~AMAutoSpinLock()
    {
      m_lock->unlock();
    }
  private:
      AMSpinLock *m_lock;
};

/*! helper macro,
 *  the lock will be locked when the AMAutoSpinlock object is created,
 *  and unlock when this object is destroyed.
 *  This helper macro will keep a section of code locked during the life cycle
 *  of the AMAutoSpinLock object.
 */
#define AUTO_SPIN_LOCK(pSpinLock) AMAutoSpinLock __auto_spin_lock__(pSpinLock)
#define __SPIN_LOCK(pSpinLock)    pSpinLock->lock()
#define __SPIN_UNLOCK(pSpinLock)  pSpinLock->unlock()

/*! @class AMMutex
 *  @brief Mutex helper class implemented by pthread_mutex APIs.
 *
 * Mutex helper class, implemented by pthread_mutex APIs.
 * @sa AMAutoLock
 * @sa AMSpinLock
 */
class AMMutex
{
    friend class AMCondition;
  public:
    /*! Constructor
     * @param recursive indicate if this mutex is a recursive mutex.
     * @return a pointer to AMMutex object, or NULL if failed to create.
     */
    static AMMutex* create(bool recursive = true);

    /*!
     * Destroy AMMutex object.
     */
    void destroy();

  public:
    /*! lock */
    void lock();

    /*! unlock */
    void unlock();

    /*! try to acquire the lock
     * @return true if can lock and keep the lock, otherwise false.
     */
    bool try_lock();

  private:
    AMMutex();
    virtual ~AMMutex();
    bool init(bool recursive);

  private:
    AMMutexData *m_mutex;
};

/*! @class AMAutoLock
 *  @brief Helper class to manage AMMutex.
 *
 *  This class works similar to AMAutoSpinLock, what's different is that this
 *  class manages AMMutex instead of AMSpinLock.
 */
class AMAutoLock
{
  public:
    /*!
     * Constructor. Call AMMutex::lock
     * @param mutex AMMutex pointer type.
     */
    AMAutoLock(AMMutex *mutex) :
      m_mutex(mutex)
    {
      m_mutex->lock();
    }

    /*!
     * Destructor. Call AMMutex::unlock
     */
    ~AMAutoLock()
    {
      m_mutex->unlock();
    }
  private:
    AMMutex *m_mutex;
};

/*! helper macro,
 *  the lock will be locked when the AMAutoLock object is created,
 *  and unlock when this object is destroyed.
 *  This helper macro will keep a section of code locked during the life cycle
 *  of AMAutoLock object.
 */
#ifndef AUTO_LOCK
#define AUTO_LOCK(pMutex) AMAutoLock __auto_lock__(pMutex)
#endif
#define __LOCK(pMutex)    pMutex->lock()
#define __UNLOCK(pMutex)  pMutex->unlock()
#define __TRYLOCK(pMutex) pMutex->trylock()
/*!
 * @}
 */
struct AMConditionData;
class AMCondition
{
  public:
    static AMCondition* create();
    void destroy();

  public:
    bool wait(AMMutex *mutex, int64_t ms = -1);
    void signal();
    void signal_all();

  private:
    AMCondition();
    virtual ~AMCondition();
    bool init();

  private:
    AMConditionData *m_cond;
};

#endif /* AM_MUTEX_H_ */
