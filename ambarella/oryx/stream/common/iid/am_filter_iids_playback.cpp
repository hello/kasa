/*******************************************************************************
 * am_filter_iids.cpp
 *
 * History:
 *   2014-11-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_amf_types.h"

/* Playback related IIDs */

/* {8e18143f-eca4-47e4-bd45-575825aec52b} */
AM_DEFINE_IID(IID_AMIPlaybackEngine,
0x8e18143f, 0xeca4, 0x47e4, 0xbd, 0x45, 0x57, 0x58, 0x25, 0xae, 0xc5, 0x2b);

/* {0f2318c2-e768-4986-8f5e-169f0ad3cd71} */
AM_DEFINE_IID(IID_AMIAudioDecoder,
0x0f2318c2, 0xe768, 0x4986, 0x8f, 0x5e, 0x16, 0x9f, 0x0a, 0xd3, 0xcd, 0x71);

/* {6903c867-f2e1-43ac-b069-824ac5b9f7df} */
AM_DEFINE_IID(IID_AMIFileDemuxer,
0x6903c867, 0xf2e1, 0x43ac, 0xb0, 0x69, 0x82, 0x4a, 0xc5, 0xb9, 0xf7, 0xdf);

/* {5c6efb56-220a-4967-b384-e854ae2e118a} */
AM_DEFINE_IID(IID_AMIPlayer,
0x5c6efb56, 0x220a, 0x4967, 0xb3, 0x84, 0xe8, 0x54, 0xae, 0x2e, 0x11, 0x8a);
