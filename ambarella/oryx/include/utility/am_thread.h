/*******************************************************************************
 * am_thread.h
 *
 * History:
 *   2014-7-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    bool             m_thread_running;
    bool             m_thread_created;
    pthread_t        m_thread_id;
};
/*!
 * @}
 */
#endif /* AM_THREAD_H_ */
