/**********************************************************************
 * NetCommand.h
 *
 * Histroy:
 *  2011年03月22日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#ifndef APP_IPCAM_CONTROLSERVER_NETCOMMAND_H
#define APP_IPCAM_CONTROLSERVER_NETCOMMAND_H
/* DHCP, STATIC - use UDP multicast
 * QUERY        - use UDP unicast
 * RTSP         - use UDP unicast
 * REBOOT       - use UDP unicast
 * GET_*        - use UDP unicast
 * SET_*        - use UDP unicast
 */
enum NetCmdType {/* Network Related */
                 NOACTION = 0,
                 DHCP     = 1,
                 STATIC   = 2,
                 QUERY    = 3,
                 RTSP     = 4,
                 REBOOT   = 5,
                 /* Video Related */
                 GET_ENCODE_SETTING = 6,
                 GET_IMAGE_SETTING  = 7,
                 GET_PRIVACY_MASK_SETTING = 8,
                 GET_VIN_VOUT_SETTING     = 9,
                 SET_ENCODE_SETTING       = 10,
                 SET_IMAGE_SETTING        = 11,
                 SET_PRIVACY_MASK_SETTING = 12,
                 SET_VIN_VOUT_SETTING     = 13,
                 NETBOOT   = 14};
#endif //APP_IPCAM_CONTROLSERVER_NETCOMMAND_H

