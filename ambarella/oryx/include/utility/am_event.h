/*******************************************************************************
 * am_event.h
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
