/*******************************************************************************
 * am_filter_record_iids.cpp
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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

/* Record related IIDs */

/* {f6e82960-8024-4769-bec6-fecaf1e05275} */
AM_DEFINE_IID(IID_AMIRecordEngine,
0xf6e82960, 0x8024, 0x4769, 0xbe, 0xc6, 0xfe, 0xca, 0xf1, 0xe0, 0x52, 0x75);

/* {1d3daaf2-00da-4cef-a81f-d3c5b09bc66c} */
AM_DEFINE_IID(IID_AMIAudioSource,
0x1d3daaf2, 0x00da, 0x4cef, 0xa8, 0x1f, 0xd3, 0xc5, 0xb0, 0x9b, 0xc6, 0x6c);

/* {af79d488-bc9c-4d3f-aa44-4451729a8172} */
AM_DEFINE_IID(IID_AMIVideoSource,
0xaf79d488, 0xbc9c, 0x4d3f, 0xaa, 0x44, 0x44, 0x51, 0x72, 0x9a, 0x81, 0x72);

/* {3eddba0a-c3b6-11e4-9d6c-4b17971879f7} */
AM_DEFINE_IID(IID_AMIEventSender,
0x3eddba0a, 0xc3b6, 0x11e4, 0x9d, 0x6c, 0x4b, 0x17, 0x97, 0x18, 0x79, 0xf7);

/* {a3d68c6e-535e-438f-98f3-d8583f95e6bf} */
AM_DEFINE_IID(IID_AMIAVQueue,
0xa3d68c6e, 0x535e, 0x438f, 0x98, 0xf3, 0xd8, 0x58, 0x3f, 0x95, 0xe6, 0xbf);

/* {27529a9f-5bd1-4a6f-b285-05eaa7b3070e} */
AM_DEFINE_IID(IID_AMIMuxer,
0x27529a9f, 0x5bd1, 0x4a6f, 0xb2, 0x85, 0x05, 0xea, 0xa7, 0xb3, 0x07, 0x0e);
