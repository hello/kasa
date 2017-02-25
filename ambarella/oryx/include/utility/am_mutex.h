/*******************************************************************************
 * am_mutex.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
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
#ifndef AM_MUTEX_H_
#define AM_MUTEX_H_

#include <mutex>
#include <atomic>
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

/*! @class AMMemLock
 *  @brief A lock class implemented by std::atomic_flag in C++11.
 *
 * This user space spin lock is implemented by std::atomic_flag in C++11.
 * This implementation is faster than pthread_mutex and std::mutex.
 * This lock CANNOT be used recursively.
 * @sa AMMutex
 */

class AMMemLock
{
  public:
    AMMemLock() = default;
    ~AMMemLock(){m_lock.clear(std::memory_order_release);}
    AMMemLock(const AMMemLock&) = delete;
    AMMemLock& operator=(const AMMemLock&) = delete;

  public:
    /*! lock */
    void lock()
    {
      while(m_lock.test_and_set(std::memory_order_acquire)){}
    }
    /*! unlokc */
    void unlock()
    {
      m_lock.clear(std::memory_order_release);
    }

  private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
};
#define AUTO_MEM_LOCK(memlock) \
  std::lock_guard<AMMemLock> __mem_lock__##memlock(memlock)

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
#ifndef AUTO_MTX_LOCK
#define AUTO_MTX_LOCK(pMutex) AMAutoLock __auto_lock__(pMutex)
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
