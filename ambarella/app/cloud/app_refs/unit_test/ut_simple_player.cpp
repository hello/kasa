/**
 * ut_simple_player.cpp
 *
 * History:
 *    2015/01/10 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"

#include "simple_player_if.h"

using namespace mw_cg;

int main(int argc, char **argv)
{
    int ret = 0;
    mw_cg::ISimplePlayer *thiz = gfCreateSimplePlayer();

    if (!thiz) {
        printf("gfCreateSimplePlayer() fail\n");
        return (-1);
    }

    ret = thiz->Initialize();
    if (0 > ret) {
        printf("thiz->Initialize() fail, ret %d\n", ret);
        return (-2);
    }

    char *url = NULL;
    if (argc >= 2) {
        url = argv[1];
    }

    ret = thiz->Play(url);
    if (0 > ret) {
        printf("thiz->Play() fail, ret %d\n", ret);
        return (-3);
    }

    printf("press 'q' to quit\n");
    char input_c = 0;
    while (1) {
        sscanf("%c", &input_c);
        if ('q' == input_c) {
            break;
        }
    }

    thiz->Destroy();
    thiz = NULL;

    return 0;
}

