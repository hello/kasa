/**
 * am_rest_api_media.h
 *
 *  History:
 *		2015年8月19日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_MEDIA_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_MEDIA_H_

#include "am_rest_api_handle.h"

class AMRestAPIMedia: public AMRestAPIHandle
{
  public:
    virtual AM_REST_RESULT  rest_api_handle();

  private:
    AM_REST_RESULT  media_recording_handle();
    AM_REST_RESULT  media_file_recording_handle();
    AM_REST_RESULT  media_event_recording_handle();

  private:
    AM_REST_RESULT  media_audio_playback_handle();
    //set audio file to system, it always must be run
    //before start action, except preview action is pause
    AM_REST_RESULT  audio_playback_set_handle();
    AM_REST_RESULT  audio_playback_start_handle();
    AM_REST_RESULT  audio_playback_stop_handle();
    AM_REST_RESULT  audio_playback_pause_handle();

};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_MEDIA_H_ */
