/*
 * Filename : ov5630.h
 *
 * History:
 *    2014/08/18 - [Hao Zeng] Create
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

#ifndef __OV5653_PRI_H__
#define __OV5653_PRI_H__

/* 		register name 				address default value	R/W	description  */
#define OV5653_SYSTEM_RESET00			0x3000	/*0x20 RW Reset for Individual Block (0: enable block; 1: reset block)*/
							/*Bit[7]:	Reset BIST*/
							/*Bit[6:5]: Not used*/
							/*Bit[4]:	Reset OTP memory*/
							/*Bit[3]:	Reset STB*/
							/*Bit[2]:	Reset 5060HZ*/
							/*Bit[1]:*/
							/*Bit[0]:*/
#define OV5653_SYSTEM_RESET01			0x3001	/*0x00 RW Reset for Individual Block */
							/*Bit[7]:	Reset AWB registers*/
							/*Bit[6]:	Not used*/
							/*Bit[5]:	Reset ISP*/
							/*Bit[4]:	Reset FC*/
							/*Bit[3]:	Reset CIF*/
							/*Bit[2]:	Reset BLC*/
							/*Bit[1]:	Reset AEC registers*/
							/*Bit[0]:	Reset AEC*/
#define OV5653_SYSTEM_RESET02			0x3002	/*0x00 RW Reset for Individual Block (0: enable block; 1: reset block)*/
							/*Bit[7]:	Reset VFIFO*/
							/*Bit[6]:	Not used*/
							/*Bit[5]:	Reset FORMAT*/
							/*Bit[4:1]: Not used*/
							/*Bit[0]:	Reset average*/
#define OV5653_SYSTEM_RESET03			0x3003	/*0x00 RW Reset for Individual Block (0: enable block; 1: reset block)*/
							/*Bit[7:4]: Not used*/
							/*Bit[3]:	Reset MIPI receiver*/
							/*Bit[2]:	Reset ISP FC*/
							/*Bit[1]:	Reset MIPI*/
							/*Bit[0]:	Reset DVP*/
#define OV5653_CLOCK_ENABLE00			0x3004	/*0xDF RW Clock Enable Control (0: disable clock; 1: enable clock)*/
							/*Bit[7]: Only*/
							/*Bit[6:5]: Not used*/
							/*Bit[4]:*/
							/*Bit[3]:*/
							/*Bit[2]:*/
							/*Bit[1]:*/
							/*Bit[0]:*/
#define OV5653_CLOCK_ENABLE01			0x3005	/*0xFF RW Clock Enable Control (0: disable clock; 1: enable clock)*/
							/*Bit[7]:	Enable AWB register clock*/
							/*Bit[6]:	Not used*/
							/*Bit[5]:	Enable ISP clock*/
							/*Bit[4]:	Enable FC clock*/
							/*Bit[3]:	Enable CIF clock*/
							/*Bit[2]:	Enable BLC clock*/
							/*Bit[1]:	Enable AEC register clock*/
							/*Bit[0]:	Enable AEC clock*/
#define OV5653_CLOCK_ENABLE02			0x3006	/*0xFF RW Clock Enable Control (0: disable clock; 1: enable clock)*/
							/*Bit[7]:	Not used*/
							/*Bit[6]:	Enable format clock*/
							/*Bit[5:1]: Not used*/
							/*Bit[0]:	Enable average clock*/
#define OV5653_CLOCK_ENABLE03			0x3007	/*0x3F RW Clock Enable Control (0: disable clock; 1: enable clock)*/
							/*Bit[5]:	Enable ISP_FC clock*/
							/*Bit[4]:	Enable MIPI PCLK clock*/
							/*Bit[3]:	Enable MIPI clock*/
							/*Bit[2]:	Enable DVP clock*/
							/*Bit[1]:	Enable VFIFO PCLK clock*/
							/*Bit[0]:	Enable VFIFO SCLK clock*/
#define OV5653_SYSTEM_CTRL0			0x3008	/*0x02 RW*/
							/*Bit[7]:	Reset registers*/
							/*Bit[6]:	Register power down*/
							/*Bit[5]:	Not used*/
							/*Bit[4]:	sc_srb_clk_syn_en*/
							/*Bit[3]:	iso_susp_sel_o*/
							/*Bit[2]:	MIPI_rst_msk_o*/
							/*Bit[1]:	MIPI_susp_msk_o*/
							/*Bit[0]:	MIPI_rst_sel_o*/
