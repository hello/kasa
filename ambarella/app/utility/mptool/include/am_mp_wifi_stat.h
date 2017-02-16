/*******************************************************************************
 * am_mp_wifi_stat.h
 *
 * History:
 *   Mar 18, 2015 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#ifndef _AM_MP_WIFI_STAT_H_
#define _AM_MP_WIFI_STAT_H_

/* Handler to process request relating to wifi issues from client. */
am_mp_err_t mptool_wifi_stat_handler(am_mp_msg_t *from, am_mp_msg_t *to);

#endif /* _AM_WIFI_STAT_H_ */
