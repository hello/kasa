/*******************************************************************************
 * am_thread.h
 *
 * History:
 *   2014-7-18 - [ypchang] created file
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
#ifndef AM_THREAD_H_
#define AM_THREAD_H_

/*! @file am_thread.h
 *  @brief This file defines AMThread class
 */
#include <pthread.h>
#include <string>

/*! @addtogroup HelperClass
 *  @{
 */
/*! @typedef AmThreadFuncType
 *  @brief Function type can be managed by AMThread as a thread function.
 */
typedef void (*AmThreadFuncType)(void*);

/*! @class AMThread
 *  @brief This class runs a function as a thread.
 */
class AMThread
{
  public:
    /*! @enum AM_THREAD_PRIORITY
     *  @brief Enumerate defines thread real-time priority.
     */
    enum AM_THREAD_PRIORITY
    {
      AM_THREAD_PRIO_LOW     = 70,//!< Low priority
      AM_THREAD_PRIO_NORMAL  = 80,//!< Normal priority
      AM_THREAD_PRIO_HIGH    = 90,//!< High priority
      AM_THREAD_PRIO_HIGHEST = 99 //!< Highest priroty
    };

  public:

    /*! Test to see if the underlying thread is running.
     * @return true if the thread is running, otherwise return false.
     */
    bool is_running() {return m_thread_running;}

    /*! Return the thread name.
     * @return const char* type thread name, or NULL if the name is not set.
     */
    const char* name() {return m_thread_name;}

  public:
    /*! Constructor, creates an AMThread object and return its pointer.
     *
     * If successfully created, the thread will starts to run after
     * this function returns.
     * @param name C style string containing the thread name.
     * @param entry The thread function.
     * @param data The user data will be passed to entry.
     * @return AMThread pointer type, or NULL if failed to create.
     */
    static AMThread* create(const char *name,
                            AmThreadFuncType entry,
                            void *data);
    /*! Overloaded function.
     *
     * @param name std::string containing the thread name.
     * @param entry The thread function.
     * @param data The user data will be passed to entry.
     * @return AMThread pointer type, or NULL if failed to create.
     */
    static AMThread* create(const std::string& name,
                            AmThreadFuncType entry,
                            void *data);

    /*! Destructor, which will destroy this object.
     *  This function will block until the underlying thread function returns.
     */
    void destroy();

    /*! Set the thread to be real-time thread and assign a priority to it.
     * @param prio AM_THREAD_PRIORITY type.
     * @return true if success, otherwise return false.
     */
    bool set_priority(int prio);

  protected:
    AMThread();
    virtual ~AMThread();
    bool init(const char *name, AmThreadFuncType entry, void *data);

  private:
    static void* static_entry(void* data);

  protected:
    AmThreadFuncType m_thread_entry;
    char            *m_thread_name;
    void            *m_thread_data;
    pthread_t        m_thread_id;
    bool             m_thread_running;
    bool             m_thread_created;
};
/*!
 * @}
 */
#endif /* AM_THREAD_H_ */
