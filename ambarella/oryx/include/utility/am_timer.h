/*******************************************************************************
 * am_timer.h
 *
 * History:
 *   Jul 21, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef AM_TIMER_H_
#define AM_TIMER_H_

/*! @file am_timer.h
 *  @brief This file defines AMTimer class
 */

/*! @addtogroup HelperClass
 *  @{
 */

/*!
 * @typedef AMTimerCallback
 * @brief Function type can be called by AMTimer when timeout occurs
 * @param data passed to the timer callback
 * @return true if continue to enable timer, false stop the timer after running
 */
typedef bool (*AMTimerCallback)(void*);

class AMTimerData;

/*! @class AMTimer
 *  @brief This class starts a timer and run callback function after timeout
 */
class AMTimer
{
  public:
    /*! Constructor, creates an AMTimer object and return its pointer
     * This constructor only creates an object, and no internal thread is
     * created, only single_shot can be called
     * @return AMTimer pointer type, or NULL if failed to create
     */
    static AMTimer* create();
    /*! Constructor, creates an AMTimer object and return its pointer.
     *
     * @param name Timer's name
     * @param timeout timeout in milliseconds
     * @param cb callback function @sa AMTimerCallback
     * @param data data passed to the timer callback
     * @return AMTimer pointer type, or NULL if failed to create.
     */
    static AMTimer* create(const char *name,
                           uint32_t timeout,
                           AMTimerCallback cb,
                           void *data);

    /*! Destroy this timer object
     */
    void destroy();

  public:
    /*! Starts the timer
     *
     * @param timeout 0 means use the default timeout value when creating
     *        the timer
     * @return true if successfully armed the timer, false if failed
     */
    bool start(uint32_t timeout = 0);

    /*! Stop the timer
     *
     * @return true if successfully stopped the timer, false if failed
     */
    bool stop();

    /*! Restart the timer
     *
     * @return true if successfully stopped the timer, false if failed
     */
    bool reset();

    /*! Set the timer callback function
     *
     * @param cb timer callback function @sa AMTimerCallback
     * @param data data passed to the callback function
     */
    void set_timer_callback(AMTimerCallback cb, void *data);

    /*! Start a single shot timer
     *
     * @param name timer name
     * @param timeout timeout value
     * @param cb timer callback which will be called after time out
     * @param data data passed to the timer callback
     * @return true if successffully started the timer, otherwise false
     */
    bool single_shot(const char *name,
                     uint32_t timeout,
                     AMTimerCallback cb,
                     void *data);

    /*! Stop last single_shot timer
     *
     * @param name timer name
     * @return the user data pointer set by single_shot
     */
    void* single_stop(const char *name);

  private:
    AMTimer(){}
    virtual ~AMTimer();
    bool init(const char *name,
              uint32_t timeout,
              AMTimerCallback cb,
              void *data,
              bool single = false);

  private:
    AMTimerData   *m_timer  = nullptr;
    bool           m_single = false;
};

/*!
 * @}
 */
#endif /* AM_TIMER_H_ */
