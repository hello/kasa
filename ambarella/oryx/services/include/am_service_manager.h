/*******************************************************************************
 * am_service_manager.h
 *
 * History:
 *   2014-9-15 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_SERVICE_MANAGER_H_
#define AM_SERVICE_MANAGER_H_
#include <vector>
#include <atomic>
#include "am_pointer.h"
#include "am_service_base.h"

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

    //add service to list, before init
    virtual int add_service(AMServiceBase *service);

    // create process, setup IPC, and call each service's init function by IPC
    // some will create their own IPC by their "init"
    virtual int init_services();

    //call each service's start function
    virtual int start_services();

    //call each service's stop function
    virtual int stop_services();

    //call each service's restart function
    virtual int restart_services();

    //report status of all
    virtual int report_services();

    //the reverse operation of "init_services"
    //can call init again after destroy
    virtual int destroy_services();

    //clean up all services and delete objects,
    //also destroy service if not destroyed
    virtual int clean_services();
    AMServiceBase * find_service(AM_SERVICE_CMD_TYPE type);
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
    // Not Implemented
    AMServiceManager(AMServiceManager const &copy) = delete;
    AMServiceManager& operator=(AMServiceManager const &copy) = delete;

  private:
    //use vector so that service manager
    //can manage a changeable count of services
    std::vector<AMServiceBase*> m_services;
    AM_SERVICE_MANAGER_STATE m_state;
    static AMServiceManager *m_instance;
    std::atomic_int m_ref_counter;
};

#endif /* AM_SERVICE_MANAGER_H_ */
