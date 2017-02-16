/**
 * simple_capserver_if.h
 *
 * History:
 *    2015/01/05 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SIMPLE_CAPSERVER_IF_H__
#define __SIMPLE_CAPSERVER_IF_H__

namespace mw_cg
{

    class ISimpleCapServer
    {
    protected:
        virtual ~ISimpleCapServer() {}

    public:
        virtual void Destroy() = 0;
        virtual int Initialize() = 0;

    public:
        virtual int Start() = 0;
        virtual int Stop() = 0;

    public:
        virtual void Mute() = 0;
        virtual void UnMute() = 0;
    };

    extern ISimpleCapServer *gfCreateSimpleCapServer();

}

#endif