#define OV5653_SC_MIPI_PCLK_DIV_CTRL			0x3009	/*0x01 RW*/
							/*Bit[7]:	MIPI_pdiv_sepa*/
							/*Bit[6]:	Not used*/
							/*Bit[5:0]: MIPI PCLK divider*/
#define OV5653_CHIP_ID_H			0x300A	/*- R Chip ID High Byte*/
#define OV5653_CHIP_ID_L			0x300B	/*- R Chip ID Low Byte*/
#define OV5653_SC_SD_SDIV			0x300D	/*0x22 RW*/
							/*Bit[7:3]: Debug mode*/
							/*Bit[2:0]: r_sdiv, divider for 50/60 detection*/
#define OV5653_SC_MIPI_SC_CTRL0			0x300E	/*0x18 RW*/
							/*Bit[7:5]: Not used*/
							/*Bit[4]:	r_phy_pd_MIPI 1:	Power down PHY HS TX*/
							/*Bit[3]:	r_phy_pd_lprx 1:	Power down PHY LP RX module*/
							/*Bit[2]:	MIPI_en 0:	DVP enable */
							/*Bit[1]:1: 1:*/
							/*Bit[0]: 0:	Use ~MIPI release 1/2 and for	lane_disable 1/2 to disable two data lanes 1:Use lane disable 1/2 to disable two data lane*/
#define OV5653_PLL_CTRL_00			0x300F	/*0x8E RW*/
							/*Bit[7:6]: R_SELD5 00: Bypass 01: Divide by 1 10: Divide by 4 11: Divide by 5*/
							/*Bit[5:3]: Debug mode*/
							/*Bit[2]:	R_DIVL 0:	One lane, divide by 2 1:	Two lanes, divide by1*/
							/*Bit[1:0]: R_SELD2P5 00: Bypass 01: Divide by 1 10: Divide by 2 11: Divide by 2.5*/
#define OV5653_PLL_CTRL_01			0x3010	/*0x8E RW*/
							/*Bit[7:4]: R_DIVS, Sdiv*/
							/*Bit[3:0]: R_DIVM, Mdiv*/
#define OV5653_PLL_CTRL_02			0x3011	/*0x8E RW*/
							/*Bit[7]:	PLL bypass*/
							/*Bit[6]:	Debug mode*/
							/*Bit[5:0]: R_DIVP, pll_div*/
#define OV5653_PLL_CTRL_03			0x3012	/*0x02 RW*/
							/*Bit[7:3]: Debug mode*/
							/*Bit[2:0]: R_PREDIV 000: Divide by 1 Only 010: Divide by 2 100: Divide by 3.  101: Divide by 4 110: Divide by 6 111:	Divide by 8*/
#define OV5653_PAD_OUTPUT_ENABLE0			0x3016	/*0x00 RW */
							/*Bit[7:2]: Debug mode*/
							/*Bit[1]:	STROBE output enable*/
							/*Bit[0]:	SIOD output enable*/
#define OV5653_PAD_OUTPUT_ENABLE1			0x3017	/*0x00 RW */
							/*Bit[7]:	FREX output enable*/
							/*Bit[6]:	VSYNC output enable*/
							/*Bit[5]:	HREF output enable*/
							/*Bit[4]:	PCLK output enable*/
							/*Bit[3:0]: D[9:6] output enable*/
#define OV5653_PAD_OUTPUT_ENABLE2			0x3018	/*0x00 RW */
							/*Bit[7:2]: D[5:0] output enable*/
							/*Bit[1:0]: Debug mode*/
#define OV5653_PAD_OUTPUT0			0x3019	/*0x00 RW*/
							/*Bit[7:2]: Debug mode*/
							/*Bit[1]:	STROBE*/
							/*Bit[0]:	SIOD*/
#define OV5653_PAD_OUTPUT1			0x301A	/*0x00 RW*/
							/*Bit[7]:	FREX*/
							/*Bit[6]:	VSYNC*/
							/*Bit[5]:	HREF*/
							/*Bit[4]:	PCLK*/
							/*Bit[3:0]: D[9:6]*/
#define OV5653_PAD_OUTPUT2			0x301B	/*0x00 RW*/
							/*Bit[7:2]: D[5:0]*/
							/*Bit[1:0]: Debug mode*/
