/*******************************************************************************
 * dlist.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "dlist.h"

static void list_add_tail(struct list_info *info, list_node *node)
{
  struct node_info *new_node =
      (struct node_info *)malloc(sizeof(struct node_info));
  memcpy(&(new_node->node), node, sizeof(list_node));

  new_node->prev = info->head.prev;
  new_node->next = &info->head;
  info->head.prev->next = new_node;
  info->head.prev = new_node;
}

static void list_for_each(struct list_info *info,
                          void (*todo)(struct list_info *,
                              struct node_info *))
{
  struct node_info *cur = info->head.next;
  for (; cur != &info->head; cur = cur->next) {
    todo(info, cur);
  }
}

static void list_for_each_safe(struct list_info *info,
                               void (*todo)(struct list_info *,
                                   struct node_info *))
{
  struct node_info *cur = info->head.next;
  struct node_info *tmp = NULL;
  for (; cur != &info->head; cur = tmp) {
    tmp = cur->next;
    todo(info, cur);
  }
}

static void list_del(struct list_info *info,
                     struct node_info *node)
{
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->prev = node;
  node->next = node;
  free(node);
}

static int list_isempty(struct list_info *info)
{
  return info->head.next == &info->head;
}

static void list_destroy(struct list_info *info)
{
  list_for_each_safe(info, info->del);
}


void list_init(struct list_info *info)
{
  info->head.prev = &info->head;
  info->head.next = &info->head;

  info->add_tail = list_add_tail;
  info->for_each = list_for_each;
  info->for_each_safe = list_for_each_safe;
  info->del = list_del;
  info->isempty = list_isempty;
  info->destroy = list_destroy;
}
