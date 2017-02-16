/*******************************************************************************
 * am_api_proxy.h
 *
 * History:
 *   2014-9-26 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_API_PROXY_H_
#define AM_API_PROXY_H_

class AMAPIProxy
{
  public:
    ~AMAPIProxy();
    static AMAPIProxy *get_instance();
    int init();

  protected:
    AMAPIProxy();
    AMIPCSyncCmdServer *m_air_api_ipc;
};

#endif /* AM_API_PROXY_H_ */