#define OV5653_PAD_SELECT_0			0x301C	/*0x00 RW*/
							/*Bit[7:2]: Debug mode*/
							/*Bit[1]:	I/O STROBE select*/
							/*Bit[0]:	I/O SIOD select*/
#define OV5653_PAD_SELECT_1			0x301D	/*0x00 RW*/
							/*Bit[7]:	I/O FREX select*/
							/*Bit[6]:	I/O VSYNC select*/
							/*Bit[5]:	I/O HREF select*/
							/*Bit[4]:	I/O PCLK select*/
							/*Bit[3:0]: I/O D[9:6] select*/
#define OV5653_PAD_SELECT_2			0x301E	/*0x00 RW*/
							/*Bit[7:2]: I/O D[5:0] select*/
							/*Bit[1:0]: Debug mode*/
#define OV5653_SYSTEM_PAD_CTRL			0x302C	/*0x02 RW*/
							/*Bit[7]:	pd_dato_en*/
							/*Bit[6:3]: iP2X3v[3:0]*/
							/*Bit[2]:	man_rst_pon*/
							/*Bit[1]:*/
							/*Bit[0]:	*/
#define OV5653_DVP_CCLK_DIV			0x302F	/*0x02 RW System DVP CCLK Divider Bit[5:0]: Divider for external CCLK sc_a_pwc_pk_o[7:0]*/
#define OV5653_SCCB_ID			0x3100	/*0x6C RW SCCB Slave ID*/
#define OV5653_SCCB_CTRL			0x3101	/*0x03 RW For SCCB Access Only*/
							/*Bit[7:2]: Debug mode*/
							/*Bit[1]:	en_ss_addr_inc*/
							/*Bit[0]:	sccb_en_o*/
#define OV5653_SCCB_SYSREG			0x3102	/*0x00 RW For SCCB Access Only*/
							/*Bit[7]:	Debug mode*/
							/*Bit[6]:	ctrl_rst_MIPIsc*/
							/*Bit[5]:	ctrl_rst_srb*/
							/*Bit[4]:	ctrl_rst_sccb_s*/
							/*Bit[3]:	ctrl_rst_pon_sccb_s*/
							/*Bit[2]:	ctrl_rst_clkmod*/
							/*Bit[1]:	ctrl_MIPI_phy_rst_o*/
							/*Bit[0]:	ctrl_pll_rst_o*/
#define OV5653_SRM_GRUP_ADR0			0x3200	/*0x00 RW*/
#define OV5653_SRM_GRUP_ADR1			0x3201	/*0x40 RW*/
#define OV5653_SRM_GRUP_ADR2			0x3202	/*0x80 RW*/
#define OV5653_SRM_GRUP_ADR3			0x3203	/*0xC0 RW*/
#define OV5653_SRM_GRUP_LEN0			0x320B	/*0x00 RW*/
#define OV5653_SRM_GRUP_LEN1			0x320C	/*0x00 RW*/
#define OV5653_SRM_GRUP_LEN3			0x320D	/*0x00 RW*/
#define OV5653_SRM_GRUP_ACCESS			0x3212 /* 0x00 RW */
							/*Bit[7]:group_hold*/
							/*Bit[6]:group_access_tm*/
							/*Bit[5]:group_launch*/
							/*Bit[4]:group_hold_end*/
							/*Bit[3:0]: group_id, 0~3 (groups for register access)*/
#define OV5653_AWB_RED_GAIN_H			0x3400	/*0x04 RW Bit[3:0]: RED gain[11:8]*/
#define OV5653_AWB_RED_GAIN_L			0x3401	/*0x00 RW Bit[7:0]: RED gain[7:0]*/
#define OV5653_AWB_GREEN_GAIN_H			0x3402	/*0x04 RW Bit[3:0]: GREEN gain[11:8]*/
#define OV5653_AWB_GREEB_GAIN_L			0x3403	/*0x00 RW Bit[7:0]: GREEN gain[7:0]*/
#define OV5653_AWB_BLUE_GAIN_H			0x3404	/*0x04 RW Bit[3:0]: BLUE gain[11:8]*/
#define OV5653_AWB_BLUE_GAIN_L			0x3405	/*0x00 RW Bit[7:0]: BLUE gain[7:0]*/
#define OV5653_AWB_MANUAL_CTRL			0x3406	/*0x00 RW Bit[0]:	AWB gain manual control enable*/

