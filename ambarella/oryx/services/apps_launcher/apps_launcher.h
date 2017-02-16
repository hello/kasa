/*******************************************************************************
 * apps_launcher.h
 *
 * History:
 *   2014-10-21 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef APPS_LAUNCHER_H_
#define APPS_LAUNCHER_H_
#include "commands/am_service_impl.h"

#define APP_LAUNCHER_MAX_SERVICE_NUM 12
class AMAppsLauncher {

  public:
    virtual ~AMAppsLauncher();

    int load_config();
    int init();
    int start();
    int stop();
    int clean();

    static AMAppsLauncher  *get_instance();
    am_service_attribute m_service_list[APP_LAUNCHER_MAX_SERVICE_NUM];

  protected:
    AMAppsLauncher();


    AMServiceManagerPtr m_service_manager;
    AMAPIProxy * m_api_proxy;
};




#endif /* APPS_LAUNCHER_H_ */
