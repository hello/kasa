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


#ifndef _MDIO_REG_REG_H_
#define _MDIO_REG_REG_H_

#define MDIO_REG_ADDRESS                         0x00000000
#define MDIO_REG_OFFSET                          0x00000000
#define MDIO_REG_VALUE_MSB                       15
#define MDIO_REG_VALUE_LSB                       0
#define MDIO_REG_VALUE_MASK                      0x0000ffff
#define MDIO_REG_VALUE_GET(x)                    (((x) & MDIO_REG_VALUE_MASK) >> MDIO_REG_VALUE_LSB)
#define MDIO_REG_VALUE_SET(x)                    (((x) << MDIO_REG_VALUE_LSB) & MDIO_REG_VALUE_MASK)

#define MDIO_ISR_ADDRESS                         0x00000020
#define MDIO_ISR_OFFSET                          0x00000020
#define MDIO_ISR_MASK_MSB                        15
#define MDIO_ISR_MASK_LSB                        8
#define MDIO_ISR_MASK_MASK                       0x0000ff00
#define MDIO_ISR_MASK_GET(x)                     (((x) & MDIO_ISR_MASK_MASK) >> MDIO_ISR_MASK_LSB)
#define MDIO_ISR_MASK_SET(x)                     (((x) << MDIO_ISR_MASK_LSB) & MDIO_ISR_MASK_MASK)
#define MDIO_ISR_REGS_MSB                        7
#define MDIO_ISR_REGS_LSB                        0
#define MDIO_ISR_REGS_MASK                       0x000000ff
#define MDIO_ISR_REGS_GET(x)                     (((x) & MDIO_ISR_REGS_MASK) >> MDIO_ISR_REGS_LSB)
#define MDIO_ISR_REGS_SET(x)                     (((x) << MDIO_ISR_REGS_LSB) & MDIO_ISR_REGS_MASK)

#define PHY_ADDR_ADDRESS                         0x00000024
#define PHY_ADDR_OFFSET                          0x00000024
#define PHY_ADDR_VAL_MSB                         2
#define PHY_ADDR_VAL_LSB                         0
#define PHY_ADDR_VAL_MASK                        0x00000007
#define PHY_ADDR_VAL_GET(x)                      (((x) & PHY_ADDR_VAL_MASK) >> PHY_ADDR_VAL_LSB)
#define PHY_ADDR_VAL_SET(x)                      (((x) << PHY_ADDR_VAL_LSB) & PHY_ADDR_VAL_MASK)


#ifndef __ASSEMBLER__

typedef struct mdio_reg_reg_s {
  volatile unsigned int mdio_reg[8];
  volatile unsigned int mdio_isr;
  volatile unsigned int phy_addr;
} mdio_reg_reg_t;

#endif /* __ASSEMBLER__ */

#endif /* _MDIO_REG_H_ */