#define OV5653_LONG_EXPO_H			0x3500	/*0x00 RW Bit[7:4]: Not used Bit[3:0]: long_exposure[19:16]*/
#define OV5653_LONG_EXPO_M			0x3501	/*0x00 RW Bit[7:0]: long_exposure[15:8]*/
#define OV5653_LONG_EXPO_L			0x3502	/*0x02 RW Bit[7:0]: long_exposure[7:0]*/
#define OV5653_MANUAL_CTRL			0x3503	/*0x00 RW*/
							/*Bit[7:6]: Not used*/
							/*Bit[5:4]: Gain latch timing delay x0: Gain has no latch delay 01: Gain delay of 1 frame 11: Gain delay of 2 frames*/
							/*Bit[3]:	Not used*/
							/*Bit[2]:	Debug mode*/
							/*Bit[1]:	AGC manual*/
							/*Bit[0]: AEC manual*/
#define OV5653_LONG_GAIN_H			0x3508	/*0x00 RW Bit[7:1]: Debug mode Bit[0]:	long_gain[8]*/
#define OV5653_LONG_GAIN_L			0x3509	/*0x00 RW Bit[7:0]: long_gain[7:0]*/
#define OV5653_AGC_ADJ_H			0x350A	/*0x00 RW Bit[7:1]: Debug mode. Bit[0]:	Gain high bit Gain = (0x350B[6]+1) ¡Á (0x350B[5]+1) ¡Á (0x350B[4]+1) ¡Á (0x350B[3:0]/16+1)*/
#define OV5653_AGC_ADJ_L			0x350B	/*0x00 RW Bit[7:0]: Gain low bits Gain = (0x350B[6]+1) ¡Á (0x350B[5]+1) ¡Á (0x350B[4]+1) ¡Á (0x350B[3:0]/16+1)*/
#define OV5653_VTS_DIFF_H			0x350C	/*0x00 RW Bit[7:0]: vts_diff[15:8] Changing this value is not recommended*/
#define OV5653_VTS_DIFF_L			0x350D	/*0x06 RW Bit[7:0]: vts_diff[7:0] Changing this value is not recommended*/

#define OV5653_ARRAY_CONTROL			0x3621	/*0x00 RW Bit[7]: Horizontal bining enable Bit[6]: Horizontal subsampling Bit[5:0]: Debug mode*/
#define OV5653_ANALOG_CONTROL_D			0x370D	/*0x04 RW Analog control Bit[6] Vertical binning enable*/
#define OV5653_TIMING_HS_H			0x3800	/*0x01 RW HREF Start Point Bit[7:4]: Debug mode Bit[3:0]: timing_hs[11:8]*/
#define OV5653_TIMING_HS_L			0x3801	/*0xB4 RW HREF Start Point Bit[7:0]: timing_hs[7:0]*/
#define OV5653_TIMING_VS_H			0x3802	/*0x00 RW VREF Start Point Bit[7:4]: Debug mode Bit[3:0]: timing_vs[11:8]*/
#define OV5653_TIMING_VS_L			0x3803	/*0x0A RW VREF Start Point Bit[7:0]: timing_vs[7:0]*/
#define OV5653_TIMING_HW_H			0x3804	/*0x0A RW HREF Width Bit[7:4]: Debug mode Bit[3:0]: timing_hw[11:8]*/
#define OV5653_TIMING_HW_L			0x3805	/*0x20 RW	HREF Width Bit[7:0]: timing_hw[7:0]*/
#define OV5653_TIMING_VH_H			0x3806	/*0x07 RW VREF Height Bit[7:4]: Debug mode Bit[3:0]: timing_vh[11:8]*/
#define OV5653_TIMING_VH_L			0x3807	/*0x98 RW VREF Height Bit[7:0]: timing_vh[7:0]*/
#define OV5653_TIMING_DVP_HO_H			0x3808	/*0x0A RW DVP Horizontal Output Size Bit[7:4]: Debug mode Bit[3:0]: timing_dvpho[11:8]*/
#define OV5653_TIMING_DVP_HO_L			0x3809	/*0x20 RW DVP Horizontal Output Size Bit[7:0]: timing_dvpho[7:0]*/
#define OV5653_TIMING_DVP_VO_H			0x380A	/*0x07 RW DVP Vertical Output Size Bit[7:4]: Debug mode*/
#define OV5653_TIMING_DVP_VO_L			0x380B	/*0x98 RW	DVP vertical output size Total Horizontal Size*/
#define OV5653_TIMING_HTS_H			0x380C	/*0x0C RW Bit[7:5]: Debug mode Bit[4:0]: timing_hts[12:8]*/
#define OV5653_TIMING_HTS_L			0x380D	/*0x2C RW Total Horizontal Size Bit[7:0]: timing_hts[7:0]*/
#define OV5653_TIMING_VTS_H			0x380E	/*0x07 RW	Total Vertical Size Bit[3:0]: timing_vts[11:8]*/
#define OV5653_TIMING_VTS_L			0x380F	/*0xB0 RW	Total Vertical Size Bit[7:0]: timing_vts[7:0]*/
#define OV5653_TIMING_HVOFFS			0x3810	/*0xC2 RW Bit[7:4]: hoffs[3:0] Bit[3:0]: voffs[3:0]*/
#define OV5653_R_FRAME_EXP1			0x3811	/*0xF0 RW Bit[7:0]: Frame exposure time[23:16]*/
#define OV5653_TIMING_TC_REG_16			0x3816	/*0x0A RW	SOF to HREF Delay (number of pixel count)*/
#define OV5653_TIMING_TC_REG_17			0x3817	/*0x24 RW*/
							/*Bit[7:4]: vs_int_r Origin of timing*/
							/*Bit[3]:*/
							/*Bit[2:0]: Frame precharge length*/
