/*******************************************************************************
 * am_image_quality_if.h
 *
 * History:
 *   Dec 31, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2018, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_IMAGE_QUALITY_IF_H_
#define AM_IMAGE_QUALITY_IF_H_

#include "am_iq_param.h"
#include "am_pointer.h"

class AMIImageQuality;

typedef AMPointer<AMIImageQuality> AMIImageQualityPtr;

class AMIImageQuality
{
    friend AMIImageQualityPtr;
  public:
    static AMIImageQualityPtr get_instance();
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool load_config() = 0;
    virtual bool save_config() = 0;
    virtual bool set_config(AM_IQ_CONFIG *config) = 0;
    virtual bool get_config(AM_IQ_CONFIG *config) = 0;
  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIImageQuality()
    {
    }
};

#endif /* AM_IMAGE_QUALITY_IF_H_ */
