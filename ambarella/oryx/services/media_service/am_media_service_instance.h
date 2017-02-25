/*******************************************************************************
 * am_media_service_instance.h
 *
 * History:
 *   2015-1-20 - [ccjing] created file
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

/*! @file am_media_service_instance.h
 *  @brief This header file contains the definition of class AMMediaService.
 */
#ifndef AM_MEDIA_SERVICE_INSTANCE_H_
#define AM_MEDIA_SERVICE_INSTANCE_H_

#include "am_record_if.h"
#include "am_playback_if.h"
#include "am_media_service_msg.h"
#include "am_mutex.h"
#include "am_record_msg.h"

enum AM_PLAYER_INSTANCE_ID
{
  PLAYER_NULL = -1,
  PLAYER_1    = 0,
  PLAYER_2,
  PLAYER_NUM,
};

class AMThread;

/*! @class AMMediaService
 *  @brief AMMediaService class provides two functions :
 *   stream record and audio playback. Media service contains an unix
 *   domain socket which can be connected by other services. Other services
 *   can send message to media service to control record and playback module.
 */
class AMMediaService
{
    /*! @typedef media_callback
     *  @brief This type define is used to set callback function for
     *   media service.
     */
    typedef void (*media_callback)(void *data);
  public:
    /*! @This function is used to create media service instance.
     *  @param The param is the callback function.
     *  @return return a point of media service instance if create it
     *   successfully, otherwise rerurn NULL.
     *  @sa init(media_callback media_callback)
     */
    static AMMediaService* create(media_callback media_callback = nullptr);

  public:
    /*! @Start media service.
     *  @return true if success, otherwise return false.
     *  @sa stop_media()
     */
    bool start_media();
    /*! @Stop media service.
     *  @return true if success, otherwise return false.
     *  @sa start_media()
     */
    bool stop_media();
    /*! @Send event for start event recording.
     *  @event_id which channel you want to send event
     *  @return true if success, otherwise return false.
     */
    bool send_event(AMEventStruct& event);

  public:
    /*! @Destroy the object of AMMediaService.
     */
    void destroy();

  public:
    /*! @Get valid playback id.
     *  @return playback id.
     *  @sa get_playback_instance()
     */
    AM_PLAYER_INSTANCE_ID get_valid_playback_id();
    /*! @Get the smart point of AMIPlayback instance.
     *  @param playback instance id which is got by get_valid_playback_id() function.
     *  @return smart point of AMIPlayback instance.
     */
    AMIPlaybackPtr get_playback_instance(AM_PLAYER_INSTANCE_ID id);
    /*! @Release playback instance which is specified by id when you finish using it.
     *  @param  playback id.
     *  @return true if success, otherwise return false.
     *  @sa get_playback_instance()
     */
    bool release_playback_instance(AM_PLAYER_INSTANCE_ID id);
    /*! @Get the smart point of AMIRecord instance.
     *  @return smart point of AMIPlayback instance.
     */
    AMIRecordPtr& get_record_instance();
  private:
    /*! @Constuctor function.
     */
    AMMediaService();
    /*! @Destructor function.
     */
    virtual ~AMMediaService();
    /*! @Initialize the object return by create()
     *  @return true if success, otherwise return false.
     *  @sa create(media_callback media_callback = nullptr).
     */
    bool init(media_callback media_callback);
    /*! @create the unix domain socket and listen the socket fd.
     *  @param The param is the unix domain socket name.
     *  @return return socket fd if success, otherwise return -1.
     */
    int unix_socket_listen(const char* service_name);
    /*! @accept the connection request on the unix domain socket.
     *  @return return a new connected socket fd if success,
     *   otherwise return -1.
     */
    int unix_socket_accept(int listen_fd);
    /*! @static thread entry function of the thread which is used to listen the
     *   unix domain socket.
     * @sa socket_thread_main_loop()
     */
    static void socket_thread_entry(void *p);
    /*! @The main loop of the thread which is used to listen the unix domain
     *   socket.
     */
    void socket_thread_main_loop();
    /*! @This function is used to create resource which is need in the main
     *   loop.
     * @return true if success, otherwise return false.
     * @sa release_resource()
     */
    bool create_resource();
    /*! @This function is used to release resource which is created in the
     *   create_resource function.
     */
    void release_resource();
    /*! @This function is used to receive message from client which is connected
     *   to the media service.
     *  @param fd is the file descriptor.
     *  @param client_msg is used to contain the message received from the
     *    file descriptor.
     *  @return AM_MEDIA_NET_OK if success, otherwise, return the net work
     *   state.
     */
    AM_MEDIA_NET_STATE recv_client_msg(int fd, AMMediaServiceMsgBlock* client_msg);
    /*! @This function is used to process the message received from other
     *   client.
     *  @param fd is the file descriptor.
     *  @param client_msg is message received from other client.
     *  @return true if success, otherwise return false.
     */
    bool process_client_msg(int fd, AMMediaServiceMsgBlock& client_msg);
    /*! @This function is used to send ack message to other client.
     *  @param proto indicators which client send message to.
     *  @return true if success, otherwise return false.
     */
    bool send_ack(AM_MEDIA_SERVICE_CLIENT_PROTO proto);
    /*! @This function is used to send playback id to other client.
     *  @param proto indicators which client send message to.
     *  @param id indicators which playback will play.
     *  @return true if success, otherwise return false.
     */
    bool send_playback_id(AM_MEDIA_SERVICE_CLIENT_PROTO proto, int32_t id);
    /*! @This function is used to get prototol name by file descriptor.
     *  @param file descriptor.
     *  @return return protocol name if success, otherwise return "Unkonwn"
     *   string.
     */
    std::string fd_to_proto_string(int fd);

  private:
    /*! @record engine callback function. This function can be used to get
     *   message from record engine.
     *  @param message from record engine.
     */
    static void record_engine_callback(AMRecordMsg& msg);
    /*! @playback engine callback function. This function can be used to get
     *   message from playback engine.
     *  @param message from playback engine.
    */
    static void playback_engine_callback(AMPlaybackMsg& msg);

  private:
    AMThread             *m_socket_thread            = nullptr;
    AMIPlaybackPtr       *m_playback                 = nullptr;
    media_callback        m_media_callback           = nullptr;
    AMIRecordPtr          m_record                   = nullptr;
    int32_t               m_playback_ref[PLAYER_NUM] = { 0 };
    int                   m_client_proto_fd[AM_MEDIA_SERVICE_CLIENT_PROTO_NUM] = {-1};
    int                   m_service_ctrl[2]          = {-1};
    int                   m_unix_socket_fd           = -1;
    std::atomic<bool>     m_run                      = {false};
    std::atomic<bool>     m_is_started               = {false};
    AMMemLock             m_lock;
    AMMemLock             m_playback_lock;
};

#endif /* AM_MEDIA_SERVICE_INSTANCE_H_ */
