// ------------------------------------------------------------------
// Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
// ------------------------------------------------------------------
//===================================================================
// Author(s): ="Atheros"
//===================================================================


#ifndef _MII_REG_REG_H_
#define _MII_REG_REG_H_

#define MII0_CNTL_ADDRESS                        0x00000000
#define MII0_CNTL_OFFSET                         0x00000000
#define MII0_CNTL_RGMII_DELAY_MSB                9
#define MII0_CNTL_RGMII_DELAY_LSB                8
#define MII0_CNTL_RGMII_DELAY_MASK               0x00000300
#define MII0_CNTL_RGMII_DELAY_GET(x)             (((x) & MII0_CNTL_RGMII_DELAY_MASK) >> MII0_CNTL_RGMII_DELAY_LSB)
#define MII0_CNTL_RGMII_DELAY_SET(x)             (((x) << MII0_CNTL_RGMII_DELAY_LSB) & MII0_CNTL_RGMII_DELAY_MASK)
#define MII0_CNTL_SPEED_MSB                      5
#define MII0_CNTL_SPEED_LSB                      4
#define MII0_CNTL_SPEED_MASK                     0x00000030
#define MII0_CNTL_SPEED_GET(x)                   (((x) & MII0_CNTL_SPEED_MASK) >> MII0_CNTL_SPEED_LSB)
#define MII0_CNTL_SPEED_SET(x)                   (((x) << MII0_CNTL_SPEED_LSB) & MII0_CNTL_SPEED_MASK)
#define MII0_CNTL_MASTER_MSB                     2
#define MII0_CNTL_MASTER_LSB                     2
#define MII0_CNTL_MASTER_MASK                    0x00000004
#define MII0_CNTL_MASTER_GET(x)                  (((x) & MII0_CNTL_MASTER_MASK) >> MII0_CNTL_MASTER_LSB)
#define MII0_CNTL_MASTER_SET(x)                  (((x) << MII0_CNTL_MASTER_LSB) & MII0_CNTL_MASTER_MASK)
#define MII0_CNTL_SELECT_MSB                     1
#define MII0_CNTL_SELECT_LSB                     0
#define MII0_CNTL_SELECT_MASK                    0x00000003
#define MII0_CNTL_SELECT_GET(x)                  (((x) & MII0_CNTL_SELECT_MASK) >> MII0_CNTL_SELECT_LSB)
#define MII0_CNTL_SELECT_SET(x)                  (((x) << MII0_CNTL_SELECT_LSB) & MII0_CNTL_SELECT_MASK)

#define STAT_CNTL_ADDRESS                        0x00000004
#define STAT_CNTL_OFFSET                         0x00000004
#define STAT_CNTL_GIG_MSB                        3
#define STAT_CNTL_GIG_LSB                        3
#define STAT_CNTL_GIG_MASK                       0x00000008
#define STAT_CNTL_GIG_GET(x)                     (((x) & STAT_CNTL_GIG_MASK) >> STAT_CNTL_GIG_LSB)
#define STAT_CNTL_GIG_SET(x)                     (((x) << STAT_CNTL_GIG_LSB) & STAT_CNTL_GIG_MASK)
#define STAT_CNTL_STEN_MSB                       2
#define STAT_CNTL_STEN_LSB                       2
#define STAT_CNTL_STEN_MASK                      0x00000004
#define STAT_CNTL_STEN_GET(x)                    (((x) & STAT_CNTL_STEN_MASK) >> STAT_CNTL_STEN_LSB)
#define STAT_CNTL_STEN_SET(x)                    (((x) << STAT_CNTL_STEN_LSB) & STAT_CNTL_STEN_MASK)
#define STAT_CNTL_CLRCNT_MSB                     1
#define STAT_CNTL_CLRCNT_LSB                     1
#define STAT_CNTL_CLRCNT_MASK                    0x00000002
#define STAT_CNTL_CLRCNT_GET(x)                  (((x) & STAT_CNTL_CLRCNT_MASK) >> STAT_CNTL_CLRCNT_LSB)
#define STAT_CNTL_CLRCNT_SET(x)                  (((x) << STAT_CNTL_CLRCNT_LSB) & STAT_CNTL_CLRCNT_MASK)
#define STAT_CNTL_AUTOZ_MSB                      0
#define STAT_CNTL_AUTOZ_LSB                      0
#define STAT_CNTL_AUTOZ_MASK                     0x00000001
#define STAT_CNTL_AUTOZ_GET(x)                   (((x) & STAT_CNTL_AUTOZ_MASK) >> STAT_CNTL_AUTOZ_LSB)
#define STAT_CNTL_AUTOZ_SET(x)                   (((x) << STAT_CNTL_AUTOZ_LSB) & STAT_CNTL_AUTOZ_MASK)


#ifndef __ASSEMBLER__

typedef struct mii_reg_reg_s {
  volatile unsigned int mii0_cntl;
  volatile unsigned int stat_cntl;
} mii_reg_reg_t;

#endif /* __ASSEMBLER__ */

#endif /* _MII_REG_H_ */
