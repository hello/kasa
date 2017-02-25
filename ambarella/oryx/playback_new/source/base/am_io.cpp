/**
 * am_io.cpp
 *
 * History:
 *  2013/09/09 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"

#include "am_framework.h"

extern IIO *gfCreateFileIO();
extern IDirANSI *gfCreateDirentDirANSI();

IIO *gfCreateIO(EIOType type)
{
  IIO *thiz = NULL;
  switch (type) {
    case EIOType_File:
      thiz = gfCreateFileIO();
      break;
    case EIOType_HTTP:
      LOG_FATAL("not implement yet\n");
      break;
    default:
      LOG_FATAL("BAD EIOType %d\n", type);
      break;
  }
  return thiz;
}

EECode IDirANSI::BuildDisplayList(SDirNodeANSI *root, CIDoubleLinkedList  *&display_list)
{
  SDirNodeANSI *node = NULL;
  DASSERT(root);
  display_list = new CIDoubleLinkedList();
  if (display_list) {
    node = firstNode(root);
    if (node) {
      LOG_NOTICE("first node, %s\n", node->name);
    } else {
      LOG_ERROR("no first node\n");
    }
    while (node) {
      if (isMediaFile(node)) {
        LOG_NOTICE("media file %s\n", node->name);
        display_list->InsertContent(NULL, (void *) node, 0);
      } else {
        LOG_NOTICE("not media file %s\n", node->name);
      }
      node = nextNode(root, node);
    }
  } else {
    LOG_FATAL("no memory\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

void IDirANSI::CleanTree(SDirNodeANSI *p_dir)
{
  cleanNode(p_dir);
}

void IDirANSI::CleanDisplayList(CIDoubleLinkedList *display_list)
{
  if (display_list) {
    delete display_list;
  }
}

TU32 IDirANSI::isMediaFile(SDirNodeANSI *node)
{
  DASSERT(node);
  if (node->is_dir) {
    return 0;
  }
  if ((!(strcmp("mp4", node->file_ext))) || (!(strcmp("MP4", node->file_ext)))) {
    return 1;
  }
  return 0;
}

SDirNodeANSI *IDirANSI::firstNode(SDirNodeANSI *root)
{
  SDirNodeANSI *node = NULL;
  if (root) {
    if (root->p_child_list) {
      node = root->p_child_list;
      while (node) {
        if (node->p_child_list) {
          node = node->p_child_list;
        } else {
          return node;
        }
      }
    } else {
      LOG_WARN("empty dir\n");
    }
  } else {
    LOG_FATAL("NULL param\n");
  }
  return NULL;
}

SDirNodeANSI *IDirANSI::firstChild(SDirNodeANSI *node)
{
  SDirNodeANSI *child = NULL;
  DASSERT(node);
  if (node) {
    if (node->p_child_list) {
      child = node->p_child_list;
      while (child) {
        if (child->p_child_list) {
          child = child->p_child_list;
        } else {
          return child;
        }
      }
    }
  }
  return NULL;
}

SDirNodeANSI *IDirANSI::nextNode(SDirNodeANSI *root, SDirNodeANSI *node)
{
  SDirNodeANSI *tmp = NULL;
  if (node != root) {
    if (node->p_next) {
      tmp = firstChild(node->p_next);
      if (tmp) {
        return tmp;
      } else {
        return node->p_next;
      }
    } else if (node->p_parent != root) {
      DASSERT(node->p_parent);
      tmp = firstChild(node->p_parent);
      if (tmp) {
        return tmp;
      } else {
        return node->p_parent;
      }
    } else {
      return NULL;
    }
  } else {
    LOG_FATAL("must not here\n");
  }
  return NULL;
}

SDirNodeANSI *IDirANSI::allocNode()
{
  SDirNodeANSI *p = (SDirNodeANSI *) DDBG_MALLOC(sizeof(SDirNodeANSI), "DAND");
  if (p) {
    memset(p, 0x0, sizeof(SDirNodeANSI));
  }
  return p;
}

void IDirANSI::releaseNode(SDirNodeANSI *node)
{
  if (node) {
    if (node->name) {
      DDBG_FREE(node->name, "DANM");
      node->name = NULL;
    }
    DASSERT(!node->p_next);
    DASSERT(!node->p_child_list);
  } else {
    LOG_FATAL("NULL node\n");
  }
}

void IDirANSI::cleanNode(SDirNodeANSI *node)
{
  if (DLikely(node)) {
    if (node->p_next) {
      cleanNode(node->p_next);
      node->p_next = NULL;
    }
    if (node->p_child_list) {
      cleanChildList(node->p_child_list);
      node->p_child_list = NULL;
    }
    releaseNode(node);
  } else {
    LOG_FATAL("NULL node\n");
  }
}

void IDirANSI::cleanChildList(SDirNodeANSI *child_list)
{
  if (DLikely(child_list)) {
    SDirNodeANSI *p_next_child = NULL;
    while (child_list) {
      p_next_child = child_list->p_next;
      cleanNode(child_list);
      child_list = p_next_child;
    }
  } else {
    LOG_FATAL("NULL node\n");
  }
}

void IDirANSI::appendChild(SDirNodeANSI *node, SDirNodeANSI *child_node)
{
  if (node && child_node) {
    child_node->p_parent = node;
    child_node->p_next = node->p_child_list;
    node->p_child_list = child_node;
  }
}

#if 0
void IDirANSI::appendChildDir(SDirNodeANSI *node, SDirNodeANSI *child_dir_node)
{
  if (node && child_dir_node) {
    child_dir_node->p_parent = node;
    child_dir_node->p_next = node->p_child_dir_list;
    node->p_child_dir_list = child_dir_node;
  }
}

void IDirANSI::appendNext(SDirNodeANSI *node, SDirNodeANSI *next_node)
{
  if (node && next_node) {
    next_node->p_parent = node->p_parent;
    next_node->p_next = node->p_next;
    node->p_next = next_node;
  }
}
#endif

IDirANSI *gfCreateIDirANSI(EDirType type)
{
  IDirANSI *thiz = NULL;
  switch (type) {
    case EDirType_Dirent:
      thiz = gfCreateDirentDirANSI();
      break;
    default:
      LOG_FATAL("BAD EIOType %d\n", type);
      break;
  }
  return thiz;
}

