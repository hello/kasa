/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_cec.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */

amba_hdmi_cec_logical_address_t ambhdmi_cec_allocate_logical_address(
        struct ambhdmi_sink *phdmi_sink)
{
        amba_hdmi_cec_logical_address_t candidate_addresses[] = {
                CEC_DEV_RECORDING1,
                CEC_DEV_RECORDING2,
                CEC_DEV_RECORDING3
        };
        u32     regbase, reg, val;
        int     i, j, timeout;

        regbase = phdmi_sink->regbase;
        for (i = 0; i < ARRAY_SIZE(candidate_addresses); i++) {
                /* Prepare to Tx */
                reg = regbase + HDMI_INT_STS_OFFSET;
                val = amba_readl(reg);
                val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_OK;
                val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL;
                amba_writel(reg, val);

                reg = regbase + CEC_TX_DATA0_OFFSET;
                val = (candidate_addresses[i] << 28) |
                      (candidate_addresses[i] << 24);
                amba_writel(reg, val);

                /* Wait for Signal Free */
                timeout = 125;
                reg = regbase + CEC_STATUS_OFFSET;
                for (j = 0; j < timeout; j++) {
                        val = amba_readl(reg);
                        if (val & CEC_STATUS_RX_STA_RX_IDLE3)
                                break;
                        mdelay(4);
                }

                /* Tx Message */
                if (j >= timeout)
                        continue;
                mdelay(2);

                reg = regbase + CEC_CTRL_OFFSET;
                val = amba_readl(reg);
                val &= ~(0xf << 26);
                val |= ((0x0 << 26) | (0x1 << 30));
                amba_writel(reg, val);

                /* Track Tx result */
                reg = regbase + HDMI_INT_STS_OFFSET;
                timeout = 1000;
                for (j = 0; j < timeout; j++) {
                        val = amba_readl(reg);
                        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                                break;
                        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                                break;
                        mdelay(1);
                }

                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                        break;

                reg = regbase + CEC_CTRL_OFFSET;
                val = amba_readl(reg);
                val &= ~(0x1 << 30);
                amba_writel(reg, val);
        }

        if (i < ARRAY_SIZE(candidate_addresses))
                return candidate_addresses[i];

        return CEC_DEV_UNREGISTERED;
}

int ambhdmi_cec_report_physical_address(struct ambhdmi_sink *phdmi_sink)
{
        u32     regbase, reg, val;
        int     i, timeout;

        regbase = phdmi_sink->regbase;

        /* Prepare to Tx */
        reg = regbase + HDMI_INT_STS_OFFSET;
        val = amba_readl(reg);
        val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_OK;
        val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL;
        amba_writel(reg, val);

        reg = regbase + CEC_TX_DATA0_OFFSET;
        val = (phdmi_sink->edid.cec.logical_address << 28) |
              (CEC_DEV_BROADCAST << 24);
        val |= (0x84 << 16);
        val |= phdmi_sink->edid.cec.physical_address;
        amba_writel(reg, val);

        reg = regbase + CEC_TX_DATA1_OFFSET;
        val = (CEC_DEV_RECORDING1 << 24);

        /* Wait for Signal Free */
        timeout = 125;
        reg = regbase + CEC_STATUS_OFFSET;
        for (i = 0; i < timeout; i++) {
                val = amba_readl(reg);
                if (val & CEC_STATUS_RX_STA_RX_IDLE3)
                        break;
                mdelay(4);
        }

        /* Tx Message */
        if (i >= timeout)
                return -1;
        mdelay(2);

        reg = regbase + CEC_CTRL_OFFSET;
        val = amba_readl(reg);
        val &= ~(0xf << 26);
        val |= ((0x4 << 26) | (0x1 << 30));
        amba_writel(reg, val);

        /* Track Tx result */
        reg = regbase + HDMI_INT_STS_OFFSET;
        timeout = 1000;
        for (i = 0; i < timeout; i++) {
                val = amba_readl(reg);
                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                        break;
                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                        break;
                mdelay(1);
        }

        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                return 0;

        reg = regbase + CEC_CTRL_OFFSET;
        val = amba_readl(reg);
        val &= ~(0x1 << 30);
        amba_writel(reg, val);

        return -1;
}

void ambhdmi_cec_receive_message(struct ambhdmi_sink *phdmi_sink)
{
        u32     regbase, reg, val;
        u32     i, j, k, blocks, buf[4];
        u8      buf2[4 * 4];

        regbase = phdmi_sink->regbase;
        buf[0] = amba_readl(regbase + CEC_RX_DATA0_OFFSET);
        val = (buf[0] & 0x0f000000) >> 24;
        if (val != phdmi_sink->edid.cec.logical_address
            && val != CEC_DEV_BROADCAST)
                return;

        reg = regbase + CEC_CTRL_OFFSET;
        val = amba_readl(reg);
        blocks = ((val >> 22) & 0xf) + 1;
        if (blocks == 1)
                return;

        buf[1] = amba_readl(regbase + CEC_RX_DATA1_OFFSET);
        buf[2] = amba_readl(regbase + CEC_RX_DATA2_OFFSET);
        buf[3] = amba_readl(regbase + CEC_RX_DATA3_OFFSET);

        for (i = 0; i < blocks; i++) {
                j = i >> 2;
                k = i & 0x3;
                switch (k) {
                case 0:
                        buf2[i] = (buf[j] & 0xff000000) >> 24;
                        break;
                case 1:
                        buf2[i] = (buf[j] & 0x00ff0000) >> 16;
                        break;
                case 2:
                        buf2[i] = (buf[j] & 0x0000ff00) >> 8;
                        break;
                case 3:
                        buf2[i] = buf[j] & 0x000000ff;
                        break;
                default:
                        break;
                }
        }

        DRV_PRINT("Received Message: Initiator = %2d, Destination = %2d\n",
                  (buf2[0] & 0xf0) >> 4, buf2[0] & 0xf);
        for (i = 0; i < ARRAY_SIZE(amba_hdmi_cec_opcode_table); i++) {
                if (amba_hdmi_cec_opcode_table[i].opcode == buf2[1]) {
                        DRV_PRINT("		  Operation: %s\n",
                                  amba_hdmi_cec_opcode_table[i].name);
                        break;
                }
        }
        if (i >= ARRAY_SIZE(amba_hdmi_cec_opcode_table)) {
                DRV_PRINT("		  Unknown Operation\n");
                return;
        }
        j = i;

        DRV_PRINT("		  Parameters: ");
        for (i = 2; i < blocks; i++)
                DRV_PRINT("%02x ", buf2[i]);
        DRV_PRINT("\n");

        if (amba_hdmi_cec_opcode_table[j].need_respond == YES)
                DRV_PRINT("		  Response Needed!\n");
        DRV_PRINT("\n");
}