#define OV5653_TIMING_TC_REG_18			0x3818	/*0x80 RW*/
							/*Bit[7]:	dkhf*/
							/*Bit[6]:	mirror*/
							/*Bit[5]:	vflip*/
							/*Bit[4:2]: Debug mode*/
							/*Bit[1]:	vsub4*/
							/*Bit[0]:	vsub2*/
#define OV5653_TIMING_TC_REG_19			0x3819	/*0x80 RW*/
							/*Bit[7:4]: SOF to HREF delay (number of line count)*/
							/*Bit[3]:	vfifo_hsize_sel*/
							/*Bit[2]:	vfifo_vsize_sel*/
							/*Bit[1:0]:	00: From vts_aeclat 01: From reg_vts 10: From vts_i 11: From vts_vs*/
#define OV5653_TIMING_TC_HS_MIRR_ADJ			0x381A	/*0x00 RW*/
							/*Bit[7]:	hs_mirror_offset_cs 0:	Add hs_mirror_offset 1:	Subtract hs_mirror_offset*/
							/*Bit[6:0]: hs_mirror_offset*/
#define OV5653_TIMING_HSYNC_START_H			0x382C	/*0x00 RW Bit[7:4]: Debug mode Bit[3:0]: hsync_start_h*/
#define OV5653_TIMING_HSYNC_START_L			0x382D	/*0x00 RW Bit[7:0]: hsync_start_l*/
#define OV5653_TIMING_HSYNC_WIDTH			0x382E	/*0x00 RW	HSYNC Width*/
#define OV5653_TIMING_TC_REG_30			0x3830	/*0x50 RW*/
							/*Bit[7]:	Digital gain manual enable*/
							/*Bit[6]:	Digital gain select*/
							/*Bit[5:4]: Gain mapping select*/
							/*Bit[3:2]: Not used*/
							/*Bit[1:0]: Manual digital gain*/
#define OV5653_TIMING_TC_REG_31			0x3831	/*0x00 RW Bit[7:1]: Debug mode Bit[0]:	r_vflip_color*/
#define OV5653_TIMING_TC_REG_32			0x3832	/*0x00 RW Bit[7:4]: Debug mode*/
#define OV5653_STROBE			0x3B00	/*0x00 RW*/
							/*Bit[7]:	Strobe on*/
							/*Bit[6]:	Reverse*/
							/*Bit[5:4]: Debug mode*/
							/*Bit[3:2]: width_in_xenon*/
							/*Bit[1:0]: Mode 00: Xenon 01: LED1 10: LED2 11: LED3*/
#define OV5653_R_FRAME_EXP2			0x3B04	/*0x04 RW Bit[7:0]: Frame exposure time[15:8]*/
#define OV5653_R_FRAME_EXP3			0x3B05	/*0x00 RW Bit[7:0]: Frame exposure time[7:0]*/

