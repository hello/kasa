/*******************************************************************************
 * am_event.h
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
#ifndef AM_EVENT_H_
#define AM_EVENT_H_

#include <stdint.h>

/*! @file am_event.h
 *  @brief This file define AMEvent class.
 */
struct AMEventData;

/*! @class AMEvent
 *  @brief A wrapper class of semaphore
 *  @defgroup HelperClass Helper Classes
 *  @{
 */
class AMEvent
{
  public:
    /*! Constructor function.
     * @return pointer to AMEvent type if success, otherwise NULL.
     */
    static AMEvent* create();

    /*! Destroy this object.
     */
    void destroy();

  public:
    /*! Block the calling thread/process.
     * If ms is larger than 0, this function blocks the calling thread/process
     * for ms milliseconds, otherwise it blocks forever.
     * @param ms integer to indicate how many milliseconds to wait
     * @return true if ms is greater than 0 and signal is received during
     *         the block, otherwise return false.
     *         if ms is less than 0, this function does not return until signal
     *         is received.
     */
    bool wait(int64_t ms = -1);

    /*! Emit a signal
     */
    void signal();

    /*! Clear the last sent signal.
     */
    void clear();

  private:
    /*!
     * Constructor
     */

    AMEvent();
    /*!
     * Destructor
     */
    virtual ~AMEvent();

    /*!
     * Initializer
     * @return true if success, otherwise false.
     */
    bool init();

  private:
    AMEventData *m_event;
};
/*!
 * @}
 */
#endif /* AM_EVENT_H_ */
