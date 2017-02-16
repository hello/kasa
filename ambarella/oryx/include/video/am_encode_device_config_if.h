/**
 * am_encode_device_config_if.h
 *
 *  History:
 *		Jan 8, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_ENCODE_DEVICE_CONFIG_IF_H_
#define _AM_ENCODE_DEVICE_CONFIG_IF_H_

#include "am_video_types.h"

struct AMEncodeParamAll;
class AMIEncodeDeviceConfig {
  public:
    virtual AM_RESULT load_config() = 0;
    virtual AM_RESULT save_config() = 0;
    virtual AM_RESULT sync() = 0;
    virtual AM_RESULT set_stream_format(AM_STREAM_ID id,
                                        AMEncodeStreamFormat *stream_format) = 0;
    virtual AM_RESULT set_stream_config(AM_STREAM_ID id,
                                        AMEncodeStreamConfig *stream_config) = 0;
    virtual AMEncodeParamAll* get_encode_param() = 0;
    virtual AM_RESULT
    set_buffer_alloc_style(AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE style) = 0;
    virtual AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE get_buffer_alloc_style() = 0;

    virtual ~AMIEncodeDeviceConfig(){}
};

#endif /* _AM_ENCODE_DEVICE_CONFIG_IF_H_ */
