/**
 * simple_player_if.h
 *
 * History:
 *    2015/01/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SIMPLE_PLAYER_IF_H__
#define __SIMPLE_PLAYER_IF_H__

namespace mw_cg
{

    class ISimplePlayer
    {
    protected:
        virtual ~ISimplePlayer() {}

    public:
        virtual void Destroy() = 0;
        virtual int Initialize(void *owner = NULL, void *p_external_msg_queue = NULL) = 0;

    public:
        virtual int Play(char *url, char *title_name = NULL) = 0;

    };

    extern ISimplePlayer *gfCreateSimplePlayer();

}

#endif

