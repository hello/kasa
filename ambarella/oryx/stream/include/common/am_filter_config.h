/*******************************************************************************
 * am_filter_config.h
 *
 * History:
 *   2014-8-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILTER_CONFIG_H_
#define AM_FILTER_CONFIG_H_

struct AMFilterRTConfig
{
    bool     enabled;
    uint32_t priority;
};

struct AMFilterConfig
{
    AMFilterRTConfig real_time;
    std::string      name;
    uint32_t         packet_pool_size;
    AMFilterConfig() :
      name(""),
      packet_pool_size(0)
    {
      real_time = {false, 10};
    }
};

#endif /* AM_FILTER_CONFIG_H_ */
