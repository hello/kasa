/*******************************************************************************
 * am_api_helper.h
 *
 * History:
 *   Jul 29, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_INCLUDE_SERVICES_AM_API_HELPER_H_
#define ORYX_INCLUDE_SERVICES_AM_API_HELPER_H_

#include <semaphore.h>

#include "am_result.h"
#include "am_mutex.h"
#include "am_ipc_uds.h"
#include "commands/am_service_impl.h"
#include <memory>

class AMAPIHelper;

/*! @defgroup airapi Air API
 *  @brief All Air APIs that can be used to interact with Oryx Services and
 *         helper class used to interact with Oryx Services.
 */

/*! @defgroup airapi-datastructure Air API Data Structure
 *  @ingroup airapi
 *  @brief All Oryx Services related data structures
 */

/*! @defgroup airapi-helperclass Air API Helper Class
 *  @ingroup airapi
 *  @brief Helper class used to connect with Oryx Services
 *         and call the functions defined by the service through command ID.
 *         Refer to @ref airapi-commandid "Air API Command IDs"
 *         to get command IDs.
 *  @{
 */

/*! @typedef AMAPIHelperPtr
 *  @brief Smart pointer type used to manage AMAPIHelper pointer.
 */
typedef std::shared_ptr<AMAPIHelper> AMAPIHelperPtr;

/*! @class AMAPIHelper
 *  @brief Helper class used to interact with Oryx Services.
 *
 *  User needs to call get_instance() of this class to acquire an instance,
 *  then use method_call() with proper parameters to interact with the
 *  underlying service.
 */
class AMAPIHelper
{
  public:
    /*! @typedef AM_IPC_NOTIFY_CB
     *  @brief   IPC notify callback function type,
     *           which is used in AMAPIHelper::register_notify_cb
     *
     *  @param   msg_data      callback function data pointer
     *  @param   msg_data_size callback function data size
     *
     *  @return  0 if success, otherwise return value less than 0.
     */
    typedef int (*AM_IPC_NOTIFY_CB)(const void *msg_data, int msg_data_size);

  public:
    /*!
     * Constructor. Get a reference of the AMAPIHelper object.
     * @return AMAPIHelperPtr type.
     */
    static AMAPIHelperPtr get_instance();

    /*!
     * Make a function call to the service by providing corresponding
     * cmd_id of the service and related command structures which are defined
     * in related am_api_xxx.h\n
     * See \ref test_video_service.cpp \ref test_audio_service.cpp for more
     * information.
     *
     * @param cmd_id Command ID
     * @param msg_data [in] related input parameter data structure pointer
     * @param msg_data_size [in] related input parameter size
     * @param result_addr [out] related function call result
     *                    data structure pointer
     * @param result_max_size [in] related function call result
     *                        data structure size
     * @sa am_air_api.h
     */
    void method_call(uint32_t cmd_id,
                     void *msg_data,
                     int msg_data_size,
                     am_service_result_t *result_addr,
                     int result_max_size = sizeof(am_service_result_t));

    /*!
     * Register a notify callback to make service notify the client possible
     *
     * @param cb will be called by service
     * @sa am_air_api.h
     */
    int register_notify_cb(AM_IPC_NOTIFY_CB cb);

  private:
    /*!
     * Initializer function.
     * @return AM_RESULT_OK if success, otherwise return value like AM_RESULT_ERR_*.
     */
    AM_RESULT construct();
    static void on_notify_callback(int32_t context, int fd);

    /*!
     * Constructor.
     */
    AMAPIHelper();

    /*!
     * Destructor.
     */
    virtual ~AMAPIHelper();
    AMAPIHelper(AMAPIHelper const &copy) = delete; //! Delete copy constructor
    AMAPIHelper& operator=(AMAPIHelper const &copy) = delete; //! Delete assign

  private:
    static AMAPIHelper *m_instance;
    static AMMemLock m_lock;
    sem_t *m_sem = nullptr;
    AM_IPC_NOTIFY_CB m_notify_cb = nullptr;
    AMIPCClientUDSPtr m_client = nullptr;
    AMIPCClientUDSPtr m_client_notify = nullptr;
};

#endif /* ORYX_INCLUDE_SERVICES_AM_API_HELPER_H_ */
