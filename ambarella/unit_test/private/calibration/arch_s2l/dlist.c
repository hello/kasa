/*
 * dlist.c
 *
 * Histroy:
 *   Dec 5, 2013 - [Shupeng Ren] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 */

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
