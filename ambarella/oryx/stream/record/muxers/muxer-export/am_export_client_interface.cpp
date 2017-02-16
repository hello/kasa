/*******************************************************************************
 * am_export_client_interface.cpp
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
#include "am_log.h"

#include "am_export_if.h"

extern AMIExportClient *am_create_export_client_uds(AMExportConfig *config);

AMIExportClient *am_create_export_client(AM_EXPORT_CLIENT_TYPE type,
                                         AMExportConfig *config)
{
  switch (type) {
    case AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET:
      return am_create_export_client_uds(config);
      break;
    default:
      ERROR("not supported type %d", type);
      break;
  }

  return NULL;
}
