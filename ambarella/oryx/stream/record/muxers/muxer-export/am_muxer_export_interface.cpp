/*******************************************************************************
 * am_muxer_export_interface.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He] created file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_export_if.h"
#include "am_muxer_codec_if.h"

extern AMIMuxerCodec *am_create_muxer_export_uds();

AMIMuxerCodec *am_create_muxer_export(int type)
{
  switch (type) {
    case AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET:
      return am_create_muxer_export_uds();
      break;
    default:
      ERROR("not supported type %d", type);
      break;
  }

  return NULL;
}

AMIMuxerCodec* get_muxer_codec(const char* config)
{
  NOTICE("[debug]: get_muxer_codec() here");
  return am_create_muxer_export(AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET);
}


