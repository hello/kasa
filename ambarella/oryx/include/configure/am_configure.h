/*******************************************************************************
 * am_configure.h
 *
 * History:
 *   2014-7-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_CONFIGURE_H_
#define AM_CONFIGURE_H_

#include "luatables.h"

class AMConfig: public LuaTable
{
  public:
    static AMConfig* create(const char *config);
    void destroy();

  public:
    bool save();
    virtual ~AMConfig(){}

  private:
    AMConfig(){}
    bool init(const char *config);
    AMConfig& operator=(const LuaTable &table);
};

#endif /* AM_CONFIGURE_H_ */
