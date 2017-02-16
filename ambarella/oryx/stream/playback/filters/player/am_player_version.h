/*******************************************************************************
 * am_player_version.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYER_VERSION_H_
#define AM_PLAYER_VERSION_H_

#define PLAYER_VERSION_MAJOR 1
#define PLAYER_VERSION_MINOR 0
#define PLAYER_VERSION_PATCH 0
#define PLAYER_VERSION_NUMBER ((PLAYER_VERSION_MAJOR << 16) | \
                               (PLAYER_VERSION_MINOR <<  8) | \
                               PLAYER_VERSION_PATCH)

#endif /* AM_PLAYER_VERSION_H_ */