#define OV5653_DVP_MODE_SELECT			0x4700	/*0x04 RW*/
							/*Bit[7:4]: Debug mode*/
							/*Bit[3]:	CCIR v select*/
							/*Bit[2]:	CCIR f select*/
							/*Bit[1]:	CCIR656 mode enable*/
							/*Bit[0]:	HSYNC mode enable*/
#define OV5653_DVP_VSYNC_WIDTH_CONTRL			0x4701	/*0x01 RW VSYNC Width (in terms of number of lines)*/
#define OV5653_DVP_HSYVSY_NEG_WIDTH_H			0x4702	/*0x01 RW VSYNC Width (pixel count, high byte)*/
#define OV5653_DVP_HSYVSY_NEG_WIDTH_L			0x4703	/*0x00 RW VSYNC Width (pixel count, low byte)*/
#define OV5653_DVP_VSYNC_MODE_DELAY_			0x4704	/*0x00 RW Bit[7:4]: Debug mode Bit[1]: Bit[0]:	*/
#define OV5653_DVP_EOF_VSYNC_DELAY_H			0x4705	/*0x00 RW SOF/EOF Negative Edge to VSYNC Positive Edge Delay, High Byte*/
#define OV5653_DVP_EOF_VSYNC_DELAY_M			0x4706	/*0x00 RW SOF/EOF Negative Edge to VSYNC Positive Edge Delay, Middle Byte*/
#define OV5653_DVP_EOF_VSYNC_DELAY_L			0x4707	/*0x00 RW SOF/EOF Negative Edge to VSYNC Positive Edge Delay, Low Byte*/
#define OV5653_DVP_POL_CTRL			0x4708	/*0x01 RW*/
							/*Bit[7]:	Clock DDR mode enable*/
							/*Bit[6]:	Debug mode*/
							/*Bit[5]:	VSYNC gated clock enable*/
							/*Bit[4]:	HREF gated clock enable*/
							/*Bit[3]:	No first for FIFO*/
							/*Bit[2]:	HREF polarity reverse*/
							/*Bit[1]:	VSYNC polarity reverse*/
							/*Bit[0]:	PCLK polarity reverse*/
#define OV5653_BIT_TEST_PATTERN			0x4709	/*0x00 RW*/
							/*Bit[7]:	FIFO bypass mode*/
							/*Bit[6:4]: Data bit swap*/
							/*Bit[3]:	Bit test mode*/
							/*Bit[2]:	10-bit bit test*/
							/*Bit[1]:	8-bit bit test*/
							/*Bit[0]:	Bit test enable*/
#define OV5653_DVP_BYP_CTRL_H			0x470A	/*0X00 RW Bypass Control High Byte*/
#define OV5653_DVP_BYP_CTRL_L			0x470B	/*0X00 RW Bypass Control Low Byte*/
#define OV5653_DVP_BYP_SEL			0x470C	/*0x00 RW*/
							/*Bit[7:5]: Debug mode*/
							/*Bit[4]:	HREF select*/
							/*Bit[3:0]: Bypass select*/
#define OV5653_VAP_CTRL00			0x5900	/*0x01 RW*/
							/*Bit[7:6]: Debug mode*/
							/*Bit[5]:	sum_en for even-line and even-column pixels 0:	Drop mode 1:	Sum mode*/
							/*Bit[4]:	sum_en for even-line and odd-column pixels 0:	Drop mode 1:	Sum mode*/
							/*Bit[3]:	sum_en for odd-line and even-column pixels 0:	Drop mode 1:	Sum mode*/
							/*Bit[2]:	sum_en for odd-line and*/
							/*Bit[1]: 0: and second pixel of thirdThis option only plays its role in 1:4 horizontal sub-sample drop mode 1:	Output the first group of 4 group*/
							/*Bit[0]:	avg_en 0:	Limitation mode 1:	Average mode*/
#define OV5653_VAP_CTRL01			0x5901	/*0x00 RW */
							/*Bit[7:4]: Debug mode*/
							/*Bit[3:2]: hsub_coef Horizontal sub-sample coefficient 00: Sub-sample 1 01: Sub-sample 2 1x: Sub-sample */
							/*Bit[1:0]: vsub_coef Vertical sub-sample coefficient Changing this value is not recommended*/

#endif /* __OV5653_PRI_H__ */

