/*******************************************************************************
 * am_encode_system_capability.h
 *
 * History:
 *   2014-8-7 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_ENCODE_SYSTEM_CAPABILITY_H_
#define AM_ENCODE_SYSTEM_CAPABILITY_H_

#include "am_video_dsp.h"

class AMEncodeSystemCapability
{
  public:
    AMEncodeSystemCapability();
    ~AMEncodeSystemCapability();


    AM_RESULT get_encode_performance(AMEncodePerformance *perf);

    AM_RESULT get_encode_performance_max(AM_ENCODE_MODE mode,
                                         AMEncodePerformance *perf);

    AM_RESULT find_mode_for_dsp_capability(AMEncodeDSPCapability *cap,
                                           AM_ENCODE_MODE *next_mode);

  protected:
    AM_RESULT get_encode_mode_capability(AM_ENCODE_MODE mode,
                                         AMEncodeDSPCapability *dsp_cap);

  protected:
    AMEncodePerformance m_current_performance; //current total performance level
};

#endif /* AM_ENCODE_SYSTEM_CAPABILITY_H_ */
