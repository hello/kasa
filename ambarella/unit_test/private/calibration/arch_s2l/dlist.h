/*******************************************************************************
 * dlist.h
 *
 * Histroy:
 *   Dec 5, 2013 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef _DLIST_H_
#define _DLIST_H_

typedef struct Point {
    float x;
    float y;
} list_node;

struct node_info {
    list_node node;
    struct node_info *prev;
    struct node_info *next;
};

struct list_info {
    struct node_info head;
    void (*add_tail)(struct list_info *,
        list_node *node);
    void (*for_each)(struct list_info *,
        void (*)(struct list_info *,
            struct node_info *));
    void (*for_each_safe)(struct list_info *,
        void (*)(struct list_info *,
            struct node_info *));
    void (*del)(struct list_info *,
        struct node_info *);
    int (*isempty)(struct list_info *);
    void (*destroy)(struct list_info *);
};

void list_init(struct list_info *);

#endif
