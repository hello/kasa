/*******************************************************************************
 * am_service_manager.h
 *
 * History:
 *   2014-9-15 - [lysun] created file
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
#ifndef AM_SERVICE_MANAGER_H_
#define AM_SERVICE_MANAGER_H_
#include "am_mutex.h"
#include "am_pointer.h"
#include "am_service_base.h"

#include <vector>

//the design tries to let service manager init all services one by one,
//and let services themselves to know their dependencies.
//so that the complexity of dependency knowledge of services only need to
//be understood by the services themselves

//the AMServiceBase object added into service manager are objects of
//the services, which are created also within same process of AMServiceManager,
//so for example, if "start" is called, service manager only calls
//AMServiceBase's method, and AMServiceBase will call IPC to delegate
//the function to remote end.
//from service manager, it looks like every thing is at local, not remote
enum AM_SERVICE_MANAGER_STATE
{
  AM_SERVICE_MANAGER_NOT_INIT = 0,
  AM_SERVICE_MANAGER_READY,
  AM_SERVICE_MANAGER_RUNNING,
  AM_SERVICE_MANAGER_ERROR
};

class AMServiceManager;
typedef AMPointer<AMServiceManager> AMServiceManagerPtr;

class AMServiceManager
{
    friend AMServiceManagerPtr;

  public:
    static AMServiceManagerPtr get_instance();

    virtual int add_service(AMServiceBase *service);
    virtual int init_services();
    virtual int start_services();
    virtual int stop_services();
    virtual int restart_services();
    virtual int report_services();
    virtual int destroy_services();
    virtual int clean_services();
    AMServiceBase *find_service(AM_SERVICE_CMD_TYPE type);
    AM_SERVICE_MANAGER_STATE get_state()
    {
      return m_state;
    }

  protected:
    AMServiceManager();
    virtual ~AMServiceManager();
    void release();
    void inc_ref();

  private:
    AMServiceManager(AMServiceManager const &copy) = delete;
    AMServiceManager& operator=(AMServiceManager const &copy) = delete;

  private:
    static AMServiceManager *m_instance;
    AM_SERVICE_MANAGER_STATE m_state = AM_SERVICE_MANAGER_NOT_INIT;
    std::vector<AMServiceBase*> m_services;
    std::atomic_int m_ref_counter = {0};
    static AMMemLock  m_mutex;
};

#endif /* AM_SERVICE_MANAGER_H_ */
