/*******************************************************************************
 * am_mp_mac_addr.h
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

#ifndef _AM_MP_MAC_ADDR_H_
#define _AM_MP_MAC_ADDR_H_

/* Handler to process request relating to mac address issues from client. */
am_mp_err_t mptool_mac_addr_handler(am_mp_msg_t *from, am_mp_msg_t *to);

#endif
