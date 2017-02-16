/**
 * am_event_sender_version.h
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_EVENT_SENDER_VERSION_H_
#define _AM_EVENT_SENDER_VERSION_H_

#define EVENT_SENDER_VERSION_MAJOR 1
#define EVENT_SENDER_VERSION_MINOR 0
#define EVENT_SENDER_VERSION_PATCH 0

#define EVENT_SENDER_VERSION_NUMBER \
    ((EVENT_SENDER_VERSION_MAJOR << 16) | \
     (EVENT_SENDER_VERSION_MINOR <<  8) | \
      EVENT_SENDER_VERSION_PATCH)

#endif /* _AM_EVENT_SENDER_VERSION_H_ */
