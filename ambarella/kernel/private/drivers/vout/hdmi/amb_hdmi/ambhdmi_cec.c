/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_cec.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
	void __iomem  *reg;
        u32     val;
        int     i, j, timeout;

        for (i = 0; i < ARRAY_SIZE(candidate_addresses); i++) {
                /* Prepare to Tx */
                reg = phdmi_sink->regbase + HDMI_INT_STS_OFFSET;
                val = readl_relaxed(reg);
                val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_OK;
                val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL;
                writel_relaxed(val, reg);

                reg = phdmi_sink->regbase + CEC_TX_DATA0_OFFSET;
                val = (candidate_addresses[i] << 28) |
                      (candidate_addresses[i] << 24);
                writel_relaxed(val, reg);

                /* Wait for Signal Free */
                timeout = 125;
                reg = phdmi_sink->regbase + CEC_STATUS_OFFSET;
                for (j = 0; j < timeout; j++) {
                        val = readl_relaxed(reg);
                        if (val & CEC_STATUS_RX_STA_RX_IDLE3)
                                break;
                        mdelay(4);
                }

                /* Tx Message */
                if (j >= timeout)
                        continue;
                mdelay(2);

                reg = phdmi_sink->regbase + CEC_CTRL_OFFSET;
                val = readl_relaxed(reg);
                val &= ~(0xf << 26);
                val |= ((0x0 << 26) | (0x1 << 30));
                writel_relaxed(val, reg);

                /* Track Tx result */
                reg = phdmi_sink->regbase + HDMI_INT_STS_OFFSET;
                timeout = 1000;
                for (j = 0; j < timeout; j++) {
                        val = readl_relaxed(reg);
                        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                                break;
                        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                                break;
                        mdelay(1);
                }

                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                        break;

                reg = phdmi_sink->regbase + CEC_CTRL_OFFSET;
                val = readl_relaxed(reg);
                val &= ~(0x1 << 30);
                writel_relaxed(val, reg);
        }

        if (i < ARRAY_SIZE(candidate_addresses))
                return candidate_addresses[i];

        return CEC_DEV_UNREGISTERED;
}

int ambhdmi_cec_report_physical_address(struct ambhdmi_sink *phdmi_sink)
{
	void __iomem  *reg;
        u32     val;
        int     i, timeout;

        /* Prepare to Tx */
        reg = phdmi_sink->regbase + HDMI_INT_STS_OFFSET;
        val = readl_relaxed(reg);
        val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_OK;
        val &= ~HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL;
        writel_relaxed(val, reg);

        reg = phdmi_sink->regbase + CEC_TX_DATA0_OFFSET;
        val = (phdmi_sink->edid.cec.logical_address << 28) |
              (CEC_DEV_BROADCAST << 24);
        val |= (0x84 << 16);
        val |= phdmi_sink->edid.cec.physical_address;
        writel_relaxed(val, reg);

        reg = phdmi_sink->regbase + CEC_TX_DATA1_OFFSET;
        val = (CEC_DEV_RECORDING1 << 24);

        /* Wait for Signal Free */
        timeout = 125;
        reg = phdmi_sink->regbase + CEC_STATUS_OFFSET;
        for (i = 0; i < timeout; i++) {
                val = readl_relaxed(reg);
                if (val & CEC_STATUS_RX_STA_RX_IDLE3)
                        break;
                mdelay(4);
        }

        /* Tx Message */
        if (i >= timeout)
                return -1;
        mdelay(2);

        reg = phdmi_sink->regbase + CEC_CTRL_OFFSET;
        val = readl_relaxed(reg);
        val &= ~(0xf << 26);
        val |= ((0x4 << 26) | (0x1 << 30));
        writel_relaxed(val, reg);

        /* Track Tx result */
        reg = phdmi_sink->regbase + HDMI_INT_STS_OFFSET;
        timeout = 1000;
        for (i = 0; i < timeout; i++) {
                val = readl_relaxed(reg);
                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                        break;
                if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_FAIL)
                        break;
                mdelay(1);
        }

        if (val & HDMI_INT_STS_CEC_TX_INTERRUPT_OK)
                return 0;

        reg = phdmi_sink->regbase + CEC_CTRL_OFFSET;
        val = readl_relaxed(reg);
        val &= ~(0x1 << 30);
        writel_relaxed(val, reg);

        return -1;
}

void ambhdmi_cec_receive_message(struct ambhdmi_sink *phdmi_sink)
{
	void __iomem  *reg;
        u32     val;
        u32     i, j, k, blocks, buf[4];
        u8      buf2[4 * 4];

        buf[0] = readl_relaxed(phdmi_sink->regbase + CEC_RX_DATA0_OFFSET);
        val = (buf[0] & 0x0f000000) >> 24;
        if (val != phdmi_sink->edid.cec.logical_address
            && val != CEC_DEV_BROADCAST)
                return;

        reg = phdmi_sink->regbase + CEC_CTRL_OFFSET;
        val = readl_relaxed(reg);
        blocks = ((val >> 22) & 0xf) + 1;
        if (blocks == 1)
                return;

        buf[1] = readl_relaxed(phdmi_sink->regbase + CEC_RX_DATA1_OFFSET);
        buf[2] = readl_relaxed(phdmi_sink->regbase + CEC_RX_DATA2_OFFSET);
        buf[3] = readl_relaxed(phdmi_sink->regbase + CEC_RX_DATA3_OFFSET);

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

